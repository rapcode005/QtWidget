#ifndef MAINWINDOW2_H
#define MAINWINDOW2_H

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
#include "openingscreen.h"

namespace Ui {
class MainWindow2;
}

class MainWindow2 : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow2(QWidget *parent = nullptr);
    ~MainWindow2();

    QString startDir;

public slots:
    void RecordingIndexChanged(int position);
    void RefreshItems();
    void FillSpeciesField(int item);
    void FillGroupingField(int item);
    void AppendASC();
    void AutoSave();
    void ProjectSettingsFinished();
    void PopulateHeaderFields();
    void CreateExportFile();
    void BuildBookmarksTableColumns();
    void updateExportHeaders();
    void ImportFlightData(QString flightDataDirectoryPath, bool copyfiles);
    void closeProgram();

signals:
    void fileOpened();

private:
    Ui::MainWindow2 *ui;
    QDialog                         *about;

    ProjectSettingsDialog          *projSettingsDialog;
    FileStructureInfoDialog        *fsiDialog;
    AdditionalFieldsDialog         *fieldDialog;
    OpeningScreenDialog            *openingScreen;    

    LogTemplateItem *currentSpecies;
    QString ascFilename = "";
    GpsHandler *gps;
    AudioPlayer *audio;
    QFile sourceFile;
    ProjectFile *proj;
    int LockedGPSIndex = -1;
    bool insertWasLastEvent = false;
    QString renameFile(const QString &fileInfo, const QString &directoryInfo); //use to rename wav and gps files.

    //
    //  this is where the inport data is stored in memory
    //
    QList<GPSRecording*> recordings;


    int    currentGpsRecording;
    int    curCoord = 0;
    int    tickToUpdate = 2;
    int    tick = 0;
    QList<LogTemplateItem> addedAdditionalFields;

    QList<LogTemplateItem*> groupings;
    QList<LogTemplateItem*> species;
    QActionGroup    *shortcutGroup;
    QSignalMapper  *specieskeyMapper;
    QSignalMapper  *groupingskeyMapper;

    ExportDialog        *exportDialog;

    QTimer *autoSaveTimer;

    QStringList allColumnNames;
    QStringList exportHeaders;

    QStringList required_columns_1 = {"Species","Quantity","Grouping"};
    QStringList required_columns_eagle = {"Species","Quantity"};
    QStringList required_columns_2 = {"Latitude", "Longitude", "Time", "Altitude", "Speed", "Course", "# Satellites", "HDOP", "Audio File"};

    QStringList surveyJsonSegment;//added 5.4.2020 //id,segment,stratum,transect,version,p1lon,p1lat,p2lon,p2lat
    QList<QStringList> surveyJsonSegmentList;//added 5.4.2020
    QStringList airGroundSurveyJsonSegment;//added 5.4.2020
    QList<QStringList> airGroundSurveyJsonSegmentList;//added 5.4.2020

    void FillSpeciesTemplates(QList<LogTemplateItem*> items);
    void FillGroupingTemplates(QList<LogTemplateItem*> items);
    //void ClearBookmarks();
    void CreateLogItemShortcuts();
    bool eventFilter(QObject *target, QEvent *event);

private slots:
    void refreshMap();//added 4.29.2020;
    void resizeEvent(QResizeEvent *event);//added 4.29.2020;
    void closeEvent(QCloseEvent *event);
    void on_actionExport_Survey_Template_triggered();
    void on_actionExit_triggered();
    void on_actionImport_Flight_Data_triggered();
    void on_actionExport_triggered();
    void on_actionOpen_Project_triggered();
    void on_actionImport_Survey_Template_triggered();
    void on_actionAdditional_Fields_triggered();
    void on_actionSpecies_Items_triggered();
    void on_actionSocial_Groupings_triggered();
    void on_actionAbout_Scribe_triggered();
    void on_actionSave_triggered();
    void on_actionLegacy_Project_Settings_triggered();
    void on_Back_clicked();
    void on_quantity_textChanged(const QString &arg1);
    void on_comboBox_2_currentTextChanged(const QString &arg1);
    void on_Play_toggled(bool checked);
    void on_Next_clicked();
    void handleBackTick();
    void on_Volume_sliderMoved(int position);
    void on_wavFileList_currentIndexChanged(int index);
    void on_insertButton_clicked();
    void on_HeaderFields_cellChanged(int row, int column);
    void repeatTrack();
    void on_deleteButton_clicked();
    void setSpecies(int,int);
    void on_Observations_cellChanged(int row, int column);
    void on_exportButton_clicked();
    void on_lockGPS_stateChanged(int arg1);
    void on_tabWidget_currentChanged(int index);//added 4.29.2020

    void readSurveyJson();//added 5.5.2020
};

#endif // MAINWINDOW2_H
