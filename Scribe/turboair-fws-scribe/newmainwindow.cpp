#include "newmainwindow.h"
#include "ui_newmainwindow.h"
#include <QFileDialog>
#include <QDir>
#include <QDialog>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QList>
#include <QQuickWidget>
#include <QQmlContext>
#include <logitemeditdialog.h>
#include <QSignalMapper>
#include <QCloseEvent>
#include "aboutdialog.h"
#include "exportdialog.h"
#include "globalsettings.h"
#include <io.h>
#include <QScreen>
#include <QQmlEngine>
#include <QQmlApplicationEngine>
#include <QMessageBox>
#include <QTextEdit>
//#include "projectinformation.h"
#include <QQmlContext>
#include <QSettings>

using namespace std;

static bool GPS_MAP_ENABLED = false;

bool lessThan1( const QStringList& v1, const QStringList& v2)
{
    return v1.first() < v2.first();
}

newmainwindow::newmainwindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::newmainwindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));

    this->setMenuBar(menuBar());
    this->setCentralWidget(ui->centralwidget);

    menuBar()->setVisible(true);
    menuBar()->setNativeMenuBar(false);
    menuBar()->show();

    if(!menuBar()->isVisible()){
        menuBar()->setVisible(true);
    }

    if(!menuBar()->isActiveWindow()){
        menuBar()->raise();
        menuBar()->activateWindow();
    }

    ui->menubar->setNativeMenuBar(false);//added 8.10.2020

    if(!ui->menubar->isVisible()){
        ui->menubar->setVisible(true);
    }

    if(!ui->menubar->isDefaultUp())
        ui->menubar->setDefaultUp(false);//the menubar/submenu will appear downwards

    this->window()->setWindowTitle("Scribe");
    ui->Play->setCheckable(true);

    boolFreshLoad = true;
    audio = new AudioPlayer(ui->Play, ui->wavFileList);
    connect(audio->wavFileList, SIGNAL(currentIndexChanged(int)), this, SLOT(RecordingIndexChanged(int)));
    audio->player->setVolume(ui->Volume->value());
    audio->player->setPlaylist(audio->fileList);
    connect(audio, &AudioPlayer::sendCurrentState, this, &newmainwindow::getCurrentStatusAudio);

    proj = new ProjectFile;

    shortcutGroup = new QActionGroup(this);
    specieskeyMapper = new QSignalMapper(this);
    groupingskeyMapper = new QSignalMapper(this);

    projSettingsDialog = new ProjectSettingsDialog(this, proj);
    exportDialog = new ExportDialog(this);

    //8.26.2020 check for settings.ini
    //qDebug() << "applicationDirPath:  " << QCoreApplication::applicationDirPath();
    QString settingsFile = QCoreApplication::applicationDirPath() + "/settings.ini";
    QFile strFile(settingsFile);
    if(strFile.exists())
        scribeSettings = new QSettings(settingsFile,QSettings::IniFormat);

    OpeningScreenDialog *openingScreen = new OpeningScreenDialog(this, proj);
    openingScreen->frmInit(strFile.exists(),settingsFile);//added 8.26.2020
    openingScreen->show();

    openingScreen->setGeometry(
                               QStyle::alignedRect(
                                   Qt::LeftToRight,
                                   Qt::AlignCenter,
                                   openingScreen->size(),
                                   qApp->screens().first()->geometry()
                               ));


    distanceConfig = new distanceconfig(this, proj);

    distanceConfig->setGeometry(
                                QStyle::alignedRect(
                                    Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    distanceConfig->size(),
                                    qApp->screens().first()->geometry()
                                ));

    //Refresh with header and additional
    connect(projSettingsDialog, SIGNAL(receivedLogDialogItems()), this,SLOT(RefreshItems()));
    //Refresh wihtout header and additional
    connect(projSettingsDialog,  &ProjectSettingsDialog::receivedLogDialogItemsWithoutSlot
            ,this, &newmainwindow::RefreshItems2);

    projSettingsDialog->setGeometry(QStyle::alignedRect(
                                   Qt::LeftToRight,
                                   Qt::AlignCenter,
                                   projSettingsDialog->size(),
                                   qApp->screens().first()->geometry()
                               ));

    //connect(exportDialog,SIGNAL(Append()),this,SLOT(AppendASC()));
    connect(proj, SIGNAL(Loaded()),projSettingsDialog,SLOT(ProjectOpened()));
    connect(projSettingsDialog, SIGNAL(finished(int)),this, SLOT(ProjectSettingsFinished()));

    //This is called when Opening Screen is done. BuildBookmarksTableColumns is called at the end.
    //connect(openingScreen, SIGNAL(PopulateHeaderFields()),this,SLOT(PopulateHeaderFields()));
    //connect(openingScreen, SIGNAL(CreateExportFile()),this, SLOT(CreateExportFile()));
    //connect(openingScreen, SIGNAL(ImportFlightData(QString, bool )),this,SLOT(ImportFlightData(QString, bool)));

    //Second Stage
    connect(openingScreen, &OpeningScreenDialog::secondLoadReady, this, [=](const bool &hasFlightData,
            const QString &existingFlightDataPath) {

        m_flightFilePath = existingFlightDataPath;

        clearHeaderFields();

        ui->HeaderFields->setStyleSheet("font-size: 13px;"
                                        "font-family: 'Segoe UI'; border: 1px solid #D8D8D8; "
                                        "background-color:#F7F7F7; border-top:none;");//rgba(0, 0, 0, 0.2)

        openingScreen->progMessage->setValue(70);

        WorkerThread2nd *workerThread = new WorkerThread2nd();
        workerThread->setProjectFile(proj);
        workerThread->setHasFlightData(hasFlightData);
        workerThread->setFlightData(existingFlightDataPath);
        workerThread->setAudioPlayer(audio);

        connect(workerThread, &WorkerThread2nd::LoadReady, this, [=](ProjectFile *newProj) {

            proj = newProj;

            connect(ui->speciesTable, SIGNAL(cellClicked(int,int)), this, SLOT(setSpecies(int,int)));

            CreateLogItemShortcuts();

            openingScreen->progMessage->setValue(160);

            BuildBookmarksTableColumns();

            connectAllHeader();

            autoSaveTimer = new QTimer(this);
            connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(AutoSave()));
            autoSaveTimer->start(20000);

            //for our problems with ` moving to the next audio file
            //new QShortcut(QKeySequence(Qt::Key_QuoteLeft), this, SLOT(handleBackTick()));//remarked 8.19.2020
            //Alt-R
            //new QShortcut(QKeySequence(tr("Alt+r")), this, SLOT(repeatTrack()));//remarked 8.19.2020
            //new QShortcut(QKeySequence(Qt::ALT + Qt::Key_R), this, SLOT(repeatTrack()));//remarked 8.19.2020
            //Alt-B
            //new QShortcut(QKeySequence(tr("Alt+b")), this, SLOT(on_Back_clicked()));//remarked 8.19.2020
            //new QShortcut(QKeySequence(Qt::ALT + Qt::Key_B), this, SLOT(on_Back_clicked()));//remarked 8.19.2020

            QObject::connect(new QShortcut(QKeySequence(Qt::Key_QuoteLeft), this), &QShortcut::activated, [=]()
            {
                handleBackTick();
            });

            QObject::connect(new QShortcut(QKeySequence(Qt::ALT + Qt::Key_R), this), &QShortcut::activated, [=]()
            {
                repeatTrack();
            });

            QObject::connect(new QShortcut(QKeySequence(Qt::ALT + Qt::Key_B), this), &QShortcut::activated, [=]()
            {
                on_Back_clicked();
            });

            //Shift-A Additional
            QObject::connect(new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_A), this), &QShortcut::activated, [=]()
            {
                ui->tabWidget->setCurrentIndex(1);
            });
            //Shift-H Default
            QObject::connect(new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_H), this), &QShortcut::activated, [=]()
            {
                ui->tabWidget->setCurrentIndex(0);
            });

            openingScreen->progMessage->okToClose = true;
            openingScreen->progMessage->close();
            openingScreen->close();

            //qDebug() << "read this hasFlightData: " << hasFlightData;
            if(!hasFlightData) {
                MessageDialog msgBox;
                msgBox.setText("Would like to import flight data?");
                msgBox.setFontSize(13);
                msgBox.setTwoButton("Yes", 60, "Not Now", 80);
                msgBox.exec();
                if (msgBox.isAccept()) {
                    on_actionImport_Flight_Data_triggered();
                }
            }

            if(!openingScreen->getSystemStatusNew()){//added 8.26.2020
                //open a recent file
                if(scribeSettings){
                    //Audio Settings
                    audio->wavFileList->setCurrentText(scribeSettings->value("Audio/wavFile").toString());
                    //qDebug() << "audio_ok";
                    QString islock = scribeSettings->value("Audio/lock").toString();
                    if(islock.toLower() == "yes"){
                        ui->lockGPS->setChecked(true);
                        on_lockGPS_clicked();
                    }else if(islock.toLower() == "no"){
                        ui->lockGPS->setChecked(false);
                        on_lockGPS_clicked();
                    }
                    //qDebug() << "lock_ok";

                    //Map Settings
                    bool isok;
                    int intmapzoom = scribeSettings->value("Map/zoom").toInt(&isok);
                    if(isok){
                        isok = false;
                        double mapcenterlat = scribeSettings->value("Map/latitude").toDouble(&isok);
                        if(isok){
                            isok = false;
                            double mapcenterlon = scribeSettings->value("Map/longitude").toDouble(&isok);
                            if(isok){
                                QString maptype = scribeSettings->value("Map/maptype").toString().trimmed();
                                //qDebug() << intmapzoom << ":" << mapcenterlat << ";" << mapcenterlon << ";" << maptype;

                                QMetaObject::invokeMethod(gps->wayHandler, "setMapRecentView",
                                    Q_ARG(QVariant, intmapzoom),
                                    Q_ARG(QVariant, mapcenterlat),
                                    Q_ARG(QVariant, mapcenterlon),
                                    Q_ARG(QVariant, maptype));
                                //qDebug() << "zoom_ok";
                            }
                        }
                    }

                    //Species Settings
                    if(scribeSettings->value("Species/selected").toString().trimmed().length() > 0 &&
                            scribeSettings->value("Species/selected").toString().trimmed() != ""){
                        for(int i=0; i< ui->comboBox_2->count(); i++){
                            if(ui->comboBox_2->itemText(i).toUpper() == scribeSettings->value("Species/selected").toString().trimmed().toUpper()){
                                ui->comboBox_2->setCurrentText(scribeSettings->value("Species/selected").toString());
                                //qDebug() << "species_ok";
                                break;
                            }
                        }
                    }

                    //Observations Settings
                    QString csvFileName = scribeSettings->value("Observation/file").toString().trimmed();
                    //qDebug() << "csvFileName: " << csvFileName;
                    if(csvFileName.length() > 0){
                        QFile csvfile(csvFileName);
                        if(csvfile.exists()){
                            QString data;
                            QStringList rowOfData;
                            QStringList rowData;

                            data.clear();
                            rowOfData.clear();
                            rowData.clear();
                            if (csvfile.open(QFile::ReadOnly))
                            {
                                data = csvfile.readAll();
                                rowOfData = data.split("\n");
                                csvfile.close();
                            }
                            QStringList headerSettingsList;
                            QStringList tmpheaderSettingsList = rowOfData[0].split(",");
                            foreach(QString str, tmpheaderSettingsList){
                                if(str.contains("\r")){
                                    str.replace("\r","");
                                }
                                if(str.contains("\n")){
                                    str.replace("\n","");
                                }
                                headerSettingsList << str;
                            }

                            //qDebug() << "headerSettingsList:" << headerSettingsList;
                            ui->Observations->blockSignals(true);
                            const QString &surveyType = proj->getSurveyType();
                            int insertAt = 0;

                            QFont cellFont;
                            cellFont.setFamily("Segoe UI");
                            cellFont.setPixelSize(13);
                            cellFont.setCapitalization(QFont::AllLowercase);

                            for (int x = 1; x < rowOfData.size(); x++){
                                rowData = rowOfData.at(x).split(",");
                                //qDebug() << "rowData.count(): " << rowData.count();
                                if(rowData.count() > 1){
                                    ui->Observations->insertRow(insertAt);
                                    const int &currentRow = insertAt;

                                    QFont fontUpper = proj->getFontItem(false);
                                    fontUpper.setCapitalization(QFont::AllUppercase);
                                    QTableWidgetItem *speciesItem = new QTableWidgetItem();
                                    speciesItem->setText(rowData[headerSettingsList.indexOf("Species")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Species"), speciesItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Species"))->setData(Qt::FontRole,QVariant(fontUpper));

                                    //qDebug() << "Species";
                                    QTableWidgetItem *quantityItem = new QTableWidgetItem();
                                    quantityItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    quantityItem->setText(rowData[headerSettingsList.indexOf("Quantity")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Quantity"), quantityItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Quantity"))->setData(Qt::FontRole,QVariant(cellFont));

                                     //qDebug() << "Quantity";
                                     //qDebug() << surveyType;
                                    if(  surveyType != "BAEA" && surveyType != "GOEA" ){
                                        if(rowData[headerSettingsList.indexOf("Grouping")] != -1){
                                            //qDebug() << "GroupingA";
                                            ui->Observations->setItem(currentRow, allColumnNames.indexOf("Grouping"), new QTableWidgetItem(rowData[headerSettingsList.indexOf("Grouping")]));
                                            //qDebug() << "GroupingB";
                                            ui->Observations->item(currentRow, allColumnNames.indexOf("Grouping"))->setData(Qt::FontRole,QVariant(cellFont));
                                            //qDebug() << "GroupingC";
                                        }
                                    }

                                    QTableWidgetItem *latitudeItem = new QTableWidgetItem();
                                    latitudeItem->setFlags(latitudeItem->flags() ^ Qt::ItemIsEditable);
                                    latitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    latitudeItem->setBackground(QBrush(QColor(153,153,153)));
                                    latitudeItem->setText(rowData[headerSettingsList.indexOf("Latitude")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Latitude"), latitudeItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Latitude"))->setData(Qt::FontRole,QVariant(cellFont));

                                    //qDebug() << "Latitude";
                                    QTableWidgetItem *longitudeItem = new QTableWidgetItem();
                                    longitudeItem->setFlags(longitudeItem->flags() ^ Qt::ItemIsEditable);
                                    longitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    longitudeItem->setBackground(QBrush(QColor(153,153,153)));
                                    longitudeItem->setText(rowData[headerSettingsList.indexOf("Longitude")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Longitude"), longitudeItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Longitude"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "Longitude";

                                    QTableWidgetItem *timeItem = new QTableWidgetItem();
                                    timeItem->setFlags(longitudeItem->flags() ^ Qt::ItemIsEditable);
                                    timeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    timeItem->setBackground(QBrush(QColor(153,153,153)));
                                    timeItem->setText(rowData[headerSettingsList.indexOf("Time")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Time"), timeItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Time"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "Time";

                                    QTableWidgetItem *altitudeItem = new QTableWidgetItem();
                                    altitudeItem->setFlags(altitudeItem->flags() ^ Qt::ItemIsEditable);
                                    altitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    altitudeItem->setBackground(QBrush(QColor(153,153,153)));
                                    altitudeItem->setText(rowData[headerSettingsList.indexOf("Altitude")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Altitude"), altitudeItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Altitude"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "Altitude";

                                    QTableWidgetItem *speedItem = new QTableWidgetItem();
                                    speedItem->setFlags(speedItem->flags() ^ Qt::ItemIsEditable);
                                    speedItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    speedItem->setBackground(QBrush(QColor(153,153,153)));
                                    speedItem->setText(rowData[headerSettingsList.indexOf("Speed")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Speed"), speedItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Speed"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "Speed";

                                    QTableWidgetItem *courseItem = new QTableWidgetItem();
                                    courseItem->setFlags(courseItem->flags() ^ Qt::ItemIsEditable);
                                    courseItem->setBackground(QBrush(QColor(153,153,153)));
                                    courseItem->setText(rowData[headerSettingsList.indexOf("Course")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Course"), courseItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Course"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "Course";

                                    QTableWidgetItem *HDOPItem = new QTableWidgetItem();
                                    HDOPItem->setFlags(HDOPItem->flags() ^ Qt::ItemIsEditable);
                                    HDOPItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    HDOPItem->setBackground(QBrush(QColor(153,153,153)));
                                    HDOPItem->setText(rowData[headerSettingsList.indexOf("HDOP")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("HDOP"), HDOPItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("HDOP"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "HDOP";

                                    QTableWidgetItem *satItem = new QTableWidgetItem();
                                    satItem->setFlags(satItem->flags() ^ Qt::ItemIsEditable);
                                    satItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                                    satItem->setBackground(QBrush(QColor(153,153,153)));
                                    satItem->setText(rowData[headerSettingsList.indexOf("# Satellites")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("# Satellites"), satItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("# Satellites"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "# Satellites";

                                    QTableWidgetItem *audioItem = new QTableWidgetItem();
                                    audioItem->setFlags(audioItem->flags() ^ Qt::ItemIsEditable);
                                    audioItem->setBackground(QBrush(QColor(153,153,153)));
                                    audioItem->setText(rowData[headerSettingsList.indexOf("Audio File")]);
                                    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Audio File"), audioItem);
                                    ui->Observations->item(currentRow, allColumnNames.indexOf("Audio File"))->setData(Qt::FontRole,QVariant(cellFont));
                                    //qDebug() << "Audio File";

                                    // Insert headers into Observations Table
                                    for (int i=0; i<proj->getHeaderCount(); i++) {
                                        QString headerKey = proj->getHeaderName(i);
                                        int columnToInsert = allColumnNames.indexOf(headerKey);
                                        QString newVal = rowData[headerSettingsList.indexOf(headerKey)];
                                        QTableWidgetItem *headerValue = new QTableWidgetItem(newVal);

                                        //not allow to be edited
                                        if(proj->getInstanceHeaderPosition(headerKey) != -1){
                                            headerValue->setFlags(headerValue->flags() & ~Qt::ItemIsEditable);
                                            headerValue->setBackground(QBrush(QColor(153,153,153)));
                                        }

                                        ui->Observations->setItem(currentRow, columnToInsert, headerValue);
                                        ui->Observations->item(currentRow, columnToInsert)->setData(Qt::FontRole,QVariant(cellFont));
                                        //qDebug() << headerKey;
                                    }

                                    //Insert additional fields into Observations Table
                                    for (int i=0; i<proj->getAdditionalFieldsCount(); i++) {
                                        const QString &additionalFieldKey = proj->getAdditionalFieldName(i);
                                        const int &columnToInsert = allColumnNames.indexOf(additionalFieldKey);
                                        const QString &newVal = rowData[headerSettingsList.indexOf(additionalFieldKey)];

                                        QTableWidgetItem *additionalFieldValue = new QTableWidgetItem(newVal);
                                        ui->Observations->setItem(currentRow, columnToInsert, additionalFieldValue);
                                        ui->Observations->item(currentRow, columnToInsert)->setData(Qt::FontRole,QVariant(cellFont));
                                        //qDebug() << additionalFieldKey;
                                    }
                                    insertAt += 1;
                                }
                                //break;
                            }

                            //if (ui->Observations->rowCount() == 1)
                                connect(ui->Observations, SIGNAL(cellDoubleClicked(int,int)),
                                    this, SLOT(on_observation_doubleClicked(int,int)));

                            if(ui->Observations->signalsBlocked())
                                ui->Observations->blockSignals(false);
                            //qDebug() << "observation_ok";
                        }
                    }
                }
            }

        });

        //Calculate Missing Sec
       /* connect(workerThread, &WorkerThread2nd::calculateMissingSec,
                this, [=](QString flightpath, QStringList flighdataD) {

            if(flighdataD.length() > 0){
                QList<QStringList> missinglist = calculateMissingSec(flighdataD);
                if (missinglist.length() > 1)
                    saveGps(flighdataD, missinglist, flightpath.replace(".GPS", "_TMP.GPS"));
            }
            progMessage->setValue(85);

        });*/

        //Progress Bar
        connect(workerThread, &WorkerThread2nd::load1, this, [=] {

            openingScreen->progMessage->setValue(92);

        });

        connect(workerThread, &WorkerThread2nd::load2, this, [=] {

            openingScreen->progMessage->setValue(100);

        });

        connect(workerThread, &WorkerThread2nd::load3, this, [=] {

            openingScreen->progMessage->setValue(108);

        });

        connect(workerThread, &WorkerThread2nd::load4, this, [=] {

            openingScreen->progMessage->setValue(117);

        });

        connect(workerThread, &WorkerThread2nd::load5, this, [=] {

            openingScreen->progMessage->setValue(124);

        });

        connect(workerThread, &WorkerThread2nd::load6, this, [=] {

            openingScreen->progMessage->setValue(136);

        });

        connect(workerThread, &WorkerThread2nd::load7, this, [=] {

            openingScreen->progMessage->setValue(151);

        });

        connect(workerThread, &WorkerThread2nd::unexpecteFinished, this, [=] {

            openingScreen->progMessage->setValue(160);
            openingScreen->progMessage->okToClose = true;
            openingScreen->progMessage->close();

            /*if (!proj->newOpen) {
                MessageDialog msgBox;
                msgBox.setText("Survey template file found in Project Directory.");
                msgBox.setFontSize(13);
                msgBox.exec();
            }*/

            if(!hasFlightData) {
                MessageDialog msgBox;
                msgBox.setText("Would like to import flight data?");
                msgBox.setFontSize(13);
                msgBox.setTwoButton("Yes", 60, "Not Now", 80);
                msgBox.exec();
                if (msgBox.isAccept()) {
                    on_actionImport_Flight_Data_triggered();
                }
            }

            //incase a error occur target location 2
            if(!openingScreen->getSystemStatusNew()){//added 8.26.2020
                //open a recent file
                if(scribeSettings){
                    qDebug() << "unexpecteFinished";
                }
            }

        });

        connect(workerThread, &WorkerThread2nd::finished, workerThread, &QObject::deleteLater);
        //Add Item from QThread to MainWindow
        connect(workerThread, &WorkerThread2nd::sendHeaderFields, this, &newmainwindow::addHeaderItemM);
        //Disable Add Field
        connect(workerThread, &WorkerThread2nd::disbleAddField,
                projSettingsDialog, &ProjectSettingsDialog::disable_field_buttons);
        //Send a record in Audio
        connect(workerThread, &WorkerThread2nd::sendRecords, this, &newmainwindow::sendRecords);
        //Plot GPS
        connect(workerThread, &WorkerThread2nd::plotGPS, this, &newmainwindow::plotGPS);
        //Plot Survey
        connect(workerThread, &WorkerThread2nd::plotBAEA, this, &newmainwindow::plotBAEA);
        connect(workerThread, &WorkerThread2nd::plotGOEA, this, &newmainwindow::plotGOEA);
        connect(workerThread, &WorkerThread2nd::plotWPHS, this, &newmainwindow::plotWPHS);
        //Play Audio
        connect(workerThread, &WorkerThread2nd::playAudio, this, &newmainwindow::playAudio);

        workerThread->start();

    });

    connect(openingScreen, SIGNAL(closeProgram()), this, SLOT(closeProgram()));
    //GPS Lock Visiblity
    connect(openingScreen, &OpeningScreenDialog::widgetStatus, this, [=](const bool &status)
    {
        ui->lockGPS->setVisible(status);
    });
    //Send a signal after update the value in distance configuration
    connect(proj, &ProjectFile::LoadedDistanceConfig, distanceConfig, &distanceconfig::distanceConfigOpened);
    connect(distanceConfig, &distanceconfig::showProgressMessage, this, [=](DistanceConfigObj *dc,
            const bool &WBPHS) {

        progMessage = new progressmessage(this);//added 7.30.2020
        progMessage->setMax(60);
        progMessage->show();

        if (!proj->newOpen) {

            MessageDialog msgBox;
            msgBox.setText("Survey template file found in Project Directory\n"
                           "and will make changes on Distance Configuration.\n\n"
                           "Do you want to apply these changes?");
            msgBox.setTwoButton("Yes", "No, do not apply");
            msgBox.resizeHeight((154 + 56) - 38);
            msgBox.resizeWidth(334);

            bool condition;

            if (WBPHS) {
                condition = QString::number(proj->getObsMaxD()) !=  QString::number(dc->getObsMaxD()) ||
                        QString::number(proj->getAirGroundMaxD()) !=  QString::number(dc->getAirMaxD());
            }
            else {
                condition = QString::number(proj->getObsMaxD()) !=  QString::number(dc->getObsMaxD());
            }

            if (condition) {

                msgBox.exec();

                if(msgBox.isAccept()) {

                    WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
                    workerThread->setDitanceConfig(dc);
                    workerThread->setIsSurveyWBPHS(WBPHS);
                    workerThread->setAction(3);
                    workerThread->setProjectFile(proj);

                    connect(workerThread, &WorkerThreadSurvey::finished, workerThread,
                            &QObject::deleteLater);


                    connect(workerThread, &WorkerThreadSurvey::load1,this, [=]() {

                        progMessage->setValue(24);

                    });

                    connect(workerThread,SIGNAL(updateProjectFile(double)), this, SLOT(updateProjectFile(double)));
                    connect(workerThread, SIGNAL(updateProjectFile(double, double)), this, SLOT(updateProjectFile(double, double)));

                    workerThread->start();

                }
                else {
                    progMessage->setValue(60);
                    progMessage->okToClose = true;
                    progMessage->close();
                    distanceConfig->close();
                }

            }
            else {
                progMessage->setValue(60);
                progMessage->okToClose = true;
                progMessage->close();
                distanceConfig->close();
            }

        }
        else {
            WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
            workerThread->setDitanceConfig(dc);
            workerThread->setIsSurveyWBPHS(WBPHS);
            workerThread->setAction(3);
            workerThread->setProjectFile(proj);

            connect(workerThread, &WorkerThreadSurvey::finished, workerThread,
                    &QObject::deleteLater);


            connect(workerThread, &WorkerThreadSurvey::load1,this, [=]() {

                progMessage->setValue(24);

            });

            connect(workerThread,SIGNAL(updateProjectFile(double)), this, SLOT(updateProjectFile(double)));
            connect(workerThread, SIGNAL(updateProjectFile(double, double)), this, SLOT(updateProjectFile(double, double)));

            workerThread->start();

        }
    });

    ui->quantity->installEventFilter(this);
    ui->HeaderFields->installEventFilter(this);
    ui->additionalfieldsTable->installEventFilter(this);
    ui->Observations->installEventFilter(this);

    startDir = QDir::homePath();
    projSettingsDialog->startDir = startDir;

    //autoSaveTimer = new QTimer(this);
    //connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(AutoSave()));
    //autoSaveTimer->start(20000);

    //for our problems with ` moving to the next audio file
    //new QShortcut(QKeySequence(Qt::Key_QuoteLeft), this, SLOT(handleBackTick()));
    //Alt-R
    //new QShortcut(QKeySequence(tr("Alt+r")), this, SLOT(repeatTrack()));
    //new QShortcut(QKeySequence(Qt::ALT + Qt::Key_R), this, SLOT(repeatTrack()));
    //Alt-B
    //new QShortcut(QKeySequence(tr("Alt+b")), this, SLOT(on_Back_clicked()));
    //new QShortcut(QKeySequence(Qt::ALT + Qt::Key_B), this, SLOT(on_Back_clicked()));
    //Shift-A Additional
    //QObject::connect(new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_A), this), &QShortcut::activated, [=]()
    //{
      //  ui->tabWidget->setCurrentIndex(1);
    //});
    //Shift-H Default
    //QObject::connect(new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_H), this), &QShortcut::activated, [=]()
    //{
      //  ui->tabWidget->setCurrentIndex(0);
    //});

    ui->insertButton->setToolTip(QString("Insert an observation above the selected observation"));
    ui->pushButton->setToolTip(QString("Delete the selected observation"));
    ui->export_2->setToolTip(QString("Save/Export values to ASC file"));

    //added 4.29.2020
    //initializong the map (code comes from ImportFlightData)
    ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/gps/main_copy.qml")));
    ui->quickWidget->setResizeMode(ui->quickWidget->SizeRootObjectToView);
    ui->quickWidget->show();

    QObject* item = ui->quickWidget->rootObject()->findChild<QObject *>("mouseareabuttonmaximizemap");
    if(item){//added 6.11.2020
        QObject::connect(item, SIGNAL(clickedButton(QString)), this, SLOT(onMaximizeMap(QString)));
    }

    //REMARKED 8.20.2020
    //Audio Button
    //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
    //setupButton(QIcon(":/img/misc/Icon ionic-md-skip-backward.png"), ui->Back);
    //setupButton(QIcon(":/img/misc/Icon ionic-md-skip-forward.png"), ui->Next);

    //Title button
    setupButton(QIcon(":/img/misc/Icon open-menu.png"), ui->pushAudio, QColor(231, 231, 231, 1));
    setupButton(QIcon(":/img/misc/Icon open-menu.png"), ui->pushMap, QColor(231, 231, 231, 1));
    setupButton(QIcon(":/img/misc/Icon open-menu.png"), ui->pushOb, QColor(231, 231, 231, 1));
    setupButton(QIcon(":/img/misc/Icon open-menu.png"), ui->pushSpHot, QColor(231, 231, 231, 1));
    setupButton(QIcon(":/img/misc/Icon open-menu.png"), ui->pushHeader, QColor(231, 231, 231 , 1));

    ui->Observations->setShowGrid(false);
}

newmainwindow::~newmainwindow()
{
    delete ui;
}

QString newmainwindow::plotName() const
{
    return m_plotName;
}

void newmainwindow::setPlotName(const QString &plotName)
{
    if (plotName == m_plotName)
        return;

    proj->headerFields.at(proj->plotnameIndex)->setText(plotName.toLower());
    m_plotName = plotName;
    emit plotNameChanged();
}

void newmainwindow::setValuesGOEA(const QString &val1,const QString &val2, const double &val3)
{
    //added 5.22.2020
    proj->headerFields.at(proj->transectIndex)->setText(val2.toLower());
    auto colC = proj->headerFieldsC;
    QComboBox *combo1 = colC[proj->bcrIndex];
    for(int ii=0; ii<combo1->count(); ii++ ){
        if(combo1->itemText(ii) != ""){
            if(combo1->itemText(ii).toInt() == val1.toInt()){
                combo1->setCurrentIndex(ii);
                break;
            }
        }
    }

    //Current Value for Transect and BCR
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    globalSettings.set("TRNPRe", val2.toLower());
    globalSettings.set("BCRPRe", val1);

    QString sval = QString("%1").arg(val3 / 1000, 0, 'g', 6); //in km
    proj->headerFields.at(proj->distanceIndex)->setText(sval);
}

void newmainwindow::setValuesWBPHS(const QString &val1, const QString &val2, const QString &val3, const double &val4)
{
    /*qDebug() << "proj->stratumIndex: " << proj->stratumIndex;
    qDebug() << "proj->transectIndex: " << proj->transectIndex;
    qDebug() << "proj->segmentIndex: " << proj->segmentIndex;*/

    proj->headerFields.at(proj->stratumIndex)->setText(val1);
    proj->headerFields.at(proj->transectIndex)->setText(val2);
    proj->headerFields.at(proj->segmentIndex)->setText(val3);

    collectPreValues();

    QString sval = QString("%1").arg(val4 / 1000, 0, 'g', 6); //in km
    proj->headerFields.at(proj->distanceIndex)->setText(sval);
}


void newmainwindow::setAGName(const QString &newAGN)
{
    proj->headerFields.at(proj->aGNameIndex)->setText(newAGN.toLower());
}

void newmainwindow::RecordingIndexChanged(int position)
{
    if(position == -1){
        return;
    }
    currentGpsRecording = position;
    curCoord = 0;
}

void newmainwindow::RefreshItems()
{
    species = projSettingsDialog->speciesItems;
    groupings = projSettingsDialog->groupingItems;

    FillSpeciesTemplates(species);
    FillGroupingTemplates(groupings);

    if (projSettingsDialog->message) {
        projSettingsDialog->progMessage->setValue(95);
    }

    CreateLogItemShortcuts();

    on_actionAdditional_Fields_triggered();

    if (projSettingsDialog->message) {
        projSettingsDialog->progMessage->setValue(101);
    }

    PopulateHeaderFields();

    if (projSettingsDialog->message) {
        projSettingsDialog->progMessage->setValue(112);
    }

    selectWavFileMap(ui->wavFileList->currentIndex());

    if (projSettingsDialog->message) {
        projSettingsDialog->progMessage->setValue(120);
        projSettingsDialog->progMessage->okToClose = true;
        projSettingsDialog->progMessage->close();
        projSettingsDialog->message = false;
        projSettingsDialog->close();
    }
}

void newmainwindow::RefreshItems2()
{
    species = projSettingsDialog->speciesItems;
    groupings = projSettingsDialog->groupingItems;

    loadSpeciesHotTable();

    FillSpeciesTemplates(species);
    FillGroupingTemplates(groupings);
    CreateLogItemShortcuts();
}

void newmainwindow::FillSpeciesField(int item)
{
    insertWasLastEvent = false;
    currentSpecies = species[item];
    ui->comboBox_2->setCurrentText(currentSpecies->itemName);
}

void newmainwindow::FillGroupingField(int item)
{
    ui->comboBox_3->setCurrentText(groupings[item]->itemName);
    ui->Observations->selectionModel()->clearSelection();
    on_insertButton_clicked();
}

void newmainwindow::AppendASC()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    int export_count = globalSettings.get("exportCount").toInt();
    globalSettings.set( "exportCount", QString( export_count++ ) );
    QString exportLocation = globalSettings.get("") + "/" + globalSettings.get("exportFilename") + ".asc";
    if (exportLocation.isEmpty()){
        return;
    }else{
        ascFilename = exportLocation;
    }
    if (ascFilename.isEmpty()){
        exportLocation = QFileDialog::getOpenFileName(this, "Append all bookmarks to existing ASC",startDir,"ASC files (*asc)");
    }else{
        exportLocation = ascFilename;
    }
    if (exportLocation.isEmpty()){
        qDebug() << "No export directory specified";
        return;
    }
    QFile exportFile(exportLocation);
    if (!exportFile.open(QIODevice::ReadWrite|QIODevice::Append)){
          qDebug() << "Couldn't open file for writing";
          return;
    }

    QTextStream stream (&exportFile);
    int numberOfColumns = ui->Observations->columnCount();

    qint64 size = exportFile.size();
    if(size == 0){
       if (stream.readLine() == nullptr){
            for(int i=0; i<numberOfColumns; i++) {
                stream << exportHeaders[i];
                if (i < numberOfColumns - 1){
                    stream << ",";
                }
            }
            stream << "\n";
        }
    }

    //TODO: Change this monstrosity once we fix up bookmarks
    for(int i=ui->Observations->rowCount()-1; i>=0; i--) {
        QMap<int, QString> values;
        for (int j=0; j<numberOfColumns; j++) {
            if (ui->Observations->item(i,j)->text().contains(','))
                values[exportHeaders.indexOf(ui->Observations->horizontalHeaderItem(j)->text())]
                        = "\"" + ui->Observations->item(i,j)->text() + "\"";
            else
                values[exportHeaders.indexOf(ui->Observations->horizontalHeaderItem(j)->text())]
                        = ui->Observations->item(i,j)->text();
        }


        for (int j=0; j<numberOfColumns; j++) {
            stream << values[j];
            if (j < numberOfColumns - 1) {
                stream << ",";
            }

        }
        stream << "\n";
    }

    exportFile.close();

    MessageDialog mb;
    mb.setTitle("Export");
    mb.setText("Saved observations successfully!");
    mb.exec();

    ascFilename = exportLocation;

//    ClearBookmarks();
    ui->Observations->clear();
    ui->Observations->setRowCount(0);

    BuildBookmarksTableColumns();
}

void newmainwindow::setupButton(const QIcon &icon, QPushButton *push)
{
    QPalette palette = *new QPalette();

    palette = push->palette();
    palette.setColor(QPalette::Button, QColor(Qt::white));
    push->setFlat(true);
    push->setText("");
    push->setPalette(palette);
    push->setAutoFillBackground(true);
    push->setIcon(icon);
}

void newmainwindow::setupButton(const QIcon &icon, QPushButton *push, const QColor &color)
{
    Q_UNUSED(icon);
    QPalette palette = *new QPalette();

    palette = push->palette();
    palette.setColor(QPalette::Button, color);
    push->setMinimumWidth(33);
    push->setMaximumHeight(33);
    push->setPalette(palette);
    push->setStyleSheet("text-align: left; font: Bold 13px 'Segoe UI'; qproperty-iconSize: 13px 13px;");
    //push->setIcon(icon);//remarked to remove the hamberger icon 8.25.2020

    if(push->objectName() == "pushSpHot"){
        push->setContextMenuPolicy(Qt::CustomContextMenu);

        //connect(push,SIGNAL(clicked()),this,SLOT(on_pushSpHot_clicked();));
        connect(push, &QPushButton::customContextMenuRequested, [this](QPoint pos){

            QAction *speciesAction = new QAction(this);
            speciesAction->setText("Edit Species/Items");
            connect(speciesAction, SIGNAL(triggered()), this, SLOT(action_editSpeciesClicked()));

            QAction *groupingAction = new QAction(this);
            groupingAction->setText("Edit Grouping");
            connect(groupingAction, SIGNAL(triggered()), this, SLOT(action_editGroupingClicked()));

            QMenu *menu=new QMenu(this);

            menu->addAction(speciesAction);
            menu->addAction(groupingAction);
            menu->popup(ui->pushSpHot->mapToGlobal(pos));
        });
    }
}



QString newmainwindow::renameFile(const QString &fileInfo, const QString &directoryInfo)
{
    std::string fileStd = fileInfo.toStdString();
    //Get Last period
    std::size_t lastIndex = fileInfo.toStdString().find_last_of(".");
    //Get File Name
    std::string fileName = fileInfo.toStdString().substr(0, lastIndex);
    //Get extension file
    std::string extFile = fileInfo.toStdString().substr(lastIndex, fileStd.length());
    if (extFile == ".GPS")
        return directoryInfo + QString::fromUtf8(extFile.c_str());
    else
        return directoryInfo + "_" + QString::fromUtf8(fileName.c_str()) + QString::fromUtf8(extFile.c_str());
}

/**
 * @brief MainWindow2::FillSpeciesTemplates
 * @param items
 * Populates species dropdown.
 */
void newmainwindow::FillSpeciesTemplates(QList<LogTemplateItem *> items)
{
    ui->comboBox_2->clear();
    QStringList s;
    for (int i = 0; i  < species.length(); i ++ ){
        s << items[i]->itemName;
    }
    s.sort();
    ui->comboBox_2->addItems(s);
    ui->comboBox_2->setFocus();
}

/**
 * @brief MainWindow2::FillGroupingTemplates
 * @param items
 * Populates grouping dropdowns
 */
void newmainwindow::FillGroupingTemplates(QList<LogTemplateItem *> items)
{
    QStringList g;
    for (int i = 0; i  < groupings.length(); i ++ ){
        g << items[i]->itemName;
    }
    //  g.sort();
    ui->comboBox_3->addItems(g);
}

void newmainwindow::ImportFlightData(const QString &sourceFlightDataDirectoryPath, const bool &copyfiles)
{

    QStringList names;

    // Make sure the project's FlightData directory exists
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    QString flightFilePath;
    QString directoryName;
    QString projectDirectory = globalSettings.get("projectDirectory");
    QString observer = globalSettings.get("observerInitials");
    QString targetFlightDataDirectoryPath("");
    if(copyfiles == true){
        if (sourceFlightDataDirectoryPath.isEmpty()){
            return;
        }

        audio->fileList->clear();
        QDir sourceDirectory(sourceFlightDataDirectoryPath);
         targetFlightDataDirectoryPath = projectDirectory + "/FlightData_" + sourceDirectory.dirName();

        if (!QDir(targetFlightDataDirectoryPath).exists()) {
            QDir projDir(projectDirectory);
            projDir.mkdir("./FlightData_" + sourceDirectory.dirName());
        }

        directoryName = sourceDirectory.dirName();

        QDirIterator projectFiles(sourceDirectory.absolutePath(), QDir::Files, QDirIterator::Subdirectories);

        while (projectFiles.hasNext()) {
            const QString &sourceFilePath = projectFiles.next();
            const QFileInfo &sourceFileInfo(sourceFilePath);
            QString targetFilePath = targetFlightDataDirectoryPath + "/" + observer + "_" +
                   renameFile(sourceFileInfo.fileName(), sourceDirectory.dirName());

            if (!QFile::copy(sourceFilePath, targetFilePath)) {
                qDebug() << "Unable to copy " << sourceFile << " to " << targetFilePath;
            }
        }
    }else{
        targetFlightDataDirectoryPath = sourceFlightDataDirectoryPath;
    }
    QDirIterator wavFileIterator(targetFlightDataDirectoryPath, QStringList() << "*.WAV", QDir::Files, QDirIterator::Subdirectories);

    QRegExp rx("-");// edit 4.25.2020 rx("(-)")
    QRegExp notNums("[^0-9]");

    //qDebug() << flightFilePath;

    //Load GPS File
    if(!directoryName.isEmpty()) {
        flightFilePath = targetFlightDataDirectoryPath + "/" + observer + "_" + directoryName + ".GPS";
    }
    else {
         QDirIterator gpsFileIterator(targetFlightDataDirectoryPath, QStringList() << "*.GPS", QDir::Files, QDirIterator::Subdirectories);
         while(gpsFileIterator.hasNext()) {
             const QString &sourceFilePath = gpsFileIterator.next();
             flightFilePath = sourceFilePath;
         }
    }

    m_flightFilePath = flightFilePath;

    QFile gpsFile(flightFilePath);
    if (!gpsFile.exists()) {
        qDebug() << "No FLIGHT.GPS file. Exiting. Import failed.";
        return;
    }

    QString lastFlightLineTime = "";
    QStringList flightLines;
    QStringList gpsTimes;
    QStringList wavFileItems; //added 4.25.2020

    if (gpsFile.open(QIODevice::ReadOnly)){
        QTextStream stream(&gpsFile);

        while(!stream.atEnd()){
            QString currentFlightLine = stream.readLine();
            QStringList column = currentFlightLine.split(",");
            if(lastFlightLineTime != column[5]){
                //Loading all lines in the FLIGHT.GPS file that aren't
                //duplicates into the flightLines QStringList
                flightLines << currentFlightLine;
                QStringList timePieces = column[5].split(":");
                gpsTimes.append("1" + timePieces[0] + timePieces[1] + timePieces[2]);
            }
            lastFlightLineTime = column[5];
        }
        gpsFile.close();
    }

    //Looping the wav files.
    while (wavFileIterator.hasNext()) {
        QString wavFilePath = wavFileIterator.next();
        QFileInfo wavFileInfo(wavFileIterator.filePath());
        QString wavFileName = wavFileInfo.fileName();

        //Break the name into a string list with [HH,MM,SS]
        QStringList query = wavFileName.split(rx);
        query.replaceInStrings(notNums, "");
        //qDebug() << "query[0] " + query[0].right(2);
        //qDebug() << "query[1] " + query[1];
        //qDebug() << "query[2] " + query[2];

        QString compareLine = "1" + query[0].right(2) + query[1] + query[2];
        QString matchLine = "";
        QString wavfile = "";//added 4.25.2020
        //qDebug() << "--Looking for " + compareLine;

        for(int i=0; i < gpsTimes.length(); i++){
            //qDebug() << "--Looking gpsTimes " + gpsTimes[i];
            //Found the exact time.
            if(compareLine == gpsTimes[i]){
                //qDebug() << "Found exact match at " << flightLines[i];
                wavfile = flightLines[i].append("," + wavFileName);//added 4.26.2020 flightLines[i].insert(0,wavFileName + ",");
                matchLine = flightLines[i];
                wavFileItems << wavfile;//added 4.26.2020
                break;
            }else if(compareLine < gpsTimes[i]){
                //We've passed it. Use the last time.
                if(i>0){
                    //qDebug() << "Using previous time " << flightLines[i-1];
                    wavfile = flightLines[i-1].append("," + wavFileName);//added 4.26.2020
                    matchLine = flightLines[i-1];
                    wavFileItems << wavfile;//added 4.26.2020
                    break;
                }else{
                    //qDebug() << "Using first time " << flightLines[0];
                    wavfile = flightLines[0].append("," + wavFileName);//added 4.26.2020
                    matchLine = flightLines[0];
                    wavFileItems << wavfile;//added 4.26.2020
                    break;
                }
            }
        }
        if(matchLine.length() == 0){
            //qDebug() << "None found. Using the last time in the file " << flightLines[flightLines.length() - 1];
            matchLine = flightLines[flightLines.length() - 1];
        }
        if(recordings.size() == 0){
            QStringList column = matchLine.split(",");
            globalSettings.set("Year",  column[2]);
            globalSettings.set("Month", column[3]);
            globalSettings.set("Day",   column[4]);

            proj->buildHeaderCell(ui->HeaderFields,proj->getInstanceHeaderPosition("Year"));
            proj->buildHeaderCell(ui->HeaderFields,proj->getInstanceHeaderPosition("Month"));
            proj->buildHeaderCell(ui->HeaderFields,proj->getInstanceHeaderPosition("Day"));
        }
        //  this is where the import data is parsed
        GPSRecording *record = new GPSRecording(QStringList(matchLine), wavFilePath);
        recordings.append(record);
        names.append(wavFileName);
        //qDebug() << "with times " << record->times;

        //-- need to do someting about this..
        audio->fileList->addMedia(QUrl::fromLocalFile(wavFilePath));
    }

    if(globalSettings.get("autoUnit") == QLatin1String("yes")){
        GPS_MAP_ENABLED = true;

        qDebug() << "Map enabled";

        //remarked to be moved at start of dialog
        /*ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/gps/main_copy.qml")));
        ui->quickWidget->setResizeMode(ui->quickWidget->SizeRootObjectToView);
        ui->quickWidget->show();*/

        gps = new GpsHandler(ui->quickWidget->rootObject(),  startDir); //handle plotting our gps coordinates

        //OK I'm guessing we need to send the GPS coordinates in the Flight GPS file to the map
        gps->ParseGpsStream(flightLines);

        //added 5.25.2020 populateWavList
        int intval = 0;
        if(ui->lockGPS->checkState() == Qt::Checked)
            intval = 1;
        else if(ui->lockGPS->checkState() == Qt::Unchecked)
            intval = 0;

        const QString &surveyType = proj->getSurveyType();
        QMetaObject::invokeMethod(gps->wayHandler, "plotWavFile",
            Q_ARG(QVariant, wavFileItems),Q_ARG(QVariant, intval),Q_ARG(QVariant, surveyType));

        //set the maximum observe distance in wayHandler //added 5.15.2020
        double obsmaxdistance = proj->getObsMaxD();
        QVariant returnedval;
        QMetaObject::invokeMethod(gps->wayHandler, "setMaxDist",
            Q_RETURN_ARG(QVariant,returnedval),
            Q_ARG(QVariant, obsmaxdistance),
            Q_ARG(QVariant, "new"),
            Q_ARG(QVariant, proj->getAirGroundMaxD()));

        if(surveyType == QLatin1String("BAEA")){
            surveyJsonPolygonList = readGeoJson2(proj->geoJSON);
            //Get All Plot Name List
            proj->loadPlotNameList();
            QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyPolygonJson",
                Q_ARG(QVariant, QVariant::fromValue(surveyJsonPolygonList)));
        }else if(surveyType == QLatin1String("GOEA")){

            surveyJsonSegmentListGOEA = readGeoJson(proj->geoJSON);

            //BCR and Transect Validation
            proj->loadGOEAProperties();

            //loadBcrTrnList();//added 5.22.2020
            /*for(int i = 0; i < surveyJsonSegmentListGOEA.length(); i++){
                qDebug() << surveyJsonSegmentListGOEA[i];
            }*/

            //BCR, BCRTrn, BegLat, BegLng, EndLat, EndLng, FID, ID, KM, TRN
            QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyJsonGOEA",
                Q_ARG(QVariant, QVariant::fromValue(surveyJsonSegmentListGOEA)));
        }else{
            surveyJsonSegmentList = readGeoJson(proj->geoJSON);

            //Load Properties for Transect Stratum Segment
            proj->loadWBPSProperties();

            //Load Properties for AGCode
            proj->loadAirGroundProperties();

            QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyJson",
                Q_ARG(QVariant, QVariant::fromValue(surveyJsonSegmentList)));

            //for reading the air ground json
            airGroundSurveyJsonSegmentList = readGeoJson(proj->airGround);

            QMetaObject::invokeMethod(gps->wayHandler, "plotAirGroundJson",
                Q_ARG(QVariant, QVariant::fromValue(airGroundSurveyJsonSegmentList)));
        }
    }

    names.sort();
    ui->Play->setChecked(true);

    //qDebug() << "wavFileList1";
    ui->wavFileList->addItems(names);
    try {
        if (!audio->fileList->isEmpty()){
            audio->player->play();
            ui->Play->setText("PAUSE");
            //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);//remarked 8.20.2020
        }
    } catch (...) {
        qDebug() << "Invalid to Play";
        //import->close();
    }
    //import->close();
}

void newmainwindow::on_actionImport_Flight_Data_triggered()
{
    QStringList names;

    // Make sure the project's FlightData directory exists
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    QString projectDirectory = globalSettings.get("projectDirectory");
    QString observer = globalSettings.get("observerInitials");

    if (projectDirectory == nullptr || projectDirectory.isEmpty()){
        MessageDialog alert;
        alert.setTitle("Error");
        alert.setText("Project directory has not been configured.\nUnable to import flight data at this time.");
        alert.exec();
        return;
    }

    const QString &sourceFlightDataDirectoryPath = QFileDialog::getExistingDirectory(nullptr, "Select The Flight Directory", startDir);


    QFile gpsFile(sourceFlightDataDirectoryPath);
    if (!gpsFile.exists()) {
        qDebug() << "No FLIGHT.GPS file. Exiting. Import failed.";
        return;
    }


    QStringList flightdata;
    if (gpsFile.open(QIODevice::ReadOnly)){
        QTextStream stream(&gpsFile);
        while(!stream.atEnd()){
            QString currentFlightLine = stream.readLine();
            flightdata << currentFlightLine;
        }
        gpsFile.close();
    }

    //added 7.4.2020 for the interpolation
    QDirIterator gpsFileIterator(sourceFlightDataDirectoryPath, QStringList() << "*.GPS", QDir::Files, QDirIterator::Subdirectories);
    QString flightFilePath = "";
    while(gpsFileIterator.hasNext()) {
        const QString &sourceFilePath = gpsFileIterator.next();
        flightFilePath = sourceFilePath;
    }

    if(flightdata.length() > 0){
        QList<QStringList> missinglist = calculateMissingSec(flightdata);
        if (missinglist.length() > 1)
            saveGps(flightdata,missinglist,flightFilePath.replace(".GPS", "_TMP.GPS"));
    }

    WorkerThreadImportFlight *workerThread = new WorkerThreadImportFlight();
    //workerThread->setFlightPath(flightFilePath);
    //workerThread->setFlightData(flightdata);
    workerThread->setCopyFiles(true);
    workerThread->setSourceFlightData(sourceFlightDataDirectoryPath);
    workerThread->setProjectFile(proj);
    workerThread->setAudioPlayer(audio);

    connect(workerThread, &WorkerThreadImportFlight::finished, workerThread, &QObject::deleteLater);
    //Send a record in Audio

    connect(workerThread, &WorkerThreadImportFlight::sendRecords, this, &newmainwindow::sendRecords);
    //Plot GPS

    connect(workerThread, &WorkerThreadImportFlight::plotGPS, this, &newmainwindow::plotGPS);
    //Plot Survey
    connect(workerThread, &WorkerThreadImportFlight::plotBAEA, this, &newmainwindow::plotBAEA);
    connect(workerThread, &WorkerThreadImportFlight::plotGOEA, this, &newmainwindow::plotGOEA);
    connect(workerThread, &WorkerThreadImportFlight::plotWPHS, this, &newmainwindow::plotWPHS);

    //Play Audio2
    connect(workerThread, &WorkerThreadImportFlight::playAudio, this, &newmainwindow::playAudio);

    //Calculate Missing Sec
    /*connect(workerThread, &WorkerThreadImportFlight::calculateMissingSec,
            this, [=](QString flightpath, QStringList flighdataD) {

        progMessage->setValue(20);
    });*/

    //Load Finished
    connect(workerThread, &WorkerThreadImportFlight::loadReady, this, [=]() {

        progMessage->setValue(100);
        progMessage->okToClose = true;
        progMessage->close();

    });


    connect(workerThread, &WorkerThreadImportFlight::load0, this, [=]() {

        progMessage->setValue(10);

    });

    //Progress bar
    connect(workerThread, &WorkerThreadImportFlight::load1, this, [=]() {

        progMessage->setValue(30);

    });

    connect(workerThread, &WorkerThreadImportFlight::load2, this, [=]() {

        progMessage->setValue(40);

    });

    connect(workerThread, &WorkerThreadImportFlight::load3, this, [=]() {

        progMessage->setValue(52);

    });

    connect(workerThread, &WorkerThreadImportFlight::load4, this, [=]() {

        progMessage->setValue(65);

    });

    connect(workerThread, &WorkerThreadImportFlight::load5, this, [=]() {

        progMessage->setValue(82);

    });

    connect(workerThread, &WorkerThreadImportFlight::unexpectedFinished, this, [=]() {

        progMessage->setValue(100);
        progMessage->okToClose = true;
        progMessage->close();

    });

    progMessage = new progressmessage(this);//added 7.30.2020
    progMessage->setMax(100);
    progMessage->show();

    workerThread->start();
}

void newmainwindow::on_lockGPS_stateChanged(int arg1)
{
    Q_UNUSED(arg1)
    /*if(arg1 == 2 && ui->wavFileList->count() > 0){
        //Add Lock

        QMetaObject::invokeMethod(gps->wayHandler, "changeMarkerToLockGPS",
            Q_ARG(QVariant, ui->wavFileList->currentText()));

        wavFileLock = ui->wavFileList->currentText();
    }else{

        //Remove lock

        QMetaObject::invokeMethod(gps->wayHandler, "selectWavFile",
            Q_ARG(QVariant, wavFileLock));

        wavFileLock = "";
    }*/
}


void newmainwindow::AutoSave()
{
   if (proj->changesMade) {
        GlobalSettings &globalSettings = GlobalSettings::getInstance();
        QString projectDirectory = globalSettings.get("projectDirectory");

        if (projectDirectory == nullptr || projectDirectory.isEmpty()){
            qDebug() << "Project directory not set. Aborting autosave.";
            return;
        }

        QString autosaveDirectory = projectDirectory + "/AutoSaves";

        if (!QDir(autosaveDirectory).exists()) {
            QDir dir(projectDirectory);
            dir.mkdir("./AutoSaves");
        }

        QDirIterator it(QDir(autosaveDirectory).absolutePath(), QDir::Files, QDirIterator::Subdirectories);
        QStringList taggedFiles;

        while (it.hasNext()) {
            if (it.filePath().endsWith("_autosave.sproj")){
                taggedFiles << it.filePath();
            }
            it.next();
        }

        taggedFiles.sort();

        // Remove older autosave files (Keep most recent 4)
        while (taggedFiles.length() > 4) {
            //qDebug() << "Removing autosave file at " << taggedFiles[0].trimmed();
            QFile curFile(taggedFiles[0]);
            curFile.remove();
            taggedFiles.removeAt(0);
        }

        QString autosaveFile =
                autosaveDirectory +
                "/" +
                QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") +
                "_autosave.sproj";

        proj->Save(autosaveFile, true);

       //qDebug()<< "Autosaved to " << autosaveFile;
   }
}

void newmainwindow::ProjectSettingsFinished()
{
    on_actionAdditional_Fields_triggered();
    PopulateHeaderFields();
}

void newmainwindow::on_actionSave_Transcribed_Data_triggered()
{
    AppendASC();
    ui->Observations->setRowCount(0);
}


void newmainwindow::on_actionOpen_Survey_Auto_Save_Backup_triggered()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    const QString &direct = QFileDialog::getOpenFileName(this, "Select Project", globalSettings.get("projectDirectory"), "Scribe project file (*sproj)");
    if (direct.isEmpty()){
        return;
    }
    qDebug() << "Load Called";
    proj->Load(direct);
    //fieldDialog->hide();//remarked 5.6.2020
    on_actionAdditional_Fields_triggered();
    PopulateHeaderFields();
}


void newmainwindow::on_actionImport_Survey_Template_triggered()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    const QString &surveyFile = QFileDialog::getOpenFileName(this,"Select Survey Template",globalSettings.get("projectDirectory"),"Scribe survey file (*.surv)");
    if (surveyFile.isEmpty()){
        return;
    }

    clearHeaderFields();
    clearAdditionalFields();

    WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
    workerThread->setProjectFile(proj);
    workerThread->setSurveyFile(surveyFile);
    workerThread->setAction(0);

    //Refresh Header Fields
    connect(workerThread, &WorkerThreadSurvey::sendHeaderFieldsNoVal,
            this, &newmainwindow::sendHeaderFieldsNoVal);
    connect(workerThread, &WorkerThreadSurvey::sendHeaderFieldsOneVal,
            this, &newmainwindow::sendHeaderFieldsOneVal);
    connect(workerThread, &WorkerThreadSurvey::sendHeaderFieldsValues,
            this, &newmainwindow::sendHeaderFieldsValues);
    connect(workerThread, &WorkerThreadSurvey::sendHeaderFieldsOneValNoAddded,
            this, &newmainwindow::sendHeaderFieldsOneValNoAddded);

    //Refresh Additional Fields
    connect(workerThread, &WorkerThreadSurvey::sendAdditionalFieldsNoVal,
            this, &newmainwindow::sendAdditionalFieldsNoVal);
    connect(workerThread, &WorkerThreadSurvey::sendAdditionalFieldsOneVal,
            this, &newmainwindow::sendAdditionalFieldsOneVal);
    connect(workerThread, &WorkerThreadSurvey::sendAdditionalFieldsValues,
            this, &newmainwindow::sendAdditionalFieldsValues);

    connect(workerThread, &WorkerThreadSurvey::finished, workerThread, &QObject::deleteLater);
    connect(workerThread, &WorkerThreadSurvey::importDone, this, [=]() {
        connectAllHeader();
        additionalConnect();

        progMessage->setValue(74);

        loadSpeciesHotTable();

        progMessage->setValue(84);

        connect(ui->speciesTable, SIGNAL(cellClicked(int,int)), this, SLOT(setSpecies(int,int)));

        CreateLogItemShortcuts();

        progMessage->setValue(90);

        BuildBookmarksTableColumns();

        if (ui->wavFileList->count() > 0) {
            selectWavFileMap(ui->wavFileList->currentIndex());
            progMessage->setValue(103);
        }

        progMessage->setValue(120);
        progMessage->okToClose = true;
        progMessage->close();
    });

    //Progress Bar
    connect(workerThread, &WorkerThreadSurvey::load1, this, [=]() {

        progMessage->setValue(24);

    });

    connect(workerThread, &WorkerThreadSurvey::load2, this, [=]() {

        progMessage->setValue(48);

    });

    connect(workerThread, &WorkerThreadSurvey::load3, this, [=]() {

        progMessage->setValue(62);

    });

    workerThread->start();

    progMessage = new progressmessage();
    progMessage->setMax(120);
    progMessage->show();
}

void newmainwindow::additionalConnect()
{    
    adjustAdditionalSize();
    connectionA.clear();
    //auto colA = proj->additionalFields;//remarked 8.11.2020
    const int &aCount = proj->countA;
    QList<CustomEventWidget*> colA = proj->additionalFields;//added 8.11.2020
    for (int t = 0; t < aCount; t++){
        const int &indexAdd = proj->additionalIndex[t];
        DocLayout *docLayoutA = new DocLayout;
        docLayoutA->setIndex(t);
        docLayoutA->setIndexTableW(indexAdd);

        if (proj->getAdditionalFieldName(indexAdd) == QLatin1String("Comment")) {
            proj->commentIndex = t;

            connection << connect(colA.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutComment);//added 8.11.2020
        }

        //Getting QSize
        connectionA << connect(colA.at(t)->document()->documentLayout(),
                    &QAbstractTextDocumentLayout::documentSizeChanged,
                    docLayoutA,
                    &DocLayout::resultA);


        //Supply QSize and fit based on content while editing
        connectionA << connect(docLayoutA,
                &DocLayout::sendSignalA,
                this,
                &newmainwindow::commentDocRect);

    }
}

void newmainwindow::additionalDisconnect()
{
    //Disable Event
    foreach (auto var, connectionA) {
        QObject::disconnect(var);
    }
}

void newmainwindow::on_actionAdditional_Fields_triggered()
{
    const int &rCount = proj->addionalFieldsNamesArray.count();

    clearAdditionalFields();

    for (int y = 0; y < rCount; y++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);//make first column readonly //6.4.2020
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->additionalfieldsTable->setItem(y, 0, item);
        QFont cellFont;//added 5.25.2020
        cellFont.setFamily("Segoe UI"); //this will set the first column to font to bold
        cellFont.setPixelSize(13); //5.25.2020
        cellFont.setBold(true);// 5.25.2020
        ui->additionalfieldsTable->item(y,0)->setData(Qt::FontRole,QVariant(cellFont)); //added 5.25.2020
        ui->additionalfieldsTable->item(y,0)->setText(proj->getAdditionalFieldName(y) + ": ");
        proj->buildAdditionalFieldCell(ui->additionalfieldsTable, y);
    }

    additionalConnect();

}

void newmainwindow::loadAdditional()
{
    proj->buildAdditionalfieldsTable(ui->additionalfieldsTable);
}


void newmainwindow::commentDocRect(const QSizeF &r, const int &index, const int &tIndex)
{
    proj->additionalFields.at(index)->setMaximumHeight(int((r.height())));
    //Strech to last
    ui->additionalfieldsTable->resizeRowToContents(tIndex);
}

void newmainwindow::commentDocRectH(const QSizeF &r, const int &index, const int &tIndex)
{
    proj->headerFields.at(index)->setMaximumHeight(int((r.height())));
    //Strech to last
    ui->HeaderFields->resizeRowToContents(tIndex);
}

void newmainwindow::stratumChanged()
{
    //Clear transect
    if(proj->transectIndex >= 0)//if(proj->transectIndex != -842150451)
        proj->headerFields.at(proj->transectIndex)->setText("");
    if(proj->segmentIndex >= 0)//if(proj->segmentIndex != -842150451)
        proj->headerFields.at(proj->segmentIndex)->setText("");
}

void newmainwindow::trasectChanged()
{
    //Clear Segment
    if(proj->segmentIndex >= 0)//if(proj->segmentIndex != -842150451)
        proj->headerFields.at(proj->segmentIndex)->setText("");
}

void newmainwindow::retainValue()
{ 
    proj->headerFields.at(proj->stratumIndex)->setText(previousWBPS[0]);
    proj->headerFields.at(proj->transectIndex)->setText(previousWBPS[1]);
    proj->headerFields.at(proj->segmentIndex)->setText(previousWBPS[2]);
}

void newmainwindow::focusOutStratum()
{
    const int &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt();

    if (!proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt() ||
            !proj->getWBPS()->checkStratum(new WBPSObj(newStratum, -1, -1))) {

        retainValue();

        MessageDialog mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Warning! The Stratum is not valid.");
        mb.exec();

        proj->headerFields.at(proj->stratumIndex)->setFocus();
    }
    else {
        collectPreValues();
    }

    obVal = proj->headerFields.at(proj->stratumIndex)->toPlainText();
}

void newmainwindow::focusOutTransect()
{
    const QString &surveyType = proj->getSurveyType();
    if(surveyType == QLatin1String("WBPHS")){
        const int &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt();
        const int &newTransect = proj->headerFields.at(proj->transectIndex)->toPlainText().toInt();

        if (!proj->headerFields.at(proj->transectIndex)->toPlainText().toInt() ||
                !proj->getWBPS()->checkTransect(new WBPSObj(newStratum, newTransect, -1))) {

            retainValue();

            MessageDialog mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! The combinations of Stratum and Transect are not valid.");
            mb.exec();

            proj->headerFields.at(proj->transectIndex)->setFocus();
        }
        else {
            collectPreValues();
        }

        //obVal = proj->headerFields.at(proj->transectIndex)->toPlainText();
    }
    else if(surveyType == QLatin1String("GOEA")){
        /*const QStringList &r = proj->getBCRTRN();
        auto colC = proj->headerFieldsC;
        QComboBox *combo1 = colC[proj->bcrIndex];
        const QString &bcrName = combo1->currentText();
        const QString &brctrnName = bcrName + "." + proj->headerFields.at(proj->transectIndex)->toPlainText();

        if (proj->getTRNPre() != proj->headerFields.at(proj->transectIndex)->toPlainText() &&
               !r.contains(brctrnName, Qt::CaseInsensitive)) {

            proj->headerFields.at(proj->transectIndex)->setText(proj->getTRNPre());

            MessageDialog msg;
            msg.setWindowTitle("Error");
            msg.setText("Warning! The combinations of Transect and BCR are not valid.");
            msg.exec();

            proj->headerFields.at(proj->transectIndex)->setFocus();

        }
        else {

            GlobalSettings &globalSettings = GlobalSettings::getInstance();
            globalSettings.set("TRNPRe",  proj->headerFields.at(proj->transectIndex)->toPlainText());

        }

        //obVal = proj->headerFields.at(proj->transectIndex)->toPlainText();*/
        validateGoeaValue();
    }
}

void newmainwindow::focusOutSegment()
{
    const int &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt();
    const int &newTransect = proj->headerFields.at(proj->transectIndex)->toPlainText().toInt();
    const int &newSegment = proj->headerFields.at(proj->segmentIndex)->toPlainText().toInt();

    if (!proj->headerFields.at(proj->segmentIndex)->toPlainText().toInt() ||
            !proj->getWBPS()->checkSegment(new WBPSObj(newStratum, newTransect, newSegment))) {

        retainValue();

        MessageDialog mb;
        mb.setWindowTitle("Error");
        mb.setText("Warning! The combinations of Stratum, Transect and Segment are not valid.");
        mb.exec();

        proj->headerFields.at(proj->segmentIndex)->setFocus();

    }
    else {
        qDebug() << "Out";
        collectPreValues();
    }

    //obVal = proj->headerFields.at(proj->segmentIndex)->toPlainText();
}


//Get Previous Value
void newmainwindow::focusInTransect()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    globalSettings.set("TRNPRe",  proj->headerFields.at(proj->transectIndex)->toPlainText());
}

void newmainwindow::focusInBCR()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    globalSettings.set("BCRPRe",  proj->headerFieldsC.at(proj->bcrIndex)->currentText());
}

//Get previous value for checking if it is edited
void newmainwindow::collectPreValues()
{
    previousWBPS.clear();
    const QString &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText();
    const QString &newTransect = proj->headerFields.at(proj->transectIndex)->toPlainText();
    const QString &newSegment = proj->headerFields.at(proj->segmentIndex)->toPlainText();
    previousWBPS.append(newStratum);
    previousWBPS.append(newTransect);
    previousWBPS.append(newSegment);
}


/*void newmainwindow::focusInStratum()
{
    //if (!retainValuePre)
    //collectPreValues();
    //retainValuePre = false;
}*/

/*void newmainwindow::focusInTransect()
{
    collectPreValues();
}*/

/*void newmainwindow::focusInSegment()
{
    //if (!retainValuePre)
        collectPreValues();
    //retainValuePre = false;
}*/


void newmainwindow::focusInPlotName()
{
    const QString &prePlotN = proj->headerFields.at(proj->plotnameIndex)->toPlainText();
    previousPlotName = prePlotN;
}

void newmainwindow::focusOutPlotName()
{
    const QString &plotName = proj->headerFields.at(proj->plotnameIndex)->toPlainText();
    if (!plotName.isEmpty()){
        const QStringList &r = proj->getAllPlotNameList();
        if (!r.contains(plotName, Qt::CaseInsensitive)) {

            proj->headerFields.at(proj->plotnameIndex)->setText(previousPlotName);

            MessageDialog mb;
            mb.setWindowTitle("Error");
            mb.setText("Invalid input Plot Name.");
            mb.exec();
            proj->headerFields.at(proj->plotnameIndex)->setFocus();
        }

    }
}


void newmainwindow::focusInAGName()
{
    const QString &preAGName =  proj->headerFields.at(proj->aGNameIndex)->toPlainText();
    previousAGName = preAGName;
}

void newmainwindow::focusOutAGName()
{
    const QString &agName = proj->headerFields.at(proj->aGNameIndex)->toPlainText();
    if (!agName.isEmpty() && agName != "offag"){
        const QStringList &r = proj->getAllAGNameList();
        if (!r.contains(agName, Qt::CaseInsensitive)) {

            proj->headerFields.at(proj->aGNameIndex)->setText(previousAGName);

            MessageDialog mb;
            mb.setWindowTitle("Error");
            mb.setText("Invalid input A_G Name.");

            proj->headerFields.at(proj->aGNameIndex)->setFocus();

        }

    }

}

void newmainwindow::focusOutBCR()
{
    /*const QStringList &r = proj->getBCRTRN();
    auto colC = proj->headerFieldsC;
    QComboBox *combo1 = colC[proj->bcrIndex];
    const QString &bcrName = combo1->currentText();
    const QString &brctrnName = bcrName + "." + proj->headerFields.at(proj->transectIndex)->toPlainText();

    if (proj->getBCRPre() != proj->headerFieldsC.at(proj->bcrIndex)->currentText() &&
          !r.contains(brctrnName, Qt::CaseInsensitive)) {

        proj->headerFieldsC.at(proj->bcrIndex)->setCurrentText(proj->getBCRPre());

        MessageDialog msg;
        msg.setWindowTitle("Error");
        msg.setText("Warning! The combinations of Transect and BCR are not valid.");
        msg.exec();

        //proj->headerFields.at(proj->transectIndex)->setFocus();
    }*/
    if (proj->getBCRPre() != proj->headerFieldsC.at(proj->bcrIndex)->currentText()) {
        validationBCR();
    }
}

void newmainwindow::focusOutComment()
{
    QString comment = proj->additionalFields.at(proj->commentIndex)->toPlainText();
    if (!comment.isEmpty()){
        if(comment.contains(",", Qt::CaseInsensitive)){
            QString newComment = comment.replace(",",";");
            proj->additionalFields.at(proj->commentIndex)->setText(newComment);
        }
    }
}

void newmainwindow::adjustSpeciesHotTable()
{

    QString surveyValues = ",";

    clearHeaderFields();

    ui->HeaderFields->setStyleSheet("font-size: 13px;"
                                    "font-family: 'Segoe UI'; border: 1px solid #D8D8D8; "
                                    "background-color:#F7F7F7; border-top:none;");

    for (int y = 0; y < proj->getHeaderCount(); y++) {
        QTableWidgetItem *item = new QTableWidgetItem();//make first column readonly
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->HeaderFields->setItem(y, 0, item);
        QFont cellFont;//added 5.25.2020
        cellFont.setFamily("Segoe UI"); //this will set the first column to font to bold
        cellFont.setPixelSize(13); //5.25.2020
        cellFont.setBold(true);// 5.25.2020
        ui->HeaderFields->item(y,0)->setData(Qt::FontRole,QVariant(cellFont)); //added 5.25.2020
        ui->HeaderFields->item(y,0)->setText(proj->getHeaderName(y) + ": ");
        //Rap - Should add also the value code as well?
        proj->buildHeaderCell(ui->HeaderFields, y);
    }

    //Connect Slot for Header
    connectAllHeader();

    //GlobalSettings &globalSettings = GlobalSettings::getInstance();
    const QString &surveyType = proj->getSurveyType();
    if(surveyType == QLatin1String("GOEA") || surveyType == QLatin1String("BAEA")){
        ui->comboBox_3->hide();
        //ui->lockGPS->show();
    }else{
        ui->comboBox_3->show();
        //ui->lockGPS->hide();
    }

    QRegExp rx("(\\;)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'

    const int &maxW = ui->speciesTable->width() - 23;

    int columnCount = 9;//updated 5.7.2020 from 5 to 9
    int numSpecies = proj->addedSpecies.length();
    int numPerCol = numSpecies / columnCount;
    if( numSpecies % columnCount > 0 ){
        numPerCol++;
    }
    int colNum = 0;
    int rowNum = 0;
    int totalLoops = numPerCol * columnCount;
    int columnWidthPer;
    int lastCol = 0;

    columnWidthPer = maxW / columnCount;

    //If there is remaining
    if (maxW % columnCount != 0) {
        int remain = (maxW % columnCount) / columnCount;
        if (remain > 0) {
            lastCol = remain;
        }
    }

    ui->speciesTable->setRowCount(0);//To clear the table.s
    ui->speciesTable->setColumnCount(0);

    ui->speciesTable->setRowCount(numPerCol);
    ui->speciesTable->setColumnCount(columnCount);

    ui->speciesTable->verticalHeader()->hide();
    ui->speciesTable->horizontalHeader()->hide();
    ui->speciesTable->setStyleSheet(QString::fromUtf8("QTableWidget{\n"
                                                      "background-color: #FFFFFF; \n"
                                                      "	border-style:solid;\n"
                                                      "	border-width:1px;\n"
                                                      "border-color: #DFDFDF; \n"
                                                      "}\n"
                                                      "QTableWidget::item{\n"
                                                      " height: 18px;\n"
                                                      "}"));//added 4.29.2020


    //ui->speciesTable->sizePolicy().setVerticalStretch(1);
    for( int i=0; i<totalLoops; i++){
        QTableWidgetItem *item = new QTableWidgetItem();
        if (colNum % 2 != 0) {
            //item->setBackgroundColor(QColor(238, 238, 238));
            item->setBackground(QBrush(QColor(238, 238, 238)));
        }
        //QAbstractItemDelegate *item = new QAbstractItemDelegate();
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->speciesTable->setContentsMargins(0,0,0,0);//added 5.7.2020
        //ui->speciesTable->setColumnWidth(colNum,98);//added 5.7.2020 set the column width

        if (colNum == numPerCol)
            ui->speciesTable->setColumnWidth(colNum, columnWidthPer + lastCol);
        else
            ui->speciesTable->setColumnWidth(colNum, columnWidthPer);

        ui->speciesTable->setItem(rowNum, colNum, item);
        QFont titleFont;
        titleFont.setFamily("Segoe UI");
        titleFont.setPixelSize(13);
        if(i < numSpecies){
            QStringList query = proj->addedSpecies[i].split(rx);
            ui->speciesTable->item(rowNum,colNum)->setText( query[0] + "  " + query[1]);
        }else{
            ui->speciesTable->item(rowNum,colNum)->setText("");
        }
        ui->speciesTable->item(rowNum,colNum)->setData(Qt::FontRole,QVariant(titleFont));
        rowNum++;

        if( rowNum == numPerCol ){
            colNum++;
            rowNum=0;
        }
     }

}

void newmainwindow::clearHeaderFields()
{
    proj->headerFields.clear();
    proj->headerIndex.clear();
    proj->headerIndexC.clear();
    proj->headerFieldsC.clear();
    proj->allHeaderV.clear();
    proj->countH = 0;
    proj->countHC = 0;

    ui->HeaderFields->setRowCount(0); //Clear them all
    ui->HeaderFields->setRowCount(proj->getHeaderCount());
    ui->HeaderFields->setColumnCount(2);

    QHeaderView *verticalHeader = ui->HeaderFields->verticalHeader();
    verticalHeader->setDefaultSectionSize(30);
}

void newmainwindow::clearAdditionalFields()
{
    proj->additionalFields.clear();
    proj->additionalIndex.clear();
    proj->countA = 0;

    ui->additionalfieldsTable->setRowCount(0); //Clearing it to rebuild
    ui->additionalfieldsTable->setRowCount(proj->addionalFieldsNamesArray.count());
    ui->additionalfieldsTable->setColumnCount(2);

    QHeaderView *verticalHeader = ui->additionalfieldsTable->verticalHeader();
    verticalHeader->setDefaultSectionSize(30);
}

void newmainwindow::loadSpeciesHotTable()
{
    QRegExp rx("(\\;)");
    const int &maxW = ui->speciesTable->width() - 23;
    int columnCount = 9;//updated 5.7.2020 from 5 to 9
    int numSpecies = proj->addedSpecies.length();
    int numPerCol = numSpecies / columnCount;
    if( numSpecies % columnCount > 0 ){
        numPerCol++;
    }
    int colNum = 0;
    int rowNum = 0;
    int totalLoops = numPerCol * columnCount;
    int columnWidthPer;
    int lastCol = 0;

    columnWidthPer = maxW / columnCount;

    //If there is remaining
    if (maxW % columnCount != 0) {
        int remain = (maxW % columnCount) / columnCount;
        if (remain > 0) {
            lastCol = remain;
        }
    }

    ui->speciesTable->setRowCount(0);//To clear the table.s
    ui->speciesTable->setColumnCount(0);

    ui->speciesTable->setRowCount(numPerCol);
    ui->speciesTable->setColumnCount(columnCount);

    ui->speciesTable->verticalHeader()->hide();
    ui->speciesTable->horizontalHeader()->hide();
    ui->speciesTable->setStyleSheet(QString::fromUtf8("QTableWidget{\n"
                                                      "background-color: #FFFFFF; \n"
                                                      "	border-style:solid;\n"
                                                      "	border-width:1px;\n"
                                                      "border-color: #DFDFDF; \n"
                                                      "}\n"
                                                      "QTableWidget::item{\n"
                                                      " height: 18px;\n"
                                                      "}"));//added 4.29.2020

    for(int i=0; i<totalLoops; i++){
        QTableWidgetItem *item = new QTableWidgetItem();

        if (colNum % 2 != 0)
            item->setBackground(QBrush(QColor(238, 238, 238)));

        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        ui->speciesTable->setContentsMargins(0,0,0,0);//added 5.7.2020

        if (colNum == numPerCol)
            ui->speciesTable->setColumnWidth(colNum, columnWidthPer + lastCol);
        else
            ui->speciesTable->setColumnWidth(colNum, columnWidthPer);

        ui->speciesTable->setItem(rowNum, colNum, item);

        QFont titleFont;
        titleFont.setFamily("Segoe UI");
        titleFont.setPixelSize(13);

        if(i < numSpecies){
            QStringList query = proj->addedSpecies[i].split(rx);
            ui->speciesTable->item(rowNum,colNum)->setText( query[0] + "  " + query[1]);
        }
        else
            ui->speciesTable->item(rowNum,colNum)->setText("");

        ui->speciesTable->item(rowNum,colNum)->setData(Qt::FontRole,QVariant(titleFont));
        rowNum++;

        if( rowNum == numPerCol ){
            colNum++;
            rowNum=0;
        }
    }
}

QString newmainwindow::populateKMLObservation(QStringList flds, QStringList obs, int idxtitle, QString style)
{
    QString sval = "";
    sval += "        <Placemark>\n";
    sval += (QString("            <name>%1</name>\n").arg(obs[idxtitle]));
    sval += ("            <description>\n");
    sval += ("<![CDATA[\n");
    sval += ("<html><body><form><table border='1' width='250px'>\n");

    QString lat,lon;
    for(int s = 0; s < flds.length(); s++){

        if(flds.at(s).toLower() == QLatin1String("longitude"))
            lon = obs.at(s);
        else if(flds.at(s).toLower() == QLatin1String("latitude"))
            lat = obs.at(s);

        sval += ("<tr style='border-color:white;'>\n"); //face='Segoe UI'
        sval += ("<td width='40%' style='background-color:#DADAFF;text-shadow: 0 0 0.2em white, 0 0 0.2em white,0 0 0.2em white;'>");
        sval += (QString("<font size='3px' style='font-family:\'Segoe UI\';font-style:regular;color:black;'><strong>%1</strong></font>\n").arg(flds.at(s)));
        sval += ("</td>\n");
        sval += (QString("<td width='55%' style='background-color:#E9E9E9;'><font size='3px' style='font-family:\'Segoe UI\';font-style:italic;color:#000000;'>%1</font>\n").arg(obs.at(s)));
        sval += ("</td>\n");
        sval += ("</tr>\n");
    }

    sval += ("</table></form></body></html>\n");
    sval += ("]]>\n");
    sval += ("        </description>\n");
    sval += (QString("            <styleUrl>%1</styleUrl>\n").arg(style));
    sval += (QString("            <gx:drawOrder>%1</gx:drawOrder>\n").arg(obs.at(0)));
    sval += ("        <Point>\n");
    sval += (QString("            <coordinates>%1,%2,0.000000</coordinates>\n").arg(lon).arg(lat));
    sval += ("        </Point>\n");
    sval += ("    </Placemark>\n");

    return sval;
}

QString newmainwindow::populateKMLFlightPath(QString strtitle, QString style)
{
    QString sval = "";
    sval += "    <Placemark>\n";
    sval += QString("        <name>%1</name>\n").arg(strtitle);
    sval += QString("        <styleUrl>%1</styleUrl>\n").arg(style);
    sval += "        <LineString>\n";
    sval += "            <tessellate>1</tessellate>\n";
    sval += "            <coordinates>\n";

    //QString lat,lon;
    for(int s = 0; s < m_flightpath.length(); s++){
        QStringList flightrow = m_flightpath[s].split(",");
        bool ok;
        float lat = flightrow[0].trimmed().toFloat(&ok);
        if(ok){
            ok = false;
            float lon = flightrow[1].trimmed().toFloat(&ok);
            if(ok){
                //sval += QString("                %1,%2,0 \n").arg(flightrow[1].trimmed()).arg(flightrow[0].trimmed());
                sval += QString("                %1,%2,0 \n").arg(lon).arg(lat);
            }
        }
    }

    sval += "            </coordinates>\n";
    sval += "        </LineString>\n";
    sval += "    </Placemark>\n";

    return sval;
}

void newmainwindow::adjustHeaderSize()
{
    ui->HeaderFields->updateGeometry();
    ui->HeaderFields->resizeColumnsToContents();
    ui->HeaderFields->setColumnWidth(0, ui->HeaderFields->columnWidth(0) + 10);

    if (ui->HeaderFields->verticalScrollBar()->isVisible() ||
            (proj->headerNamesArray.size() > 12 && this->height() <= 720))
        ui->HeaderFields->setColumnWidth(1, ui->HeaderFields->width() - ui->HeaderFields->columnWidth(0) - 25);
    else
        ui->HeaderFields->setColumnWidth(1, ui->HeaderFields->width() - ui->HeaderFields->columnWidth(0) - 10);

}

void newmainwindow::adjustAdditionalSize()
{
    ui->additionalfieldsTable->updateGeometry();
    ui->additionalfieldsTable->resizeColumnsToContents();
    ui->additionalfieldsTable->setColumnWidth(0, ui->additionalfieldsTable->columnWidth(0) + 10);
    const int &width = ui->additionalfieldsTable->width();

    if (ui->additionalfieldsTable->verticalScrollBar()->isVisible() ||
            (proj->addionalFieldsNamesArray.size() > 12 && this->height() <= 720))
       ui->additionalfieldsTable->setColumnWidth(1,
                                           width -
                                           ui->additionalfieldsTable->columnWidth(0) - 25);
    else
       ui->additionalfieldsTable->setColumnWidth(1,
                                           width -
                                       ui->additionalfieldsTable->columnWidth(0) - 10);


    /*ui->additionalfieldsTable->resizeColumnsToContents();
    ui->additionalfieldsTable->setColumnWidth(0, ui->additionalfieldsTable->columnWidth(0) + 10);

    if (ui->additionalfieldsTable->verticalScrollBar()->)
        ui->additionalfieldsTable->setColumnWidth(1,
                                                  ui->additionalfieldsTable->width() -
                                                  ui->additionalfieldsTable->columnWidth(0) - 10);
    else
        ui->additionalfieldsTable->setColumnWidth(1,
                                                  ui->additionalfieldsTable->width() -
                                                  ui->additionalfieldsTable->columnWidth(0) - 25);*/
}

void newmainwindow::PopulateHeaderFields()
{
    QString surveyValues = ",";

    clearHeaderFields();

    ui->HeaderFields->setStyleSheet("font-size: 13px;"
                                    "font-family: 'Segoe UI'; border: 1px solid #D8D8D8; "
                                    "background-color:#F7F7F7; border-top:none;");

    for (int y = 0; y < proj->getHeaderCount(); y++) {
        QTableWidgetItem *item = new QTableWidgetItem();//make first column readonly added 6.4.2020
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);//make first column readonly added 6.4.2020
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->HeaderFields->setItem(y, 0, item);
        QFont cellFont;//added 5.25.2020
        cellFont.setFamily("Segoe UI"); //this will set the first column font to bold
        cellFont.setPixelSize(13); //5.25.2020
        cellFont.setBold(true);// 5.25.2020
        ui->HeaderFields->item(y,0)->setData(Qt::FontRole,QVariant(cellFont)); //added 5.25.2020
        ui->HeaderFields->item(y,0)->setText(proj->getHeaderName(y) + ": ");
        //Rap - Should add also the value code as well?
        proj->buildHeaderCell(ui->HeaderFields, y);
    }

    ui->HeaderFields->resizeColumnToContents(1);

    //Connect Slot for Header
    connectAllHeader();

    //GlobalSettings &globalSettings = GlobalSettings::getInstance();
    /*const QString &surveyType = proj->getSurveyType();
    if(surveyType == QLatin1String("GOEA") || surveyType == QLatin1String("BAEA")){
        ui->comboBox_3->hide();
        //ui->lockGPS->show();
    }else{
        ui->comboBox_3->show();
        //ui->lockGPS->hide();
    }

    QRegExp rx("(\\;)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'

    const int &maxW = ui->speciesTable->width() - 23;

    int columnCount = 9;//updated 5.7.2020 from 5 to 9
    int numSpecies = proj->addedSpecies.length();
    int numPerCol = numSpecies / columnCount;
    if( numSpecies % columnCount > 0 ){
        numPerCol++;
    }
    int colNum = 0;
    int rowNum = 0;
    int totalLoops = numPerCol * columnCount;
    int columnWidthPer;
    int lastCol = 0;

    columnWidthPer = maxW / columnCount;

    //If there is remaining
    if (maxW % columnCount != 0) {
        int remain = (maxW % columnCount) / columnCount;
        if (remain > 0) {
            lastCol = remain;
        }
    }

    ui->speciesTable->setRowCount(0);//To clear the table.s
    ui->speciesTable->setColumnCount(0);

    ui->speciesTable->setRowCount(numPerCol);
    ui->speciesTable->setColumnCount(columnCount);

    ui->speciesTable->verticalHeader()->hide();
    ui->speciesTable->horizontalHeader()->hide();
    ui->speciesTable->setStyleSheet(QString::fromUtf8("QTableWidget{\n"
                                                      "background-color: #FFFFFF; \n"
                                                      "	border-style:solid;\n"
                                                      "	border-width:1px;\n"
                                                      "border-color: #DFDFDF; \n"
                                                      "}\n"
                                                      "QTableWidget::item{\n"
                                                      " height: 18px;\n"
                                                      "}"));//added 4.29.2020


    //ui->speciesTable->sizePolicy().setVerticalStretch(1);
    for( int i=0; i<totalLoops; i++){
        QTableWidgetItem *item = new QTableWidgetItem();
        if (colNum % 2 != 0) {
            item->setBackground(QBrush(QColor(238, 238, 238)));
        }
        //QAbstractItemDelegate *item = new QAbstractItemDelegate();
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        //item->setFont(fnt);//added 4.29.2020
        //ui->speciesTable->setRowHeight(rowNum, 15);//added 5.7.2020 set the rowheight
        ui->speciesTable->setContentsMargins(0,0,0,0);//added 5.7.2020
        //ui->speciesTable->setColumnWidth(colNum,98);//added 5.7.2020 set the column width

        if (colNum == numPerCol)
            ui->speciesTable->setColumnWidth(colNum, columnWidthPer + lastCol);
        else
            ui->speciesTable->setColumnWidth(colNum, columnWidthPer);

        ui->speciesTable->setItem(rowNum, colNum, item);
        QFont titleFont;
        titleFont.setFamily("Segoe UI");
        titleFont.setPixelSize(13);
        if(i < numSpecies){
            QStringList query = proj->addedSpecies[i].split(rx);
            ui->speciesTable->item(rowNum,colNum)->setText( query[0] + "  " + query[1].toLower());
        }else{
            ui->speciesTable->item(rowNum,colNum)->setText("");
        }
        ui->speciesTable->item(rowNum,colNum)->setData(Qt::FontRole,QVariant(titleFont));
        rowNum++;

        if( rowNum == numPerCol ){
            colNum++;
            rowNum=0;
        }
     }*/

    loadSpeciesHotTable();

    connect(ui->speciesTable, SIGNAL(cellClicked(int,int)), this, SLOT(setSpecies(int,int)));

    CreateLogItemShortcuts();
    BuildBookmarksTableColumns();
}

void newmainwindow::CreateLogItemShortcuts()
{
    disconnect (specieskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillSpeciesField(int))) ;
    disconnect (groupingskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillGroupingField(int))) ;

    //qDebug() << "Deleting Old Shortcuts";

    for (int s = 0; s < shortcutGroup->actions().length(); s++){
       QAction *shortcutAction = shortcutGroup->actions()[s];
       this->removeAction(shortcutAction);
       //qDebug()<<"Action Removed" << shortcutAction->shortcut().toString();
    }

    qDeleteAll(shortcutGroup->actions());
    shortcutGroup->setExclusive(false);

    QString errorMessage = "Error! The following shortcuts are already used, they won't be saved! \r\n";

    QStringList usedHotkeys;
    //this is cheeky, but a lot less time consuming, define hardcoded shortcuts here
    usedHotkeys.append("Ctrl+S");
    usedHotkeys.append("Ctrl+Shift+S");
    usedHotkeys.append("Ctrl+O");
    usedHotkeys.append("Space");
    usedHotkeys.append("Return");
    usedHotkeys.append("Backspace");
    usedHotkeys.append("Tab");
    usedHotkeys.append("Ctrl+Z");
    usedHotkeys.append(";");
    usedHotkeys.append("`");
    usedHotkeys.append("0");
    usedHotkeys.append("1");
    usedHotkeys.append("2");
    usedHotkeys.append("3");
    usedHotkeys.append("4");
    usedHotkeys.append("5");
    usedHotkeys.append("6");
    usedHotkeys.append("7");
    usedHotkeys.append("8");
    usedHotkeys.append("9");
    usedHotkeys.append("Alt+R");
    usedHotkeys.append("Alt+B");
    usedHotkeys.append("Shift+A"); //Additional tab
    usedHotkeys.append("Shift+H"); //Default tab

    defaultUsedHotKeys = usedHotkeys;//added 7.24.2020

    //CHECK FOR SIMILLAR USER KEYS, COME BACK TO THIS
    QStringList userDefinedKeys;

    //qDebug() << "Creating Shortcuts";

    bool conflictingKeys = false;

    for (int i = 0; i  < species.length(); i ++ ){
        QAction *shortcutAction = new QAction(this);
        shortcutGroup->addAction(shortcutAction);
        this->addAction(shortcutAction);

        QString keyVal = species[i]->shortcutKey.toString();
        //qDebug() << "Ai: " << keyVal;
        if(keyVal.length() > 0){
            if(usedHotkeys.indexOf(keyVal,0) != -1){
                conflictingKeys = true;
                errorMessage += "\r\n - '" + keyVal + "'";
                goto testconflictkeys;
            }else{
                usedHotkeys.append(keyVal);
                shortcutAction->setShortcut(species[i]->shortcutKey);
                shortcutAction->setShortcutContext(Qt::WindowShortcut);
            }
        }

        userDefinedKeys.append(shortcutAction->shortcut().toString());
        connect(shortcutAction, SIGNAL(triggered()), specieskeyMapper, SLOT(map())) ;
        specieskeyMapper -> setMapping (shortcutAction, i);
    }

    for (int i = 0; i  < groupings.length(); i ++ ){
        QAction *shortcutAction = new QAction(this);
        shortcutGroup->addAction(shortcutAction);
        this->addAction(shortcutAction);

        QString keyVal = groupings[i]->shortcutKey.toString();
        //qDebug() << "Bi: " << keyVal;
        if(keyVal.length() > 0){
            if(usedHotkeys.indexOf(keyVal,0) != -1){
                conflictingKeys = true;
                errorMessage += "\r\n - '" + keyVal + "'";
                goto testconflictkeys;
            }else{
                usedHotkeys.append(groupings[i]->shortcutKey.toString());
                shortcutAction->setShortcut(groupings[i]->shortcutKey);
                shortcutAction->setShortcutContext(Qt::WindowShortcut);
            }
        }

        userDefinedKeys.append(shortcutAction->shortcut().toString());
        connect(shortcutAction, SIGNAL(triggered()), groupingskeyMapper, SLOT(map()));
        groupingskeyMapper -> setMapping (shortcutAction, i);
    }


    for (int i = 0; i < userDefinedKeys.length(); i++){
        bool errored = false;
        QString str = userDefinedKeys.at(i);

        for (int d = 0; d < userDefinedKeys.length(); d++){
            if (str == userDefinedKeys[d] && i != d && str != ""){
                if (!errored){

                    errored = true;
                    conflictingKeys = true;
                    errorMessage += "\r\n - '" + shortcutGroup->actions()[d]->shortcut().toString() + "'(Already defined by user)";
                    QKeySequence empty("");
                    userDefinedKeys[i] = "";
                    shortcutGroup->actions()[i]->setShortcut(empty);
                    goto testconflictkeys;//added 7.23.2020
                }
            }
        }
    }

    testconflictkeys:
    if (conflictingKeys){
        projSettingsDialog->activeButton->click();
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText(errorMessage);
        mb.exec();
        return;
    }
   connect (groupingskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillGroupingField(int))) ;
   connect (specieskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillSpeciesField(int))) ;
}

