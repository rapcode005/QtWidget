#include "exportdialog.h"
#include "ui_exportdialog.h"
#include <QObject>
//old, mostly just so statiscians don't get bent out of shape
ExportDialog::ExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

ExportDialog::~ExportDialog()
{
    delete ui;
}
void ExportDialog::Setup(QString boxFilling){
    //if we have something to preview, show it, otherwise show no data
    if (!boxFilling.isEmpty()){
        ui->textEdit->setText(boxFilling);
    }else{
        ui->textEdit->setText("No Data!");
    }
}

void ExportDialog::on_pushButton_clicked()
{
    //send a signal to our main window to
    emit Append();
    hide();
}
