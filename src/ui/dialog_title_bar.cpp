#include "ui/dialog_title_bar.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QToolButton>

#include "ui/win_btn_icon.h"

namespace perception {
namespace ui {

// ---- 通用对话框标题栏工厂（FramelessDialog / ThemedFileDialog /
// LayoutSettingsDialog 共享） ----
// 复用 QSS objectName (titleBarRow/titleBarTitle/titleBarIcon/winCloseBtn) 与
// makeWinBtnIcon 图标，保证所有无边框窗口标题栏外观一致、随主题。
QWidget* buildDialogTitleBar(QDialog* owner, const QString& title) {
    auto* bar = new QWidget(owner);
    bar->setObjectName(QStringLiteral("titleBarRow"));
    bar->setFixedHeight(30);
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(6);

    auto* icon = new QLabel(bar);
    icon->setObjectName(QStringLiteral("titleBarIcon"));
    icon->setPixmap(QPixmap(QStringLiteral(":/perception/icons/icons/png/app/app-icon-24.png")));
    layout->addWidget(icon);

    auto* titleLabel = new QLabel(bar);
    titleLabel->setObjectName(QStringLiteral("titleBarTitle"));
    titleLabel->setText(title);
    layout->addWidget(titleLabel);
    layout->addStretch();

    auto* closeBtn = new QToolButton(bar);
    closeBtn->setObjectName(QStringLiteral("winCloseBtn"));
    closeBtn->setFixedSize(40, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setAutoRaise(true);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setIconSize(QSize(16, 16));
    closeBtn->setIcon(makeWinBtnIcon(WinBtnKind::Close, bar->palette()));
    closeBtn->setToolTip(QObject::tr("Close"));
    QObject::connect(closeBtn, &QToolButton::clicked, owner, [owner] { owner->close(); });
    layout->addWidget(closeBtn);

    return bar;
}

}  // namespace ui
}  // namespace perception