bool newmainwindow::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        for(int i = 0; i < shortcutGroup->actions().length(); i++){
            if (shortcutGroup->actions()[i]->shortcut().toString().length() == 1){
                if (keyEvent->key() == shortcutGroup->actions()[i]->shortcut().toString()
                        || keyEvent->key() == Qt::Key_Return) {
                    if (target == ui->quantity && ((keyEvent->key()==Qt::Key_Enter)
                                                   || (keyEvent->key()==Qt::Key_Return)) ) {
                        //Enter or return was pressed while in the quantity box
                        ui->Observations->selectionModel()->clearSelection();
                        //QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
                        //if (ui->quantity->text().toInt()) {
                        on_insertButton_clicked();
                    }else{
                        //qDebug() << "Ignoring hot key: " << (char)keyEvent->key() << "for" << target;
                    }

                    if(target != ui->HeaderFields && target != ui->Observations &&
                            target != ui->additionalfieldsTable){
                        //qDebug() << "Target is not a header field.";
                        event->ignore();
                        return true;
                    }
                }
            }
        }
    }

    if(target == ui->additionalfieldsTable
            && event->type() == QEvent::LayoutRequest) {
        adjustAdditionalSize();
        return true;
    }

    if(target == ui->HeaderFields
            && event->type() == QEvent::LayoutRequest) {
        adjustHeaderSize();
        return true;
    }

    if((target == ui->HeaderFields || target == ui->Observations
        || target == ui->additionalfieldsTable) && event->type() == QEvent::FocusIn){
        //Disconnect Hot Keys
        //qDebug() << "Disconnecting all hot keys.";
        disconnect (specieskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillSpeciesField(int))) ;
        disconnect (groupingskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillGroupingField(int))) ;
        qDeleteAll(shortcutGroup->actions());
        shortcutGroup->setExclusive(false);
    }
    if((target == ui->HeaderFields || target == ui->Observations
        || target == ui->additionalfieldsTable) && event->type() == QEvent::FocusOut){
        //Re-connect Hot Keys
        //qDebug() << "Re-connecting all hot keys.";
        //qDebug() << "F";
        CreateLogItemShortcuts();
    }

    return QMainWindow::eventFilter(target, event);
}

