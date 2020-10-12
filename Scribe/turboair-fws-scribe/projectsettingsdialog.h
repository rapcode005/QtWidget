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
//#include "customJsonArray.h"

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

    //Set the add
    QJsonArray headerNames, preHeaderNames;
    QJsonArray headerValues, preHeaderValues;
    QJsonArray headerValuesChk, preHeaderValuesChk;

    QStringList preHeaderFieldsChk, preAdditionalFieldsChk;
    QStringList preHAFieldsCb, preAFieldsCb;

    int preHeaderCount, preAdditionalCount;

    //Set the additional field values
    QJsonArray additionalFieldNames, preAdditionalFieldNames;
    QJsonArray additionalFieldValues, preAdditionalFieldValues;
    QJsonArray additionalFieldValuesChk, preAdditionalFieldValuesChk;

    progressmessage *progMessage;
    bool message, loadHeaderAdditional, openingWindow = false;

    QStyleOptionHeader *opt;
    //void updateHeaders(const QString &item1, const QString &item2, const int &checkvalue);
    void PrepHeadFieldValues();
    //void PrepAdditionalFieldValues();
    //void updateAdditional(const QString &item1, const QString &item2, const int &checkvalue);
    void PrepAdditionalFieldValues(QTableWidget *sourceWidget);
    //void addCheckBoxAt(int row_number, int column_number,int state, QTableWidget *qtwidget);//added 4.21.2020
    void addCheckBoxAt(int row_number, int column_number,int state, QTableWidget *qtwidget);
    void on_buttonBox_clicked(QAbstractButton *button); //connect this to a button.

    bool eventFilter(QObject *target, QEvent *event);

    bool hasChangesMade();

    QJsonValue getValues(const QJsonValue &val) const;
    QStringList defaultUsedHotkeys;//added 7.24.2020
public slots:
    void GetSpeciesAndGrouping();
    //Without refresh the addiotional slot and signal
    void GetSpeciesAndGroupingWithoutSlot();
    void ProjectOpened();
    void disable_field_buttons();
    //void saveFile();

signals:
    void receivedLogDialogItems();
    void receivedLogDialogItemsWithoutSlot();
    void speciesDialogUpdate(QStringList items);
    void groupingsDialogUpdate(QStringList items);
    void refreshSignals();
    void saveButton();
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


    void on_pushButton_clicked();

private:

    Ui::ProjectSettingsDialog *ui;
    QRegExp rx;
    LogItemEditDialog *speciesDialog;
    LogItemEditDialog *groupingDialog;
    void doSave();//added 4.30.2020
    int count;
    QString changeFields;
};

#endif // PROJECTSETTINGSDIALOG_H
