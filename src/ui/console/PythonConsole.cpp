// Python.h 必须最先包含：Qt 的 qobjectdefs.h 定义了 slots/signals/emit 宏，
// 若先包含 Qt 头，object.h 中 "PyType_Slot *slots" 会被展开导致 C2059（CPython C API 规范）。
// 说明：pyconfig.h 在 _DEBUG 下会 #define Py_DEBUG，使 Py_INCREF/Py_DECREF 引用调试版
// Python 专用符号（_Py_INCREF_IncRefTotal 等）。Anaconda 只有发布版 python313.dll，
// MSVC Debug 链接会 LNK2019；兼容实现见本目录 python_debug_shim.cpp。
// 注意：此处不可取消 _DEBUG 宏——会破坏 MSVC STL 的 RuntimeLibrary/_ITERATOR_DEBUG_LEVEL
// 一致性（LNK2038），只能通过 shim 提供缺失符号。
// 也不可定义 Py_NO_ENABLE_SHARED：它会使数据符号（PyAPI_DATA，如 _Py_NoneStruct）退化为
// 普通引用，而导入库只提供 __imp_<name>（数据无 thunk 别名）→ LNK2001。
// 调试符号的解决：保持 dllimport 引用（__imp_<name>），由 python_debug_shim.cpp 提供
// 普通实现，并经 /alternatename 链接选项映射（见 src/app/CMakeLists.txt）。
#include <Python.h>

#include "core/log/logger.h"

#include "ui/console/PythonConsole.h"

#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>

#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_manager.h"

