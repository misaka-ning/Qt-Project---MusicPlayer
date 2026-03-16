/********************************************************************************
** Form generated from reading UI file 'songunit.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SONGUNIT_H
#define UI_SONGUNIT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SongUnit
{
public:
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer;
    QLabel *imagelabel;
    QVBoxLayout *verticalLayout;
    QLabel *namelabel;
    QLabel *artistlabel;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QWidget *SongUnit)
    {
        if (SongUnit->objectName().isEmpty())
            SongUnit->setObjectName("SongUnit");
        SongUnit->resize(720, 82);
        gridLayout_2 = new QGridLayout(SongUnit);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer, 0, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(20, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 1, 0, 1, 1);

        imagelabel = new QLabel(SongUnit);
        imagelabel->setObjectName("imagelabel");
        imagelabel->setMinimumSize(QSize(50, 50));
        imagelabel->setMaximumSize(QSize(50, 50));

        gridLayout->addWidget(imagelabel, 1, 1, 1, 1);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        namelabel = new QLabel(SongUnit);
        namelabel->setObjectName("namelabel");

        verticalLayout->addWidget(namelabel);

        artistlabel = new QLabel(SongUnit);
        artistlabel->setObjectName("artistlabel");

        verticalLayout->addWidget(artistlabel);


        gridLayout->addLayout(verticalLayout, 1, 2, 1, 1);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(verticalSpacer_2, 2, 1, 1, 1);


        gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);


        retranslateUi(SongUnit);

        QMetaObject::connectSlotsByName(SongUnit);
    } // setupUi

    void retranslateUi(QWidget *SongUnit)
    {
        SongUnit->setWindowTitle(QCoreApplication::translate("SongUnit", "Form", nullptr));
        imagelabel->setText(QString());
        namelabel->setText(QCoreApplication::translate("SongUnit", "\351\237\263\344\271\220\345\220\215", nullptr));
        artistlabel->setText(QCoreApplication::translate("SongUnit", "\344\275\234\346\233\262\345\256\266", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SongUnit: public Ui_SongUnit {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SONGUNIT_H
