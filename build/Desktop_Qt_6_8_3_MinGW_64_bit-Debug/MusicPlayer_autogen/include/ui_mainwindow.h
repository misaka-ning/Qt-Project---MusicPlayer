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
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
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
    QGridLayout *gridLayout_2;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer_5;
    QLabel *imagelabel;
    QSpacerItem *horizontalSpacer_4;
    QSpacerItem *verticalSpacer_2;
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
        gridLayout_2 = new QGridLayout(centralwidget);
        gridLayout_2->setObjectName("gridLayout_2");
        verticalSpacer = new QSpacerItem(20, 213, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 0, 1, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(90, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_5, 1, 0, 1, 1);

        imagelabel = new QLabel(centralwidget);
        imagelabel->setObjectName("imagelabel");

        gridLayout_2->addWidget(imagelabel, 1, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(764, 20, QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_4, 1, 2, 1, 1);

        verticalSpacer_2 = new QSpacerItem(20, 213, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_2->addItem(verticalSpacer_2, 2, 1, 1, 1);

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


        gridLayout_2->addWidget(Controlwidget, 3, 0, 1, 3);

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
        imagelabel->setText(QString());
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
