#include "moremenu.h"
#include "ui_moremenu.h"

MoreMenu::MoreMenu(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MoreMenu)
{
    ui->setupUi(this);
    setFixedSize(190, 176);

    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);

    connect(ui->addMusic, &QPushButton::clicked, this, &MoreMenu::addMusicClicked);
    connect(ui->removeCurrentSong, &QPushButton::clicked, this, &MoreMenu::removeCurrentSongClicked);
    // 将当前登录态透传给上层，统一处理“登录/退出登录”动作。
    connect(ui->authNetease, &QPushButton::clicked, this, [this]() {
        emit authNeteaseClicked(m_loggedIn);
    });
    setLoginState(false, QString());
}

MoreMenu::~MoreMenu()
{
    delete ui;
}

void MoreMenu::setLoginState(bool loggedIn, const QString &nickname)
{
    if (!ui || !ui->loginStatusLabel) return;
    m_loggedIn = loggedIn;
    // 未登录或昵称为空时显示“未登录”，避免显示空白。
    const QString shown = (!loggedIn || nickname.trimmed().isEmpty()) ? QStringLiteral("未登录") : nickname.trimmed();
    ui->loginStatusLabel->setText(shown);
    if (ui->authNetease) {
        ui->authNetease->setText(loggedIn ? QStringLiteral("退出登录") : QStringLiteral("登录网易云"));
    }
}
