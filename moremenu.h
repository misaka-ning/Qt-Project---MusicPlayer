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
    void setLoginState(bool loggedIn, const QString &nickname = QString());
    bool isLoggedIn() const { return m_loggedIn; }

signals:
    void addMusicClicked();
    void removeCurrentSongClicked();
    void authNeteaseClicked(bool loggedIn);

private:
    Ui::MoreMenu *ui;
    bool m_loggedIn{false};
};

#endif // MOREMENU_H
