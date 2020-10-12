#include "defaulthotkeys.h"
#include "ui_defaulthotkeys.h"

DefaultHotKeys::DefaultHotKeys(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DefaultHotKeys)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    ui->setupUi(this);

}

DefaultHotKeys::~DefaultHotKeys()
{
    delete ui;
}

void DefaultHotKeys::initForm()
{
    ui->listWidget->clear();
    if(defaultUsedHotkeys.length() > 0)
        ui->listWidget->addItems(defaultUsedHotkeys);
}
