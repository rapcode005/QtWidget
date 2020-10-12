#ifndef NEWMAINWINDOW_H
#define NEWMAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
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
#include "gpsrecording.h"
#include "exportdialog.h"
#include "openingscreen.h"
#include "distanceconfig.h"
#include "docLayout.h"
#include "wbps.h"
#include "wbpsObj.h"
#include "customeventwidget.h"
#include "customcombobox.h"
#include <QFocusEvent>
#include "aboutscribe.h"
#include "userguide.h"
#include "projectinformation.h"
#include "aboutdialog.h"
#include "messagebox.h"
#include "workerthread2nd.h"
#include "workerThreadImportFlight.h"
#include "progressmessage.h"
#include "workerThreadSurvey.h"

namespace Ui {
class newmainwindow;
}

class newmainwindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(QString plotName READ plotName WRITE setPlotName NOTIFY plotNameChanged) // added 5.15.2020

public:
    explicit newmainwindow(QWidget *parent = nullptr);
    ~newmainwindow();

    QString startDir;

    QString plotName() const;//added 5.15.2020
    void setPlotName(const QString &plotName);//added 5.15.2020

    void setValuesGOEA(const QString &val1, const QString &val2);
    void setValuesWBPHS(const QString &val1, const QString &val2, const QString &val3);

    void setAGName(const QString &newAGN);

    void on_actionAdditional_Fields_triggered();
    //Refresh Both Header and Additional
    void refreshHeaderAdditional();

public slots:
    void RecordingIndexChanged(int position);
    void RefreshItems();
    void RefreshItems2();
    void FillSpeciesField(int item);
    void FillGroupingField(int item);
    void AppendASC();
    void AutoSave();
    void ProjectSettingsFinished();
    void PopulateHeaderFields();
    void CreateExportFile();
    void BuildBookmarksTableColumns();
    void updateExportHeaders();
    void ImportFlightData(const QString &flightDataDirectoryPath, const bool &copyfiles);
    void closeProgram();
    void updatePlotName();
    //Second Load finished
    void addHeaderItemM(QTableWidgetItem *item, const int &index, const int &condition);
    //Import Flight Data Function
    void sendRecords(GPSRecording *records, const QString &matchLine);
    void plotGPS(const bool &autoUnit, const QStringList &flightLines, const QStringList &wavFileItems);
    void plotBAEA(const QList<QStringList> &surveyJsonPolygonList);
    void plotGOEA(const QList<QStringList> &surveyJsonSegmentListGOEA);
    void plotWPHS(const QList<QStringList> &surveyJsonSegmentList,
                  const QList<QStringList> &airGroundSurveyJsonSegmentList);
    void playAudio(const QStringList &names);
    //Additioanl Fields Refresh
    void sendAdditionalFieldsOneVal(QTableWidgetItem *item, const int &index);
    void sendAdditionalFieldsValues(QTableWidgetItem *item, const int &index);
    void sendAdditionalFieldsNoVal(QTableWidgetItem *item, const int &index);
    //Refresh Header Fields
    void sendHeaderFieldsOneVal(QTableWidgetItem *item, const int &index);
    void sendHeaderFieldsOneValNoAddded(QTableWidgetItem *item, const int &index);
    void sendHeaderFieldsValues(QTableWidgetItem *item, const int &index);
    void sendHeaderFieldsNoVal(QTableWidgetItem *item, const int &index);

signals:
    void fileOpened();
    void plotNameChanged();//added 5.15.2020
    //Add Item from QThread to MainWindow

