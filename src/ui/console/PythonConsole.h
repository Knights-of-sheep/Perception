// ===== PythonConsole：底部 Python 命令行窗口（内嵌 CPython REPL）=====
// 范式：ParaView / MATLAB / FreeCAD 的同款「进程内解释器 + 文本前端」。
//   - 命令共享持久命名空间（跨命令保留变量）
//   - sys.stdout/stderr 重定向到控件（自定义 Python 流对象回调 C++）
//   - codeop 判定续行（>>> / ... 提示符）
//   - 上下键历史、Ctrl+C 清行、Tab 缩进、多行粘贴即执行
// 说明：本类用 pimpl 隔离 Python.h，头文件不暴露 CPython 类型。
//       退出时需调用 shutdown()（main 中 app.exec() 返回后、QApplication 析构前）。
#pragma once

#include <QPlainTextEdit>
#include <QString>
#include <QStringList>

class QColor;
class QInputMethodEvent;
class QKeyEvent;
class QMimeData;
class QMouseEvent;

namespace perception {
namespace ui {

class PythonConsole : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit PythonConsole(QWidget* parent = nullptr);
    ~PythonConsole() override;

    // 追加输出到控件末尾（stdout/stderr 重定向回调；外部也可调用）
    void appendOutput(const QString& text, const QColor& color = QColor());
    bool isExecuting() const { return executing_; }

    // 执行一段脚本（按行喂入 REPL，语义与逐行输入一致；供测试/后续脚本功能）
    void runScript(const QString& script);

    // 释放 Python 运行时（程序退出前调用一次；析构不负责 finalize）
    void shutdown();
    // 主题切换后刷新提示符/输出配色（由 MainWindow::applyTheme 调用）
    void refreshColors();

    // 已执行的命令历史（供"导出命令"写入 .py 文件；多行块为单条）
    QStringList history() const;
    // 清空控制台显示（重置为版本横幅 + 新提示符；不清 Python 命名空间）
    void clearConsole();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void insertFromMimeData(const QMimeData* source) override;

private:
    void initPython();                       // Py_Initialize + 引导脚本 + 输出重定向
    void initColors();                       // 提示符/输出/错误配色（取当前主题）
    void runCommand(const QString& src);     // 判定完整性 -> 执行 -> 结果/异常展示
    void runBlock(const QString& full);      // 执行完整命令块（含续行块提交）
    void runPastedText(const QString& text); // 多行粘贴：整块执行
    void printError();                       // traceback 格式化展示
    void showPrompt(bool pending = false);   // 追加 ">>> " / "... "
    QString currentInput() const;            // 末行提示符后文本
    void replaceInput(const QString& text);  // 替换末行输入（保留提示符）
    void clearInput();
    void navigateHistory(int dir);
    void ensureCursorInInput();              // 光标强制回到末行输入区
    int  promptLength() const;               // 当前末行提示符长度
    // 输入过滤：全角标点转半角，丢弃所有非 ASCII 字符（中文等），
    // 中文输入法下无论选什么候选，进入控制台的只有 ASCII 字符
    static QString filterInput(const QString& s);

    struct Impl;
    Impl* d_ = nullptr;
    bool executing_ = false;
};

}  // namespace ui
}  // namespace perception
