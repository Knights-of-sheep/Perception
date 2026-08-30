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
// 006 US4：手写 CPython C API 已全部迁移至 pybind11（pybind11.h 内含 Python.h，
// 仍须最先包含以规避 Qt slots/signals/emit 宏与 object.h 的冲突）。
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>  // py::initialize_interpreter / finalize_interpreter / is_initialized
#include <pybind11/eval.h>   // py::exec（引导脚本，Py_file_input 等价）
#include <pybind11/stl.h>

// 006 US4：pybind11 2.13 默认命名空间为 pybind11，py 是别名（官方约定）。
// 头文件仅前向声明 pybind11::error_already_set；本 TU 以别名统一 py:: 书写。
namespace py = pybind11;

#include "core/log/log_level.h"
#include "core/log/logger.h"  // ILogBridge 转发目标（_cpp_log 契约 python-log-bridge.md）

#include "python/api/i_window_factory.h"  // 006 US4：create_window 实现侧接口

#include "ui/console/PythonConsole.h"
#include "ui/console/bridge_api.h"  // 006 US4：IOutputSink / ILogBridge 虚接口

#include <QColor>
#include <QCoreApplication>
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

#include <memory>  // std::unique_ptr（WindowFactoryAdapter 持有）

#include "ui/theme/theme_catalog.h"
#include "ui/theme/theme_manager.h"

