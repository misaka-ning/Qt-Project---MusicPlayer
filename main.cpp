#include "mainwindow.h"

#include <QApplication>
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/res/misaka.png"));

    MainWindow w;
    w.show();
    return a.exec();
}
