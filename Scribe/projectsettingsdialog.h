#ifndef PROJECTSETTINGSDIALOG_H
#define PROJECTSETTINGSDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include "projectfile.h"
#include "projectsettings.h"
#include "logitemeditdialog.h"
#include <QTableWidget>
#include <QCloseEvent>
#include "docLayout.h"
#include <QCheckBox>
#include "projectsettingobj.h"
#include "progressmessage.h"

namespace Ui {
    class ProjectSettingsDialog;
}

class ProjectSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    ProjectSettingsDialog(QWidget *parent = nullptr, ProjectFile *pro = nullptr);
    ~ProjectSettingsDialog();

    QString         dirPath;
    QString         fileName;
    ProjectFile            *proj;
    QString             startDir;
    QPushButton     *activeButton;

    QList<LogTemplateItem*> speciesItems;
    QList<LogTemplateItem*> groupingItems;
    QTableWidget *sourceWidget;

    progressmessage *progMessage;
    bool message = false;

    void PrepHeadFieldValues();
    void PrepAdditionalFieldValues();
    //void PrepAdditionalFieldValues(QTableWidget *sourceWidget);
    //void addCheckBoxAt(int row_number, int column_number,int state, QTableWidget *qtwidget);//added 4.21.2020
    void addCheckBoxAt(int row_number, int column_number,int state, QTableWidget *qtwidget);
    void on_buttonBox_clicked(QAbstractButton *button); //connect this to a button.

public slots:
    void GetSpeciesAndGrouping();
    //Without refresh the addiotional slot and signal
    void GetSpeciesAndGroupingWithoutSlot();
    void ProjectOpened();
    void disable_field_buttons();

signals:
    void receivedLogDialogItems();
    void receivedLogDialogItemsWithoutSlot();
    void speciesDialogUpdate(QStringList items);
    void groupingsDialogUpdate(QStringList items);
    void refreshSignals();
    //Thread for DoSave
    void sendHeaderValues(const QStringList &vals, const int &curindex);
    void sendAddedValues(const QStringList &vals,  const int &curindex);
    //void receivedClicked(const int &index);

private slots:

    void on_addField_pressed();
    void on_removeField_pressed();
    void on_addField_2_pressed();
    void on_removeField_2_pressed();
    void on_pushButton_pressed();
    void on_pushButton_2_pressed();
    void on_buttonBox_clicked();
    void closeEvent(QCloseEvent *event);


private:

    Ui::ProjectSettingsDialog *ui;
    QRegExp rx;
    LogItemEditDialog *speciesDialog;
    LogItemEditDialog *groupingDialog;
    void doSave();//added 4.30.2020
    int count;
    //QList<QCheckBox*> chkOberservation;
    //void on_observationHeader_clicked(const int &index);
};

#endif // PROJECTSETTINGSDIALOG_H
