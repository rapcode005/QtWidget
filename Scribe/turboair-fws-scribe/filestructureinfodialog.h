#ifndef FILESTRUCTUREINFODIALOG_H
#define FILESTRUCTUREINFODIALOG_H

#include <QDialog>
#include <QtWidgets>

#include "audioplayer.h"
namespace Ui {
class FileStructureInfoDialog;
}

class FileStructureInfoDialog : public QDialog
{
    Q_OBJECT

public:
    FileStructureInfoDialog(QWidget *parent = 0);
    ~FileStructureInfoDialog();
    void FillList(QStringList filelist);
    QListWidget *listWidget;
    AudioPlayer *audio;

private slots:
    void on_listWidget_doubleClicked(const QModelIndex &index);

private:
    Ui::FileStructureInfoDialog *ui;
};

#endif // FILESTRUCTUREINFODIALOG_H
