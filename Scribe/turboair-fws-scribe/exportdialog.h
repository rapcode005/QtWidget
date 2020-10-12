#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QObject>
namespace Ui {
class ExportDialog;
}

class ExportDialog : public QDialog
{
    Q_OBJECT
signals:
    void Append();
public:
    explicit ExportDialog(QWidget *parent = nullptr);
    ~ExportDialog();
    void Setup(QString boxFilling);

private slots:
    void on_pushButton_clicked();

private:
    Ui::ExportDialog *ui;
};

#endif // EXPORTDIALOG_H
