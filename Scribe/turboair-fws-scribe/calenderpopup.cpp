#include "calenderpopup.h"
#include "ui_calenderpopup.h"

CalenderPopup::CalenderPopup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CalenderPopup)
{
    ui->setupUi(this);
}

CalenderPopup::~CalenderPopup()
{
    delete ui;
}
