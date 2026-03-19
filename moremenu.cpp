#include "moremenu.h"
#include "ui_moremenu.h"

MoreMenu::MoreMenu(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MoreMenu)
{
    ui->setupUi(this);
    setFixedSize(150, 50);

    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);

    connect(ui->addMusic, &QPushButton::clicked, this, &MoreMenu::addMusicClicked);
}

MoreMenu::~MoreMenu()
{
    delete ui;
}