namespace perception {
namespace ui {

// 006 US4：桥接（ConsoleOut 重定向 / _cpp_log / create_window / 引导脚本）已迁移至
// pybind11 模块 perception_console（src/python/console/）与 perception_py
// （src/python/command/）；本类经 IOutputSink / ILogBridge 虚接口与模块交互。

namespace {

// Python 版本横幅（等价替代 Py_GetVersion()）：取 sys.version 完整字符串。
QString pythonVersionBanner() {
    py::gil_scoped_acquire gil;
    try {
        const std::string v = py::str(
            py::module_::import("sys").attr("version")).cast<std::string>();
        return QString::fromStdString(v);
    } catch (const py::error_already_set&) {
        return QStringLiteral("Python");
    }
}

// 记录 Python 初始化期失败到统一日志：与 printError 的退化分支（仅 str(value)，
// 会丢失 traceback/__cause__）互补，e.what() 是 pybind11 完整格式化（类型名+消息+
// traceback+chained exceptions），且不依赖引导脚本（此时 _format_traceback 可能
// 尚未安装）。输出到 %APPDATA%/Perception/logs/app.log（main.cpp 装配）。
void logPythonInitError(const py::error_already_set& e) {
    const char* what = e.what();
    if (!what || !*what) return;
    perception::core::log::Logger::instance().log(
        perception::core::log::LogLevel::Error, "python.console", what);
}

// perception_py 的 IWindowFactory 适配器（006 US4）：
// create_window 模块函数 → 本适配器 → createWindowCallback（MainWindow 注册，
// 调 createSubwindow 真实创建子窗口）。宿主未连接（回调为空）返回空串 → None。
class WindowFactoryAdapter : public perception::python::IWindowFactory {
public:
    explicit WindowFactoryAdapter(PythonConsole* console) : console_(console) {}
    std::string createWindow(const std::string& title) override {
        if (!console_) return std::string();
        return console_->requestCreateWindow(QString::fromStdString(title)).toStdString();
    }

private:
    PythonConsole* console_;  // 借用指针（生命周期归本类持有者）
};

}  // namespace

// ===== 内部状态（pimpl，避免头文件暴露 pybind11 类型）=====
struct PythonConsole::Impl {
    py::object globals;          // 持久命名空间（跨命令保留变量）
    py::object checkComplete;    // _check_complete（续行判定器）
    py::object formatTraceback;  // _format_traceback（traceback 格式化）
    QString pending;             // 续行累积
    QStringList history;                // 已执行命令（多行条目导航时跳过）
    int historyIndex = 0;
    bool imeComposing = false;          // 输入法正在组字（拼音候选未确认）
    QTextCharFormat promptFormat;       // ">>> " 提示符
    QTextCharFormat inputFormat;        // 输入区文字（键盘输入 / 历史回填 / 续行缩进）
    QTextCharFormat outputFormat;       // 普通输出
    QTextCharFormat resultFormat;       // 表达式结果
    QColor errorColor;                  // stderr / traceback
    CreateWindowCallback createWindowCallback;  // create_window 桥回调（返回生成的 id）
    std::unique_ptr<perception::python::IWindowFactory> windowFactory;  // 006 US4：注册至 perception_py
};

PythonConsole::PythonConsole(QWidget* parent)
    : QPlainTextEdit(parent), d_(new Impl) {
    setObjectName(QStringLiteral("pythonConsole"));
    setFont(QFont(QStringLiteral("Consolas"), 10));
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setFrameShape(QFrame::NoFrame);
    initColors();
    initPython();
    // 仅解释器初始化成功后才显示横幅（pythonVersionBanner 依赖运行中的解释器）
    if (pythonInitialized_) {
        appendOutput(pythonVersionBanner());
        appendOutput(tr(" — Perception embedded console (Ctrl+O: open data files; paste multiple lines to execute)\n"));
    }
    showPrompt();
}

PythonConsole::~PythonConsole() {
    // 不在此 finalize：解释器由 main 退出前 shutdown() 显式完成（py::finalize_interpreter），
    // 避免与 QApplication 对象析构顺序纠缠。
    delete d_;
}

void PythonConsole::refreshColors() { initColors(); }

QStringList PythonConsole::history() const { return d_->history; }

void PythonConsole::clearConsole() {
    clear();  // 清空全部显示（含输入区）
    d_->pending.clear();
    if (pythonInitialized_) {
        appendOutput(pythonVersionBanner());
        appendOutput(tr(" — Perception embedded console (Ctrl+O: open data files; paste multiple lines to execute)\n"));
    }
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
    try {
        py::initialize_interpreter();
    } catch (const std::exception& e) {
        const char* what = e.what();
        if (what && *what) {
            perception::core::log::Logger::instance().log(
                perception::core::log::LogLevel::Error, "python.console", what);
        }
        appendOutput(tr("Failed to initialize the Python interpreter: %1\n")
                         .arg(QString::fromLocal8Bit(e.what())), d_->errorColor);
        showPrompt();
        return;
    }
    pythonInitialized_ = true;  // 解释器就绪（shutdown 依据此标志）
    d_->globals = py::dict();
    d_->globals["__builtins__"] = py::module_::import("builtins");

    // 模块搜索路径：.pyd 与 exe 同级（CMAKE_BINARY_DIR），插入 sys.path 首位
    py::module_ sys = py::module_::import("sys");
    sys.attr("path").attr("insert")(
        0, QCoreApplication::applicationDirPath().toStdString());

    // 安装 REPL 桥（perception_console 模块）：注入 _cpp_log、执行引导脚本、
    // 重定向 sys.stdout/sys.stderr -> 本控件。本实例经 py::capsule 传入模块
    //（pyd 经 IOutputSink/ILogBridge 虚接口回调，不链接 UI 静态库）。
    try {
        py::module_ consoleModule = py::module_::import("perception_console");
        consoleModule.attr("install_console_bridge")(
            d_->globals,
            py::capsule(static_cast<console::IOutputSink*>(this), "IOutputSink"),
            py::capsule(static_cast<console::IOutputSink*>(this), "IOutputSink"),
            py::capsule(static_cast<console::ILogBridge*>(this), "ILogBridge"));
    } catch (const py::error_already_set& e) {
        // 桥安装失败（用户反馈 2026-08-30：启动时控制台首行仅见 "initialization failed"）。
        // 此阶段 _format_traceback 尚未就绪，printError 走退化分支会丢失 __cause__；
        // 先把 e.what()（pybind11 完整格式化：类型名+消息+traceback+chained exceptions）
        // 记入日志兜底（%APPDATA%/Perception/logs/app.log），再回显控件。
        logPythonInitError(e);
        printError(e);
        return;
    }
    // 命令层模块（perception_py）：注册 IWindowFactory（本类适配器，经 capsule
    // 传入模块——pyd 不链接 UI 静态库），并把 create_window 真实命令注入 REPL
    // globals（契约 python-create-window.md 的注入机制）。
    try {
        py::module_ commandModule = py::module_::import("perception_py");
        d_->windowFactory = std::make_unique<WindowFactoryAdapter>(this);
        commandModule.attr("register_window_factory")(
            py::capsule(static_cast<perception::python::IWindowFactory*>(
                            d_->windowFactory.get()),
                        "IWindowFactory"));
        d_->globals["create_window"] = commandModule.attr("create_window");
    } catch (const py::error_already_set& e) {
        logPythonInitError(e);
        printError(e);
        return;
    }
    d_->checkComplete = d_->globals["_check_complete"];
    d_->formatTraceback = d_->globals["_format_traceback"];

    // 释放 GIL：事件循环运行期间不持有（后续执行按需获取）
    py::gil_scoped_release release;
}

void PythonConsole::shutdown() {
    if (!pythonInitialized_) return;  // 解释器未初始化（pybind11 2.13 无 is_initialized）
    {
        py::gil_scoped_acquire gil;
        // 清命令层 IWindowFactory 注册表（防悬垂；适配器即将析构）
        if (d_->windowFactory) {
            try {
                py::module_::import("perception_py")
                    .attr("register_window_factory")(py::none());
            } catch (const py::error_already_set&) {
            }
        }
        d_->windowFactory.reset();
        d_->globals = py::object();        // 释放持久命名空间引用
        d_->checkComplete = py::object();
        d_->formatTraceback = py::object();
    }
    py::finalize_interpreter();
    pythonInitialized_ = false;
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

void PythonConsole::executeCommand(const QString& command) {
    // FR-027：无论经 PyShell 手工输入还是菜单栏触发，所执行的命令文本 MUST 回显到输出区。
    // 手工输入路径下输入区已含该文本（runCommand 步骤 2 会跳过插入）；菜单等外部入口触发时
    // 此处将命令文本插入当前输入行（紧跟提示符，与手工输入行布局一致），不额外换行——
    // 避免在空提示符后强制换行造成"命令独立成行"的回显错位（用户反馈 2026-08-29）。
    ensureCursorInInput();
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    if (!currentInput().trimmed().isEmpty()) {
        cur.insertText(QStringLiteral("\n"));  // 输入行已有内容：先换行避免文本粘连
    }
    cur.insertText(command, d_->inputFormat);
    setTextCursor(cur);
    setCurrentCharFormat(d_->inputFormat);

    // 与 PyShell 手工输入走同一执行路径：runCommand 判定完整性 -> runBlock 执行；
    // create_window 的返回值（窗口 id 字符串）由 runBlock 对表达式结果自动 repr 打印（FR-027）。
    // 记入命令历史（与手工回车 / runPastedText 语义一致）：否则菜单触发的命令执行后
    // 按上键（navigateHistory）无法查回（用户反馈 2026-08-30）。
    const QString entry = command.trimmed();
    if (!entry.isEmpty()) {
        d_->history.removeAll(entry);  // 去重：重复命令移到最新
        d_->history.append(entry);
        d_->historyIndex = d_->history.size();  // 置末尾，上键从最新开始
    }
    runCommand(command);
}

void PythonConsole::setCreateWindowCallback(CreateWindowCallback callback) {
    d_->createWindowCallback = std::move(callback);
}

QString PythonConsole::requestCreateWindow(const QString& title) {
    // 经回调创建（MainWindow::createSubwindow，契约 python-create-window.md）：
    // title 空白时由回调侧默认 = id；返回生成的 id（"Plot_" + 全局递增序号）。
    if (!d_->createWindowCallback) {
        appendOutput(tr("create_window: subwindow host is not connected\n"), d_->errorColor);
        return QString();
    }
    return d_->createWindowCallback(title.trimmed());
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

    // 防御：桥安装失败（initPython 提前 return）时 _check_complete 未就绪，
    // 直接调用空 object 会抛 pybind11 内部错误——提示并回到新提示符而非崩溃。
    if (!d_->checkComplete) {
        appendOutput(tr("console bridge is not initialized\n"), d_->errorColor);
        showPrompt();
        return;
    }

    executing_ = true;
    QString verdict = QStringLiteral("complete");
    {
        py::gil_scoped_acquire gil;
        try {
            py::object vObj = d_->checkComplete(block.toStdString());
            verdict = QString::fromStdString(py::cast<std::string>(vObj));
        } catch (const py::error_already_set& e) {
            printError(e);  // 判定器自身异常（极罕见）：打印后终止，不再执行命令
        }
    }

    if (verdict == QLatin1String("incomplete")) {
        d_->pending = block;  // 累积整块（含当前行）
        executing_ = false;
        showPrompt(true);  // "... " 后自动缩进
        return;
    }

    // 挂起的 Ctrl+C / 中断信号：标准 REPL 行为——不执行命令，
    // 打印 KeyboardInterrupt 并回到新提示符（不清空历史，中断是一次性的）
    if (verdict == QLatin1String("interrupt")) {
        d_->pending.clear();
        executing_ = false;
        appendOutput(QStringLiteral("KeyboardInterrupt\n"), d_->errorColor);
        showPrompt();
        return;
    }

    // 3) 完整 / 语法错误：执行整块（语法错误由 Python 抛 traceback）
    d_->pending.clear();
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
    {
        py::gil_scoped_acquire gil;
        try {
            // _run_single：compile(...,'single') + exec —— 等价替代 Py_single_input，
            // 表达式语句经 sys.displayhook 自动求值打印到重定向后的 sys.stdout。
            d_->globals["_run_single"](full.toStdString(), d_->globals);
        } catch (const py::error_already_set& e) {
            printError(e);
        }
    }
    executing_ = false;
    showPrompt();
}

void PythonConsole::printError(const py::error_already_set& e) {
    // error_already_set 已归一化异常：type / value / trace 三元组。
    py::object type = e.type();
    py::object value = e.value();
    py::object tb = e.trace();

    QString text;
    if (d_->formatTraceback && value) {
        try {
            // _format_traceback 的签名是 (tb)，tb 为 (type, value, tb) 三元组；
            // 以元组作单个参数调用（防止元组被展开成多个位置参数）。
            py::tuple tbTuple(3);
            tbTuple[0] = type;
            tbTuple[1] = value;
            tbTuple[2] = tb ? tb : py::none();
            py::object txt = d_->formatTraceback(tbTuple);
            text = QString::fromStdString(py::cast<std::string>(txt));
        } catch (const py::error_already_set&) {
            // 格式化时的残留错误：error_already_set 析构自动清解释器错误状态
        }
    }
    if (text.isEmpty()) {
        // 退化分支（引导脚本未就绪，如 initPython 早期 import 失败）：优先用
        // e.what() 完整格式化（类型名+消息+traceback+__cause__），避免只显示
        // "initialization failed" 而掩盖真实根因（用户反馈 2026-08-30）；
        // 仅当 what() 为空（二次异常）再降级为 str(value)。
        const char* what = e.what();
        if (what && *what) {
            text = QString::fromUtf8(what);
        } else {
            try {
                text = QString::fromStdString(
                    py::cast<std::string>(py::str(value ? value : type)));
            } catch (const py::error_already_set&) {
            }
        }
    }
    // 执行前已换行，错误文本直接输出（末尾换行留空到新提示符）
    appendOutput(text + QLatin1Char('\n'), d_->errorColor);
}

// ===== IOutputSink / ILogBridge 虚接口实现（006 US4）=====
void PythonConsole::write(const char* text, int len, bool isStderr) {
    appendOutput(QString::fromUtf8(text, len), isStderr ? d_->errorColor : QColor());
}

void PythonConsole::flush() {
    // Qt 控件无缓冲：空实现
}

void PythonConsole::log(int level, const char* source, const char* message) {
    // _cpp_log 契约 python-log-bridge.md：level 为 C++ LogLevel 整数值，
    // source 形如 "name:lineno"；错误在模块侧（makeCppLog）已防御，此处直通。
    perception::core::log::Logger::instance().log(
        static_cast<perception::core::log::LogLevel>(level),
        source ? source : "python",
        message ? message : "");
}

}  // namespace ui
}  // namespace perception