void newmainwindow::BuildBookmarksTableColumns()
{
    if (ui->Observations->rowCount() > 0) {
        return;
    }

    ui->Observations->clear();
    allColumnNames.clear();
    ui->Observations->verticalHeader()->setVisible(false);

    //hide for eagle surveys (or better yet, hide if no groupings enabled)
    //GlobalSettings &globalSettings = GlobalSettings::getInstance();
    /*if( globalSettings.get("surveyType") == QLatin1String("BAEA") ||
            globalSettings.get("surveyType") == QLatin1String("GOEA") ){
        allColumnNames << required_columns_eagle;
    }else{
        allColumnNames << required_columns_1;
    }*/
    if (proj->getSurveyType() == QLatin1String("BAEA") ||
         proj->getSurveyType()  == QLatin1String("GOEA") ) {
        allColumnNames << required_columns_eagle;
    }
    else{
        allColumnNames << required_columns_1;
    }
    allColumnNames << proj->getHeaderNamesList();
    allColumnNames << required_columns_2;
    allColumnNames << proj->getAdditionalFieldsNamesList();

    ui->Observations->setColumnCount(allColumnNames.length());

    QFont titleFont;
    titleFont.setFamily("Segoe UI");
    titleFont.setPixelSize(13);

    for (int i=0; i<allColumnNames.length(); i++) {
        QTableWidgetItem *item = new QTableWidgetItem(allColumnNames[i]);
        item->setTextAlignment(Qt::AlignLeft);
        ui->Observations->setHorizontalHeaderItem(i, item);
        ui->Observations->horizontalHeaderItem(i)->setData(Qt::FontRole,QVariant(titleFont));
    }

    /*const QString &styleSheet = "::section {"
                         "spacing: 10px;"
                         "background-color: #CCCCCC;"
                         "color: #333333;"
                         "border: 1px solid #333333;"
                         "margin: 1px;"
                         "text-align: right;"
                         "font-family: arial;"
                         "font-size: 12px; }";*/

    /*const QString &styleSheet = "::section {"
                                "background-color: #FAFAFA;"
                                "border:none; "
                                "border-bottom: 1px solid #E2E2E2;"
                                "border-right: 1px solid #E2E2E2;}";*/

    ui->Observations->setFrameShape(QFrame::NoFrame);//added 6.1.2020
    ui->Observations->setFrameShadow(QFrame::Plain);//added 6.1.2020
    ui->Observations->verticalHeader()->setDefaultSectionSize(22);//added 6.1.2020

    /*
     * QAbstractButton *button =  ui->Observations->findChild<QAbstractButton *>();//added 6.1.2020
    if(button){
        QVBoxLayout *lay = new QVBoxLayout(button);
        lay->setContentsMargins(0, 0, 0, 0);
        QLabel *label = new QLabel("#");
        label->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
        label->setContentsMargins(0, 0, 0, 0);
        lay->addWidget(label);
    }*/

    updateExportHeaders();

    ui->Observations->setAlternatingRowColors(true);
    ui->Observations->setStyleSheet("alternate-background-color: #E9E9E9; background-color: #FFFFFF;");
}

