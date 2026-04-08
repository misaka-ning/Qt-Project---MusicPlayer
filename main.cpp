#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

/** @brief 程序入口：创建 QApplication、设置窗口图标、显示主窗口并进入事件循环。 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QIcon appIcon(":/res/misaka.png");
    app.setWindowIcon(appIcon);

    MainWindow w;
    w.show();
    return app.exec();
}
