#ifndef LOGITEMEDITDIALOG_H
#define LOGITEMEDITDIALOG_H

#include <QDialog>
#include <QList>
#include "logtemplateitem.h"
#include <QDebug>
#include <QTableWidgetItem>

#include "defaulthotkeys.h"
namespace Ui {
class LogItemEditDialog;
}

/**
 *  Let's deprecate this file
 *
 */
class LogItemEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogItemEditDialog(QWidget *parent = nullptr,
                               QString windowTitle = "No Title",
                               bool useCheck = false,
                               QString title = "");
    ~LogItemEditDialog();
    QList<LogTemplateItem*> logItems;
    QString fields;
    bool openNew;
    bool duplicate = false;
    void loadCurrentList(const QStringList &n);
    void RefreshItems(const QStringList &items);
    void retainPreItems(const QStringList &items);
    void loadOtherItems(const QStringList &n);//added 7.23.2020
    QStringList defaultUsedHotkeys;
    QStringList preItems;

public slots:
    void ReceiveItems(const QStringList &items);
    void reject();
    void OnEditingFinished();

signals:
    void Done();
    void Loaded();
    void Rejected();
private:
    Ui::LogItemEditDialog *ui;
    bool                useCheckBox;
    QRegExp             rx;
    QList<QKeySequenceEdit*> edits;
    QTimer *hotKeyTimer;
    void on_buttonBox_accepted(); //This is seems doesn't to be in slot.
    bool hasChangeMade();

    QStringList otherItems;//added 7.23.2020
    void validateItems(); //added 7.23.2020
    DefaultHotKeys *defaultKeysDlg;

private slots:
    void AddItem(QString name, QString shortcut = "", bool useGrouping = false);
    void RemoveRowItem(int row);
    void on_pushButton_pressed();
    //void on_buttonBox_accepted();

    void on_pushButton_2_pressed();
    void StopRecording(QKeySequence key);
    void on_tableWidget_viewportEntered();
    void CheckEdititing();
    void onSave();//added 5.3.2020

    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    void on_buttonBox_clicked();//added 5.3.2020
    void on_buttonBox_pressed();
    void on_btnrefreshlist_pressed();
    void on_btndefaultlist_pressed();
};

#endif // LOGITEMEDITDIALOG_H