void newmainwindow::updateExportHeaders()
{
    exportHeaders.clear();
    exportHeaders << proj->getHeaderNamesList();
    /*GlobalSettings &globalSettings = GlobalSettings::getInstance();
    if( globalSettings.get("surveyType") == QLatin1String("BAEA") ||  globalSettings.get("surveyType") == QLatin1String("GOEA") ){
        const QStringList &otherColumns = {"Audio File", "Latitude", "Longitude", "Altitude", "Course", "Speed", "Time", "# Satellites", "HDOP", "Species", "Quantity"};
        exportHeaders << otherColumns;
    }else{
        const QStringList &otherColumns = {"Audio File", "Latitude", "Longitude", "Altitude", "Course", "Speed", "Time", "# Satellites", "HDOP", "Species", "Quantity", "Grouping"};
        exportHeaders << otherColumns;
    }*/
    if (proj->getSurveyType() == QLatin1String("BAEA") || proj->getSurveyType() == QLatin1String("GOEA")) {
        const QStringList &otherColumns = {"Audio File", "Latitude", "Longitude", "Altitude", "Course", "Speed", "Time", "# Satellites", "HDOP", "Species", "Quantity"};
        exportHeaders << otherColumns;
    }
    else{
        const QStringList &otherColumns = {"Audio File", "Latitude", "Longitude", "Altitude", "Course", "Speed", "Time", "# Satellites", "HDOP", "Species", "Quantity", "Grouping"};
        exportHeaders << otherColumns;
    }

    exportHeaders << proj->getAdditionalFieldsNamesList();
}

void newmainwindow::closeProgram()
{
    this->close();
}

void newmainwindow::updatePlotName()
{
    /*qDebug() << "call c++ from qml";
    qDebug() << "m_plotName: " << m_plotName;
    setPlotName(m_plotName);*/
}

void newmainwindow::addHeaderItemM(QTableWidgetItem *item,
                                   const int &index , const int &condition)
{
    const QString &css = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#FFFFFF; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    const QString &cssreadonly = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#999999; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    const QJsonArray &vals = proj->headerValuesArray[index].toArray();
    const QString &headerKey = proj->getHeaderName(index);

    ui->HeaderFields->setItem(index, 0, item);
    switch (condition) {
        case 0: {
            CustomEventWidget *s = new CustomEventWidget();
            s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            s->setStyleSheet(css);
            s->setTitle(headerKey);
            s->setMinimumHeight(30);

            const int &ind = proj->allHeaderV.indexOf(headerKey);
            const QString &val = vals[0].toString();
            proj->headerFields.at(ind)->setText(val);
            break;
        }
        case 1: {
            CustomEventWidget *s = new CustomEventWidget();
            s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            //s->setStyleSheet(css);
            s->setTitle(headerKey);
            s->setMinimumHeight(30);

            //Check fields which are readonly.
            if(proj->getInstanceHeaderPosition(headerKey) != -1) {
                s->setReadOnly(true);
                s->setStyleSheet(cssreadonly);
            }else
                s->setStyleSheet(css);


            const QString &val = vals[0].toString();
            s->setText(val);

            proj->headerFields.append(s);
            proj->headerIndex.append(index);
            proj->countH += 1;
            proj->allHeaderV.append(headerKey);

            ui->HeaderFields->setCellWidget(index, 1, s);
            break;
        }
        case 2: {
            CustomComboBox *myComboBox2  = new CustomComboBox();
            myComboBox2->setEditable(false);
            myComboBox2->addItem("");
            myComboBox2->setMinimumHeight(30);

            for (int i = 0; i < vals.count(); i++){
                myComboBox2->addItem(vals[i].toString());
            }

            myComboBox2->setStyleSheet(css);
            proj->headerFieldsC.append(myComboBox2);
            proj->headerIndexC.append(index);
            proj->countHC += 1;

            ui->HeaderFields->setCellWidget(index, 1, myComboBox2);
            break;
        }
        default: {
            CustomEventWidget *s = new CustomEventWidget();
            s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            //s->setStyleSheet(css);
            s->setMinimumHeight(30);

            if(proj->getInstanceHeaderPosition(headerKey) != -1) {
                s->setStyleSheet(cssreadonly);
                s->setReadOnly(true);
                s->setText(proj->getInstanceHeaderDataValue(headerKey));
            }else
                s->setStyleSheet(css);

            proj->headerFields.append(s);
            proj->headerIndex.append(index);
            proj->countH += 1;
            proj->allHeaderV.append(headerKey);

            ui->HeaderFields->setCellWidget(index, 1, s);
            break;
        }
    }
}

void newmainwindow::sendRecords(GPSRecording *records, const QString &matchLine)
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    if(recordings.size() == 0){
        const QStringList &column = matchLine.split(",");
        globalSettings.set("Year",  column[2]);
        globalSettings.set("Month", column[3]);
        globalSettings.set("Day",   column[4]);

        proj->buildHeaderCell(ui->HeaderFields,proj->getInstanceHeaderPosition("Year"));
        proj->buildHeaderCell(ui->HeaderFields,proj->getInstanceHeaderPosition("Month"));
        proj->buildHeaderCell(ui->HeaderFields,proj->getInstanceHeaderPosition("Day"));
    }
    recordings.append(records);
}

void newmainwindow::plotGPS(const bool &autoUnit, const QStringList &flightLines, const QStringList &wavFileItems)
{
    GPS_MAP_ENABLED = autoUnit;

    gps = new GpsHandler(ui->quickWidget->rootObject(),  startDir); //handle plotting our gps coordinates

    //OK I'm guessing we need to send the GPS coordinates in the Flight GPS file to the map
    gps->ParseGpsStream(flightLines);

    //added 5.25.2020 populateWavList
    int intval = 0;
    if(ui->lockGPS->checkState() == Qt::Checked)
        intval = 1;
    else if(ui->lockGPS->checkState() == Qt::Unchecked)
        intval = 0;

    const QString &surveyType = proj->getSurveyType();
    QMetaObject::invokeMethod(gps->wayHandler, "plotWavFile",
        Q_ARG(QVariant, wavFileItems),Q_ARG(QVariant, intval),
                              Q_ARG(QVariant, surveyType));

    //set the maximum observe distance in wayHandler //added 5.15.2020
    double obsmaxdistance = proj->getObsMaxD();
    QVariant returnedval;
    QMetaObject::invokeMethod(gps->wayHandler, "setMaxDist",
        Q_RETURN_ARG(QVariant,returnedval),
        Q_ARG(QVariant, obsmaxdistance),
        Q_ARG(QVariant, "new"),
        Q_ARG(QVariant, proj->getAirGroundMaxD()));

    //added 6.18.2020
    m_flightpath = flightLines;
}

void newmainwindow::plotBAEA(const QList<QStringList> &surveyJsonPolygonList)
{
    QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyPolygonJson",
        Q_ARG(QVariant, QVariant::fromValue(surveyJsonPolygonList)));
}

void newmainwindow::plotGOEA(const QList<QStringList> &surveyJsonSegmentListGOEA)
{
    //BCR, BCRTrn, BegLat, BegLng, EndLat, EndLng, FID, ID, KM, TRN
    QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyJsonGOEA",
        Q_ARG(QVariant, QVariant::fromValue(surveyJsonSegmentListGOEA)));
}

void newmainwindow::plotWPHS(const QList<QStringList> &surveyJsonSegmentList,
                             const QList<QStringList> &airGroundSurveyJsonSegmentList)
{
    QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyJson",
        Q_ARG(QVariant, QVariant::fromValue(surveyJsonSegmentList)));

    QMetaObject::invokeMethod(gps->wayHandler, "plotAirGroundJson",
        Q_ARG(QVariant, QVariant::fromValue(airGroundSurveyJsonSegmentList)));

}

void newmainwindow::playAudio(const QStringList &names)
{
    //<qDebug() << "playAudio";
    ui->Play->setChecked(true);

    if(boolFreshLoad){//this will trigger the system not to play the audio at first load
        //qDebug() << "firstload";
        ui->wavFileList->blockSignals(true);
        //qDebug() << "wavFileList2";
        ui->wavFileList->addItems(names);
        boolFreshLoad = false;
        if(ui->wavFileList->signalsBlocked())
            ui->wavFileList->blockSignals(false);

        //added 8.25.2020
        //audio->status = 0;
    }else{
        //qDebug() << "wavFileList3";
        ui->wavFileList->addItems(names);

        try {
            if (!audio->fileList->isEmpty()){
                audio->player->play();
                ui->Play->setText("PAUSE");
                //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);//REMARKED 8.20.2020
            }
        } catch (...) {
            qDebug() << "Invalid to Play";
        }
    }


}

void newmainwindow::sendAdditionalFieldsOneVal(QTableWidgetItem *item, const int &index)
{
    const QString &css = "font-size:  13px;"
                  "font-family: 'Segoe UI';"
                  "background-color:#FFFFFF;"
                  "margin:1px;"
                  "border-style:solid;"
                  "border-width:1px;"
                  "border-color:rgba(0, 0, 0, 0.2);"
                  "padding-left:4px;";

    ui->additionalfieldsTable->setItem(index, 0, item);
    const QJsonArray &vals = proj->addionalFieldsValuesArray[index].toArray();

    CustomEventWidget *s = new CustomEventWidget();
    s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s->setStyleSheet(css);
    s->setMinimumHeight(30);
    s->setText(vals[0].toString().toLower());

    proj->additionalFields.append(s);
    proj->additionalIndex.append(index);
    ui->additionalfieldsTable->setCellWidget(index, 1, proj->additionalFields.at(proj->countA));
    proj->countA += 1;
}

void newmainwindow::sendAdditionalFieldsValues(QTableWidgetItem *item, const int &index)
{
    const QString &css = "font-size:  13px;"
                  "font-family: 'Segoe UI';"
                  "background-color:#FFFFFF;"
                  "margin:1px;"
                  "border-style:solid;"
                  "border-width:1px;"
                  "border-color:rgba(0, 0, 0, 0.2);"
                  "padding-left:4px;";

    ui->additionalfieldsTable->setItem(index, 0, item);
    const QJsonArray &vals = proj->addionalFieldsValuesArray[index].toArray();

    CustomComboBox *myComboBox2 = new CustomComboBox();
    myComboBox2->setStyleSheet(css);
    myComboBox2->setMinimumHeight(30);
    myComboBox2->setEditable(false);
    myComboBox2->addItem("");
    for (int i = 0; i < vals.count(); i ++){
        myComboBox2->addItem(vals[i].toString().toLower());
    }
    ui->additionalfieldsTable->setCellWidget(index,1,myComboBox2);
}

void newmainwindow::sendAdditionalFieldsNoVal(QTableWidgetItem *item, const int &index)
{
    const QString &css = "font-size:  13px;"
                  "font-family: 'Segoe UI';"
                  "background-color:#FFFFFF;"
                  "margin:1px;"
                  "border-style:solid;"
                  "border-width:1px;"
                  "border-color:rgba(0, 0, 0, 0.2);"
                  "padding-left:4px;";

    ui->additionalfieldsTable->setItem(index, 0, item);

    CustomEventWidget *s = new CustomEventWidget();
    s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s->setStyleSheet(css);

    s->setMinimumHeight(30);
    proj->additionalFields.append(s);
    proj->additionalIndex.append(index);
    ui->additionalfieldsTable->setCellWidget(index, 1,  proj->additionalFields.at( proj->countA));
    proj->countA += 1;
}

void newmainwindow::sendHeaderFieldsOneVal(QTableWidgetItem *item, const int &index)
{
    const QString &css = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#FFFFFF; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    const QJsonArray &vals = proj->headerValuesArray[index].toArray();
    const QString &headerKey = proj->getHeaderName(index);

    ui->HeaderFields->setItem(index, 0, item);

    CustomEventWidget *s = new CustomEventWidget();
    s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s->setStyleSheet(css);
    s->setTitle(headerKey);
    s->setMinimumHeight(30);

    //Check fields which are readonly.
    if(proj->getInstanceHeaderPosition(headerKey) != -1) {
        s->setReadOnly(true);
    }

    const QString &val = vals[0].toString();
    //s->setText(val);
    s->setText(val.toLower());

    proj->headerFields.append(s);
    proj->headerIndex.append(index);
    proj->countH += 1;
    proj->allHeaderV.append(headerKey);

    ui->HeaderFields->setCellWidget(index, 1, s);
}

void newmainwindow::sendHeaderFieldsOneValNoAddded(QTableWidgetItem *item, const int &index)
{
    const QString &css = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#FFFFFF; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    const QJsonArray &vals = proj->headerValuesArray[index].toArray();
    const QString &headerKey = proj->getHeaderName(index);

    ui->HeaderFields->setItem(index, 0, item);

    CustomEventWidget *s = new CustomEventWidget();
    s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s->setStyleSheet(css);
    //s->setTitle(headerKey);
    s->setMinimumHeight(30);

    const int &ind = proj->allHeaderV.indexOf(headerKey);
    //const QString &val = vals[0].toString();
    const QString &val = vals[0].toString().toLower();
    proj->headerFields.at(ind)->setText(val);
}

void newmainwindow::sendHeaderFieldsValues(QTableWidgetItem *item, const int &index)
{
    const QString &css = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#FFFFFF; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    const QJsonArray &vals = proj->headerValuesArray[index].toArray();

    ui->HeaderFields->setItem(index, 0, item);

    CustomComboBox *myComboBox2  = new CustomComboBox();
    myComboBox2->setEditable(false);
    myComboBox2->addItem("");
    myComboBox2->setMinimumHeight(30);

    for (int i = 0; i < vals.count(); i++){
        //myComboBox2->addItem(vals[i].toString());
        myComboBox2->addItem(vals[i].toString().toLower());
    }

    myComboBox2->setStyleSheet(css);
    proj->headerFieldsC.append(myComboBox2);
    proj->headerIndexC.append(index);
    proj->countHC += 1;

    ui->HeaderFields->setCellWidget(index, 1, myComboBox2);
}

void newmainwindow::sendHeaderFieldsNoVal(QTableWidgetItem *item, const int &index)
{
    const QString &css = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#FFFFFF; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    const QString &headerKey = proj->getHeaderName(index);

    ui->HeaderFields->setItem(index, 0, item);

    CustomEventWidget *s = new CustomEventWidget();
    s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s->setStyleSheet(css);
    s->setMinimumHeight(30);

    if(proj->getInstanceHeaderPosition(headerKey) != -1) {
        s->setReadOnly(true);
        s->setText(proj->getInstanceHeaderDataValue(headerKey));
    }

    proj->headerFields.append(s);
    proj->headerIndex.append(index);
    proj->countH += 1;
    proj->allHeaderV.append(headerKey);

    ui->HeaderFields->setCellWidget(index, 1, s);
}

void newmainwindow::CreateExportFile()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    QString exportLocation = globalSettings.get("projectDirectory") + "/" + globalSettings.get("exportFilename") + ".asc";
    QFile exportFile(exportLocation);
    int export_count = globalSettings.get("exportCount").toInt();
    if( export_count > 0 ||  exportFile.size() > 0 ){
        globalSettings.set( "exportCount", QString( export_count ) );
        projSettingsDialog->disable_field_buttons();
    }
    //qDebug() << exportLocation << ": " << exportFile;
    if (!exportFile.open(QIODevice::WriteOnly|QIODevice::Append)){
          //qDebug() << "Couldn't open file for writing";
          return;
    }

    exportFile.close();

}

void newmainwindow::refreshMap()
{
    if (auto pw = ui->quickWidget->parentWidget()){
        ui->quickWidget->resize(pw->size());
    }
}

void newmainwindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    loadSpeciesHotTable();
    if (ui->wavFileList->currentIndex() >= 0)
        QMetaObject::invokeMethod(gps->wayHandler, "setCenterPosition",
            Q_ARG(QVariant, ui->wavFileList->currentIndex()));
}

void newmainwindow::closeEvent(QCloseEvent *event)
{
    //Disable Event
    disconnectAllHeader();
     if (proj->changesMade || ui->Observations->rowCount() > 0){
        MessageDialog msgBox;
        msgBox.setText("Do you want to save your observations before exiting\n");
        msgBox.setThreeButton("Save", "Don't Save", "Cancel");
        msgBox.exec();
        if(msgBox.isAccept()) {
            //event->accept();
            AutoSave();
            AppendASC();
            saveScribeSettings(true);
            event->accept();
        }else if(msgBox.isReject()){
            saveScribeSettings(false);
            event->accept();
        }else{
            event->ignore();
            //Enable Event
            connectAllHeader();
        }
        /*msgBox.setText("Are you sure you want to exit?");
        msgBox.setTwoButton("Close", 60, "Cancel", 60);
        msgBox.exec();
        if(msgBox.isAccept()) {
            event->accept();
        }
        else{
            event->ignore();
            //Enable Event
            connectAllHeader();
        }*/
    }else{
        event->accept();
    }
}



void newmainwindow::on_actionCreate_Survey_Template_triggered()
{
    QFileDialog dialog(this);
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    dialog.setFileMode(QFileDialog::Directory);
    QString nm;
    const QString &OutputFolder = dialog.getExistingDirectory(this, tr("Select a folder to place the survey template .surv file in"), globalSettings.get("projectDirectory"));
    if(OutputFolder.length() > 0){

        WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
        workerThread->setProjectFile(proj);
        workerThread->setDir(OutputFolder);
        workerThread->setAction(1);

        connect(workerThread, &WorkerThreadSurvey::createDone, this, [=]() {

            progMessage->setValue(100);
            progMessage->okToClose = true;
            progMessage->close();

            MessageDialog msgBox;
            msgBox.setText("Survey template created successfully.");
            msgBox.setFontSize(13);
            msgBox.exec();

        });

        connect(workerThread, &WorkerThreadSurvey::load1, this, [=]() {

            progMessage->setValue(40);

        });

        connect(workerThread, &WorkerThreadSurvey::load1, this, [=]() {

            progMessage->setValue(70);

        });

        connect(workerThread, &WorkerThreadSurvey::finished,
                workerThread, &QObject::deleteLater);

        workerThread->start();

        progMessage = new progressmessage(this);//added 7.30.2020
        progMessage->setMax(100);
        progMessage->show();

    }
}



void newmainwindow::on_actionSave_triggered()
{
    progMessage = new progressmessage();
    progMessage->setMax(80);

    WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
    workerThread->setAction(10);
    workerThread->setProjectFile(proj);

    //Run after auto save
    connect(workerThread, &WorkerThreadSurvey::saveDone, this, [=]() {
        progMessage->setValue(80);
        progMessage->okToClose = true;
        progMessage->close();

    });
    connect(workerThread, &WorkerThreadSurvey::finished, workerThread, &QObject::deleteLater);

    //Progress Bar
    connect(workerThread, &WorkerThreadSurvey::load1, this, [=]() {

        progMessage->setValue(15);

    });

    connect(workerThread, &WorkerThreadSurvey::load1, this, [=]() {

        progMessage->setValue(43);

    });

    connect(workerThread, &WorkerThreadSurvey::load1, this, [=]() {

        progMessage->setValue(68);

    });

    connect(workerThread, &WorkerThreadSurvey::unexpectedFinish, this, [=]() {
        progMessage->setValue(80);
        progMessage->okToClose = true;
        progMessage->close();
    });

    workerThread->start();
    progMessage->show();
    //AutoSave();

    qDebug() << "saved exit";
}

void newmainwindow::on_actionExit_triggered()
{
    closeProgram();
}


void newmainwindow::on_actionProject_Settings_triggered()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    const int &export_count = globalSettings.get("exportCount").toInt();
    if( export_count > 0 || ui->Observations->rowCount() > 0 ){
       projSettingsDialog->disable_field_buttons();
    }

    projSettingsDialog->openingWindow = true;
    projSettingsDialog->PrepHeadFieldValues();
    projSettingsDialog->sourceWidget = ui->additionalfieldsTable;
    projSettingsDialog->PrepAdditionalFieldValues(ui->additionalfieldsTable);//added 5.6.2020
    projSettingsDialog->defaultUsedHotkeys = defaultUsedHotKeys;

    projSettingsDialog->show();
    projSettingsDialog->setFocus();
}


void newmainwindow::on_actionAbout_Scribe_triggered()
{
    /*aboutScribe = new aboutscribe();
    aboutScribe->show();*///remarked 5.29.2020
    aboutDialog = new AboutDialog(this);//added 5.29.2020 //added this 7.30.2020
    aboutDialog->show();
}

void newmainwindow::on_actionUser_Guide_triggered()
{
    userGuide = new userguide(this);//added this 7.30.2020
    userGuide->show();
}

void newmainwindow::validateGoeaValue()
{
    if (proj->getTRNPre() != proj->headerFields.at(proj->transectIndex)->toPlainText()) {
        validationTransectGOEA();
    }
}

void newmainwindow::validationTransectGOEA()
{
    const QString &trnName = proj->headerFields.at(proj->transectIndex)->toPlainText().trimmed();
    auto colC = proj->headerFieldsC;
    QComboBox *combo1 = colC[proj->bcrIndex];
    const QString &bcrName = combo1->currentText();
    const QString &brctrnName = bcrName + "." + trnName;
    const QStringList &r = proj->getBCRTRN();

    if (!r.contains(brctrnName, Qt::CaseInsensitive)) {
        if (proj->getBCRPre() == proj->headerFieldsC.at(proj->bcrIndex)->currentText()) {
            MessageDialog msgBox;
            msgBox.setText("Do you want to change the BCR?");
            msgBox.setTwoButton("Yes", 50, "No", 50);
            msgBox.exec();
            if (msgBox.isAccept()) {
                //proj->headerFieldsC.at(proj->bcrIndex)->setFocus();
                //proj->headerFieldsC.at(proj->transectIndex)->setText("");
            }
            else {
                MessageDialog msg;
                msg.setWindowTitle("Error");
                msg.setText("Warning! The combinations of Transect and BCR are not valid.");
                msg.exec();
                proj->headerFields.at(proj->transectIndex)->setFocus();
            }
        }
        else {
            MessageDialog msg;
            msg.setWindowTitle("Error");
            msg.setText("Warning! The combinations of Transect and BCR are not valid.");
            msg.exec();
            proj->headerFields.at(proj->transectIndex)->setFocus();
        }
    }
}


