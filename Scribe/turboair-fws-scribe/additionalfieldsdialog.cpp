#include "additionalfieldsdialog.h"
#include "ui_additionalfieldsdialog.h"
#include <QtWidgets>
AdditionalFieldsDialog::AdditionalFieldsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdditionalFieldsDialog)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    setWindowModality(Qt::NonModal);
    //setWindowFlag(Qt::WindowStaysOnBottomHint);
    table = ui->additionalfieldsTable;
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

}
AdditionalFieldsDialog::~AdditionalFieldsDialog()
{
    delete ui;
}
