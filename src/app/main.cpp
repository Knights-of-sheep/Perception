#include <QApplication>
#include <QMessageBox>

// M0 骨架：仅验证工具链与构建流程。
// M4 端到端 MVP 时将替换为真实主窗口（src/ui）。
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMessageBox::information(nullptr, "Perception", "M0 skeleton");
    return app.exec();
}