void newmainwindow::on_insertButton_clicked()
{
    const QString &surveyType = proj->getSurveyType();
    if(surveyType == QLatin1String("BAEA")){
        if(returnedValueBAEA.length() > 0 && returnedValueBAEA.contains(",") == true){
            //qDebug() << (returnedValueBAEA);
            QStringList splt = returnedValueBAEA.split(",");
            if(splt[1] == QLatin1String("0")){
                MessageDialog msgBox;
                msgBox.setText("This observation is beyond the\n"
                               "maximum distance to the survey boundary.\n\n"
                               "Do you want to proceed?");
                msgBox.setTwoButton("Yes", "No, do not proceed");
                msgBox.resizeHeight((154 + 56) - 38);
                msgBox.resizeWidth(334);
                msgBox.exec();
                if (!msgBox.isAccept())
                    return;

                QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                    Q_ARG(QVariant,"white"));
            }
        }

        //warn for BEGFLT/ENDFLT existance
        if(currentSpecies->itemName.toUpper() == QLatin1String("ENDFLT")){
            bool boolexist = false;
            for(int i = 0; i < ui->Observations->rowCount(); i++){
                if(ui->Observations->item(i,0)->text().toUpper() == QLatin1String("BEGFLT")){
                    boolexist = true;
                    break;
                }
            }
            if(!boolexist){
                MessageDialog msgBox;
                msgBox.setText("Please take notice that you haven't\n"
                               "entered any BEGFLT in the observation list.\n\n"
                               "Do you want to proceed?");
                msgBox.setTwoButton("Yes", "No, do not proceed");
                msgBox.resizeHeight((154 + 56) - 38);
                msgBox.resizeWidth(334);
                msgBox.exec();
                if (!msgBox.isAccept())
                    return;
            }
        }

        //warn for BEGAREA/ENDAREA existance
        if(currentSpecies->itemName.toUpper() == QLatin1String("ENDAREA")){
            bool boolexist = false;
            for(int i = 0; i < ui->Observations->rowCount(); i++){
                if(ui->Observations->item(i,0)->text().toUpper() == QLatin1String("BEGAREA")){
                    boolexist = true;
                    break;
                }
            }
            if(!boolexist){
                MessageDialog msgBox;
                msgBox.setText("Please take notice that you haven't\n"
                               "entered any BEGAREA in the observation list.\n\n"
                               "Do you want to proceed?");
                msgBox.setTwoButton("Yes", "No, do not proceed");
                msgBox.resizeHeight((154 + 56) - 38);
                msgBox.resizeWidth(334);
                msgBox.exec();
                if (!msgBox.isAccept())
                    return;
            }
        }

    }else if(surveyType == QLatin1String("WBPHS")){
        if(returnedValueWPBHS.length() > 0 && returnedValueWPBHS.contains(",") == true){
            QStringList splt = returnedValueWPBHS.split(",");
            if(splt[5] == QLatin1String("0")){
                MessageDialog msgBox;
                msgBox.setText("This observation is beyond the\n"
                               "maximum distance to the survey boundary.\n\n"
                               "Do you want to proceed?");
                msgBox.setTwoButton("Yes", "No, do not proceed");
                msgBox.resizeHeight((154 + 56) - 38);
                msgBox.resizeWidth(334);
                msgBox.exec();
                if (!msgBox.isAccept())
                    return;
                QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                    Q_ARG(QVariant,"white"));
            }
        }
    }else if(surveyType == QLatin1String("GOEA")){
        //validateGoeaValue();

        if(returnedValueGOEA.length() > 0 && returnedValueGOEA.contains(",") == true){
            QStringList splt = returnedValueGOEA.split(",");
            if(splt[2] == QLatin1String("0")){
                MessageDialog msgBox;
                msgBox.setText("This observation is beyond the\n"
                               "maximum distance to the survey boundary.\n\n"
                               "Do you want to proceed?");
                msgBox.setTwoButton("Yes", "No, do not proceed");
                msgBox.resizeHeight((154 + 56) - 38);
                msgBox.resizeWidth(334);
                msgBox.exec();
                if (!msgBox.isAccept())
                    return;

                QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                    Q_ARG(QVariant,"white"));
            }
        }

        //warn for BEGFLT/ENDFLT existance
        if(currentSpecies->itemName.toUpper() == QLatin1String("ENDFLT")){
            bool boolexist = false;
            for(int i = 0; i < ui->Observations->rowCount(); i++){
                if(ui->Observations->item(i,0)->text().toUpper() == QLatin1String("BEGFLT")){
                    boolexist = true;
                    break;
                }
            }
            if(!boolexist){
                MessageDialog msgBox;
                msgBox.setText("Please take notice that you haven't\n"
                               "entered any BEGFLT in the observation list.\n\n"
                               "Do you want to proceed?");
                msgBox.setTwoButton("Yes", "No, do not proceed");
                msgBox.resizeHeight((154 + 56) - 38);
                msgBox.resizeWidth(334);
                msgBox.exec();
                if (!msgBox.isAccept())
                    return;
            }
        }

    }

    int insertAt = 0;
    QModelIndexList selection = ui->Observations->selectionModel()->selectedRows();
    if(selection.count() > 0){
         QModelIndex index = selection.at(0);
         insertAt = index.row();
         ui->Observations->selectionModel()->clearSelection();
    }

    // Make sure we have audio recordings
    if (recordings.length() < 1){
        /*QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Error! There are no recordings!!");
        mb.exec();*/
        MessageDialog msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText("Error! There are no recordings!");
        msgBox.exec();
        return;
    }

    // Make sure we have species
    if (species.length()  < 1){
        /*QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Error! There are no species defined!");
        mb.exec();*/
        MessageDialog msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText("Error! There are no species defined!");
        msgBox.exec();
        return;
    }

    //qDebug() <<  ui->comboBox_3->currentText() << " and " <<  ui->quantity->text()  << " right!";
    if( ui->comboBox_3->currentText() == QLatin1String("FLKDRAKE") && ( ui->quantity->text() == QLatin1String("1") ||  ui->quantity->text().isEmpty() ||  ui->quantity->text() == QLatin1String("Quantity") )){
        MessageDialog msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText("Are you crazy? You can't have Flock Drakes in quantities of 1");
        msgBox.exec();
        return;
    }

    // Make sure we have groupings
    if (!currentSpecies->groupingEnabled){
        if (ui->comboBox_3->currentText() != "Open"){
            MessageDialog msgBox;
            msgBox.setWindowTitle("Error");
            msgBox.setText("Warning! Social groupings were not enabled for this species!");
            msgBox.exec();
            return;
        }
    }else if(currentSpecies->groupingEnabled){//added 7.11.2020
        if (ui->comboBox_3->currentText() == QLatin1String("Open") && ( ui->quantity->text() == QLatin1String("1") ||  ui->quantity->text() == QLatin1String("2"))){
            MessageDialog msgBox;
            msgBox.setWindowTitle("Error");
            msgBox.setText("The single or pair key must be used for this species!");
            msgBox.exec();
            return;
        }else if(ui->comboBox_3->currentText() == QLatin1String("Open") && ( ui->quantity->text() == QLatin1String("3") ||  ui->quantity->text() == QLatin1String("4"))){
            MessageDialog msgBox;
            msgBox.setTwoButton("OK", 60, "Cancel", 60);
            msgBox.setText("Hit <Enter> again if you want these 3 to 4 species to be in the\n"
                           "Open Category. Click on <Cancel> to change them to singles,\n"
                           "pairs, and/or flocked drakes." );
            msgBox.exec();
            if(msgBox.isAccept()) {

            }else{
                return;
            }
        }
    }


    BookMark *book = new BookMark();

    //solving the no quantity of species problem
    QString quant = ui->quantity->text().trimmed().remove(QRegularExpression("[^0-9]"));

    //for defaulting back to a value, will change to phils liking
    ui->quantity->setText("");

    QString latLongCoordinates;
    GPSRecordingEntry gpsEntry;

    //if(LockedGPSIndex == -1){
    latLongCoordinates = recordings[currentGpsRecording]->coords[curCoord];
    gpsEntry = recordings[currentGpsRecording]->entries[0];
    //}else{
      //  latLongCoordinates = recordings[LockedGPSIndex]->coords[curCoord];
        //gpsEntry = recordings[LockedGPSIndex]->entries[0];
   // }

    QStringList coords = latLongCoordinates.split(",");
    book->species = currentSpecies->itemName;
    book->grouping = ui->comboBox_3->currentText();
    book->latitude = coords[0];
    book->longitude = coords[1];
    book->audioFileName = ui->wavFileList->currentText();
    book->timestamp = gpsEntry.timestamp;
    book->altitude = book->altitude.setNum(gpsEntry.altitude);
    book->speed = book->speed.setNum(gpsEntry.speed);
    //book->course = gpsEntry.course;
    book->course = gpsEntry.course.toLower();
    book->numSatellites = QString::number(gpsEntry.number_of_satellites);
    book->hdop = book->hdop.setNum(gpsEntry.hdop);


    // Add quantity to Bookmark
    if (quant.isEmpty() || quant == QLatin1String("0") || quant.toInt() == 0){
        if(surveyType == QLatin1String("WBPHS")){
            book->quantity = 1;
        }else{
            book->quantity = 0;
        }
    }else{
        book->quantity = quant.toInt();
    }
    QString quantValue = "";
    if(book->quantity > 0){
        quantValue = QString::number(book->quantity);
    }

    //need to insert all new rows at top.
    ui->Observations->insertRow(insertAt);
    const int &currentRow = insertAt;

    QFont cellFont;//added 5.7.2020 unnecessary code
    cellFont.setFamily("Segoe UI"); //this will set the first column to font to bold
    cellFont.setPixelSize(13); //added5.7.2020
    cellFont.setCapitalization(QFont::AllLowercase);

    QFont fontUpper = proj->getFontItem(false);
    fontUpper.setCapitalization(QFont::AllUppercase);

    //ui->Observations->setItem(currentRow, allColumnNames.indexOf("Species"), new QTableWidgetItem(book->species));
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Species"), new QTableWidgetItem(book->species));
    ui->Observations->item(currentRow, allColumnNames.indexOf("Species"))->setData(Qt::FontRole,QVariant(fontUpper));

    QTableWidgetItem *quantityItem = new QTableWidgetItem();
    quantityItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    quantityItem->setText(quantValue);

    //ui->Observations->setItem(currentRow, allColumnNames.indexOf("Quantity"), new QTableWidgetItem(quantValue));
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Quantity"), quantityItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Quantity"))->setData(Qt::FontRole,QVariant(cellFont));
    //connect(ui->Observations->item(currentRow, allColumnNames.indexOf("Quantity")), )
    //hide for eagle surveys (or better yet, hide if no groupings enabled)

    if(surveyType != "BAEA" && surveyType != "GOEA" ){
        ui->Observations->setItem(currentRow, allColumnNames.indexOf("Grouping"), new QTableWidgetItem(book->grouping.toLower()));
        ui->Observations->item(currentRow, allColumnNames.indexOf("Grouping"))->setData(Qt::FontRole,QVariant(cellFont));
    }

    QTableWidgetItem *latitudeItem = new QTableWidgetItem(book->latitude);
    latitudeItem->setFlags(latitudeItem->flags() ^ Qt::ItemIsEditable);
    latitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    latitudeItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Latitude"), latitudeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Latitude"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *longitudeItem = new QTableWidgetItem(book->longitude);
    longitudeItem->setFlags(longitudeItem->flags() ^ Qt::ItemIsEditable);
    longitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    longitudeItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Longitude"), longitudeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Longitude"))->setData(Qt::FontRole,QVariant(cellFont));
    //ui->Observations->setItem(currentRow, allColumnNames.indexOf("Longitude"), longitudeItem);

    QTableWidgetItem *timeItem = new QTableWidgetItem(book->timestamp);
    timeItem->setFlags(longitudeItem->flags() ^ Qt::ItemIsEditable);
    timeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    timeItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Time"), timeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Time"))->setData(Qt::FontRole,QVariant(cellFont));
    //timeItem->setFlags(timeItem->flags() ^ Qt::ItemIsEditable);
    //ui->Observations->setItem(currentRow, allColumnNames.indexOf("Time"), timeItem);

    QTableWidgetItem *altitudeItem = new QTableWidgetItem(book->altitude);
    altitudeItem->setFlags(altitudeItem->flags() ^ Qt::ItemIsEditable);
    altitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    altitudeItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Altitude"), altitudeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Altitude"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *speedItem = new QTableWidgetItem(book->speed);
    speedItem->setFlags(speedItem->flags() ^ Qt::ItemIsEditable);
    speedItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    speedItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Speed"), speedItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Speed"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *courseItem = new QTableWidgetItem(book->course);
    courseItem->setFlags(courseItem->flags() ^ Qt::ItemIsEditable);
    courseItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Course"), courseItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Course"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *HDOPItem = new QTableWidgetItem(book->hdop);
    HDOPItem->setFlags(HDOPItem->flags() ^ Qt::ItemIsEditable);
    HDOPItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    HDOPItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("HDOP"), HDOPItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("HDOP"))->setData(Qt::FontRole,QVariant(cellFont));


    QTableWidgetItem *satItem = new QTableWidgetItem(book->numSatellites);
    satItem->setFlags(satItem->flags() ^ Qt::ItemIsEditable);
    satItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    satItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("# Satellites"), satItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("# Satellites"))->setData(Qt::FontRole,QVariant(cellFont));


    QTableWidgetItem *audioItem = new QTableWidgetItem(book->audioFileName);
    audioItem->setFlags(audioItem->flags() ^ Qt::ItemIsEditable);
    audioItem->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Audio File"), audioItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Audio File"))->setData(Qt::FontRole,QVariant(cellFont));


    // Insert headers into Observations Table
    for (int i=0; i<proj->getHeaderCount(); i++) {
        QString headerKey = proj->getHeaderName(i);
        int columnToInsert = allColumnNames.indexOf(headerKey);
        QString val;

        val = proj->getHeaderValue(ui->HeaderFields, i);

        //Check exist in All Fields that unchecked
        if (proj->headerFieldsChk.contains(headerKey)) {
            //Clear the value
            CustomEventWidget *textEditItem = dynamic_cast<CustomEventWidget*>(ui->HeaderFields->cellWidget(i, 1));
            textEditItem->setText("");
        }
        else if(proj->hAFieldsCb.contains(headerKey)) {
            //Clear the value
            CustomComboBox *comboBoxItem = dynamic_cast<CustomComboBox*>(ui->HeaderFields->cellWidget(i, 1));
            comboBoxItem->setCurrentIndex(0);
        }

        QTableWidgetItem *headerValue = new QTableWidgetItem(val);

        //not allow to be edited
        if(proj->getInstanceHeaderPosition(headerKey) != -1){
            headerValue->setFlags(headerValue->flags() & ~Qt::ItemIsEditable);
            headerValue->setBackground(QBrush(QColor(153,153,153)));//added 7.16.2020
        }

        ui->Observations->setItem(currentRow, columnToInsert, headerValue);
        ui->Observations->item(currentRow, columnToInsert)->setData(Qt::FontRole,QVariant(cellFont));

    }

    // Insert additional fields into Observations Table
    for (int i=0; i<proj->getAdditionalFieldsCount(); i++) {
        const QString &additionalFieldKey = proj->getAdditionalFieldName(i);
        const int &columnToInsert = allColumnNames.indexOf(additionalFieldKey);

        const QString &val = proj->getAdditionalFieldValue(ui->additionalfieldsTable, i);

        //Check exist in All Fields that unchecked
        if (proj->additionalFieldsChk.contains(additionalFieldKey)) {
            //Clear the value
            CustomEventWidget *textEditItem = dynamic_cast<CustomEventWidget*>(ui->additionalfieldsTable->cellWidget(i, 1));
            textEditItem->setText("");
        }
        else if(proj->aFieldsCb.contains(additionalFieldKey)) {
            //Clear the value
            CustomComboBox *comboBoxItem = dynamic_cast<CustomComboBox*>(ui->additionalfieldsTable->cellWidget(i, 1));
            comboBoxItem->setCurrentIndex(0);
        }

        QTableWidgetItem *additionalFieldValue = new QTableWidgetItem(val);
        ui->Observations->setItem(currentRow, columnToInsert, additionalFieldValue);
        ui->Observations->item(currentRow, columnToInsert)->setData(Qt::FontRole,QVariant(cellFont));
    }

    //qDebug() << "book2";
    // Is this for the map----------------??
    /*
     QVariant wayDesc;
     wayDesc = book->species + " " + QString::number(book->quantity) + " " + book->grouping;
     QMetaObject::invokeMethod(gps->wayHandler, "addBookMarkAtCurrent",
     Q_ARG(QVariant, wayDesc));
    */

    ui->comboBox_3->setCurrentIndex(0);
    ui->quantity->setFocus();
    insertWasLastEvent = true;

    if (ui->Observations->rowCount() == 1)
        connect(ui->Observations, SIGNAL(cellDoubleClicked(int,int)),
            this, SLOT(on_observation_doubleClicked(int,int)));

}

void newmainwindow::disconnectObservation() {

    foreach (auto var, connectionObservation) {
        QObject::disconnect(var);
    }
    connectionObservation.clear();
    pressDoubleClick = false;
}

bool newmainwindow::condition(const QString &grouping, const QString &quantity) {

    if(grouping == QLatin1String("flkdrake") &&
             (quantity == QLatin1String("1")
              ||  quantity.isEmpty()
              ||  quantity == QLatin1String("Quantity"))) {
        MessageDialog msgBox;
        msgBox.setWindowTitle("Error");
        msgBox.setText("Are you crazy? You can't have Flock Drakes in quantities of 1");
        msgBox.exec();
        return false;
    }
    else if(!currentSpeciesOb->groupingEnabled) {
        if (grouping != "open") {
            MessageDialog msgBox;
            msgBox.setWindowTitle("Error");
            msgBox.setText("Warning! Social groupings were not enabled for this species!");
            msgBox.exec();
            return true;
        }
    }
    else if(currentSpeciesOb->groupingEnabled) {
        if (grouping == QLatin1String("Open") &&
                (quantity == QLatin1String("1") ||  quantity == QLatin1String("2"))){
            MessageDialog msgBox;
            msgBox.setWindowTitle("Error");
            msgBox.setText("The single or pair key must be used for this species!");
            msgBox.exec();
            return false;
        }
        else if(grouping == QLatin1String("Open") &&
                (quantity == QLatin1String("3") || quantity == QLatin1String("4"))){
            MessageDialog msgBox;
            msgBox.setTwoButton("OK", 60, "Cancel", 60);
            msgBox.setText("Hit <Enter> again if you want these 3 to 4 species to be in the\n"
                            "Open Category. Click on <Cancel> to change them to singles,\n"
                            "pairs, and/or flocked drakes." );
            msgBox.exec();
            if(msgBox.isAccept()) {
                return true;
            }else{
                return false;
            }
        }
    }
    return true;
}

