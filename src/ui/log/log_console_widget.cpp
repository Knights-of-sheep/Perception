#include "ui/log/log_console_widget.h"

#include <QFont>
#include <QPalette>
#include <QTextCursor>
#include <QTextCharFormat>

namespace perception {
namespace ui {

LogConsoleWidget::LogConsoleWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setObjectName(QStringLiteral("logConsole"));
    setReadOnly(true);           // 纯输出区，禁止编辑
    setUndoRedoEnabled(false);   // 只读区无需撤销栈
    setMaximumBlockCount(10000); // 上限 1 万行，防长时间运行内存膨胀
    setLineWrapMode(QPlainTextEdit::NoWrap); // 日志惯例：不自动换行
    QFont f = font();
    f.setFamily(QStringLiteral("Consolas")); // 等宽字体，便于时间戳/级别列对齐
    f.setPointSize(9);
    setFont(f);
    // 背景色由 QSS（#logConsole）控制，此处仅保证无 QSS 时也可读
    setPlaceholderText(QStringLiteral("日志输出（INFO / WARN / ERROR …）"));

    // 启动 marker：绕过 sink 链路直接向 widget 写一行高对比红字，
    // 用于诊断 "widget 是否可见/能显示" vs "ConsoleSink 没派发"。
    // 后续日志正常到来时不会清掉它（最大块数 10K 行）。
    QTextCharFormat fmt;
    fmt.setForeground(QColor(0xFF, 0x6B, 0x6B));
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertText(QStringLiteral("=== log console ready ===\n"), fmt);
}

void LogConsoleWidget::appendOutput(const QString& text, const QColor& color)
{
    // QPlainTextEdit + setReadOnly(true) 下，setCharFormat + insertText 两步法偶尔会让
    // 字符格式不应用到新插入的文本（Qt 历史遗留）。用 insertText(text, fmt) 一次性传入
    // 字符格式最稳。source: Qt 5.0+ QTextCursor::insertText(const QString&, const QTextCharFormat&)
    QTextCharFormat fmt;
    fmt.setForeground(color);
    QTextCursor cur = textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertText(text, fmt);
    ensureCursorVisible(); // 始终跟随最新日志
}

}  // namespace ui
}  // namespace perception