namespace perception {
namespace ui {

namespace {

// ---- 自定义 sys.stdout/stderr 流对象：write() 回调 C++ 追加到控件 ----
struct ConsoleOutObject {
    PyObject_HEAD
    PyObject* ptr;       // PyLong 包装的 PythonConsole*
    PyObject* is_stderr; // 0/1：错误通道用错误色
};

PyObject* ConsoleOut_write(PyObject* self, PyObject* args) {
    const char* text = nullptr;
    Py_ssize_t len = 0;
    if (!PyArg_ParseTuple(args, "s#", &text, &len)) return nullptr;
    auto* obj = reinterpret_cast<ConsoleOutObject*>(self);
    if (obj->ptr && PyLong_Check(obj->ptr)) {
        auto* console = static_cast<PythonConsole*>(PyLong_AsVoidPtr(obj->ptr));
        const bool isStderr = obj->is_stderr && PyLong_AsLong(obj->is_stderr);
        QColor color;
        if (isStderr) color = console->property("stderrColor").value<QColor>();
        console->appendOutput(QString::fromUtf8(text, int(len)), color);
    }
    return PyLong_FromSsize_t(len);
}

PyObject* ConsoleOut_flush(PyObject*, PyObject*) { Py_RETURN_NONE; }

PyObject* ConsoleOut_writelines(PyObject* self, PyObject* args) {
    // 简化实现：逐行转 write（绝大多数库只用 write/flush）
    PyObject* lines = nullptr;
    if (!PyArg_ParseTuple(args, "O", &lines)) return nullptr;
    PyObject* it = PyObject_GetIter(lines);
    if (!it) return nullptr;
    PyObject* line;
    while ((line = PyIter_Next(it)) != nullptr) {
        if (PyUnicode_Check(line)) {
            PyObject* t = PyTuple_Pack(1, line);
            PyObject* r = ConsoleOut_write(self, t);
            Py_XDECREF(r);
            Py_XDECREF(t);
        }
        Py_DECREF(line);
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) return nullptr;
    Py_RETURN_NONE;
}

int ConsoleOut_init(PyObject* self, PyObject* args, PyObject*) {
    PyObject* ptr = nullptr;
    PyObject* isStderr = nullptr;
    if (!PyArg_ParseTuple(args, "OO", &ptr, &isStderr)) return -1;
    auto* obj = reinterpret_cast<ConsoleOutObject*>(self);
    Py_INCREF(ptr);   Py_XDECREF(obj->ptr);   obj->ptr = ptr;
    Py_INCREF(isStderr); Py_XDECREF(obj->is_stderr); obj->is_stderr = isStderr;
    return 0;
}

PyMethodDef ConsoleOut_methods[] = {
    {"write", ConsoleOut_write, METH_VARARGS, nullptr},
    {"flush", ConsoleOut_flush, METH_NOARGS, nullptr},
    {"writelines", ConsoleOut_writelines, METH_VARARGS, nullptr},
    {nullptr, nullptr, 0, nullptr},
};

PyType_Slot ConsoleOut_slots[] = {
    {Py_tp_new, reinterpret_cast<void*>(PyType_GenericNew)},
    {Py_tp_init, reinterpret_cast<void*>(ConsoleOut_init)},
    {Py_tp_methods, ConsoleOut_methods},
    {0, nullptr},
};

PyType_Spec ConsoleOut_spec = {
    "perception.console.ConsoleOut",
    sizeof(ConsoleOutObject),
    0,
    Py_TPFLAGS_DEFAULT,
    ConsoleOut_slots,
};

// _cpp_log：Python logging handler 的落点（FR-008）。
// 桥接 Python logging → C++ 统一日志流；错误在内部捕获，不影响 Python 执行。
PyObject* cpp_log_impl(PyObject*, PyObject* args) {
    int level = 1;
    const char* source = nullptr;
    const char* message = nullptr;
    if (!PyArg_ParseTuple(args, "iss", &level, &source, &message)) return nullptr;
    // 防御：level 越界回退 Info；不抛异常逃逸
    if (level < 0 || level > 4) level = 1;
    perception::core::log::Logger::instance().log(
        static_cast<perception::core::log::LogLevel>(level),
        source ? source : "python",
        message ? message : "");
    Py_RETURN_NONE;
}

PyMethodDef cpp_log_def[] = {
    {"_cpp_log", cpp_log_impl, METH_VARARGS, "C++ 统一日志桥接（Python logging 专用）"},
    {nullptr, nullptr, 0, nullptr},
};

// 引导脚本：续行判定（codeop.compile_command 为无状态的标准判定器，
// 返回 None=不完整 / code=完整 / 抛异常=语法错误；与 code.InteractiveConsole 同源）
// + traceback 格式化 + 空 stdin（防止 help() 等交互函数读输入时阻塞 GUI）
// + PerceptionLogHandler：Python logging → _cpp_log 桥接（FR-008，契约 python-log-bridge.md）
const char kBootstrap[] =
    "import codeop, sys, traceback\n"
    "def _check_complete(src):\n"
    "    try:\n"
    "        result = codeop.compile_command(src, '<console>', symbol='single')\n"
    "    except KeyboardInterrupt:\n"
    "        return 'interrupt'  # 挂起的 Ctrl+C：标准 REPL 中断当前命令，不执行\n"
    "    except (SyntaxError, OverflowError, ValueError, TypeError):\n"
    "        return 'error'\n"
    "    return 'incomplete' if result is None else 'complete'\n"
    "def _format_traceback(tb):\n"
    "    return ''.join(traceback.format_exception(*tb))\n"
    "class _EmptyIn:\n"
    "    def read(self, *a, **k): return ''\n"
    "    def readline(self, *a, **k): return ''\n"
    "    def readlines(self, *a, **k): return []\n"
    "    def isatty(self): return False\n"
    "sys.stdin = _EmptyIn()\n"
    "import logging\n"
    "_LEVEL_MAP = {10: 0, 20: 1, 30: 2, 40: 3, 50: 4}  # Python -> C++ LogLevel\n"
    "class PerceptionLogHandler(logging.Handler):\n"
    "    def emit(self, record):\n"
    "        try:\n"
    "            level = _LEVEL_MAP.get(record.levelno, 1)\n"
    "            source = '%s:%d' % (record.name, record.lineno)\n"
    "            _cpp_log(level, source, record.getMessage())\n"
    "        except Exception:\n"
    "            self.handleError(record)  # 桥接失败不影响 Python 执行\n"
    "logging.getLogger().addHandler(PerceptionLogHandler())\n"
    "logging.getLogger().setLevel(logging.DEBUG)\n";

}  // namespace

// ===== 内部状态（pimpl，避免头文件暴露 CPython）=====
struct PythonConsole::Impl {
    PyObject* globals = nullptr;        // 持久命名空间（跨命令保留变量）
    PyObject* checkComplete = nullptr;  // _check_complete（借用引用，globals 持有）
    PyObject* formatTraceback = nullptr;
    PyObject* sysStdout = nullptr;      // 重定向对象（持有引用）
    PyObject* sysStderr = nullptr;
    PyObject* outType = nullptr;
    QString pending;                    // 续行累积
    QStringList history;                // 已执行命令（多行条目导航时跳过）
    int historyIndex = 0;
    bool imeComposing = false;          // 输入法正在组字（拼音候选未确认）
    QTextCharFormat promptFormat;       // ">>> " 提示符
    QTextCharFormat inputFormat;        // 输入区文字（键盘输入 / 历史回填 / 续行缩进）
    QTextCharFormat outputFormat;       // 普通输出
    QTextCharFormat resultFormat;       // 表达式结果
    QColor errorColor;                  // stderr / traceback
};

PythonConsole::PythonConsole(QWidget* parent)
    : QPlainTextEdit(parent), d_(new Impl) {
    setObjectName(QStringLiteral("pythonConsole"));
    setFont(QFont(QStringLiteral("Consolas"), 10));
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setFrameShape(QFrame::NoFrame);
    initColors();
    initPython();
    appendOutput(QString::fromUtf8(Py_GetVersion()));
    appendOutput(tr(" — Perception embedded console (Ctrl+O: open data files; paste multiple lines to execute)\n"));
    showPrompt();
}

PythonConsole::~PythonConsole() {
    // 不在此 finalize：Py_Finalize 由 main 退出前 shutdown() 显式完成，
    // 避免与 QApplication 对象析构顺序纠缠。
    delete d_;
}

void PythonConsole::refreshColors() { initColors(); }

QStringList PythonConsole::history() const { return d_->history; }

void PythonConsole::clearConsole() {
    clear();  // 清空全部显示（含输入区）
    d_->pending.clear();
    appendOutput(QString::fromUtf8(Py_GetVersion()));
    appendOutput(tr(" — Perception embedded console (Ctrl+O: open data files; paste multiple lines to execute)\n"));
    showPrompt();
}

void PythonConsole::initColors() {
    const auto& c = ThemeManager::current()->colors;
    d_->promptFormat.setForeground(c.accent);
    d_->promptFormat.setFontWeight(QFont::Bold);
    d_->inputFormat.setForeground(c.text);
    d_->outputFormat.setForeground(c.text);
    // 输入/输出/结果统一前景色，避免"输入和结果颜色不一样"的色差观感；
    // 仅提示符加粗强调、错误用危险色区分。
    d_->resultFormat.setForeground(c.text);
    d_->errorColor = c.danger;
    setProperty("stderrColor", d_->errorColor);
}

// ===== Python 运行时 =====
void PythonConsole::initPython() {
    Py_Initialize();
    if (!Py_IsInitialized()) {
        appendOutput(tr("Failed to initialize the Python interpreter.\n"), d_->errorColor);
        showPrompt();
        return;
    }
    d_->globals = PyDict_New();
    PyDict_SetItemString(d_->globals, "__builtins__", PyEval_GetBuiltins());

    // 注入 _cpp_log 桥接（必须在 kBootstrap 执行前，handler 引用它）
    PyObject* cppLogFunc = PyCFunction_New(&cpp_log_def[0], nullptr);
    if (cppLogFunc) {
        PyDict_SetItemString(d_->globals, "_cpp_log", cppLogFunc);
        Py_DECREF(cppLogFunc);
    }

    PyRun_String(kBootstrap, Py_file_input, d_->globals, d_->globals);
    if (PyErr_Occurred()) printError();
    d_->checkComplete = PyDict_GetItemString(d_->globals, "_check_complete");
    d_->formatTraceback = PyDict_GetItemString(d_->globals, "_format_traceback");

    // 重定向 sys.stdout / sys.stderr -> 本控件
    d_->outType = PyType_FromSpec(&ConsoleOut_spec);
    if (d_->outType) {
        PyObject* argsOut =
            PyTuple_Pack(2, PyLong_FromVoidPtr(this), PyLong_FromLong(0));
        PyObject* argsErr =
            PyTuple_Pack(2, PyLong_FromVoidPtr(this), PyLong_FromLong(1));
        d_->sysStdout = PyObject_CallObject(d_->outType, argsOut);
        d_->sysStderr = PyObject_CallObject(d_->outType, argsErr);
        PySys_SetObject("stdout", d_->sysStdout);
        PySys_SetObject("stderr", d_->sysStderr);
        Py_DECREF(argsOut);
        Py_DECREF(argsErr);
    }

    // 释放 GIL：事件循环运行期间不持有
    PyEval_SaveThread();
}

void PythonConsole::shutdown() {
    if (!Py_IsInitialized()) return;
    PyGILState_STATE gil = PyGILState_Ensure();
    Py_XDECREF(d_->sysStdout);   d_->sysStdout = nullptr;
    Py_XDECREF(d_->sysStderr);   d_->sysStderr = nullptr;
    Py_XDECREF(d_->outType);     d_->outType = nullptr;
    Py_XDECREF(d_->globals);     d_->globals = nullptr;
    d_->checkComplete = nullptr;
    d_->formatTraceback = nullptr;
    PyGILState_Release(gil);
    Py_FinalizeEx();
}

// ===== 输出 =====
void PythonConsole::appendOutput(const QString& text, const QColor& color) {
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    if (color.isValid()) {
        QTextCharFormat f;
        f.setForeground(color);
        cur.insertText(text, f);
    } else {
        cur.insertText(text, d_->outputFormat);
    }
    setTextCursor(cur);
    ensureCursorVisible();
}

// ===== 提示符 / 输入区 =====
int PythonConsole::promptLength() const {
    const QTextBlock last = document()->lastBlock();
    const QString line = last.text();
    if (line.startsWith(QStringLiteral(">>> "))) return 4;
    if (line.startsWith(QStringLiteral("... "))) return 4;
    return 0;
}

QString PythonConsole::currentInput() const {
    QString line = document()->lastBlock().text();
    const int len = promptLength();
    if (len > 0) line.remove(0, len);
    return line;
}

void PythonConsole::showPrompt(bool pending) {
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    // 保留输入行模式：提示符始终显示在下一行行首（与标准 REPL 一致）
    if (cur.positionInBlock() != 0) {
        cur.insertText(QStringLiteral("\n"));
    }
    cur.insertText(pending ? QStringLiteral("... ") : QStringLiteral(">>> "),
                   d_->promptFormat);
    // 续行提示符后自动缩进 4 空格：与 IDLE 行为一致，方便用户继续输入
    if (pending) cur.insertText(QStringLiteral("    "), d_->inputFormat);
    setTextCursor(cur);
    ensureCursorVisible();
    // 后续键盘输入沿用输入区格式（避免默认格式与回填格式不一致的色差）
    setCurrentCharFormat(d_->inputFormat);
}

void PythonConsole::replaceInput(const QString& text) {
    QTextCursor cur = textCursor();
    const QTextBlock last = document()->lastBlock();
    // 只选中"提示符后"的输入区，保留 >>> / ... 前缀
    cur.setPosition(last.position() + promptLength());
    cur.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cur.removeSelectedText();
    cur.insertText(text, d_->inputFormat);
    setTextCursor(cur);
    setCurrentCharFormat(d_->inputFormat);
}

void PythonConsole::clearInput() {
    replaceInput(QString());
}

void PythonConsole::ensureCursorInInput() {
    QTextCursor cur = textCursor();
    const QTextBlock last = document()->lastBlock();
    if (cur.block() != last) {
        cur.movePosition(QTextCursor::End);
        setTextCursor(cur);
    } else {
        const int start = last.position() + promptLength();
        if (cur.position() < start) {
            cur.setPosition(start);
            setTextCursor(cur);
        }
    }
}

void PythonConsole::navigateHistory(int dir) {
    if (d_->history.isEmpty()) return;
    // 跳过多行条目（多行恢复会破坏输入区结构，MVP 先跳过）
    const int step = (dir > 0) ? 1 : -1;
    int idx = d_->historyIndex;
    for (;;) {
        idx += step;
        if (idx < 0) return;
        if (idx >= d_->history.size()) {
            d_->historyIndex = d_->history.size();
            if (currentInput() != QString()) clearInput();
            return;
        }
        if (!d_->history.at(idx).contains(QLatin1Char('\n'))) {
            d_->historyIndex = idx;
            replaceInput(d_->history.at(idx));
            return;
        }
    }
}

// ===== 输入事件 =====
// 输入过滤：全角标点 -> 半角（兼容中文输入法）；一切非 ASCII 字符（汉字等）
// 丢弃，保证控制台只接收 ASCII 输入，无论输入法选了什么候选。
QString PythonConsole::filterInput(const QString& s) {
    QString out;
    out.reserve(s.size());
    for (const QChar c : s) {
        const ushort u = c.unicode();
        if (u == 0x3000)            out += QLatin1Char(' ');   // 全角空格
        else if (u == 0x3001)       out += QLatin1Char(',');   // 顿号
        else if (u == 0x3002)       out += QLatin1Char('.');   // 句号
        else if (u == 0x201C || u == 0x201D) out += QLatin1Char('"');   // 双引号
        else if (u == 0x2018 || u == 0x2019) out += QLatin1Char('\'');  // 单引号
        else if (u >= 0xFF01 && u <= 0xFF5E) out += QChar(u - 0xFEE0);  // 全角 ASCII 区
        else if (u < 0x80)          out += c;                  // ASCII 字符（含控制字符）保留
        // 其余（汉字/日文/韩文等 u >= 0x80）：丢弃
    }
    return out;
}

void PythonConsole::keyPressEvent(QKeyEvent* e) {
    if (executing_) { e->accept(); return; }
    ensureCursorInInput();

    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        // 输入法拼音候选未提交时按回车：先提交候选词，不触发执行命令
        if (d_->imeComposing) {
            QGuiApplication::inputMethod()->commit();
            d_->imeComposing = false;
            e->accept();
            return;
        }
        const QString line = currentInput();
        // 保留输入行回显（标准 REPL 行为），输出从下一行开始
        if (!line.trimmed().isEmpty()) {
            d_->history.removeAll(line);
            d_->history.append(line);
        }
        d_->historyIndex = d_->history.size();
        runCommand(line);
        break;
    }
    case Qt::Key_Backspace:
        if (textCursor().position() <= document()->lastBlock().position() + promptLength()) {
            e->accept();
            return;
        }
        QPlainTextEdit::keyPressEvent(e);
        break;
    case Qt::Key_Delete:
        if (textCursor().position() < document()->lastBlock().position() + promptLength()) {
            e->accept();
            return;
        }
        QPlainTextEdit::keyPressEvent(e);
        break;
    case Qt::Key_Home: {
        QTextCursor cur = textCursor();
        cur.setPosition(document()->lastBlock().position() + promptLength());
        setTextCursor(cur);
        break;
    }
    case Qt::Key_Up:
        navigateHistory(-1);
        break;
    case Qt::Key_Down:
        navigateHistory(+1);
        break;
    case Qt::Key_Tab:
        insertPlainText(QStringLiteral("    "));
        break;
    case Qt::Key_C:
        if (e->modifiers() & Qt::ControlModifier) {
            clearInput();  // Ctrl+C：清空当前输入行
            break;
        }
        Q_FALLTHROUGH();
    default: {
        // 中文输入法下的直接键入（无修饰键）：过滤全角标点、丢弃非 ASCII 字符
        if (!(e->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
            && !e->text().isEmpty()) {
            const QString filtered = filterInput(e->text());
            if (!filtered.isEmpty()) {
                setCurrentCharFormat(d_->inputFormat);  // 强制输入区配色，避免色差
                insertPlainText(filtered);
            }
            e->accept();  // 消费掉事件，防止基类插入原文（非 ASCII 字符）
            break;
        }
        QPlainTextEdit::keyPressEvent(e);
        break;
    }
    }
    ensureCursorInInput();
    setCurrentCharFormat(d_->inputFormat);
}

void PythonConsole::inputMethodEvent(QInputMethodEvent* event) {
    // 组合状态：preedit 非空说明输入法正在组字（拼音候选未确认）
    d_->imeComposing = !event->preeditString().isEmpty();
    // 输入法确认提交的字符串：全角转半角 + 丢弃非 ASCII（保持控制台只收 ASCII）
    const QString commit = filterInput(event->commitString());
    if (commit != event->commitString()) {
        event->setCommitString(commit, event->replacementStart(), event->replacementLength());
    }
    QPlainTextEdit::inputMethodEvent(event);
    setCurrentCharFormat(d_->inputFormat);  // 恢复输入区格式（防止 preedit 切换色）
    ensureCursorInInput();
}

void PythonConsole::mousePressEvent(QMouseEvent* e) {
    QPlainTextEdit::mousePressEvent(e);
    ensureCursorInInput();  // 不允许把光标点到历史输出区
}

void PythonConsole::insertFromMimeData(const QMimeData* source) {
    if (!source->hasText()) return;
    // 输入过滤：全角标点转半角，丢弃非 ASCII（中文等）
    const QString text = filterInput(source->text());
    if (text.contains(QLatin1Char('\n'))) {
        runPastedText(text);  // 多行粘贴：逐行回显执行
        return;
    }
    ensureCursorInInput();
    setCurrentCharFormat(d_->inputFormat);
    insertPlainText(text);  // 手动插入过滤后文本（基类会插原文）
}

// ===== 执行 =====
void PythonConsole::runScript(const QString& script) {
    // 按行喂给 REPL（与逐行输入同语义：续行自动累积、表达式自动求值）
    const QStringList lines = script.split(QLatin1Char('\n'));
    for (const QString& line : lines) runCommand(line);
}

void PythonConsole::runPastedText(const QString& text) {
    // 多行粘贴：按行回显+执行（与 CPython REPL 行为一致）
    // 整段当一条 runCommand 会被拼成 "a=1b=2c=3print(...)" → SyntaxError
    QStringList lines = text.split(QLatin1Char('\n'));
    // 去尾随空行（粘贴常见尾部 \n），保留中间空行
    while (lines.size() > 1 && lines.last().isEmpty()) lines.removeLast();
    if (lines.isEmpty()) return;

    // 整段记为一条历史（与原行为一致）
    const QString histEntry = text.trimmed();
    if (!histEntry.isEmpty()) {
        d_->history.removeAll(histEntry);
        d_->history.append(histEntry);
        d_->historyIndex = d_->history.size();
    }

    // 按行交给 runCommand：单行命令立即执行；续行块（if/for/def）由 pending 累积
    for (const QString& line : lines) {
        runCommand(line);
    }
}

void PythonConsole::runCommand(const QString& src) {
    // 1) 空行：续行块内提交执行（与 InteractiveConsole 一致），否则仅新提示符
    if (src.trimmed().isEmpty()) {
        if (d_->pending.isEmpty()) {
            showPrompt();
            return;
        }
        const QString full = d_->pending + QLatin1Char('\n');
        d_->pending.clear();
        runBlock(full);
        return;
    }

    // 2) 回显到控件：键盘交互模式下 currentInput 已有用户输入（非空非自动缩进）→ 跳过；
    //    runScript / --console-script 模式下末行只有提示符或自动缩进 → 插入 src
    const QString curInput = currentInput();
    if (curInput.isEmpty() || curInput == QStringLiteral("    ")) {
        QTextCursor cur = textCursor();
        cur.movePosition(QTextCursor::End);
        cur.insertText(src, d_->inputFormat);
        setTextCursor(cur);
        setCurrentCharFormat(d_->inputFormat);
    }

    // 3) 整块判定：pending（之前续行累积）+ 当前行 一起编译
    //    防止 "for i in a:\n    print(i)" 中续行体被独立判定为完整直接执行
    const QString block = d_->pending.isEmpty()
                              ? QString(src)
                              : (d_->pending + QLatin1Char('\n') + src);

    executing_ = true;
    PyGILState_STATE gil = PyGILState_Ensure();

    QString verdict = QStringLiteral("complete");
    PyObject* vObj = PyObject_CallFunction(
        d_->checkComplete, "s", block.toUtf8().constData());
    if (vObj) {
        if (const char* v = PyUnicode_AsUTF8(vObj)) verdict = QString::fromUtf8(v);
        Py_DECREF(vObj);
    } else {
        printError();  // 判定器自身异常（极罕见）：打印后终止，不再执行命令
    }

    if (verdict == QLatin1String("incomplete")) {
        d_->pending = block;  // 累积整块（含当前行）
        PyGILState_Release(gil);
        executing_ = false;
        showPrompt(true);  // "... " 后自动缩进
        return;
    }

    // 挂起的 Ctrl+C / 中断信号：标准 REPL 行为——不执行命令，
    // 打印 KeyboardInterrupt 并回到新提示符（不清空历史，中断是一次性的）
    if (verdict == QLatin1String("interrupt")) {
        d_->pending.clear();
        PyGILState_Release(gil);
        executing_ = false;
        appendOutput(QStringLiteral("KeyboardInterrupt\n"), d_->errorColor);
        showPrompt();
        return;
    }

    // 3) 完整 / 语法错误：执行整块（语法错误由 Python 抛 traceback）
    d_->pending.clear();
    PyGILState_Release(gil);
    executing_ = false;
    runBlock(block);
}

void PythonConsole::runBlock(const QString& full) {
    // 输出区从新行开始（输入行已保留回显）
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    if (cur.positionInBlock() != 0) {
        cur.insertText(QStringLiteral("\n"));
        setTextCursor(cur);
    }

    executing_ = true;
    PyGILState_STATE gil = PyGILState_Ensure();
    // Py_single_input：表达式自动求值，与交互式一致
    PyObject* result = PyRun_String(full.toUtf8().constData(), Py_single_input,
                                    d_->globals, d_->globals);
    if (result) {
        if (result != Py_None) {
            PyObject* reprObj = PyObject_Repr(result);
            QString reprText;
            if (reprObj) {
                if (const char* s = PyUnicode_AsUTF8(reprObj))
                    reprText = QString::fromUtf8(s);
                Py_DECREF(reprObj);
            }
            appendOutput(reprText, d_->resultFormat.foreground().color());
        }
        Py_DECREF(result);
    } else {
        printError();
    }
    PyGILState_Release(gil);
    executing_ = false;
    showPrompt();
}

void PythonConsole::printError() {
    PyObject *type = nullptr, *value = nullptr, *tb = nullptr;
    PyErr_Fetch(&type, &value, &tb);
    if (!type) return;
    PyErr_NormalizeException(&type, &value, &tb);
    PyErr_Clear();

    QString text;
    if (d_->formatTraceback && value) {
        // _format_traceback 的签名是 (tb)，tb 为 (type, value, tb) 三元组。
        // 必须用 PyObject_CallFunctionObjArgs 把元组作为单个参数传入，
        // 否则 PyObject_Call 会把三元组展开成 3 个位置参数 -> TypeError，
        // 掩盖真正的异常并残留错误状态（引发后续 SystemError）。
        PyObject* tbTuple = PyTuple_Pack(3, type, value, tb ? tb : Py_None);
        PyObject* txt = tbTuple
            ? PyObject_CallFunctionObjArgs(d_->formatTraceback, tbTuple, nullptr)
            : nullptr;
        if (txt) {
            if (const char* s = PyUnicode_AsUTF8(txt)) text = QString::fromUtf8(s);
            Py_DECREF(txt);
        }
        Py_XDECREF(tbTuple);
        if (PyErr_Occurred()) PyErr_Clear();  // 清理格式化时的残留错误，避免污染后续调用
    }
    if (text.isEmpty()) {
        PyObject* str = PyObject_Str(value ? value : type);
        if (str) {
            if (const char* s = PyUnicode_AsUTF8(str)) text = QString::fromUtf8(s);
            Py_DECREF(str);
        }
    }
    // 执行前已换行，错误文本直接输出（末尾换行留空到新提示符）
    appendOutput(text + QLatin1Char('\n'), d_->errorColor);

    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(tb);
}

}  // namespace ui
}  // namespace perception