private slots:
    void refreshMap();//added 4.29.2020;
    void resizeEvent(QResizeEvent *event);//added 4.29.2020;
    void closeEvent(QCloseEvent *event);
    void on_actionImport_Flight_Data_triggered();
    void on_actionSave_Transcribed_Data_triggered();

    void on_lockGPS_stateChanged(int arg1);

    void on_actionOpen_Survey_Auto_Save_Backup_triggered();

    void on_actionImport_Survey_Template_triggered();

    void on_actionCreate_Survey_Template_triggered();

    void loadAdditional();

    void on_actionSave_triggered();

    void on_actionExit_triggered();

    void on_actionProject_Settings_triggered();

    void on_actionAbout_Scribe_triggered();

    void on_actionUser_Guide_triggered();

    //void on_HeaderFields_cellChanged(int row, int column);

    void on_insertButton_clicked();

    void on_Play_toggled(bool checked);

    void on_Back_clicked();

    void on_Next_clicked();

    void repeatTrack();

    void on_tabWidget_currentChanged(int index);

    void on_quantity_textChanged(const QString &arg1);

    void handleBackTick();

    void on_comboBox_2_currentTextChanged(const QString &arg1);

    //void on_deleteButton_clicked();

    void setSpecies(int row, int column);

    void on_wavFileList_currentIndexChanged(int index);

    QList<QStringList> readGeoJson(QJsonObject jObj);//added 5.5.2020
    QList<QStringList> readGeoJsonAir(QJsonObject jObj);

    //void additionalFieldsResize();

    void commentDocRect(const QSizeF& r, const int &index, const int &tIndex);

    void commentDocRectH(const QSizeF& r, const int &index, const int &tIndex);

    //stratum Changed
    void stratumChanged();
    void trasectChanged();

    //Custom Text Edit Event
    void focusOutStratum();
    void focusOutTransect();
    void focusOutSegment();

    //void collectPreValues();

    //void focusInStratum();
    void focusInTransect();
    void focusInBCR();

    //void focusInSegment();

    //void focusInPlotName();
    void focusOutPlotName();

    //void focusInAGName();
    void focusOutAGName();

    void focusOutBCR();

    void on_lockGPS_clicked();

    void on_actionDistance_Configuration_triggered();

    void on_pushButton_clicked();

    QList<QStringList> readGeoJson2(QJsonObject jObj);//added 5.13.2020
    //QList<QStringList> readAirJson(QJsonObject jObj);

    //Save Distance Configuration
    void updateProjectFile(const double &newObsMaxD);
    void updateProjectFile(const double &newObsMaxD, const double &newAirMaxD);

    void plotnameChanged();

    void focusOutEvent(QFocusEvent *event);
    void focusInEvent(QFocusEvent *event);

    //void on_actionAbout_Scribe_triggered(bool checked);

    void on_actionProject_Information_triggered();

    void getCurrentStatusAudio(const int &status);

    void on_export_2_clicked();

private:
    Ui::newmainwindow *ui;

    //QDialog                         *about;

    ProjectSettingsDialog          *projSettingsDialog;
    FileStructureInfoDialog        *fsiDialog;
    //AdditionalFieldsDialog         *fieldDialog;
    distanceconfig                 *distanceConfig;
    ProjectInformation             *projectInformation;

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

    //QTextEdit *commentText;

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

    QJsonObject fieldValue {
        {"Comment", 1}
    };

    QList<QStringList> surveyJsonSegmentList;//added 5.4.2020 id,segment,stratum,transect,version,p1lon,p1lat,p2lon,p2lat...
    QList<QStringList> airGroundSurveyJsonSegmentList;//added 5.4.2020 agcode,p1lon,p1lat,p2lon,p2lat...
    QList<QStringList> surveyJsonPolygonList; //added 5.14.2020 PlotLabel,p1lon,p1lat,p2lon,p2lat...
    QList<QStringList> surveyJsonSegmentListGOEA; //added 5.18.2020 //BCR, BCRTrn, BegLat, BegLng, EndLat, EndLng, FID, ID, KM, TRN

    void FillSpeciesTemplates(QList<LogTemplateItem*> items);
    void FillGroupingTemplates(QList<LogTemplateItem*> items);
    //void ClearBookmarks();
    void CreateLogItemShortcuts();
    bool eventFilter(QObject *target, QEvent *event);

    void setupButton(const QIcon &icon, QPushButton *push);
    void setupButton(const QIcon &icon, QPushButton *push, const QColor &color);

    int commentRowIndex;
    int commentRowIndexH;

    int indexLock = -1;
    QString wavFileLock = "";
    bool curlocked  = false;

    //void lockGPS();

    int stratumrowid = -1;//added 5.13.2020
    int transectrowid = -1;//added 5.13.2020
    int segmentrowidx = -1;//added 5.13.2020

    QString m_plotName; //added 5.15.2020

    void validateGoeaValue();
    void validationTransectGOEA();

    //Restriction
    //QStringList numberOnly = {"Year", "Distance"};
    //QStringList GPS = {"Stratum", "Transect"};

    QString returnedValueBAEA;//added 5.21.2020
    QString returnedValueGOEA;
    QString returnedValueWPBHS;

    QString flightData;
    bool hasFlight;

    //Select WavFile refresh Map
    void selectWavFileMap(const int &index);

    //HeaderFields Slot
    QList<QMetaObject::Connection> connection, connectionA;
    void connectAllHeader();
    void disconnectAllHeader();

    void additionalConnect();
    void additionalDisconnect();

    void validationBCR();

    void adjustSpeciesHotTable();

    aboutscribe *aboutScribe;
    userguide *userGuide;
    AboutDialog *aboutDialog;
    progressmessage *progMessage;

    void clearHeaderFields();
    void clearAdditionalFields();
    void loadSpeciesHotTable();
};

#endif // NEWMAINWINDOW_H
