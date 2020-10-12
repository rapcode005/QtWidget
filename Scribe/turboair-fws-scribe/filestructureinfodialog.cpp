#include "filestructureinfodialog.h"
#include "ui_filestructureinfodialog.h"
//shows a list of audio files on the left
FileStructureInfoDialog::FileStructureInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileStructureInfoDialog)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    listWidget = ui->listWidget;
    ui->listWidget->clear();
    setWindowModality(Qt::NonModal);
    //setWindowFlag(Qt::WindowStaysOnBottomHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

FileStructureInfoDialog::~FileStructureInfoDialog()
{
    delete ui;
}
void FileStructureInfoDialog::FillList(QStringList filelist){
    //fill up list with strings of filenames
    ui->listWidget->addItems(filelist);
}

void FileStructureInfoDialog::on_listWidget_doubleClicked(const QModelIndex &index)
{
    //when we click a list item, tell our audioplayer to update what file it's playing
    audio->fileList->setCurrentIndex(index.row());
}
