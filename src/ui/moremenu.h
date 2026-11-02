#ifndef MOREMENU_H
#define MOREMENU_H

#include <QWidget>
#include <QString>

namespace Ui {
class MoreMenu;
}

class MoreMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MoreMenu(QWidget *parent = nullptr);
    ~MoreMenu();
    // 根据登录态刷新菜单中的文案显示。
    void setLoginState(bool loggedIn, const QString &nickname = QString());
    bool isLoggedIn() const { return m_loggedIn; }

signals:
    // 打开本地文件添加音乐。
    void addMusicClicked();
    // 从播放列表移除当前曲目。
    void removeCurrentSongClicked();
    // 登录按钮点击，参数表示当前是否已登录（用于决定登录/退出）。
    void authNeteaseClicked(bool loggedIn);

private:
    Ui::MoreMenu *ui;
    bool m_loggedIn{false};
};

#endif // MOREMENU_H
