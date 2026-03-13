/********************************************************************************
** Form generated from reading UI file 'musicplaylist.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MUSICPLAYLIST_H
#define UI_MUSICPLAYLIST_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MusicPlaylist
{
public:
    QVBoxLayout *verticalLayout_2;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout;
    QWidget *widget;
    QVBoxLayout *verticalLayout_3;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *MusicPlaylist)
    {
        if (MusicPlaylist->objectName().isEmpty())
            MusicPlaylist->setObjectName("MusicPlaylist");
        MusicPlaylist->resize(400, 300);
        verticalLayout_2 = new QVBoxLayout(MusicPlaylist);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        scrollArea = new QScrollArea(MusicPlaylist);
        scrollArea->setObjectName("scrollArea");
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 398, 298));
        verticalLayout = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout->setObjectName("verticalLayout");
        widget = new QWidget(scrollAreaWidgetContents);
        widget->setObjectName("widget");
        verticalLayout_3 = new QVBoxLayout(widget);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);


        verticalLayout->addWidget(widget);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout_2->addWidget(scrollArea);


        retranslateUi(MusicPlaylist);

        QMetaObject::connectSlotsByName(MusicPlaylist);
    } // setupUi

    void retranslateUi(QWidget *MusicPlaylist)
    {
        MusicPlaylist->setWindowTitle(QCoreApplication::translate("MusicPlaylist", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MusicPlaylist: public Ui_MusicPlaylist {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MUSICPLAYLIST_H
