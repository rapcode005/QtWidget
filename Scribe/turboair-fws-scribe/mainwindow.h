#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QFile>
#include <QDir>
#include <QMediaPlayer>
#include <QtWidgets>
#include "filehandler.h"
#include "gpshandler.h"
#include "audioplayer.h"
#include "projectsettingsdialog.h"
#include "filestructureinfodialog.h"
#include "bookmark.h"
#include "additionalfieldsdialog.h"
#include "gpsrecording.h"
#include "exportdialog.h"
#include "projectsavefile.h"
#include "openingscreen.h"



namespace Ui {
class MainWindow;
}

/**
 *  mainwindow.cpp is deprecated/replaced by mainwindow2.cpp
 *  per Jason's commit # 3180a50
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public slots:
    void removeLog(QVariant index);
    void RecordingIndexChanged(int position);
    void RefreshItems();
    void FillSpeciesField(int item);
    void FillGroupingField(int item);
    void ExportASC();
    void AppendASC();
    void ItemsLoaded(QList<BookMark *> items);
    void AutoSave();
    void ProjectSettingsFinished();
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString BookmarksToAsc();
    QString startDir;
signals:
    void fileOpened();
private slots:

    void on_pushButton_clicked();

    void on_actionSave_As_triggered();

    void on_actionOpen_survey_triggered();

    void on_actionSave_project_triggered();

    void on_horizontalSlider_2_sliderMoved(int position);

    void on_next_clicked();

    void on_previous_clicked();

    void on_pushButton_10_clicked();

    void on_actionExport_triggered();

    void on_pushButton_3_clicked();

    bool eventFilter(QObject *watched, QEvent *event);
    void on_actionRecordings_List_triggered();

    void on_actionImport_Flight_Data_triggered();

    void on_actionAdditional_Fields_triggered();

    void on_actionProject_Settings_triggered();

    void on_checkBox_4_stateChanged(int arg1);

    void on_pushButton_2_clicked();

    void on_actionExit_triggered();

    void on_actionOpen_Project_triggered();

    void on_actionSave_project_2_triggered();

    void on_actionSave_project_as_triggered();

    void closeEvent (QCloseEvent *event);

    void on_verticalSlider_valueChanged(int value);

    void on_lineEdit_2_textChanged(const QString &arg1);

    void on_comboBox_2_currentTextChanged(const QString &arg1);

private:
    Ui::MainWindow    *ui;

    ProjectSettingsDialog          *proDialog;
    FileStructureInfoDialog        *fsiDialog;
    AdditionalFieldsDialog         *fieldDialog;
    OpeningScreenDialog            *openingScreen;


    LogTemplateItem *currentSpecies;
    QString                   ascFilename = "";
    //FileHandler            *fil;
    GpsHandler           *gps;
    AudioPlayer           *audio;
    QFile                       sourceFile;

    ProjectFile            *proj;
    ProjectSettings   *settings;

    ProjectSaveFile     *projectFile;

    QList<BookMark*>  bookmarks;

    //
    //  this is where the inport data is stored in memory
    //
    QList<GPSRecording*> recordings;


    int    currentGpsRecording;
    int    curCoord = 0;
    int    tickToUpdate = 2;
    int    tick = 0;
    QList<LogTemplateItem> addedFields;

    QList<LogTemplateItem*> groupings;
    QList<LogTemplateItem*> species;
    QActionGroup    *shortcutGroup;
    QSignalMapper  *specieskeyMapper;
    QSignalMapper  *groupingskeyMapper;

    ExportDialog        *exportDialog;

    QTimer *autoSaveTimer;

    void FillSpeciesTemplates(QList<LogTemplateItem*> items);
    void FillGroupingTemplates(QList<LogTemplateItem*> items);
    void ClearBookmarks();
    void CreateLogItemShortcuts();
};

#endif // MAINWINDOW_H
