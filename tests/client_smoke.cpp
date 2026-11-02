// Exercise compiled resources and the real MainWindow without media or accounts.
#include "ui/mainwindow.h"
#include <QApplication>
#include <QDirIterator>
#include <QFile>
#include <QImage>
#include <QTimer>
#include <cstdio>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QDirIterator files(":/res", QDir::Files, QDirIterator::Subdirectories);
    int images = 0;
    while (files.hasNext()) {
        const QString path = files.next();
        if (QImage(path).isNull()) {
            std::fprintf(stderr, "Cannot decode resource: %s\n", qPrintable(path));
            return 1;
        }
        ++images;
    }
    QFile style(":/style.qss");
    if (images != 22 || !style.open(QIODevice::ReadOnly) || style.readAll().isEmpty()) {
        std::fprintf(stderr, "Missing images or stylesheet\n");
        return 1;
    }
    MainWindow window;
    window.show();
    QTimer::singleShot(300, &app, [&]() {
        app.exit(window.isVisible() && window.size().isValid() ? 0 : 1);
    });
    return app.exec();
}