void newmainwindow::on_observation_doubleClicked(int row, int column)
{
    pressDoubleClick = true;
    const QString &currentValue = ui->Observations->item(row, column)->text();
    const QString &header = ui->Observations->horizontalHeaderItem(column)->text();
    //Get All Header Lists
    const QStringList &headerLists = proj->getHeaderNamesList();
    //Get All Additional Lists
    const QStringList &addiotionalLists = proj->getAdditionalFieldsNamesList();
    const QString &surveyType = proj->getSurveyType();
    const QString &speciesV =
            ui->Observations->item(row, 0)->text();

    QFont cellFont;
    cellFont.setFamily("Segoe UI");
    cellFont.setPixelSize(13);
    cellFont.setCapitalization(QFont::AllLowercase);

    obRow = row;
    obCol = column;

    //Get Current Species item for Observation
    for(int i = 0; i < species.length(); i++){
        if (species[i]->itemName == speciesV){
            currentSpeciesOb = species[i];
            break;
        }
    }

    //Check for Header
    if (headerLists.indexOf(header) >= 0) {
        const int &index = headerLists.indexOf(header);

        if (proj->headerNamesArray.contains(header)) {

            if(proj->isHeaderSingleValue(index)) {

                CustomEventWidget *textEditItem = new CustomEventWidget();
                textEditItem->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                textEditItem->setFont(cellFont);

                previousObTrnSeg.clear();
                if (header == QLatin1String("Transect")) {

                    if (surveyType == QLatin1String("WBPHS")) {
                        //Setup Indices
                        obsIndex.stratumIndex = column - 1;
                        obsIndex.transectIndex = column;
                        obsIndex.segmentIndex = column + 1;

                        //Segment
                        CustomEventWidget *textEditItem1 = new CustomEventWidget();
                        textEditItem1->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        textEditItem1->setFont(cellFont);

                        const QString &currentValue1 = ui->Observations->item(row, column + 1)->text();
                        textEditItem1->setText(currentValue1);
                        ui->Observations->setCellWidget(row, column + 1, textEditItem1);

                        //Transect
                        textEditItem->setText(currentValue);
                        ui->Observations->setCellWidget(row, column, textEditItem);

                        previousObTrnSeg.append(currentValue); // Transect
                        previousObTrnSeg.append(currentValue1); // Segment

                        //Text Changed for Transect
                        connectionObservation << connect(textEditItem, &QTextEdit::textChanged,
                                                         this, [=]() {
                            //Clear Segment
                            dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.segmentIndex))->setText("");

                        });

                        //Check Transect Stratum Segment Combination if transect Edited
                        connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                            const QString &header = ui->Observations->horizontalHeaderItem(obCol)->text();

                            const QFont &cellFont = proj->getFontItem(false);

                            const QString &val =
                                    dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->toPlainText();

                            if (header == QLatin1String("Transect")) {

                                const int &newTransect = val.toInt();
                                const int &newStratum =ui->Observations->item(obRow, obsIndex.stratumIndex)->text().toInt();

                                if (!val.toInt() ||
                                        !proj->getWBPS()->checkTransect(new WBPSObj(newStratum, newTransect, -1))) {

                                    ui->Observations->setItem(obRow, obsIndex.transectIndex, new QTableWidgetItem(previousObTrnSeg[0])); // Transect
                                    ui->Observations->setItem(obRow, obsIndex.segmentIndex, new QTableWidgetItem(previousObTrnSeg[1])); // Segment

                                    ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                    ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                    MessageDialog mb;
                                    //mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                                    mb.setWindowTitle("Error");
                                    mb.setText("Warning! The combinations of Stratum and Transect are not valid.");
                                    mb.exec();

                                    ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                    ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);

                                    disconnectObservation();
                                }
                                else {
                                    obCol = obsIndex.segmentIndex;
                                    //Focus on Segment TextEdit
                                    //dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->setFocus();
                                }
                            }
                            else {

                                const int &newSegment = val.toInt();
                                const int &newTransect =
                                        dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.transectIndex))->toPlainText().toInt();
                                const int &newStratum = ui->Observations->item(obRow, obsIndex.stratumIndex)->text().toInt();


                                if (!val.toInt() ||
                                        !proj->getWBPS()->checkSegment(new WBPSObj(newStratum, newTransect, newSegment))) {

                                    ui->Observations->setItem(obRow, obsIndex.segmentIndex, new QTableWidgetItem(previousObTrnSeg[1])); // Segment
                                    ui->Observations->setItem(obRow, obsIndex.transectIndex, new QTableWidgetItem(previousObTrnSeg[0])); // Transect

                                    ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                    ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                    MessageDialog mb;
                                    mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                                    mb.setWindowTitle("Error");
                                    mb.setText("Warning! The combinations of Stratum and Transect are not valid.");
                                    mb.exec();

                                    ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                    ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);

                                    disconnectObservation();
                                }
                                else {

                                    //Successfully edit the value
                                    QTableWidgetItem *headerFieldValue = new QTableWidgetItem(val);
                                    ui->Observations->setItem(obRow, obsIndex.segmentIndex, headerFieldValue);
                                    ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                    ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                              new QTableWidgetItem(QString::number(newTransect)));
                                    ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                    ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);
                                    ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);

                                    disconnectObservation();
                                }

                            }


                        });

                    }
                    // GOEA
                    else {
                        obsIndex.transectIndex = column;
                        obsIndex.bcrIndex = column - 1;

                        //Transect
                        CustomEventWidget *textEditItem = new CustomEventWidget();
                        textEditItem->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        textEditItem->setFont(cellFont);

                        const QString &currentValue = ui->Observations->item(row, obsIndex.transectIndex)->text();
                        textEditItem->setText(currentValue);
                        ui->Observations->setCellWidget(row, obsIndex.transectIndex, textEditItem);

                        //BCR
                        const QJsonArray &vals = proj->headerValuesArray[index - 1].toArray();

                        CustomComboBox *newComboBoxItem = new CustomComboBox();
                        newComboBoxItem->setEditable(false);
                        newComboBoxItem->addItem("");
                        newComboBoxItem->setMinimumHeight(30);

                        for (int i = 0; i < vals.count(); i++){
                            newComboBoxItem->addItem(vals[i].toString());
                        }

                        newComboBoxItem->setCurrentText(ui->Observations->item(row, obsIndex.bcrIndex)->text());
                        ui->Observations->setCellWidget(row, obsIndex.bcrIndex, newComboBoxItem);

                        previousTrnBcr.clear();
                        previousTrnBcr.append(currentValue);
                        previousTrnBcr.append(ui->Observations->item(row, obsIndex.bcrIndex)->text());

                        connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {
                            const QFont &cellFont = proj->getFontItem(false);

                            //Transect
                            CustomEventWidget *trn = dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol));
                            const QString &trnName =trn->toPlainText();

                            //BCR
                            CustomComboBox *bcr =  dynamic_cast<CustomComboBox*>(ui->Observations->cellWidget(obRow, obsIndex.bcrIndex));
                            const QString &bcrName = bcr->currentText();

                            const QString &brctrnName = bcrName + "." + trnName;
                            const QStringList &r = proj->getBCRTRN();

                            if (!r.contains(brctrnName, Qt::CaseInsensitive)) {
                                if (previousTrnBcr[1] != bcrName) {
                                    ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                             new QTableWidgetItem(previousTrnBcr[0])); // Transect
                                    ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                    ui->Observations->setItem(obRow, obsIndex.bcrIndex,
                                                              new QTableWidgetItem(previousTrnBcr[1])); // BCR
                                    ui->Observations->item(obRow, obsIndex.bcrIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                    ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                    ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);

                                    MessageDialog msg;
                                    msg.setWindowTitle("Error");
                                    msg.setText("Warning! The combinations of Transect and BCR are not valid.");
                                    msg.exec();

                                    disconnectObservation();
                                }
                                else {
                                    ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                              new QTableWidgetItem(previousTrnBcr[0])); // Transect
                                    //disconnectObservation();
                                }
                            }
                            else {

                                //Successfully edit the value
                                ui->Observations->setItem(obRow, obsIndex.bcrIndex,
                                                          new QTableWidgetItem(bcrName));
                                ui->Observations->item(obRow, obsIndex.bcrIndex)->setData(Qt::FontRole,QVariant(cellFont));


                                ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                          new QTableWidgetItem(trnName));
                                ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);

                                disconnectObservation();
                            }

                        });

                        //Press Enter Key in Transect. Checking the BCR and Transect without editing BCR
                        connectionObservation << connect(textEditItem, &CustomEventWidget::enterKey, this, [=]() {

                            //Transect
                            CustomEventWidget *trn = dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol));
                            const QString &trnName =trn->toPlainText().trimmed();

                            //BCR
                            CustomComboBox *bcr =  dynamic_cast<CustomComboBox*>(ui->Observations->cellWidget(obRow, obsIndex.bcrIndex));
                            const QString &bcrName = bcr->currentText();

                            const QString &brctrnName = bcrName + "." + trnName;
                            const QStringList &r = proj->getBCRTRN();

                            if (!r.contains(brctrnName, Qt::CaseInsensitive)) {
                                    ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                             new QTableWidgetItem(previousTrnBcr[0])); // Transect
                                    ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                    ui->Observations->setItem(obRow, obsIndex.bcrIndex,
                                                              new QTableWidgetItem(previousTrnBcr[1])); // BCR
                                    ui->Observations->item(obRow, obsIndex.bcrIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                    ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                    ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);

                                    MessageDialog msg;
                                    msg.setWindowTitle("Error");
                                    msg.setText("Warning! The combinations of Transect and BCR are not valid.");
                                    msg.exec();

                                    disconnectObservation();
                            }
                            else {

                                //Successfully edit the value
                                ui->Observations->setItem(obRow, obsIndex.bcrIndex,
                                                          new QTableWidgetItem(bcrName));
                                ui->Observations->item(obRow, obsIndex.bcrIndex)->setData(Qt::FontRole,QVariant(cellFont));


                                ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                          new QTableWidgetItem(trnName));
                                ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);

                                disconnectObservation();
                            }


                        });
                    }

                }
                else if(header == QLatin1String("Stratum")) {
                    //Setup Indices
                    obsIndex.stratumIndex = column;
                    obsIndex.transectIndex = column + 1;
                    obsIndex.segmentIndex = column + 2;

                    //Segment
                    CustomEventWidget *textEditItem2 = new CustomEventWidget();
                    textEditItem2->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    textEditItem2->setFont(cellFont);

                    const QString &currentValue2 = ui->Observations->item(row, obsIndex.segmentIndex)->text();
                    textEditItem2->setText(currentValue2);
                    ui->Observations->setCellWidget(row, obsIndex.segmentIndex, textEditItem2);

                    //Transect
                    CustomEventWidget *textEditItem1 = new CustomEventWidget();
                    textEditItem1->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    textEditItem1->setFont(cellFont);

                    const QString &currentValue1 = ui->Observations->item(row, obsIndex.transectIndex)->text();
                    textEditItem1->setText(currentValue1);
                    ui->Observations->setCellWidget(row, obsIndex.transectIndex, textEditItem1);

                    //Stratum
                    textEditItem->setText(currentValue);
                    ui->Observations->setCellWidget(row, column, textEditItem);

                    previousObTrnSeg.append(currentValue); // Stratum
                    previousObTrnSeg.append(currentValue1); // Transect
                    previousObTrnSeg.append(currentValue2); // Segment

                    //Text Changed for Stratum
                    connectionObservation << connect(textEditItem, &QTextEdit::textChanged,
                                                     this, [=]() {
                        //Clear Segment and Transect
                        dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.segmentIndex))->setText("");
                        dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.transectIndex))->setText("");

                    });

                    //Check Transect Stratum Segment Combination if transect Edited
                    connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                        const QString &header = ui->Observations->horizontalHeaderItem(obCol)->text();

                        const QFont &cellFont = proj->getFontItem(false);

                        const QString &val =
                                dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->toPlainText();

                        const int &newStratum = val.toInt();

                        if (header == QLatin1String("Stratum")) {

                            if (!val.toInt() ||
                                    !proj->getWBPS()->checkStratum(new WBPSObj(newStratum, -1, -1))) {

                                ui->Observations->setItem(obRow, obsIndex.stratumIndex, new QTableWidgetItem(previousObTrnSeg[0])); // Stratum
                                ui->Observations->setItem(obRow, obsIndex.transectIndex, new QTableWidgetItem(previousObTrnSeg[1])); // Transect
                                ui->Observations->setItem(obRow, obsIndex.segmentIndex, new QTableWidgetItem(previousObTrnSeg[2])); // Segment

                                ui->Observations->item(obRow, obsIndex.stratumIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                MessageDialog mb;
                                //mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                                mb.setWindowTitle("Error");
                                mb.setText("Warning! Stratum is not valid.");
                                mb.exec();

                                ui->Observations->removeCellWidget(obRow, obsIndex.stratumIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);

                                disconnectObservation();
                            }
                            else {

                                obCol = obsIndex.transectIndex;
                                //Focus on Transect TextEdit
                               // dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->setFocus();
                            }

                        }
                        else if(header == QLatin1String("Transect")) {

                            const QString &transect = dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.transectIndex))->toPlainText();

                            const int &newStratum =
                                    dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.stratumIndex))->toPlainText().toInt();
                            const int &newTransect =
                                    transect.toInt();


                            if(!transect.toInt() ||
                                    !proj->getWBPS()->checkTransect(new WBPSObj(newStratum, newTransect, -1))) {

                                ui->Observations->setItem(obRow, obsIndex.stratumIndex, new QTableWidgetItem(previousObTrnSeg[0])); // Stratum
                                ui->Observations->setItem(obRow, obsIndex.transectIndex, new QTableWidgetItem(previousObTrnSeg[1])); // Transect
                                ui->Observations->setItem(obRow, obsIndex.segmentIndex, new QTableWidgetItem(previousObTrnSeg[2])); // Segment

                                ui->Observations->item(obRow, obsIndex.stratumIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                MessageDialog mb;
                                //mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                                mb.setWindowTitle("Error");
                                mb.setText("Warning! The combinations of Stratum and Transect are not valid.");
                                mb.exec();

                                ui->Observations->removeCellWidget(obRow, obsIndex.stratumIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);

                                disconnectObservation();

                            }
                            else {

                                obCol = obsIndex.segmentIndex;
                                //Focus on Segment TextEdit
                                //dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->setFocus();
                            }
                        }
                        else {

                            const int &newStratum =
                                    dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.stratumIndex))->toPlainText().toInt();
                            const int &newTransect =
                                    dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.transectIndex))->toPlainText().toInt();

                            const QString &segment = dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.segmentIndex))->toPlainText();
                            const int &newSegment =
                                    segment.toInt();

                            if (!segment.toInt() ||
                                    !proj->getWBPS()->checkSegment(new WBPSObj(newStratum, newTransect, newSegment))) {

                                ui->Observations->setItem(obRow, obsIndex.stratumIndex, new QTableWidgetItem(previousObTrnSeg[0])); // Stratum
                                ui->Observations->setItem(obRow, obsIndex.transectIndex, new QTableWidgetItem(previousObTrnSeg[1])); // Transect
                                ui->Observations->setItem(obRow, obsIndex.segmentIndex, new QTableWidgetItem(previousObTrnSeg[2])); // Segment

                                ui->Observations->item(obRow, obsIndex.stratumIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));
                                ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                MessageDialog mb;
                                //mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                                mb.setWindowTitle("Error");
                                mb.setText("Warning! The combinations of Stratum, Transect and Segment are not valid.");
                                mb.exec();

                                ui->Observations->removeCellWidget(obRow, obsIndex.stratumIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);

                                disconnectObservation();

                            }
                            else {

                                //Successfully edit the value
                                QTableWidgetItem *headerFieldValue = new QTableWidgetItem(val);
                                ui->Observations->setItem(obRow, obsIndex.segmentIndex, headerFieldValue);
                                ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));


                                ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                          new QTableWidgetItem(QString::number(newTransect)));
                                ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));


                                ui->Observations->setItem(obRow, obsIndex.stratumIndex,
                                                          new QTableWidgetItem(QString::number(newStratum)));
                                ui->Observations->item(obRow, obsIndex.stratumIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                ui->Observations->removeCellWidget(obRow, obsIndex.stratumIndex);

                                disconnectObservation();

                            }
                        }

                    });

                }
                else if(header == QLatin1String("Segment")) {
                    //Setup Indices
                    obsIndex.stratumIndex = column - 2;
                    obsIndex.transectIndex = column - 1;
                    obsIndex.segmentIndex = column;

                    //Segment
                    CustomEventWidget *textEditItem = new CustomEventWidget();
                    textEditItem->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    textEditItem->setFont(cellFont);

                    const QString &currentValue = ui->Observations->item(row, obsIndex.segmentIndex)->text();
                    textEditItem->setText(currentValue);
                    ui->Observations->setCellWidget(row, obsIndex.segmentIndex, textEditItem);

                    previousObTrnSeg.append(currentValue); // Segment

                    connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                        const QFont &cellFont = proj->getFontItem(false);

                        const QString &val =
                                dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->toPlainText();

                        const int &newSegment = val.toInt();
                        const int &newTransect = ui->Observations->item(obRow, obsIndex.transectIndex)->text().toInt();
                        const int &newStratum = ui->Observations->item(obRow, obsIndex.stratumIndex)->text().toInt();

                        if (!val.toInt() ||
                                !proj->getWBPS()->checkSegment(new WBPSObj(newStratum, newTransect, newSegment))) {

                            ui->Observations->setItem(obRow, obsIndex.segmentIndex, new QTableWidgetItem(previousObTrnSeg[0])); // Segment
                            ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));

                            MessageDialog mb;
                            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                            mb.setWindowTitle("Error");
                            mb.setText("Warning! The combinations of Stratum, Transect and Segment are not valid.");
                            mb.exec();

                            ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);

                            disconnectObservation();
                        }
                        else {

                            //Successfully edit the value
                            QTableWidgetItem *headerFieldValue = new QTableWidgetItem(val);
                            ui->Observations->setItem(obRow, obsIndex.segmentIndex, headerFieldValue);
                            ui->Observations->item(obRow, obsIndex.segmentIndex)->setData(Qt::FontRole,QVariant(cellFont));

                            ui->Observations->removeCellWidget(obRow, obsIndex.segmentIndex);

                            disconnectObservation();
                        }
                    });
                }
                else if(header == QLatin1String("A_G Name")) {

                    //A_G Name
                    CustomEventWidget *textEditItem = new CustomEventWidget();
                    textEditItem->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    textEditItem->setFont(cellFont);

                    const QString &currentValue = ui->Observations->item(row, column)->text();
                    textEditItem->setText(currentValue);
                    ui->Observations->setCellWidget(row, column, textEditItem);

                    //Current Value
                    preVal = currentValue;

                    connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                        const QFont &cellFont = proj->getFontItem(false);

                        const QString &val =
                                dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->toPlainText();

                        if (!val.isEmpty() && val != "offag"){
                            const QStringList &r = proj->getAllAGNameList();
                            if (!r.contains(val, Qt::CaseInsensitive)) {

                                ui->Observations->setItem(obRow, obCol,
                                                          new QTableWidgetItem(preVal)); // A_G Name
                                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                                MessageDialog mb;
                                mb.setWindowTitle("Error");
                                mb.setText("Invalid input A_G Name.");
                                mb.exec();

                                ui->Observations->removeCellWidget(obRow, obCol);

                                disconnectObservation();

                            }
                            else {

                                //Successfully edit the value
                                QTableWidgetItem *headerFieldValue = new QTableWidgetItem(val);
                                ui->Observations->setItem(obRow, obCol, headerFieldValue);
                                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                                ui->Observations->removeCellWidget(obRow, obCol);

                                disconnectObservation();

                            }
                        }
                        else {

                            //Blank Value
                            QTableWidgetItem *headerFieldValue = new QTableWidgetItem("offag");
                            ui->Observations->setItem(obRow, obCol, headerFieldValue);
                            ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                            ui->Observations->removeCellWidget(obRow, obCol);

                            disconnectObservation();

                        }

                    });

                }
                else if(header == QLatin1String("PlotName")) {
                    //Plot Name
                    CustomEventWidget *textEditItem = new CustomEventWidget();
                    textEditItem->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    textEditItem->setFont(cellFont);

                    const QString &currentValue = ui->Observations->item(row, column)->text();
                    textEditItem->setText(currentValue);
                    ui->Observations->setCellWidget(row, column, textEditItem);

                    preVal = currentValue;

                    connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                        const QFont &cellFont = proj->getFontItem(false);

                        const QString &val =
                                dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obCol))->toPlainText();

                        if (!val.isEmpty()){
                            const QStringList &r = proj->getAllPlotNameList();
                            if (!r.contains(val, Qt::CaseInsensitive)) {

                                ui->Observations->setItem(obRow, obCol,
                                                          new QTableWidgetItem(preVal)); // Plotname
                                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                                MessageDialog mb;
                                mb.setWindowTitle("Error");
                                mb.setText("Invalid input Plotname.");
                                mb.exec();

                                ui->Observations->removeCellWidget(obRow, obCol);

                                disconnectObservation();

                            }
                            else {

                                //Successfully edit the value
                                QTableWidgetItem *headerFieldValue = new QTableWidgetItem(val);
                                ui->Observations->setItem(obRow, obCol, headerFieldValue);
                                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                                ui->Observations->removeCellWidget(obRow, obCol);

                                disconnectObservation();

                            }
                        }
                        else {

                            //Blank Value
                            QTableWidgetItem *headerFieldValue = new QTableWidgetItem("");
                            ui->Observations->setItem(obRow, obCol, headerFieldValue);
                            ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                            ui->Observations->removeCellWidget(obRow, obCol);

                            disconnectObservation();

                        }

                    });
                }

            }
            else {

                if (header == QLatin1String("BCR")) {

                    obsIndex.transectIndex = column + 1;
                    obsIndex.bcrIndex = column;

                    //Transect
                    CustomEventWidget *textEditItem = new CustomEventWidget();
                    textEditItem->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    textEditItem->setFont(proj->getFontItem(false));

                    const QString &currentValue = ui->Observations->item(row, obsIndex.transectIndex)->text();
                    textEditItem->setText(currentValue);
                    ui->Observations->setCellWidget(row, obsIndex.transectIndex, textEditItem);

                    //BCR
                    const QJsonArray &vals = proj->headerValuesArray[index].toArray();

                    CustomComboBox *newComboBoxItem = new CustomComboBox();
                    newComboBoxItem->setEditable(false);
                    newComboBoxItem->addItem("");
                    newComboBoxItem->setMinimumHeight(30);

                    for (int i = 0; i < vals.count(); i++){
                        newComboBoxItem->addItem(vals[i].toString());
                    }

                    newComboBoxItem->setCurrentText(ui->Observations->item(row, obsIndex.bcrIndex)->text());
                    ui->Observations->setCellWidget(row, obsIndex.bcrIndex, newComboBoxItem);

                    previousTrnBcr.clear();
                    previousTrnBcr.append(currentValue);
                    previousTrnBcr.append(newComboBoxItem->currentText());

                    connectionObservation << connect(newComboBoxItem, &CustomComboBox::currentTextChanged, this,
                                                     [=](const QString &text) {
                           obVal = text;
                    });

                    connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                        const QFont &cellFont = proj->getFontItem(false);

                        //Transect
                        CustomEventWidget *trn = dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.transectIndex));
                        const QString &trnName =trn->toPlainText();

                        //BCR
                        QString bcrName;
                        CustomComboBox *bcr =  dynamic_cast<CustomComboBox*>(ui->Observations->cellWidget(obRow, obCol));
                        if (bcr) {
                            bcrName = bcr->currentText();
                        }
                        else {
                            bcrName = ui->Observations->item(obRow, obCol)->text();
                        }

                        const QString &brctrnName = bcrName + "." + trnName;
                        const QStringList &bcrTrn = proj->getBCRTRN();

                        if (!bcrTrn.contains(brctrnName, Qt::CaseInsensitive)) {

                            if (previousTrnBcr[0] != trnName) {
                                ui->Observations->setItem(obRow, obCol,
                                                         new QTableWidgetItem(previousTrnBcr[1])); // BCR
                                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                                ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                          new QTableWidgetItem(previousTrnBcr[0])); // Transect
                                ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                                ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                                 ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);

                                MessageDialog msg;
                                msg.setWindowTitle("Error");
                                msg.setText("Warning! The combinations of Transect and BCR are not valid.");
                                msg.exec();

                                disconnectObservation();
                            }
                            //else {



                               // ui->Observations->setItem(obRow, obsIndex.bcrIndex,
                                 //                         new QTableWidgetItem(obVal));//BCR
                            //}
                        }
                        else {

                            //SuccessFully edit the value
                            ui->Observations->setItem(obRow, obsIndex.bcrIndex,
                                                      new QTableWidgetItem(bcrName));
                            ui->Observations->item(obRow, obsIndex.bcrIndex)->setData(Qt::FontRole,QVariant(cellFont));


                            ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                      new QTableWidgetItem(trnName));
                            ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                            ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);
                            ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);

                            disconnectObservation();
                        }

                    });

                    //Press Enter Key in BCR. Checking the BCR and Transect without editing Transect
                    connectionObservation << connect(newComboBoxItem, &CustomComboBox::enterKey, this, [=]() {

                        const QFont &cellFont = proj->getFontItem(false);

                        //Transect
                        CustomEventWidget *trn = dynamic_cast<CustomEventWidget*>(ui->Observations->cellWidget(obRow, obsIndex.transectIndex));
                        const QString &trnName =trn->toPlainText().trimmed();

                        //BCR
                        QString bcrName;
                        CustomComboBox *bcr =  dynamic_cast<CustomComboBox*>(ui->Observations->cellWidget(obRow, obCol));
                        if (bcr) {
                            bcrName = bcr->currentText();
                        }
                        else {
                            bcrName = ui->Observations->item(obRow, obCol)->text();
                        }

                        const QString &brctrnName = bcrName + "." + trnName;
                        const QStringList &bcrTrn = proj->getBCRTRN();

                        if (!bcrTrn.contains(brctrnName, Qt::CaseInsensitive)) {

                            ui->Observations->setItem(obRow, obCol,
                                                     new QTableWidgetItem(previousTrnBcr[1])); // BCR
                            ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                            ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                      new QTableWidgetItem(previousTrnBcr[0])); // Transect
                            ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                            ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);
                            ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);

                            MessageDialog msg;
                            msg.setWindowTitle("Error");
                            msg.setText("Warning! The combinations of Transect and BCR are not valid.");
                            msg.exec();

                            disconnectObservation();
                        }
                        else {

                            //SuccessFully edit the value
                            ui->Observations->setItem(obRow, obsIndex.bcrIndex,
                                                      new QTableWidgetItem(bcrName));
                            ui->Observations->item(obRow, obsIndex.bcrIndex)->setData(Qt::FontRole,QVariant(cellFont));


                            ui->Observations->setItem(obRow, obsIndex.transectIndex,
                                                      new QTableWidgetItem(trnName));
                            ui->Observations->item(obRow, obsIndex.transectIndex)->setData(Qt::FontRole,QVariant(cellFont));

                            ui->Observations->removeCellWidget(obRow, obsIndex.bcrIndex);
                            ui->Observations->removeCellWidget(obRow, obsIndex.transectIndex);

                            disconnectObservation();
                        }


                    });

                }
                else {

                    QJsonArray vals = proj->headerValuesArray[index].toArray();

                    CustomComboBox *newComboBoxItem = new CustomComboBox();
                    newComboBoxItem->setEditable(false);
                    newComboBoxItem->addItem("");
                    newComboBoxItem->setMinimumHeight(30);

                    for (int i = 0; i < vals.count(); i++){
                        newComboBoxItem->addItem(vals[i].toString());
                    }

                    newComboBoxItem->setCurrentText(currentValue);
                    newComboBoxItem->setFocus();

                    ui->Observations->setCellWidget(row, column, newComboBoxItem);

                    obVal = newComboBoxItem->currentText();

                    connectionObservation << connect(newComboBoxItem, &CustomComboBox::currentTextChanged, this,
                                                  [=](const QString &text) {
                        obVal = text;
                    });

                    connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                         ui->Observations->removeCellWidget(obRow, obCol);

                         QTableWidgetItem *headerFieldValue = new QTableWidgetItem(obVal);
                         ui->Observations->setItem(obRow, obCol, headerFieldValue);
                         ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                         disconnectObservation();

                    });
                }
            }

        }
    }
    else if(addiotionalLists.indexOf(header) >= 0) {
        const int &index = addiotionalLists.indexOf(header);
        if (!proj->isAdditionalFieldSingleValue(index)) {

            QJsonArray vals = proj->addionalFieldsValuesArray[index].toArray();

            CustomComboBox *newComboBoxItem = new CustomComboBox();
            newComboBoxItem->setEditable(false);
            newComboBoxItem->addItem("");
            newComboBoxItem->setMinimumHeight(30);

            for (int i = 0; i < vals.count(); i++){
                newComboBoxItem->addItem(vals[i].toString());
            }

            newComboBoxItem->setCurrentText(currentValue);
            newComboBoxItem->setFocus();

            ui->Observations->setCellWidget(row, column, newComboBoxItem);

            obVal = newComboBoxItem->currentText();

            connectionObservation << connect(newComboBoxItem, &CustomComboBox::currentTextChanged, this,
                                          [=](const QString &text) {
                obVal = text;
            });

            connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

                 ui->Observations->removeCellWidget(obRow, obCol);

                 QTableWidgetItem *headerFieldValue = new QTableWidgetItem(obVal);
                 ui->Observations->setItem(obRow, obCol, headerFieldValue);
                 ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                 disconnectObservation();

            });

        }
    }
    else if(header == QLatin1String("Quantity")) {
        QLineEdit *quantity = new QLineEdit();
        quantity->setFont(proj->getFontItem(false));

        quantity->setText(currentValue);
        ui->Observations->setCellWidget(row, column, quantity);

        preVal = currentValue;

        connectionObservation << connect(quantity, &QLineEdit::textChanged, this, [=](const QString &arg1)
        {
            QString s = arg1;
            s = s.remove(QRegularExpression("[^0-9]"));
            quantity->setText(s);
        });

        connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

            const QFont &cellFont = proj->getFontItem(false);

            const QString &val =
                    dynamic_cast<QLineEdit*>(ui->Observations->cellWidget(obRow, obCol))->text();

            if (proj->getSurveyType() == QLatin1String("WBPHS")) {
                const QString &grouping = ui->Observations->item(obRow, obCol + 1)->text();
                const bool &result = condition(grouping, val);
                if (result) {
                    ui->Observations->setItem(obRow, obCol, new QTableWidgetItem(val));
                    ui->Observations->item(obRow, obCol)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                    ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                    ui->Observations->removeCellWidget(obRow, obCol);
                }
                else {
                    ui->Observations->setItem(obRow, obCol, new QTableWidgetItem(preVal));
                    ui->Observations->item(obRow, obCol)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                    ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                    ui->Observations->removeCellWidget(obRow, obCol);
                }
            }
            else {
                ui->Observations->setItem(obRow, obCol, new QTableWidgetItem(val));
                ui->Observations->item(obRow, obCol)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

                ui->Observations->removeCellWidget(obRow, obCol);
            }

            disconnectObservation();
        });
    }
    else if(header == QLatin1String("Species")) {
        QStringList s;
        CustomComboBox *newComboBoxItem = new CustomComboBox();
        newComboBoxItem->setEditable(false);
        newComboBoxItem->setMinimumHeight(30);

        for (int i = 0; i  < ui->comboBox_2->count(); i ++ ){
            s << ui->comboBox_2->itemText(i);
        }

        newComboBoxItem->addItems(s);
        newComboBoxItem->setCurrentText(currentValue);
        newComboBoxItem->setFocus();

        ui->Observations->setCellWidget(obRow, obCol, newComboBoxItem);

        obVal = newComboBoxItem->currentText();
        preVal = newComboBoxItem->currentText();

        connectionObservation << connect(newComboBoxItem, &CustomComboBox::currentTextChanged, this,
                                      [=](const QString &text) {
            obVal = text;

            //Get Current Species in Observation
            for(int i = 0; i < species.length(); i++){
                if (species[i]->itemName == text){
                    currentSpeciesOb = species[i];
                    return;
                }
            }

        });

        connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

            const QString &quantity = ui->Observations->item(obRow, obCol + 1)->text();
            if (proj->getSurveyType() == QLatin1String("WBPHS")) {
                const QString &grouping = ui->Observations->item(obRow, obCol + 2)->text();
                const bool &result = condition(grouping, quantity);
                if (result) {
                    QFont upper = proj->getFontItem(false);
                    upper.setCapitalization(QFont::AllUppercase);

                    ui->Observations->removeCellWidget(obRow, obCol);

                    QTableWidgetItem *headerFieldValue = new QTableWidgetItem(obVal);
                    ui->Observations->setItem(obRow, obCol, headerFieldValue);
                    ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(upper));
                }
                else {
                    QFont upper = proj->getFontItem(false);
                    upper.setCapitalization(QFont::AllUppercase);

                    ui->Observations->removeCellWidget(obRow, obCol);

                    QTableWidgetItem *headerFieldValue = new QTableWidgetItem(preVal);
                    ui->Observations->setItem(obRow, obCol, headerFieldValue);
                    ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(upper));
                }
            }
            else {

                QFont upper = proj->getFontItem(false);
                upper.setCapitalization(QFont::AllUppercase);

                ui->Observations->removeCellWidget(obRow, obCol);

                QTableWidgetItem *headerFieldValue = new QTableWidgetItem(obVal);
                ui->Observations->setItem(obRow, obCol, headerFieldValue);
                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(upper));

            }

            disconnectObservation();

        });
    }
    else if(header == QLatin1String("Grouping")) {
        QStringList s;
        CustomComboBox *newComboBoxItem = new CustomComboBox();
        newComboBoxItem->setEditable(false);
        newComboBoxItem->setMinimumHeight(30);

        for (int i = 0; i  < ui->comboBox_3->count(); i ++ ){
            s << ui->comboBox_3->itemText(i).toLower();
        }

        newComboBoxItem->addItems(s);
        newComboBoxItem->setCurrentText(currentValue);
        newComboBoxItem->setFocus();

        ui->Observations->setCellWidget(obRow, obCol, newComboBoxItem);

        obVal = newComboBoxItem->currentText();

        preVal = currentValue;

        connectionObservation << connect(newComboBoxItem, &CustomComboBox::currentTextChanged, this,
                                      [=](const QString &text) {
            obVal = text;
        });

        connectionObservation << connect(ui->Observations, &QTableWidget::itemSelectionChanged, this ,[=]() {

            const QString &quantity = ui->Observations->item(obRow, obCol - 1)->text();
            const bool &result = condition(obVal, quantity);

            if (result) {

                ui->Observations->removeCellWidget(obRow, obCol);

                QTableWidgetItem *headerFieldValue = new QTableWidgetItem(obVal);
                ui->Observations->setItem(obRow, obCol, headerFieldValue);
                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

            }
            else {

                ui->Observations->removeCellWidget(obRow, obCol);

                QTableWidgetItem *headerFieldValue = new QTableWidgetItem(preVal);
                ui->Observations->setItem(obRow, obCol, headerFieldValue);
                ui->Observations->item(obRow, obCol)->setData(Qt::FontRole,QVariant(cellFont));

            }

            disconnectObservation();

        });
    }
}


void newmainwindow::on_Play_toggled(bool checked)
{
    Q_UNUSED(checked)
    insertWasLastEvent = false;

    if (audio->player->position() == 0){
        int currIndex = ui->wavFileList->currentIndex();
        audio->fileList->setCurrentIndex(currIndex);

        //on_wavFileList_currentIndexChanged(currIndex);//added 8.19.2020
        audio->wavFileList->setCurrentIndex(currIndex);
    }
    switch(audio->status) {
        case 2:{//added 8.25.2020
            //qDebug() << "audio->status: " << audio->status;
            /*audio->player->play();
            ui->Play->setText("PAUSE");*/
            break;
        }
        case 0: {
            //qDebug() << "audio->status: " << audio->status;
            audio->player->play();
            //Change Icon
            ui->Play->setText("PAUSE");
            audio->status = 0;//added 8.25.2020
            //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);//remarked 8.20.2020
            break;
        }
        default: {
            //qDebug() << "audio->status: " << audio->status;
            audio->player->pause();
            ui->Play->setText("PLAY");
            //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);//remarked 8.20.2020
            break;
        }
    }
}

void newmainwindow::on_Back_clicked()
{
    insertWasLastEvent = false;
    int currIndex = ui->wavFileList->currentIndex();
    if (currIndex > 0){
     audio->fileList->setCurrentIndex(currIndex - 1);
    }else{
        //remarked if the your on the first wavfile,
        //the system doesn't allow to back click to the last wavfile
        //audio->fileList->setCurrentIndex(audio->fileList->mediaCount() - 1);
        return;
    }
    //qDebug() << "read backclick";
    audio->player->play();
    //Change Icon
    ui->Play->setText("PAUSE");
    //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);//REMARKED 8.20.2020
}

void newmainwindow::on_Next_clicked()
{
    insertWasLastEvent = false;
    int currIndex = ui->wavFileList->currentIndex();

    if(currIndex < audio->fileList->mediaCount()-1){
        audio->fileList->setCurrentIndex(currIndex + 1);
    }else{
        return;
    }

    if (audio->player->state() != QMediaPlayer::PlayingState){
        audio->player->play();
        //Change Icon
        ui->Play->setText("PAUSE");
        //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);//remarked 8.20.2020
    }
}

void newmainwindow::repeatTrack()
{
    //qDebug() << "repeat track";
    if (!audio->fileList->isEmpty()){
        int currIndex = ui->wavFileList->currentIndex();
        if(currIndex < audio->fileList->mediaCount()-1){
            audio->fileList->setCurrentIndex(currIndex);
        }else{
            return;
        }

        audio->player->play();
        //audio->status = 0;
        ui->Play->setText("PAUSE");
        //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);//remarked 8.20.2020
    }
    //insertWasLastEvent = false;
    //audio->player->setPosition(0);//reamrked 8.25.2020
    //audio->status = 2;//8.25.2020 changed from 1 to 2
    //Change Icon>>>
    //ui->Play->setText("PAUSE");
    //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);//remarked 8.20.2020
    //on_Play_toggled(true);//8.25.2020 change from false to true
}

void newmainwindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index)
}

void newmainwindow::on_quantity_textChanged(const QString &arg1)
{
    QString s = arg1;
    s = s.remove(QRegularExpression("[^0-9]"));
    ui->quantity->setText(s);

    if (arg1.contains('`')){
        handleBackTick();
    }
}

void newmainwindow::handleBackTick()
{
    //qDebug() << "handleBackTick";
    const int &currIndex = ui->wavFileList->currentIndex();
    if(currIndex == audio->fileList->mediaCount()-1){
        return;
    }

    if(ui->quantity->text().toInt() && !insertWasLastEvent)
        this->on_insertButton_clicked();

    this->on_Next_clicked();
}

void newmainwindow::on_comboBox_2_currentTextChanged(const QString &arg1)
{
    for(int i = 0; i < species.length(); i++){
        if (species[i]->itemName == arg1){
            currentSpecies = species[i];
            break;
        }
    }

    //added 8.13.2020
    validateConditionalValueFields(arg1, QLatin1String("species"), false);

    ui->quantity->setFocus();
}

/*void newmainwindow::on_deleteButton_clicked()
{
    //check selected row in table, and remove that thing
    ui->Observations->removeRow(ui->Observations->currentRow());
    //also need to remove it from application data structure----------------??
}*/

void newmainwindow::setSpecies(int row, int column)
{
    QTableWidgetItem *itm = ui->speciesTable->item( row, column );
    const QString &s = itm->text();
    if(s.length() < 1){
      return;
    }
    QStringList species_code = s.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    const int &index = ui->comboBox_2->findText( species_code[0] );
    if ( index != -1 ) { // -1 for not found
       ui->comboBox_2->setCurrentIndex(index);
    }
}

void newmainwindow::selectWavFileMap(const int &index)
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    audio->fileList->setCurrentIndex(index);
    audio->wavFileList->setItemData(index, QColor("red"), Qt::BackgroundColorRole);

    //8.25.2020 need to play the audio file when a wavfile is selected
    if (audio->player->state() != QMediaPlayer::PlayingState){
        audio->player->play();
        ui->Play->setText("PAUSE");
    }


    //added 4.27.2020
    const QString &wavfile = ui->wavFileList->itemText(index);
    QVariant returnedValue = "";//added 5.17.2020
    if(index >= 0 && globalSettings.get("autoUnit") == QLatin1String("yes")){//updated from = to >= to select the first wave file from the list and zoom to it
        QMetaObject::invokeMethod(gps->wayHandler, "selectWavFile",
            Q_RETURN_ARG(QVariant, returnedValue),
            Q_ARG(QVariant, wavfile),
            Q_ARG(QVariant, proj->getAirGroundMaxD()),
            Q_ARG(QVariant, proj->getObsMaxD()));
            //Q_ARG(QVariant, globalSettings.get("autoUnit")));
        const QString &surveyType = proj->getSurveyType();
        if(surveyType == QLatin1String("BAEA")){
            setPlotName("");

            if(returnedValue.toString().isEmpty())
                return;
            else{

                returnedValueBAEA = returnedValue.toString();

                QStringList splt = returnedValue.toString().split(",");
                QString plotnm = splt[0];
                setPlotName(plotnm);

                if (splt.count() > 1) {
                    const int &isWithinMaxDist = splt[1].toInt();
                    if(isWithinMaxDist == 0){//over max distance
                        QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                            Q_ARG(QVariant,"yellow"));
                    }else{
                        QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                            Q_ARG(QVariant,"white"));
                    }
                }
            }
        }else if (surveyType == QLatin1String("WBPHS")) {
            //Check Air Ground
            QStringList splt = returnedValue.toString().split(",");
            const QString &agnm = splt[1];
            const int &isWithinMaxDist = splt[0].toInt();
            if(isWithinMaxDist == 1){
                setAGName(agnm);
            }else {
                setAGName("offag");
            }

            //for the stratum transect and segment
            if(!returnedValue.toString().isEmpty()){
                returnedValueWPBHS = returnedValue.toString();
                const QString &stratum = splt[2];
                const QString &transect = splt[3];
                const QString &segment = splt[4];
                const double &dist = splt[6].toDouble();
                setValuesWBPHS(stratum,transect,segment,dist);

                const int &isWithinMaxDist2 = splt[5].toInt();
                if(isWithinMaxDist2 == 0)//over max distance
                        QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                            Q_ARG(QVariant,"yellow"));
                else
                    QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                        Q_ARG(QVariant,"white"));

                const QString &boolalert = splt[7];
                if(boolalert == "yes"){
                    QMessageBox msgbox;
                    msgbox.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                    msgbox.setWindowTitle("Validate AirGround Distance");
                    msgbox.setText("The observation point is beyond the air ground distance threshold.");
                    msgbox.exec();
                }

            }
        }else if(surveyType == QLatin1String("GOEA")){
            if(returnedValue.toString().isEmpty())
                return;
            else{
                returnedValueGOEA = returnedValue.toString();
                QStringList splt = returnedValue.toString().split(",");
                QString bcr = splt[0];
                QString trn = splt[1];
                const double &dist = splt[3].toDouble();

                setValuesGOEA(bcr,trn, dist);

                const int &isWithinMaxDist = splt[2].toInt();
                if(isWithinMaxDist == 0){//over max distance
                        QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                            Q_ARG(QVariant,"yellow"));
                }else{
                    QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                        Q_ARG(QVariant,"white"));
                }
            }
        }

        //added 8.16.2020
        validateConditionalValueFields(ui->comboBox_2->currentText().trimmed(), QLatin1String("species"), true);
    }

}

