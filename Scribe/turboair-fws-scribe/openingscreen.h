#ifndef OPENINGSCREENDIALOG_H
#define OPENINGSCREENDIALOG_H

#include <QMainWindow>
#include <QAbstractButton>
#include "projectfile.h"
#include "projectsettings.h"
#include <customevent.h>
#include "messagebox.h"
#include "progressmessage.h"
#include "workingThread.h"

namespace Ui {
class OpeningScreenDialog;
}

class OpeningScreenDialog : public QMainWindow
{
    Q_OBJECT

public:

    OpeningScreenDialog(QWidget *parent, ProjectFile *pro = nullptr);
    ~OpeningScreenDialog();
    void closeEvent (QCloseEvent *event);

    QString                    dirPath;
    QString                   fileName;
    QString                settingsDir;
    QStringList       settingFileNames;
    QString                   startDir;
    ProjectSettings   *settings;
    ProjectFile            *proj;
    //MessageDialog  *progMessage;
    progressmessage *progMessage;

    bool clearToClose = false, secondStage = false;

    void frmInit(bool hasSetting, QString settingFile);//added 8.26.2020
    QSettings *scribeSettings;//added 8.26.2020
    void clearControls();//added 8.26.2020

    void setSystemStatusNew(const bool &boolStatus);//added 8.26.2020
    bool getSystemStatusNew() const;//added 8.26.2020

    void refreshControls();

signals:
    void PopulateHeaderFields();
    void CreateExportFile();
    void ImportFlightData(QString sourceFlightDataDirectoryPath, bool copyfiles);
    void closeProgram();
    //GPS Lock
    void widgetStatus(const bool &status);
    //Second Load
    void secondLoadReady(const bool &hasFlightData, const QString &existingFlightDataPath);
    //Use for insert Button in Main Window
    //void insertButtonSurvey(const QString &survey, const int &value);

public slots:
    void firstLoadDone(const bool &hasFlightData, const QString &existingFlightDataPath);

private slots:
    void on_saveButton_clicked();

    void on_observerInitials_textChanged(const QString &arg1);

    void on_exportFilename_textChanged(const QString &arg1);

    void on_exportFilename_cursorPositionChanged(int arg1, int arg2);

    void on_surveyType_currentIndexChanged(int index);

    void on_rdoYes_clicked();

    void on_rdoNo_clicked();

    void action_openProjectDir(bool checked = false);

    void on_rdoRecent_clicked();

    void on_rdoNew_clicked();

private:
    Ui::OpeningScreenDialog *ui;

    int saveTextCursor = 0;
    int saveTextValueLength = 0;

    void addJsonFile(const QString &survey);
    void addJsonFile(const QString &survey, const QString airGround);

    void selectSurveyWBPHS();
    void selectSurveyEagle();
    void selectSurveyOther();

    CustomEvent *customEvent;
    CustomEvent *surveryEvent;
    CustomEvent *airGroundEvent;

    QString autoUnit = "no";

    bool status;

    void directoryText(const bool &status);

    QString currentSurvey();

    bool m_statusNew;

    //QList<int> surveyInt;
};
#endif // OPENINGSCREENDIALOG_H




