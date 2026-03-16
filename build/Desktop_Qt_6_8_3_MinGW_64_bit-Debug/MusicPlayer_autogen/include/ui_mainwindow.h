/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "marqueelabel.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout_5;
    QGridLayout *gridLayout_4;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_8;
    QPushButton *minimizeButton;
    QPushButton *maximizeButton;
    QPushButton *closeButton;
    QSpacerItem *horizontalSpacer_9;
    QGridLayout *gridLayout_3;
    QSpacerItem *verticalSpacer;
    QWidget *Lyricswidget;
    QGridLayout *gridLayout_2;
    QLabel *songnamelabel;
    QListWidget *lyricsListWidget;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *horizontalSpacer;
    QLabel *imagelabel;
    QSpacerItem *horizontalSpacer_4;
    QSpacerItem *verticalSpacer_2;
    QSpacerItem *horizontalSpacer_10;
    QVBoxLayout *verticalLayout_4;
    QSlider *Slider;
    QWidget *Controlwidget;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_3;
    QVBoxLayout *verticalLayout;
    MarqueeLabel *namelabel;
    MarqueeLabel *artistlabel;
    QWidget *widget;
    QGridLayout *gridLayout;
    QSpacerItem *leftSpacer;
    QHBoxLayout *horizontalLayout;
    QPushButton *modeButton;
    QPushButton *prevButton;
    QPushButton *playButton;
    QPushButton *nextButton;
    QPushButton *listButton;
    QSpacerItem *rightSpacer;
    QVBoxLayout *verticalLayout_2;
    MarqueeLabel *artistlabel_2;
    QSpacerItem *horizontalSpacer_6;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1060, 760);
        MainWindow->setMinimumSize(QSize(1060, 760));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout_5 = new QGridLayout(centralwidget);
        gridLayout_5->setObjectName("gridLayout_5");
        gridLayout_4 = new QGridLayout();
        gridLayout_4->setObjectName("gridLayout_4");
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_8);

        minimizeButton = new QPushButton(centralwidget);
        minimizeButton->setObjectName("minimizeButton");

        horizontalLayout_3->addWidget(minimizeButton);

        maximizeButton = new QPushButton(centralwidget);
        maximizeButton->setObjectName("maximizeButton");

        horizontalLayout_3->addWidget(maximizeButton);

        closeButton = new QPushButton(centralwidget);
        closeButton->setObjectName("closeButton");
        closeButton->setMaximumSize(QSize(2000, 2000));

        horizontalLayout_3->addWidget(closeButton);

        horizontalSpacer_9 = new QSpacerItem(20, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_9);


        gridLayout_4->addLayout(horizontalLayout_3, 0, 0, 1, 1);

        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName("gridLayout_3");
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(verticalSpacer, 0, 1, 1, 1);

        Lyricswidget = new QWidget(centralwidget);
        Lyricswidget->setObjectName("Lyricswidget");
        Lyricswidget->setMinimumSize(QSize(500, 0));
        gridLayout_2 = new QGridLayout(Lyricswidget);
        gridLayout_2->setObjectName("gridLayout_2");
        songnamelabel = new QLabel(Lyricswidget);
        songnamelabel->setObjectName("songnamelabel");
        songnamelabel->setMinimumSize(QSize(150, 0));
        QFont font;
        font.setFamilies({QString::fromUtf8("Cooper")});
        font.setPointSize(21);
        songnamelabel->setFont(font);

        gridLayout_2->addWidget(songnamelabel, 0, 1, 1, 1);

        lyricsListWidget = new QListWidget(Lyricswidget);
        lyricsListWidget->setObjectName("lyricsListWidget");
        lyricsListWidget->setMinimumSize(QSize(500, 0));

        gridLayout_2->addWidget(lyricsListWidget, 1, 0, 1, 3);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_2, 0, 0, 1, 1);

        horizontalSpacer_7 = new QSpacerItem(292, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_7, 0, 2, 1, 1);


        gridLayout_3->addWidget(Lyricswidget, 0, 3, 3, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 1, 0, 1, 1);

        imagelabel = new QLabel(centralwidget);
        imagelabel->setObjectName("imagelabel");

        gridLayout_3->addWidget(imagelabel, 1, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_4, 1, 2, 1, 1);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(verticalSpacer_2, 2, 1, 1, 1);

        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_10, 1, 4, 1, 1);


        gridLayout_4->addLayout(gridLayout_3, 1, 0, 1, 1);


        gridLayout_5->addLayout(gridLayout_4, 0, 0, 1, 1);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName("verticalLayout_4");
        Slider = new QSlider(centralwidget);
        Slider->setObjectName("Slider");
        Slider->setOrientation(Qt::Orientation::Horizontal);

        verticalLayout_4->addWidget(Slider);

        Controlwidget = new QWidget(centralwidget);
        Controlwidget->setObjectName("Controlwidget");
        Controlwidget->setMinimumSize(QSize(0, 100));
        Controlwidget->setMaximumSize(QSize(16777215, 100));
        horizontalLayout_2 = new QHBoxLayout(Controlwidget);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalSpacer_3 = new QSpacerItem(20, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        namelabel = new MarqueeLabel(Controlwidget);
        namelabel->setObjectName("namelabel");
        namelabel->setMinimumSize(QSize(200, 0));
        namelabel->setMaximumSize(QSize(200, 16777215));

        verticalLayout->addWidget(namelabel);

        artistlabel = new MarqueeLabel(Controlwidget);
        artistlabel->setObjectName("artistlabel");
        artistlabel->setMinimumSize(QSize(200, 0));
        artistlabel->setMaximumSize(QSize(200, 16777215));

        verticalLayout->addWidget(artistlabel);


        horizontalLayout_2->addLayout(verticalLayout);

        widget = new QWidget(Controlwidget);
        widget->setObjectName("widget");
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName("gridLayout");
        leftSpacer = new QSpacerItem(100, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(leftSpacer, 0, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        modeButton = new QPushButton(widget);
        modeButton->setObjectName("modeButton");

        horizontalLayout->addWidget(modeButton);

        prevButton = new QPushButton(widget);
        prevButton->setObjectName("prevButton");

        horizontalLayout->addWidget(prevButton);

        playButton = new QPushButton(widget);
        playButton->setObjectName("playButton");

        horizontalLayout->addWidget(playButton);

        nextButton = new QPushButton(widget);
        nextButton->setObjectName("nextButton");

        horizontalLayout->addWidget(nextButton);

        listButton = new QPushButton(widget);
        listButton->setObjectName("listButton");

        horizontalLayout->addWidget(listButton);

        horizontalLayout->setStretch(2, 2);
        horizontalLayout->setStretch(4, 1);

        gridLayout->addLayout(horizontalLayout, 0, 1, 1, 1);

        rightSpacer = new QSpacerItem(180, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(rightSpacer, 0, 2, 1, 1);


        horizontalLayout_2->addWidget(widget);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        artistlabel_2 = new MarqueeLabel(Controlwidget);
        artistlabel_2->setObjectName("artistlabel_2");
        artistlabel_2->setMinimumSize(QSize(200, 0));
        artistlabel_2->setMaximumSize(QSize(200, 16777215));

        verticalLayout_2->addWidget(artistlabel_2);


        horizontalLayout_2->addLayout(verticalLayout_2);

        horizontalSpacer_6 = new QSpacerItem(20, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_6);


        verticalLayout_4->addWidget(Controlwidget);


        gridLayout_5->addLayout(verticalLayout_4, 1, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        minimizeButton->setText(QString());
        maximizeButton->setText(QString());
        closeButton->setText(QString());
        songnamelabel->setText(QCoreApplication::translate("MainWindow", "\350\277\231\346\230\257\346\255\214\346\233\262\345\220\215 ", nullptr));
        imagelabel->setText(QCoreApplication::translate("MainWindow", "\345\233\276\347\211\207", nullptr));
        namelabel->setText(QCoreApplication::translate("MainWindow", "\351\237\263\344\271\220\345\220\215", nullptr));
        artistlabel->setText(QCoreApplication::translate("MainWindow", "\344\275\234\346\233\262\345\256\266", nullptr));
        modeButton->setText(QString());
        prevButton->setText(QString());
        playButton->setText(QString());
        nextButton->setText(QString());
        listButton->setText(QString());
        artistlabel_2->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