void newmainwindow::connectAllHeader()
{
    adjustHeaderSize();
    connection.clear();
    //For Text Edit
    QList<CustomEventWidget*> col = proj->headerFields;
    const int &hCount = proj->countH;
    for (int t = 0; t < hCount; t++){
        DocLayout *docLayoutH = new DocLayout();
        docLayoutH->setIndex(t);
        docLayoutH->setIndexTableW(proj->headerIndex[t]);
        const int &indexAdd = proj->headerIndex[t];

        //Get custom textEdit index
        if (proj->getHeaderName(indexAdd) == QLatin1String("Stratum")) {
            proj->stratumIndex = t;

            connection << connect(col.at(t), &QTextEdit::textChanged,
                    this, &newmainwindow::stratumChanged);

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutStratum);

            //Focus in
            //connection << connect(col.at(t), &CustomEventWidget::focusIn,
              //      this, &newmainwindow::focusInStratum);

        }
        else if(proj->getHeaderName(indexAdd) == QLatin1String("Transect")) {
            proj->transectIndex = t;

            if (proj->getSurveyType() == QLatin1String("WBPHS")) {
                connection << connect(col.at(t), &QTextEdit::textChanged,
                    this, &newmainwindow::trasectChanged);
            }

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutTransect);

            if (proj->getSurveyType() == QLatin1String("GOEA")) {
                //Focus in
                connection << connect(col.at(t), &CustomEventWidget::focusIn,
                    this, &newmainwindow::focusInTransect);
            }
        }
        else if(proj->getHeaderName(indexAdd) == QLatin1String("Segment")){
            proj->segmentIndex = t;

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutSegment);

            //Focus in
           // connection << connect(col.at(t), &CustomEventWidget::focusIn,
             //      this, &newmainwindow::focusInSegment);

        }else if (proj->getHeaderName(indexAdd) == QLatin1String("PlotName")) {//added 5.18.2020
            proj->plotnameIndex = t;

            connection << connect(col.at(t), &QTextEdit::textChanged,
                    this, &newmainwindow::plotnameChanged);

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutPlotName);

            //Focus in
            connection << connect(col.at(t), &CustomEventWidget::focusIn,
                    this, &newmainwindow::focusInPlotName);

        }else if (proj->getHeaderName(indexAdd) == QLatin1String("A_G Name")) {
            proj->aGNameIndex = t;

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutAGName);

            //Focus in
            connection << connect(col.at(t), &CustomEventWidget::focusIn,
                    this, &newmainwindow::focusInAGName);

        }else if(proj->getHeaderName(indexAdd) == QLatin1String("Distance")){//added 6.19.2020
            proj->distanceIndex = t;
        }

        //Disabe resizing for value which not editable
        if (proj->getInstanceHeaderPosition(proj->getHeaderName(indexAdd)) == -1) {
            connection << connect(col.at(t)->document()->documentLayout(),
                        &QAbstractTextDocumentLayout::documentSizeChanged,
                        docLayoutH,
                        &DocLayout::result);

            //To get current index to resize the text edit
            connection << connect(docLayoutH,
                    &DocLayout::sendSignal,
                    this,
                    &newmainwindow::commentDocRectH);
        }
    }

    //For ComboBox
    auto colC = proj->headerFieldsC;
    const int &hCountC = proj->countHC;
    for(int h = 0; h < hCountC; h++) {
         DocLayout *docLayoutH = new DocLayout();
         docLayoutH->setIndex(h);
         docLayoutH->setIndexTableW(proj->headerIndexC[h]);

         const int &indexAdd = proj->headerIndexC[h];

         if (proj->getHeaderName(indexAdd) == QLatin1String("BCR")) {
            proj->bcrIndex = h;
            connection << connect(colC.at(h), &CustomComboBox::focusIn,
                    this, &newmainwindow::focusInBCR);

            connection << connect(colC.at(h), &CustomComboBox::focusOut,
                    this, &newmainwindow::focusOutBCR);

            connection << connect(colC.at(h),SIGNAL(currentIndexChanged(const QString&)),
                    this,SLOT(checkBCRValue(const QString&)));//added 8.17.2020

        }
    }
}

void newmainwindow::disconnectAllHeader()
{
    //Disable Event
    foreach (auto var, connection) {
        QObject::disconnect(var);
    }
}


void newmainwindow::on_wavFileList_currentIndexChanged(int index)
{
    if (!curlocked) {
        selectWavFileMap(index);
        ui->lockGPS->setChecked(false);
    }
    else {
        QMetaObject::invokeMethod(gps->wayHandler, "changeMarkerToLockGPS",
            Q_ARG(QVariant, wavFileLock),
            Q_ARG(QVariant, ui->lockGPS->isChecked()),
            Q_ARG(QVariant, ui->wavFileList->currentText()));
    }
}

