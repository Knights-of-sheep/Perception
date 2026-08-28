// ===== 子窗口视图交互单测（004-dock-layout-manager）=====
// 覆盖：标题栏按钮等高等宽（用户需求）、关闭按钮（FR-022）、
// 点击任意区域触发 selected（Assumptions：选中的子窗口，边框高亮的机制层）。
// 需要 GUI 平台（QApplication + QTest），在 Windows 桌面会话下运行。
#include "ui/subwindow/subwindow_view.h"

#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include <QToolButton>
#include <QWidget>

#include <cstdio>
#include <QStringList>

using perception::ui::SubwindowView;

namespace {
const QStringList kBtnNames = {
    QStringLiteral("subwindowMaxBtn"),     QStringLiteral("subwindowHideBtn"),
    QStringLiteral("subwindowRestoreBtn"), QStringLiteral("subwindowPrevBtn"),
    QStringLiteral("subwindowNextBtn"),    QStringLiteral("subwindowCloseBtn"),
};
}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    SubwindowView view(QStringLiteral("plot_1"));
    view.show();
    view.resize(300, 200);
    QCoreApplication::processEvents();

    // ---- 关闭按钮存在且始终可见（FR-022）----
    auto* closeBtn = view.findChild<QToolButton*>(QStringLiteral("subwindowCloseBtn"));
    if (!closeBtn || !closeBtn->isVisible()) {
        fprintf(stderr, "close button missing or hidden\n");
        return 1;
    }

    // ---- 六个标题栏按钮等高等宽（统一尺寸）----
    QSize refSize;
    for (const QString& name : kBtnNames) {
        auto* btn = view.findChild<QToolButton*>(name);
        if (!btn) {
            fprintf(stderr, "button %s missing\n", qPrintable(name));
            return 1;
        }
        if (refSize.isValid() && btn->size() != refSize) {
            fprintf(stderr, "button %s size %dx%d != ref %dx%d\n", qPrintable(name),
                    btn->width(), btn->height(), refSize.width(), refSize.height());
            return 1;
        }
        refSize = btn->size();
    }

    // ---- 点击内容区 -> selected 信号（选中机制）----
    {
        QSignalSpy spy(&view, &SubwindowView::selected);
        auto* content = view.findChild<QWidget*>(QStringLiteral("subwindowContent"));
        if (!content) {
            fprintf(stderr, "content widget missing\n");
            return 1;
        }
        QTest::mouseClick(content, Qt::LeftButton);
        if (spy.count() < 1) {
            fprintf(stderr, "clicking content did not emit selected\n");
            return 1;
        }
    }
    // ---- 点击标题栏空白 -> selected（整窗可选中）----
    {
        QSignalSpy spy(&view, &SubwindowView::selected);
        auto* titleBar = view.findChild<QWidget*>(QStringLiteral("subwindowTitleBar"));
        if (!titleBar) {
            fprintf(stderr, "title bar widget missing\n");
            return 1;
        }
        QTest::mouseClick(titleBar, Qt::LeftButton, Qt::NoModifier, QPoint(6, 13));
        if (spy.count() < 1) {
            fprintf(stderr, "clicking title bar did not emit selected\n");
            return 1;
        }
    }
    // ---- 点击关闭按钮 -> closeRequested 且不触发选中 ----
    {
        QSignalSpy closeSpy(&view, &SubwindowView::closeRequested);
        QSignalSpy selSpy(&view, &SubwindowView::selected);
        QTest::mouseClick(closeBtn, Qt::LeftButton);
        if (closeSpy.count() != 1) {
            fprintf(stderr, "clicking close button did not emit closeRequested\n");
            return 1;
        }
        if (selSpy.count() != 0) {
            fprintf(stderr, "clicking close button unexpectedly emitted selected\n");
            return 1;
        }
    }

    puts("subwindow_view_test: ALL PASS");
    return 0;
}