QList<QStringList> newmainwindow::readGeoJson(QJsonObject jObj)
{
    QList<QStringList> jsonSegmentList;
    QStringList lineSegment;
    QJsonObject::iterator i;
    for (i = jObj.begin(); i != jObj.end(); ++i) {
        if(i.key().toUpper() == QLatin1String("FEATURES") ){
            if (i.value().isObject()) {
                QJsonObject innerObject = i.value().toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); ++j) {
                    if (j.value().isObject()) {
                        //qDebug() << '\t' << "OBJECT" << j.key();
                        // ...
                    }
                    // ...
                }
            }else if (i.value().isArray()) {
                QJsonArray jsonArray = i.value().toArray();
                foreach (const QJsonValue & value, jsonArray) {
                    lineSegment.clear();//clears the list

                    QJsonObject obj = value.toObject();
                    foreach(const QString& key, obj.keys()) {

                        QString strval;
                        QStringList templist;
                        QStringList templist2;

                        QJsonValue value = obj.value(key);
                        if(value.isArray()){
//                                qDebug() << "is array = ";
//                                qDebug() << "Key = " << key << ", Value = " << value.toString();
                        }else if(value.isObject()){
                            if(key.toUpper() == QLatin1String("PROPERTIES") ){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    templist << value2.toVariant().toString(); //QString::number(value2.toDouble());
                                }
                            }else if(key.toUpper() == QLatin1String("GEOMETRY")){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    if(key2.toUpper() == QLatin1String("COORDINATES")){
                                        if(value2.isObject()){
                                        }else if(value2.isArray()){
                                            QJsonArray jsonArray3 = value2.toArray();
                                            foreach (const QJsonValue & value3, jsonArray3) {
                                                if(value3.isArray()){
                                                    QJsonArray jsonArray4 = value3.toArray();
                                                    foreach (const QJsonValue & value4, jsonArray4) {
                                                        if(value4.isArray()){//added 7.21.2020
                                                            QJsonArray jsonArray5 = value4.toArray();
                                                            foreach (const QJsonValue & value5, jsonArray5) {
                                                                templist2.append(value5.toVariant().toString());
                                                            }
                                                        }else{
                                                            templist2.append(value4.toVariant().toString());
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        lineSegment.append(templist2);
                        if(templist.length() > 0){
                            for(int i = 0; i < templist.length(); i++){
                                lineSegment.insert(i,templist[i]);
                            }
                        }
                    }
                    jsonSegmentList.append(lineSegment);
                }
            }
        }
    }

    return jsonSegmentList;
}

QList<QStringList> newmainwindow::readGeoJsonAir(QJsonObject jObj)
{
    QList<QStringList> jsonSegmentList;
    QStringList lineSegment;
    QJsonObject::iterator i;
    for (i = jObj.begin(); i != jObj.end(); ++i) {
        if(i.key().toUpper() == QLatin1String("FEATURES") ){
            if (i.value().isObject()) {
                QJsonObject innerObject = i.value().toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); ++j) {
                    if (j.value().isObject()) {
                        //qDebug() << '\t' << "OBJECT" << j.key();
                        // ...
                    }
                    // ...
                }
            }else if (i.value().isArray()) {
                QJsonArray jsonArray = i.value().toArray();
                foreach (const QJsonValue & value, jsonArray) {
                    lineSegment.clear();//clears the list

                    QJsonObject obj = value.toObject();
                    foreach(const QString& key, obj.keys()) {

                        QString strval;
                        QStringList templist;
                        QStringList templist2;

                        QJsonValue value = obj.value(key);
                        if(value.isArray()){
//                                qDebug() << "is array = ";
//                                qDebug() << "Key = " << key << ", Value = " << value.toString();
                        }else if(value.isObject()){
                            if(key.toUpper() == QLatin1String("PROPERTIES") ){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    templist << value2.toVariant().toString(); //QString::number(value2.toDouble());
                                }
                            }
                            else if(key.toUpper() == QLatin1String("GEOMETRY")){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    if(key2.toUpper() == QLatin1String("COORDINATES")){
                                        if(value2.isObject()){
                                        }else if(value2.isArray()){
                                            QJsonArray jsonArray3 = value2.toArray();
                                            foreach (const QJsonValue & value3, jsonArray3) {
                                                if(value3.isArray()){
                                                    QJsonArray jsonArray4 = value3.toArray();
                                                    foreach (const QJsonValue & value4, jsonArray4) {
                                                        QJsonArray jsonArray5 = value4.toArray();
                                                        foreach (const QJsonValue & value5, jsonArray5) {
                                                            templist2.append(value5.toVariant().toString());
                                                        }
                                                    }
                                                }
                                            }

                                        }
                                    }
                                }
                            }

                        }
                        lineSegment.append(templist2);
                        if(templist.length() > 0){
                            for(int i = 0; i < templist.length(); i++){
                                lineSegment.insert(i,templist[i]);
                            }
                        }
                    }
                    jsonSegmentList.append(lineSegment);
                }
            }
        }
    }

    return jsonSegmentList;
}


void newmainwindow::on_lockGPS_clicked()
{
    if(ui->lockGPS->isChecked() && ui->wavFileList->count() > 0){
        //Add Lock
        QMetaObject::invokeMethod(gps->wayHandler, "changeMarkerToLockGPS",
            Q_ARG(QVariant, ui->wavFileList->currentText()),
            Q_ARG(QVariant, ui->lockGPS->isChecked()),
            Q_ARG(QVariant, ui->wavFileList->currentText()));

        indexLock = ui->wavFileList->currentIndex();
        wavFileLock = ui->wavFileList->currentText();
        curlocked = true;

    }else{

        //Remove Lock
        QMetaObject::invokeMethod(gps->wayHandler, "changeMarkerToLockGPS",
          Q_ARG(QVariant, ui->wavFileList->currentText()),
          Q_ARG(QVariant, ui->lockGPS->isChecked()),
            Q_ARG(QVariant, ui->wavFileList->currentText()));

        if (ui->wavFileList->count() > 0) {
            //Select Current Selected
            QVariant returnedValue;//added 5.17.2020
            QMetaObject::invokeMethod(gps->wayHandler, "selectWavFile",
                Q_RETURN_ARG(QVariant, returnedValue),//added 5.17.2020
                Q_ARG(QVariant, wavFileLock),
                Q_ARG(QVariant, proj->getAirGroundMaxD()),
                Q_ARG(QVariant, proj->getObsMaxD()));
            /*QString surveyType = proj->getSurveyType();
            if(surveyType == QLatin1String("BAEA")){
                //the returnedValue.toString() format is (BAEA - plotname,1 or 0);
            }*/

            //Select Current wav file if unchecked the GPS Locked
            selectWavFileMap(ui->wavFileList->currentIndex());

        }

        curlocked = false;
        wavFileLock = "";
        indexLock = -1;
    }
}

void newmainwindow::on_actionDistance_Configuration_triggered()
{
    distanceConfig->exec();
    distanceConfig->setFocus();
}

void newmainwindow::on_pushButton_clicked()
{
    //check selected row in table, and remove that thing
    ui->Observations->removeRow(ui->Observations->currentRow());
    //also need to remove it from application data structure----------------??
}

QList<QStringList> newmainwindow::readGeoJson2(QJsonObject jObj)
{
    QList<QStringList> jsonGeomList;
    QStringList geomCoor;

    QJsonObject::iterator i;
    for (i = jObj.begin(); i != jObj.end(); ++i) {
        if(i.key().toUpper() == QLatin1String("FEATURES") ){
            if (i.value().isObject()) {
                //qDebug() << "is object";

            }else if(i.value().isArray()){
                //qDebug() << "is array";
                QJsonArray jsonArray = i.value().toArray();
                foreach (const QJsonValue & value, jsonArray) {
                    if (value.isObject()) {
                        geomCoor.clear();

                        //qDebug() << "is object";
                        QJsonObject innerObject = value.toObject();
                        QJsonObject::iterator j;
                        for (j = innerObject.begin(); j != innerObject.end(); ++j) {

                            QStringList templist;
                            QStringList templist2;

                            if (j.value().isObject()) {
                                //qDebug() << '\t' << " J OBJECT" << j.key();
                                if(j.key().toUpper() == QLatin1String("GEOMETRY") ){
                                    //qDebug() << '\t' << "GEOMETRY: " << j.key();
                                    QJsonObject innerObject2 = j.value().toObject();
                                    QJsonObject::iterator k;
                                    for (k = innerObject2.begin(); k != innerObject2.end(); ++k) {
                                        if (k.value().isObject()) {
                                            //qDebug() << '\t' << "OBJECT" << k.key();
                                            // ...
                                        }else if(k.value().isArray()){
                                            //QJsonObject innerObject3 = k.value().toObject();
                                            QJsonArray jsonArray2 = k.value().toArray();
                                            foreach (const QJsonValue & value2, jsonArray2) {
                                                //QJsonObject obj2 = value2.toObject();
                                                if(k.key().toUpper() == QLatin1String("COORDINATES")){
                                                    if(value2.isObject()){
                                                    }else if(value2.isArray()){
                                                        QJsonArray jsonArray3 = value2.toArray();
                                                        foreach (const QJsonValue & value3, jsonArray3) {
                                                            if(value3.isArray()){
                                                                QJsonArray jsonArray4 = value3.toArray();
                                                                foreach (const QJsonValue & value4, jsonArray4) {
                                                                    QJsonArray jsonArray5 = value4.toArray();
                                                                    foreach (const QJsonValue & value5, jsonArray5) {
                                                                        templist2.append(value5.toVariant().toString());
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }else if(j.key().toUpper() == QLatin1String("PROPERTIES") ){
                                    //qDebug() << '\t' << "PROPERTIES: " << j.key();
                                    QJsonObject obj2 = j.value().toObject();
                                    foreach(const QString& key2, obj2.keys()) {
                                        QJsonValue value2 = obj2.value(key2);
                                        templist << value2.toVariant().toString(); //QString::number(value2.toDouble());
                                    }
                                }
                            }else if(j.value().isArray()){
                                //qDebug() << '\t' << "array" << j.key();
                            }
                            geomCoor.append(templist2);
                            if(templist.length() > 0){
                                for(int i = 0; i < templist.length(); i++){
                                    geomCoor.insert(i,templist[i]);
                                }
                            }
                        }
                        //break;
                        jsonGeomList.append(geomCoor);
                    }else if(value.isArray()){
                        //qDebug() << "is array";
                    }
                }
            }
        }
    }
    //jsonGeomList.append(geomCoor);
    /*for(int i = 0; i < surveyJsonPolygonList.length(); i++){
        qDebug() << surveyJsonPolygonList[i];
    }*/
    return jsonGeomList;
}

void newmainwindow::updateProjectFile(const double &newObsMaxD)
{
    //Save in project File
    //proj->setObsMaxD(newObsMaxD);
    //proj->changesMade = true;

    if(GPS_MAP_ENABLED){
        const QString &surveyType = proj->getSurveyType();
        if(surveyType == QLatin1String("BAEA")){
            //set the maximum observe distance in wayHandler //added 5.15.2020
            QVariant returnedval;
            QMetaObject::invokeMethod(gps->wayHandler, "setMaxDist",
                Q_RETURN_ARG(QVariant,returnedval),
                Q_ARG(QVariant, newObsMaxD),
                Q_ARG(QVariant, "update"),
                Q_ARG(QVariant, "0"));

            setPlotName("");

            progMessage->setValue(40);

            if(!returnedval.toString().isEmpty()){

                returnedValueBAEA = returnedval.toString();
                QStringList splt = returnedval.toString().split(",");
                QString plotnm = splt[0];
                const int &isWithinMaxDist = splt[1].toInt();
                setPlotName(plotnm);
                if(isWithinMaxDist == 0){//over max distance
                    //popup messagebox yes no messagebox yellow
                    QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                        Q_ARG(QVariant,"yellow"));
                }else{
                    QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                        Q_ARG(QVariant,"white"));
                }
            }
        }else if(surveyType == QLatin1String("GOEA")){
            //set the maximum observe distance in wayHandler //added 5.15.2020
            QVariant returnedval;
            QMetaObject::invokeMethod(gps->wayHandler, "setMaxDist",
                Q_RETURN_ARG(QVariant,returnedval),
                Q_ARG(QVariant, newObsMaxD),
                Q_ARG(QVariant, "update"),
                Q_ARG(QVariant, proj->getAirGroundMaxD()));

            progMessage->setValue(40);

            if(!returnedval.toString().isEmpty()){

                returnedValueGOEA = returnedval.toString();
                QStringList splt = returnedval.toString().split(",");
                QString bcr = splt[0];
                QString trn = splt[1];
                const int &isWithinMaxDist = splt[2].toInt();
                const double &dist = splt[3].toDouble();
                setValuesGOEA(bcr,trn,dist);
                if(isWithinMaxDist == 0){//over max distance
                    //popup messagebox yes no messagebox yellow
                    QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                        Q_ARG(QVariant,"yellow"));
                }else{
                    QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                        Q_ARG(QVariant,"white"));
                }

            }
        }

        //added 8.16.2020
        validateConditionalValueFields(ui->comboBox_2->currentText().trimmed(), QLatin1String("species"), true);
    }

    progMessage->setValue(60);
    progMessage->okToClose = true;
    progMessage->close();
    distanceConfig->close();
}

void newmainwindow::updateProjectFile(const double &newObsMaxD, const double &newAirMaxD)
{
    //Save in project File
    //proj->setObsMaxD(newObsMaxD);
    //proj->setAirGroundMaxD(newAirMaxD);
    //proj->changesMade = true;

    //Check Air Ground Max
    if(GPS_MAP_ENABLED){
        QVariant  returnedval;
        QMetaObject::invokeMethod(gps->wayHandler, "setMaxDist",
            Q_RETURN_ARG(QVariant, returnedval),
            Q_ARG(QVariant, newObsMaxD),
            Q_ARG(QVariant, "update"),
            Q_ARG(QVariant, newAirMaxD));

        progMessage->setValue(32);

        QStringList splt = returnedval.toString().split(",");
        const QString &agnm = splt[1];
        const int &isWithinMaxDist = splt[0].toInt();
        if(isWithinMaxDist == 1){
            setAGName(agnm);
        }else {
            setAGName("offag");
        }

        progMessage->setValue(40);

        //for the stratum transect and segment
        returnedValueWPBHS = returnedval.toString();
        const QString &stratum = splt[2];
        const QString &transect = splt[3];
        const QString &segment = splt[4];
        const int &isWithinMaxDist2 = splt[5].toInt();
        const double &dist = splt[6].toDouble();

        setValuesWBPHS(stratum, transect, segment, dist);
        if(isWithinMaxDist2 == 0){//over max distance
            //popup messagebox yes no messagebox yellow
            QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                Q_ARG(QVariant,"yellow"));
        }else{
            QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                Q_ARG(QVariant,"white"));
        }

        //added 8.16.2020
        validateConditionalValueFields(ui->comboBox_2->currentText().trimmed(), QLatin1String("species"), true);

        const QString &boolalert = splt[7];
        if(boolalert == "yes"){
            QMessageBox msgbox;
            msgbox.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            msgbox.setWindowTitle("Validate AirGround Distance");
            msgbox.setText("The observation point is beyond the air ground distance threshold.");
            msgbox.exec();
        }

        progMessage->setValue(50);
    }

    progMessage->okToClose = true;
    progMessage->setValue(60);
    progMessage->close();
    distanceConfig->close();
}

void newmainwindow::plotnameChanged()
{
    //validate value from the list
    const QString &sval = proj->headerFields.at(proj->plotnameIndex)->toPlainText().trimmed();
    //qDebug() << "sval: " << sval;
    bool valexist = false;
    if(sval.isEmpty())
        return;

    for(int i = 0; i < surveyJsonPolygonList.length(); i++){
        const QStringList &slist = surveyJsonPolygonList[i];
        if(slist[0] == sval){
            valexist = true;
            break;
        }
    }

    //qDebug() << surveyJsonPolygonList;

    /*if(!valexist){
        QMessageBox msgbox;
        msgbox.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        msgbox.setWindowTitle("Validate Plot Name");
        msgbox.setText(sval + " is not a valid plot name value.");
        msgbox.exec();
    }*/
}

void newmainwindow::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    disconnectAllHeader();
}

void newmainwindow::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    connectAllHeader();
}

void newmainwindow::validationBCR()
{
    const QString &trnName = proj->headerFields.at(proj->transectIndex)->toPlainText().trimmed();
    auto colC = proj->headerFieldsC;
    CustomComboBox *combo1 = colC[proj->bcrIndex];
    const QString &bcrName = combo1->currentText();
    const QString &brctrnName = bcrName + "." + trnName;
    const QStringList &r = proj->getBCRTRN();

    if (!r.contains(brctrnName, Qt::CaseInsensitive) &&
            proj->getTRNPre() == proj->headerFields.at(proj->transectIndex)->toPlainText()) {
        MessageDialog msgBox;
        msgBox.setText("Do you want to change the Transect?");
        msgBox.setTwoButton("Yes", 50, "No", 50);
        msgBox.exec();
        if (msgBox.isAccept()) {
            proj->headerFields.at(proj->transectIndex)->setFocus();
            proj->headerFields.at(proj->transectIndex)->setText("");
        }
        else {
            MessageDialog msgBox;
            msgBox.setWindowTitle("Error");
            msgBox.setText("Warning! The combinations of transect and BCR are not valid.");
            msgBox.exec();
        }
    }
}



void newmainwindow::on_actionProject_Information_triggered()
{
    projectInformation = new ProjectInformation(this);
    projectInformation->show();
}

void newmainwindow::getCurrentStatusAudio(const int &status)
{
    //qDebug() << "getCurrentStatusAudio: " << status;
    switch(status) {
        case 2:{
            //added 8.19.2020
            ui->Play->setText("PLAY");
            audio->status = 0;
            //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);//remarked 8.20.2020
            break;
        }
        case 1: {
            ui->Play->setText("PLAY");
            //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);//remarked 8.20.2020
            break;
        }
        default:{
            ui->Play->setText("PAUSE");
            //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
            if(ui->wavFileList->currentIndex() == ui->wavFileList->count() - 1){//added 8.19.2020
                ui->Play->setText("PLAY");
                //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
            }
            break;
        }
    }
}

void newmainwindow::on_export_2_clicked()
{
    if (!pressDoubleClick) {
        AppendASC();
        ui->Observations->setRowCount(0);
    }
    else {
        MessageDialog msgBox;
        msgBox.setTitle("Error");
        msgBox.setText("Editing observation is not complete yet.");
        msgBox.exec();
    }
}

void newmainwindow::onMaximizeMap(QString str)//int intval
{
    //qDebug() << "onMaximizeMap: " << str ;
    if(str == QLatin1String("1")){
        ui->groupBox_6->hide();
        ui->grpObser->hide();

        ui->groupBox->hide();
        ui->grpAudio->hide();

        ui->groupBox_2->hide();
        ui->grpSpHot->hide();
    }else{
        ui->groupBox_6->show();
        ui->grpObser->show();

        //ui->groupBox_5->setVisible(boolval);
        //ui->grpHF->setVisible(boolval);

        ui->groupBox->show();
        ui->grpAudio->show();

        ui->groupBox_2->show();
        ui->grpSpHot->show();
    }

    this->repaint();
    this->update();
}

QList<QStringList> newmainwindow::getConditionalValueFieldsPrevValues(QList<QStringList> &headerList)
{
    QList<QStringList> newList;
    QStringList sList;
    foreach(QStringList slist, headerList){
        if(slist.count() == 2){
            sList.empty();
            QString sFieldName = slist[0];
            const int &ind = proj->allHeaderV.indexOf(sFieldName);
            if(ind == -1){
                //select the customcombobox get current text
                for(int h = 0; h < proj->headerIndexC.count(); h++) {
                    const int &indexAdd = proj->headerIndexC[h];
                    if (proj->getHeaderName(indexAdd) == sFieldName) {
                        auto combo1 = proj->headerFieldsC.at(h);

                        sList.append(sFieldName);
                        sList.append(combo1->currentText().trimmed());
                        newList.append(sList);
                        sList.clear();
                        break;
                    }
                }
            }else if(ind > -1){
                //get the CustomEventWidget and get existing value
                sList.append(sFieldName);
                sList.append(proj->headerFields.at(ind)->toPlainText().trimmed());
                newList.append(sList);
                sList.clear();
            }//end if
        }
    }//end foreach

    return newList;
}

void newmainwindow::checkBCRValue(const QString& bcrval)
{
    //added 8.17.2020 conditional field values for BCR
    if(bcrval.toUpper() == QLatin1String("MW")){
        QList<QStringList> headerConditionalVals = getHeaderConditionalValues( bcrval.toUpper(),
            QLatin1String("Headers"), QLatin1String("BCR"));
        foreach(QStringList slist,headerConditionalVals){
            if(slist.count() == 3){
                QString sType = slist[0];
                QString sFieldName = slist[1];
                QString sFielsVals = slist[2];

                bool boolheaderexist = false;
                if(sType.toUpper() == QLatin1String("HEADERS")){
                    for(int i=0; i < ui->HeaderFields->rowCount();i++){
                        QString header = ui->HeaderFields->item(i,0)->text().left(ui->HeaderFields->item(i,0)->text().length() - 2);
                        if(header.toUpper() == sFieldName.toUpper()){
                            boolheaderexist = true;
                            break;
                        }
                    }
                    //if does not exist add new item at header fields
                    if(!boolheaderexist){
                        QTableWidgetItem *item = new QTableWidgetItem();
                        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
                        item->setData(Qt::FontRole,QVariant(proj->getFontItem()));
                        item->setText(sFieldName + ": ");
                        ui->HeaderFields->insertRow(proj->getHeaderNamesList().count());
                        ui->HeaderFields->setItem(proj->getHeaderNamesList().count(), 0, item);
                        const QString &css = "font-size:  13px; "
                                    "font-family: 'Segoe UI'; "
                                    "background-color:#FFFFFF; "
                                    "margin:1px;"
                                    "border: 1px solid rgba(0, 0, 0, 0.2);"
                                    "padding-left:4px";

                        CustomEventWidget *s = new CustomEventWidget();
                        s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                        s->setStyleSheet(css);
                        s->setTitle(sFieldName);
                        s->setMinimumHeight(30);
                        s->setText(sFielsVals);
                        ui->HeaderFields->setCellWidget(proj->getHeaderNamesList().count(), 1, s);
                    }
                }
            }
        }
        tempHeaderConditionalVals = headerConditionalVals;
    }else{
        //remove the newly added header fields in conditional values
        foreach(QStringList slist, tempHeaderConditionalVals){
            if(slist.count() == 3){
                QString sType = slist[0];
                QString sFieldName = slist[1];
                QString sFielsVals = slist[2];
                if(sType.toUpper() == QLatin1String("HEADERS")){
                    for(int i=0; i < ui->HeaderFields->rowCount();i++){
                        QString header = ui->HeaderFields->item(i,0)->text().left(ui->HeaderFields->item(i,0)->text().length() - 2);
                        if(header.toUpper() == sFieldName.toUpper()){
                            ui->HeaderFields->removeRow(i);
                        }
                    }
                }
            }
        }
        tempHeaderConditionalVals.clear();
    }
}

void newmainwindow::on_actionExport_Map_to_KML_triggered()
{
    QString filename = QFileDialog::getSaveFileName(
            this,
            tr("Save Document"),
            QDir::currentPath(),
            tr("KML (*.kml)") );
    if( !filename.isNull() ){
        try {
            //qDebug() << filename;
            QFile data(filename);
            if (data.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream out(&data);
                //kml header
                out << "<?xml version='1.0' encoding='UTF-8'?> ";
                out << "<kml xmlns='http://www.opengis.net/kml/2.2' ";
                out << "xmlns:gx='http://www.google.com/kml/ext/2.2' ";
                out << "xmlns:kml='http://www.opengis.net/kml/2.2' ";
                out << "xmlns:atom='http://www.w3.org/2005/Atom'> " << "\r\n";

                //kml body
                out << "<Document> " << "\r\n";
                out << QString("    <name>%1</name>").arg(filename) << "\r\n";
                out << "    <StyleMap id='m_ylw-pushpin'> " << "\r\n";
                out << "        <Pair> " << "\r\n";
                out << "            <key>normal</key> " << "\r\n";
                out << "            <styleUrl>#s_ylw-pushpin</styleUrl> " << "\r\n";
                out << "        </Pair> " << "\r\n";
                out << "        <Pair> " << "\r\n";
                out << "            <key>highlight</key> " << "\r\n";
                out << "            <styleUrl>#s_ylw-pushpin_hl</styleUrl> " << "\r\n";
                out << "        </Pair> " << "\r\n";
                out << "    </StyleMap> " << "\r\n";
                out << "    <Style id='s_ylw-pushpin_hl'> " << "\r\n";
                out << "        <IconStyle> " << "\r\n";
                out << "            <scale>1.3</scale> " << "\r\n";
                out << "            <Icon> " << "\r\n";
                out << "                <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href> " << "\r\n";
                out << "            </Icon> ";
                out << "            <hotSpot x='20' y='2' xunits='pixels' yunits='pixels'/> " << "\r\n";
                out << "        </IconStyle> " << "\r\n";
                out << "    </Style> " << "\r\n";
                out << "    <Style id='s_ylw-pushpin'> " << "\r\n";
                out << "        <IconStyle> " << "\r\n";
                out << "            <scale>1.1</scale> " << "\r\n";
                out << "            <Icon> " << "\r\n";
                out << "                <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href> " << "\r\n";
                out << "            </Icon> " << "\r\n";
                out << "            <hotSpot x='20' y='2' xunits='pixels' yunits='pixels'/> " << "\r\n";
                out << "        </IconStyle> " << "\r\n";
                out << "    </Style> " << "\r\n";

                out << "    <Style id='sn_ylw-pushpin'> " << "\r\n";
                out << "        <IconStyle> " << "\r\n";
                out << "            <scale>1.1</scale> " << "\r\n";
                out << "            <Icon> " << "\r\n";
                out << "                <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href> " << "\r\n";
                out << "            </Icon> " << "\r\n";
                out << "            <hotSpot x='20' y='2' xunits='pixels' yunits='pixels'/> " << "\r\n";
                out << "        </IconStyle> " << "\r\n";
                out << "        <BalloonStyle> " << "\r\n";
                out << "        </BalloonStyle> " << "\r\n";
                out << "        <LineStyle> " << "\r\n";
                out << "            <color>ff0000ff</color> " << "\r\n";
                out << "            <width>1.8</width> " << "\r\n";
                out << "        </LineStyle> " << "\r\n";
                out << "    </Style> " << "\r\n";
                out << "    <Style id='sh_ylw-pushpin'> " << "\r\n";
                out << "        <IconStyle> " << "\r\n";
                out << "            <scale>1.3</scale> " << "\r\n";
                out << "            <Icon> " << "\r\n";
                out << "                <href>http://maps.google.com/mapfiles/kml/pushpin/ylw-pushpin.png</href> " << "\r\n";
                out << "            </Icon> " << "\r\n";
                out << "            <hotSpot x='20' y='2' xunits='pixels' yunits='pixels'/> " << "\r\n";
                out << "        </IconStyle> " << "\r\n";
                out << "        <BalloonStyle> " << "\r\n";
                out << "        </BalloonStyle> " << "\r\n";
                out << "        <LineStyle> " << "\r\n";
                out << "            <color>ff0000ff</color> " << "\r\n";
                out << "            <width>1.8</width> " << "\r\n";
                out << "        </LineStyle> " << "\r\n";
                out << "    </Style> " << "\r\n";
                out << "    <StyleMap id='msn_ylw-pushpin'> " << "\r\n";
                out << "        <Pair> " << "\r\n";
                out << "            <key>normal</key> " << "\r\n";
                out << "            <styleUrl>#sn_ylw-pushpin</styleUrl> " << "\r\n";
                out << "        </Pair> " << "\r\n";
                out << "        <Pair> " << "\r\n";
                out << "            <key>highlight</key> " << "\r\n";
                out << "            <styleUrl>#sh_ylw-pushpin</styleUrl> " << "\r\n";
                out << "        </Pair> " << "\r\n";
                out << "    </StyleMap> " << "\r\n";

                int numberOfColumns = ui->Observations->columnCount();
                QStringList flds;

                for(int i=0; i < ui->Observations->rowCount(); i++){
                    QStringList objVal;
                    int idxtitle = 0;
                    for (int j = 0; j < numberOfColumns; j++) {
                        objVal << ui->Observations->item(i,j)->text();
                        if(i == 0)
                            flds << ui->Observations->horizontalHeaderItem(j)->text();

                        if(ui->Observations->horizontalHeaderItem(j)->text().toLower() == QLatin1String("audio file"))
                            idxtitle = j;
                    }

                    //create placemark observations
                    QString sval = populateKMLObservation(flds,objVal,idxtitle, "s_ylw-pushpin");
                    out << sval;
                }

                //added7.7.2020
                //check for the asc file for saved observations
                GlobalSettings &globalSettings = GlobalSettings::getInstance();
                QString exportLocation = globalSettings.get("projectDirectory") + "/" + globalSettings.get("exportFilename") + ".asc";
                QFile exportFile(exportLocation);
                QStringList ascLines;
                if (exportFile.open(QIODevice::ReadOnly)){
                    QTextStream stream(&exportFile);

                    while(!stream.atEnd()){
                        QString currentAscLine = stream.readLine();
                        ascLines << currentAscLine;
                    }
                    exportFile.close();
                }

                if(ascLines.length() > 1){
                    int numberOfColumns1 = ascLines[0].split(",").length();
                    int idxtitle = 0;
                    for(int i=0; i < ascLines.length(); i++){
                        QStringList objVal;
                        if(i==0){
                            flds.clear();
                            flds << ascLines[0].split(",");
                            for(int k=0; k < flds.length(); k++){
                                if(flds[k].toLower() == QLatin1String("audio file")){
                                    idxtitle = k;
                                    //qDebug() << "idxtitle" << idxtitle;
                                }
                            }
                        }else{
                            QStringList ascRow = ascLines[i].split(",");
                            for (int j = 0; j < numberOfColumns1; j++) {
                                objVal << ascRow[j];
                            }
                            //create placemark observations
                            QString sval = populateKMLObservation(flds,objVal,idxtitle, "s_ylw-pushpin");
                            out << sval;
                        }
                    }
                }

                //create placemark flight path
                QString sval1 = populateKMLFlightPath("Flight Path","msn_ylw-pushpin");
                out << sval1;

                out << "</Document> " << "\r\n";
                //kml footer
                out << "</kml>";
            }

            MessageDialog msgBox;
            msgBox.setText("You have successfully exported the file.");
            msgBox.setFontSize(13);
            msgBox.exec();
        } catch(...) {
            MessageDialog msgBox;
            msgBox.setText("Something went wrong \n\rwhile exporting the kml.");
            msgBox.setFontSize(13);
            msgBox.exec();
        }
    }
}

void newmainwindow::on_actionInterpolate_Flight_GPS_Data_triggered()
{
    interpolatedialog = new interpolategps(this);
    interpolatedialog->m_flightpath = m_flightpath;
    interpolatedialog->m_flightFilePath = m_flightFilePath;

    //qDebug() << m_flightpath.length();
    //qDebug() << m_flightFilePath;
    interpolatedialog->initForm(gps);

    interpolatedialog->show();
}

void newmainwindow::action_editSpeciesClicked()
{
    speciesDialog = new LogItemEditDialog(this, "Species/Items Edit", true,
                                          "Specimen and Hotkeys");
    speciesDialog->setModal(true);
    connect(speciesDialog,SIGNAL(Done()),projSettingsDialog,SLOT(GetSpeciesAndGroupingWithoutSlot()));
    connect(speciesDialog,SIGNAL(Loaded()),projSettingsDialog,SLOT(GetSpeciesAndGrouping()));
    /*connect(this,SIGNAL(speciesDialogUpdate(QStringList)),speciesDialog,SLOT(ReceiveItems(QStringList)));*/

    speciesDialog->openNew = proj->newOpen;
    speciesDialog->loadCurrentList(proj->addedSpecies);
    if(defaultUsedHotKeys.count() == 0)
        CreateLogItemShortcuts();
    speciesDialog->defaultUsedHotkeys = defaultUsedHotKeys;
    speciesDialog->loadOtherItems(proj->addedGroupings);
    speciesDialog->RefreshItems(speciesDialog->preItems);
    speciesDialog->show();
}

void newmainwindow::action_editGroupingClicked()
{
    groupingDialog = new LogItemEditDialog(this, "Edit Grouping", false,
                                           "Groupings");
    groupingDialog->setModal(true);
    connect(groupingDialog,SIGNAL(Done()),projSettingsDialog,SLOT(GetSpeciesAndGroupingWithoutSlot()));
    //connect(this,SIGNAL(groupingsDialogUpdate(QStringList)),groupingDialog,SLOT(ReceiveItems(QStringList)));

    groupingDialog->openNew = proj->newOpen;
    groupingDialog->loadCurrentList(proj->addedGroupings);
    if(defaultUsedHotKeys.count() == 0)
        CreateLogItemShortcuts();
    groupingDialog->defaultUsedHotkeys = defaultUsedHotKeys;
    groupingDialog->loadOtherItems(proj->addedSpecies);
    groupingDialog->show();
}

/*void newmainwindow::on_pushSpHot_clicked()
{
    //qDebug() << "pushSpHot_clicked1";
    QMenu *menu=new QMenu(this);
    menu->addAction(new QAction("Action 1", this));
    menu->addAction(new QAction("Action 2", this));
    menu->addAction(new QAction("Action 3", this));
    menu->popup(this->mapToGlobal(pos));
}*/

QList<QStringList> newmainwindow::calculateMissingSec(QStringList flightdata)
{
    QList<QStringList> missinglist;
    gps = new GpsHandler(ui->quickWidget->rootObject(),  startDir);
    try{
        int cntr1 = 0;
        //qDebug() << "AA";

        for(int s = 0; s < (flightdata.length() -1); s++){
            QStringList missing;
            QStringList flightrow = flightdata[s].split(",");
            //qDebug() << "flightrow: " << flightrow;
            if(flightrow.length() >= 12){
                QString strtime = flightrow[5].trimmed();
                QStringList spltime = strtime.split(":");
                QDateTime t1(
                        QDate(flightrow[2].trimmed().toInt(),
                            flightrow[3].trimmed().toInt(),
                            flightrow[4].trimmed().toInt()),
                        QTime(spltime[0].trimmed().toInt(),
                            spltime[1].trimmed().toInt(),
                            spltime[2].trimmed().toInt())
                        );


                QStringList flightrow1 = flightdata[s+1].split(",");

                if(flightrow1.count() < 6)
                    break;

                QString strtime1 = flightrow1[5].trimmed();
                QStringList spltime1 = strtime1.split(":");
                QDateTime t2(
                        QDate(flightrow1[2].trimmed().toInt(),
                            flightrow1[3].trimmed().toInt(),
                            flightrow1[4].trimmed().toInt()),
                        QTime(spltime1[0].trimmed().toInt(),
                            spltime1[1].trimmed().toInt(),
                            spltime1[2].trimmed().toInt())
                        );

                if(t1.secsTo(t2) >= 2){
                    if(t1.secsTo(t2) == 2){
                        missing.clear();
                        QStringList sval;
                        sval << QString::number(flightrow[0].toDouble(),'f',8)
                                << QString::number(flightrow[1].toDouble(),'f',8)
                                << QString::number(flightrow1[0].toDouble(),'f',8)
                                << QString::number(flightrow1[1].toDouble(),'f',8)
                                << QString::number(2)
                                << QString::number(1);

                        QVariant returnedval;

                        QMetaObject::invokeMethod(gps->wayHandler, "findCoor",
                            Q_RETURN_ARG(QVariant,returnedval),
                            Q_ARG(QVariant, sval));
                        QStringList splt = returnedval.toString().split(",");
                        QString lat = splt[0];
                        QString lon = splt[1];

                        missing << QString::number(lat.toDouble(),'f',8);
                        missing << QString::number(lon.toDouble(),'f',8);

                        QDateTime dt = t1.addSecs(1);
                        QString sdate = dt.toString("yyyy.MM.dd.hh:mm:ss");
                        QStringList lstsdate = sdate.split(".");

                        for(int i=0; i<lstsdate.length();i++ ){
                            missing << lstsdate[i].trimmed();
                        }

                        int j=6;
                        do{
                            missing << "";
                            j+=1;
                        }while(j < flightrow.length());

                        //qDebug() << missing;
                        missinglist.append(missing);
                        cntr1 += 1;

                    }else if(t1.secsTo(t2) > 2){
                        //qDebug() << "secto: " << t1.secsTo(t2);
                        int cntr = 1;
                        do{
                            missing.clear();
                            QStringList sval;
                            sval << QString::number(flightrow[0].toDouble(),'f',8)
                                    << QString::number(flightrow[1].toDouble(),'f',8)
                                    << QString::number(flightrow1[0].toDouble(),'f',8)
                                    << QString::number(flightrow1[1].toDouble(),'f',8)
                                    << QString::number(t1.secsTo(t2))
                                    << QString::number(cntr);

                            QVariant returnedval;
                            QMetaObject::invokeMethod(gps->wayHandler, "findCoor",
                                Q_RETURN_ARG(QVariant,returnedval),
                                Q_ARG(QVariant, sval));
                            QStringList splt = returnedval.toString().split(",");
                            QString lat = splt[0];
                            QString lon = splt[1];

                            missing << QString::number(lat.toDouble(),'f',8);
                            missing << QString::number(lon.toDouble(),'f',8);

                            QDateTime dt = t1.addSecs(cntr);
                            QString sdate = dt.toString("yyyy.MM.dd.hh:mm:ss");
                            QStringList lstsdate = sdate.split(".");

                            for(int i=0; i<lstsdate.length();i++ ){
                                missing << lstsdate[i].trimmed();
                            }

                            int j=6;
                            do{
                                missing << "";
                                j+=1;
                            }while(j < flightrow.length());

                            //qDebug() << missing;
                            missinglist.append(missing);

                            cntr1 += 1;
                            cntr += 1;
                        }while(cntr < t1.secsTo(t2));
                    }
                }
            }
        }

        return missinglist;

    }catch(QException &ex){
        //QMessageBox msg
        qDebug()  << "QException: " << ex.what();
        return missinglist;
    }catch (...) {
        return missinglist;
    }
}

void newmainwindow::saveGps(QStringList flightpath, QList<QStringList> missinglist, QString filename)
{
    try {
        QList<QStringList> newlist;
        if(flightpath.length() > 1 && missinglist.length() > 1){
            for(int i=0;i< flightpath.length();i++){
                QStringList strlst;
                QStringList flightrow = flightpath[i].split(",");
                QStringList flightrow1 = flightpath[1].split(",");
                if(flightrow.length() >= 12){
                    QString strtime = flightrow[5].trimmed();
                    QStringList spltime = strtime.split(":");
                    QDateTime t1(
                            QDate(flightrow[2].trimmed().toInt(),
                                flightrow[3].trimmed().toInt(),
                                flightrow[4].trimmed().toInt()),
                            QTime(spltime[0].trimmed().toInt(),
                                spltime[1].trimmed().toInt(),
                                spltime[2].trimmed().toInt())
                            );
                    //Get Decimal from seconds
                    QString secondVal;
                    if (spltime[2].trimmed().contains(".")) {
                        QStringList spltSecond = spltime[2].trimmed().split(".");
                        secondVal = "." + spltSecond[1];
                    }
                    /*QDateTime t1 = QDateTime::fromString(flightrow[2].trimmed() + "-"
                            + flightrow[3].trimmed() +flightrow[4].trimmed() + " " +
                            strtime);*/
                    strlst << t1.toString() + secondVal;
                    for(int ii = 0; ii < flightrow.length(); ii++){
                        if(ii < flightrow1.length()){
                            if(flightrow[ii].trimmed() == QLatin1String("*"))
                                strlst << flightrow[ii].trimmed().replace("*","");
                            else
                                strlst << flightrow[ii].trimmed();
                        }
                    }

                    newlist << strlst;
                }
            }

            for(int i=0;i<missinglist.length();i++){
                //qDebug() << "missinglist: " << missinglist[i];
                QStringList row = missinglist[i];
                QStringList row1 = missinglist[1];
                QStringList strlst;
                for(int ii=0;ii<row.length();ii++){
                    strlst.clear();
                    QString strtime = row[5];
                    QStringList spltime = strtime.split(":");
                    QDateTime t1(
                            QDate(row[2].toInt(),
                                row[3].toInt(),
                                row[4].toInt()),
                            QTime(spltime[0].trimmed().toInt(),
                                spltime[1].trimmed().toInt(),
                                spltime[2].trimmed().toInt())
                            );
                    strlst << t1.toString();
                    for(int iii = 0; iii < row.length(); iii++){
                        if(iii < row1.length())
                            strlst << row[iii].trimmed();
                    }
                }
                newlist << strlst;
            }

            /*for(int s=0;s < newlist.length();s++){
                qDebug() << newlist[s];
            }*/

        }else{
            /*MessageDialog msgBox;
            msgBox.setText("Something went wrong \n\rwhile saving the GPS file.");
            msgBox.setFontSize(13);
            msgBox.exec();
            return;*/
            qDebug() << "Something went wrong \n\rwhile saving the GPS file.";
        }

        //qDebug() << filename;
        QFile data(filename);
        if (data.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&data);

            if(newlist.length() > 0){
                std::sort( newlist.begin(), newlist.end(),  lessThan1 );
            }

            // Print out the list data so we can see that it got sorted ok
            for( int i = 0; i < newlist.size(); i++ )
            {
                QStringList newrow = newlist[i];
                newrow.removeFirst();
                QString data = newrow.join( "," );
                //qDebug() << QString( "Item %1: %2" ).arg( i + 1 ).arg( data );

                out << data << "\n";
            }
        }
        /*MessageDialog msgBox;
        msgBox.setText("You have successfully saved the file.");
        msgBox.setFontSize(13);
        QFont font("Segoe UI");
        msgBox.setFont(font);
        msgBox.exec();*/
        qDebug() << "You have successfully saved the file.";
    } catch(...) {
        /*MessageDialog msgBox;
        msgBox.setText("Something went wrong \n\rwhile saving the GPS file.");
        msgBox.setFontSize(13);
        QFont font("Segoe UI");
        msgBox.setFont(font);
        msgBox.exec();*/
        qDebug() << "Something went wrong \n\rwhile saving the GPS file.";
    }
}

void newmainwindow::validateConditionalValueFields(const QString &arg, QString category, bool boolWarning)
{
    if(!arg.isEmpty()){
        bool boolconditionalval = false;
        QJsonArray jsonArray = proj->getConditionalValueFieldsArray();

        QList<QStringList> headerList;
        QList<QStringList> addnlList;
        foreach (const QJsonValue &value, jsonArray) {
            QJsonObject innerObject = value.toObject();
            QString sType = innerObject["Type"].toString();
            QString sValue = innerObject["Value"].toString();

            QStringList headerVals;
            foreach (const QJsonValue &v, innerObject["SetValues"].toArray()) {
                if(sType.toLower() == category &&
                        sValue.toUpper() == arg.toUpper()){

                    headerVals.clear();
                    boolconditionalval = true;

                    QJsonObject obj2 = v.toObject();
                    QString sType2 = obj2["Type"].toString();
                    QString sFieldName2 = obj2["FieldName"].toString();

                    //qDebug() << "sValue1: " << sValue;
                    foreach(const QString& key1, obj2.keys()) {
                        //qDebug() << "key1: " << key1;
                        if(key1.contains(QLatin1String("Values"),Qt::CaseInsensitive)){
                            if(sType2.toUpper() == QLatin1String("HEADERS")){
                                if(!sFieldName2.isEmpty()){
                                    QJsonArray jsonArray1 = obj2.value(key1).toArray();
                                    if(jsonArray1.count() > 1){
                                        QStringList svals;
                                        foreach (const QJsonValue & value1, jsonArray1) {
                                            svals.append(value1.toVariant().toString());
                                        }
                                        headerVals.append(sFieldName2);
                                        headerVals.append(svals.join(","));
                                        headerList.append(headerVals);
                                    }else if(jsonArray1.count() == 0){
                                        headerVals.append(sFieldName2);
                                        headerVals.append(QLatin1String(""));
                                        headerList.append(headerVals);
                                    }else if(jsonArray1.count() == 1){
                                        headerVals.append(sFieldName2);
                                        headerVals.append(jsonArray1[0].toVariant().toString());
                                        headerList.append(headerVals);
                                    }
                                }
                            }
                        }
                    }//end foreach
                }
            }//end foreach
        }//end foreach

        if(headerList.count() == 0){
            if(!tempHeaderList.isEmpty()){
                conditionalFieldValuesEnabled(tempHeaderList,true,true);
                tempHeaderList.clear();
            }
        }else{
            if(boolWarning){//added 8.16.2020
                MessageDialog msgBox;
                msgBox.setText("Please bear in mind that your\n"
                               "selected species on the observation window\n"
                               "is part of the conditional values.");
                msgBox.setFontSize(13);
                msgBox.setTwoButton("Change Species", 180, "Continue", 80);
                msgBox.exec();
                if (msgBox.isAccept()) {
                    setSpecies(0,0);
                    return;
                }
            }

            if(boolconditionalval && tempHeaderList.isEmpty()){
                tempHeaderList = getConditionalValueFieldsPrevValues(headerList);
                /*foreach(QStringList slist,tempHeaderList){
                    qDebug() << slist;
                }*/
            }
            conditionalFieldValuesEnabled(headerList,false,true);
        }
    }
}

void newmainwindow::conditionalFieldValuesEnabled(QList<QStringList> &headerList, bool boolEnableControl, bool boolSetVaues)
{
    foreach(QStringList slist, headerList){
        //qDebug() << slist.count() << slist;
        if(slist.count() == 2){
            QString sFieldName = slist[0];
            QString sFieldValue = slist[1];

            QStringList splt = sFieldValue.split(",");
            //qDebug() << "splt.count():" << splt.count();

            const int &ind = proj->allHeaderV.indexOf(sFieldName);
            //qDebug() << "ind:" << ind;
            if(ind == -1){
                //select the customcombobox 1st index and disable the control
                for(int h = 0; h < proj->headerIndexC.count(); h++) {
                    const int &indexAdd = proj->headerIndexC[h];
                    if (proj->getHeaderName(indexAdd) == sFieldName) {
                        auto combo1 = proj->headerFieldsC.at(h);
                        //qDebug() << "pw1:" << pw1->metaObject()->className();

                        combo1->setEnabled(boolEnableControl);
                        if(boolSetVaues){
                            if(splt.count() == 0){
                                combo1->setCurrentIndex(0);//set the current index to null value
                            }else if(splt.count() == 1){
                                bool noval = true;
                                for(int ii=0; ii<combo1->count(); ii++ ){
                                    if(combo1->itemText(ii) == splt[0].trimmed()){
                                        combo1->setCurrentIndex(ii);
                                        noval = false;
                                        break;
                                    }
                                }
                                if(noval)//if no value matches the conditional fields
                                    combo1->setCurrentIndex(0);
                            }else if(splt.count() > 1){
                                //conditional value fields has many values
                            }
                        }

                        //boolconditionalval = true;
                        break;
                    }
                }
                //set the values for the combobox

            }else if(ind > -1){
                //clear the CustomEventWidget and set value from surv json
                proj->headerFields.at(ind)->clear();
                //boolconditionalval = true;

                QString sval = QLatin1String("");

                auto pw1 = proj->headerFields.at(ind);
                //qDebug() << "pw1:" << pw1->metaObject()->className();
                pw1->setEnabled(boolEnableControl);

                if(boolSetVaues){
                    if(splt.count() == 0){
                        proj->headerFields.at(ind)->setText(sval);
                    }else if(splt.count() == 1){
                        proj->headerFields.at(ind)->setText(splt[0].trimmed());
                    }else if(splt.count() > 1){
                        for(int i=0; i < splt.count(); i++) {
                            //for values more than 1
                        }
                    }
                }

            }//end if
        }
    }//end foreach
}

QList<QStringList> newmainwindow::getHeaderConditionalValues(const QString &arg, QString category, QString sField)
{
    QList<QStringList> addnlList;
    QJsonArray jsonArray = proj->getConditionalValueFieldsArray();

    foreach (const QJsonValue &value, jsonArray) {
        QJsonObject innerObject = value.toObject();
        QString sType = innerObject["Type"].toString();
        QString sValue = innerObject["Value"].toString();

        foreach(const QString& key, innerObject.keys()) {
            //qDebug() << "key: " << key;
            if(key.contains(QLatin1String("FieldName"),Qt::CaseInsensitive)){

                QString sFieldNm = innerObject["FieldName"].toString();

                QStringList addnlVals;
                foreach (const QJsonValue &v, innerObject["SetValues"].toArray()) {
                    if(sType.toUpper() == category.toUpper() &&
                            sValue.toUpper() == arg.toUpper() &&
                            sFieldNm.toUpper() == sField.toUpper()){

                        addnlVals.clear();
                        QJsonObject obj2 = v.toObject();
                        QString sType2 = obj2["Type"].toString();
                        QString sFieldName2 = obj2["FieldName"].toString();

                        foreach(const QString& key1, obj2.keys()) {
                            //qDebug() << "key1: " << key1;
                            if(key1.contains(QLatin1String("Values"),Qt::CaseInsensitive)){
                                if(!sFieldName2.isEmpty()){
                                    QJsonArray jsonArray1 = obj2.value(key1).toArray();
                                    if(jsonArray1.count() > 1){
                                        QStringList svals;
                                        foreach (const QJsonValue & value1, jsonArray1) {
                                            svals.append(value1.toVariant().toString());
                                        }
                                        addnlVals.append(sType2);
                                        addnlVals.append(sFieldName2);
                                        addnlVals.append(svals.join(","));
                                        addnlList.append(addnlVals);
                                        addnlVals.clear();
                                    }else if(jsonArray1.count() == 0){
                                        addnlVals.append(sType2);
                                        addnlVals.append(sFieldName2);
                                        addnlVals.append(QLatin1String(""));
                                        addnlList.append(addnlVals);
                                        addnlVals.clear();
                                    }else if(jsonArray1.count() == 1){
                                        addnlVals.append(sType2);
                                        addnlVals.append(sFieldName2);
                                        addnlVals.append(jsonArray1[0].toVariant().toString());
                                        addnlList.append(addnlVals);
                                        addnlVals.clear();
                                    }
                                }
                            }
                        }//end foreach
                    }
                }//end foreach
            }
        }
    }//end foreach

    return addnlList;
}

void newmainwindow::saveScribeSettings(bool saveAsc)
{
    try{
        QString settingsFile = QCoreApplication::applicationDirPath() + "/settings.ini";
        QFile setfile(settingsFile);
        if(!setfile.exists()){
            //qDebug() << "dont exist; ";
            if (!setfile.open(QIODevice::WriteOnly|QIODevice::Append)){
                //qDebug() << "Couldn't open file for writing";
                MessageDialog msgBox;
                msgBox.setText("The system needs permission to add\n the scribe settings file.");
                msgBox.setFontSize(13);
                msgBox.exec();
                return;
            }else
                scribeSettings = new QSettings(settingsFile,QSettings::IniFormat);
        }

        GlobalSettings &globalSettings = GlobalSettings::getInstance();

        //RecentOpeningScreen settings
        scribeSettings->setValue("RecentOpeningScreen/surveyType",proj->getSurveyType());
        scribeSettings->setValue("RecentOpeningScreen/saveFilename",globalSettings.get("exportFilename"));
        scribeSettings->setValue("RecentOpeningScreen/folderDirectory",globalSettings.get("projectDirectory"));
        scribeSettings->setValue("RecentOpeningScreen/obsInit",globalSettings.get("observerInitials"));
        scribeSettings->setValue("RecentOpeningScreen/seat",globalSettings.get("observerSeat"));
        scribeSettings->setValue("RecentOpeningScreen/autoUnit",globalSettings.get("autoUnit"));
        scribeSettings->setValue("RecentOpeningScreen/surveyJson",globalSettings.get("geoFile"));
        scribeSettings->setValue("RecentOpeningScreen/airGroundJson",globalSettings.get("airGroundFile"));

        //Map settings
        QString returnval = "";
        QVariant returnedValue = "";//added 5.17.2020
        QMetaObject::invokeMethod(gps->wayHandler, "getMapRecentSetting",
            Q_RETURN_ARG(QVariant, returnedValue));
        if(!returnedValue.toString().isEmpty()){
            QStringList splt = returnedValue.toString().split(",");
            int intZoom = qRound(splt[0].toDouble());
            scribeSettings->setValue("Map/zoom", QString::number(intZoom));
            scribeSettings->setValue("Map/latitude", splt[1]);
            scribeSettings->setValue("Map/longitude", splt[2]);
            scribeSettings->setValue("Map/maptype", splt[3]);
        }

        //Audio settings
        QString islock = "no";
        scribeSettings->setValue("Audio/wavFile", ui->wavFileList->currentText().trimmed());
        if(ui->lockGPS->isChecked())
            islock = "yes";
        scribeSettings->setValue("Audio/lock", islock);

        //Species settings
        scribeSettings->setValue("Species/selected", ui->comboBox_2->currentText().trimmed());

        //Observation settings
        QString csvFileName = globalSettings.get("projectDirectory") + "/unsaved_observation.csv";
        if(!saveAsc){//save the observation as csv file
            QFile csvfile(csvFileName);
            if (!csvfile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text)){
                //qDebug() << "Couldn't open file for writing";
                MessageDialog msgBox;
                msgBox.setText("The system needs permission to add\n the unsaved observation file.");
                msgBox.setFontSize(13);
                msgBox.exec();
                return;
            }
            QTextStream stream (&csvfile);
            int numberOfColumns = ui->Observations->columnCount();

            qint64 size = csvfile.size();
            if(size == 0){
               if (stream.readLine() == nullptr){
                    for(int i=0; i<numberOfColumns; i++) {
                        stream << exportHeaders[i];
                        if (i < numberOfColumns - 1){
                            stream << ",";
                        }
                    }
                    stream << "\n";
                }
            }

            for(int i=ui->Observations->rowCount()-1; i>=0; i--) {
                QMap<int, QString> values;
                for (int j=0; j<numberOfColumns; j++) {
                    if (ui->Observations->item(i,j)->text().contains(','))
                        values[exportHeaders.indexOf(ui->Observations->horizontalHeaderItem(j)->text())]
                                = "\"" + ui->Observations->item(i,j)->text() + "\"";
                    else
                        values[exportHeaders.indexOf(ui->Observations->horizontalHeaderItem(j)->text())]
                                = ui->Observations->item(i,j)->text();
                }

                for (int j=0; j<numberOfColumns; j++) {
                    stream << values[j];
                    if (j < numberOfColumns - 1) {
                        stream << ",";
                    }
                }
                stream << "\n";
            }
            csvfile.close();
            scribeSettings->setValue("Observation/file", csvFileName);
        }else{
            QFile csvfile(csvFileName);
            if(csvfile.exists()){
                bool boolok = csvfile.remove();
                if(!boolok){
                    MessageDialog msgBox;
                    msgBox.setText("The system is having trouble removing\n unsaved observation file.");
                    msgBox.setFontSize(13);
                    msgBox.exec();
                    return;
                }
            }
            scribeSettings->setValue("Observation/file", "");
        }
        scribeSettings->sync();

        if(setfile.isOpen())
            setfile.close();

    }catch(QException err){
        qDebug() << err.what();
    }

}

