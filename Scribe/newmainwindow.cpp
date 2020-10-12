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

using namespace std;

static bool GPS_MAP_ENABLED = false;

newmainwindow::newmainwindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::newmainwindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));

    connection.clear();

    this->window()->setWindowTitle("Scribe");
    ui->Play->setCheckable(true);

    audio = new AudioPlayer(ui->Play, ui->wavFileList);
    //connect(audio->fileList, SIGNAL(currentIndexChanged(int)), this, SLOT(RecordingIndexChanged(int))); //it seems this code is not working
    connect(audio->wavFileList, SIGNAL(currentIndexChanged(int)), this, SLOT(RecordingIndexChanged(int)));
    audio->player->setVolume(ui->Volume->value());
    audio->player->setPlaylist(audio->fileList);
    connect(audio, &AudioPlayer::sendCurrentState, this, &newmainwindow::getCurrentStatusAudio);

    proj = new ProjectFile;
    //proj->setCurMainWindows(this);

    shortcutGroup = new QActionGroup(this);
    specieskeyMapper = new QSignalMapper(this);
    groupingskeyMapper = new QSignalMapper(this);

    projSettingsDialog = new ProjectSettingsDialog(this, proj);
    //fieldDialog = new AdditionalFieldsDialog(this);//remarked 5.6.2020
    exportDialog = new ExportDialog(this);
    OpeningScreenDialog *openingScreen = new OpeningScreenDialog(this, proj);
    openingScreen->show();

    //Hide Window
    //this->hide();

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

    //connect(projSettingsDialog, &ProjectSettingsDialog::refreshSignals, this, &newmainwindow::additionalConnect);

    projSettingsDialog->setGeometry(    QStyle::alignedRect(
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

        ui->HeaderFields->setRowCount(0); //Clear them all
        ui->HeaderFields->setRowCount(proj->getHeaderCount());
        ui->HeaderFields->setColumnCount(2);
        ui->HeaderFields->setColumnWidth(1, 184);

        proj->headerFields.clear();
        proj->headerIndex.clear();
        proj->countH = 0;

        proj->headerFieldsC.clear();
        proj->headerIndexC.clear();
        proj->countHC = 0;

        connection.clear();

        proj->allHeaderV.clear();

        ui->HeaderFields->setStyleSheet("font-size: 13px;"
                                        "font-family: 'Segoe UI'; border: 1px solid #D8D8D8; "
                                        "background-color:#F7F7F7; border-top:none;");//rgba(0, 0, 0, 0.2)

        WorkerThread2nd *workerThread = new WorkerThread2nd();
        workerThread->setProjectFile(proj);
        workerThread->setHasFlightData(hasFlightData);
        workerThread->setFlightData(existingFlightDataPath);
        workerThread->setAudioPlayer(audio);

        connect(workerThread, &WorkerThread2nd::LoadReady, this, [=]() {

            connect(ui->speciesTable, SIGNAL(cellClicked(int,int)), this, SLOT(setSpecies(int,int)));

            CreateLogItemShortcuts();
            BuildBookmarksTableColumns();

            ui->quantity->installEventFilter(this);
            ui->HeaderFields->installEventFilter(this);
            ui->Observations->installEventFilter(this);

            //startDir = QDir::homePath();
            //projSettingsDialog->startDir = startDir;

            AutoSave();

            autoSaveTimer = new QTimer(this);
            connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(AutoSave()));
            autoSaveTimer->start(20000);

            //for our problems with ` moving to the next audio file
            new QShortcut(QKeySequence(Qt::Key_QuoteLeft), this, SLOT(handleBackTick()));
            //Alt-R
            new QShortcut(QKeySequence(tr("Alt+r")), this, SLOT(repeatTrack()));
            new QShortcut(QKeySequence(Qt::ALT + Qt::Key_R), this, SLOT(repeatTrack()));
            //Alt-B
            new QShortcut(QKeySequence(tr("Alt+b")), this, SLOT(on_Back_clicked()));
            new QShortcut(QKeySequence(Qt::ALT + Qt::Key_B), this, SLOT(on_Back_clicked()));
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

        });

        connect(workerThread, &WorkerThread2nd::finished, workerThread, &QObject::deleteLater);
        //Add Item from QThread to MainWindow
        connect(workerThread, &WorkerThread2nd::sendHeaderFields, this, &newmainwindow::addHeaderItemM);
        //Disable Add Field
        connect(workerThread, &WorkerThread2nd::disbleAddField, projSettingsDialog, &ProjectSettingsDialog::disable_field_buttons);
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

    connect(openingScreen, SIGNAL(closeProgram()),this,SLOT(closeProgram()));
    //GPS Lock Visiblity
    connect(openingScreen, &OpeningScreenDialog::widgetStatus, this, [=](const bool &status)
    {
        ui->lockGPS->setVisible(status);
    });
    //Send a signal after update the value in distance configuration
    connect(proj, &ProjectFile::LoadedDistanceConfig, distanceConfig, &distanceconfig::distanceConfigOpened);
    //connect(distanceConfig, SIGNAL(saveNewValues(double)), this, SLOT(updateProjectFile(double)));
    //connect(distanceConfig, SIGNAL(saveNewValues(double, double)), this, SLOT(updateProjectFile(double, double)));
    connect(distanceConfig, &distanceconfig::showProgressMessage, this, [=](DistanceConfigObj *dc,
            const bool &WBPHS) {

        //distanceConfig->hide();

        progMessage = new progressmessage();
        progMessage->setText("Configuring distance...");
        //progMessage->setAttribute(Qt::WA_TranslucentBackground,  true);
        ///progMessage->show();

        WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
        workerThread->setDitanceConfig(dc);
        workerThread->setIsSurveyWBPHS(WBPHS);
        workerThread->setAction(3);
        workerThread->setProjectFile(proj);

        connect(workerThread, &WorkerThreadSurvey::finished, workerThread,
                &QObject::deleteLater);
        connect(workerThread,SIGNAL(updateProjectFile(double)), this, SLOT(updateProjectFile(double)));
        connect(workerThread, SIGNAL(updateProjectFile(double, double)), this, SLOT(updateProjectFile(double, double)));

        workerThread->start();

        progMessage->show();

    });

    //ui->quantity->installEventFilter(this);
    //ui->HeaderFields->installEventFilter(this);
    //ui->Observations->installEventFilter(this);

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

    //qDebug() << "MainWindow loaded without errors.";

    //added 4.29.2020
    //initializong the map (code comes from ImportFlightData)
    ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/gps/main_copy.qml")));
    ui->quickWidget->setResizeMode(ui->quickWidget->SizeRootObjectToView);
    ui->quickWidget->show();

    //Audio Button
    setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
    setupButton(QIcon(":/img/misc/Icon ionic-md-skip-backward.png"), ui->Back);
    setupButton(QIcon(":/img/misc/Icon ionic-md-skip-forward.png"), ui->Next);

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
    //qDebug() << "C++: " << plotName;
    if (plotName == m_plotName)
            return;

    proj->headerFields.at(proj->plotnameIndex)->setText(plotName);
    m_plotName = plotName;
    emit plotNameChanged();
}

void newmainwindow::setValuesGOEA(const QString &val1,const QString &val2)
{
    //added 5.22.2020
    proj->headerFields.at(proj->transectIndex)->setText(val2);
    auto colC = proj->headerFieldsC;
    //Disable bcrIndexChange slot if promatically apply the value.
    //disconnect(colC.at(proj->bcrIndex), SIGNAL(currentIndexChanged(int)),
      //      this, SLOT(bcrIndexChange(int)));
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
    globalSettings.set("TRNPRe", val2);
    globalSettings.set("BCRPRe", val1);
}

void newmainwindow::setValuesWBPHS(const QString &val1, const QString &val2, const QString &val3)
{
    /*qDebug() << "proj->stratumIndex: " << proj->stratumIndex;
    qDebug() << "proj->transectIndex: " << proj->transectIndex;
    qDebug() << "proj->segmentIndex: " << proj->segmentIndex;*/

    proj->headerFields.at(proj->stratumIndex)->setText(val1);
    proj->headerFields.at(proj->transectIndex)->setText(val2);
    proj->headerFields.at(proj->segmentIndex)->setText(val3);
}


void newmainwindow::setAGName(const QString &newAGN)
{
    //if(newAGN == m_aGName)
      //  return;

    proj->headerFields.at(proj->aGNameIndex)->setText(newAGN);
    //m_aGName = newAGN;
    //emit aGNAmeChanged();
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
    CreateLogItemShortcuts();

    //refreshHeaderAdditional();
    on_actionAdditional_Fields_triggered();
    PopulateHeaderFields();

    qDebug() << "Message";
    qDebug() << projSettingsDialog->message;

    if (projSettingsDialog->message) {
        projSettingsDialog->message = false;
        projSettingsDialog->progMessage->okToClose = true;
        projSettingsDialog->progMessage->close();
        projSettingsDialog->close();
    }
}

void newmainwindow::RefreshItems2()
{
    species = projSettingsDialog->speciesItems;
    groupings = projSettingsDialog->groupingItems;

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
    QString exportLocation = globalSettings.get("projectDirectory") + "/" + globalSettings.get("exportFilename") + ".asc";
    if (exportLocation.isEmpty()){
        //qDebug() << "No export directory specified";
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
        //qDebug() << "No export directory specified";
        return;
    }
    QFile exportFile(exportLocation);
    if (!exportFile.open(QIODevice::ReadWrite|QIODevice::Append)){
          //qDebug() << "Couldn't open file for writing";
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
            values[exportHeaders.indexOf(ui->Observations->horizontalHeaderItem(j)->text())] = ui->Observations->item(i,j)->text();
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

    QMessageBox mb;

    mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    mb.setWindowTitle("Export");
    mb.setText("Saved observations successfully!");
    mb.exec();

    ascFilename = exportLocation;

//    ClearBookmarks();
    ui->Observations->clear();
    ui->Observations->setRowCount(0);

    BuildBookmarksTableColumns();

    //qDebug() << "Successfully exported " << exportLocation;
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
    QPalette palette = *new QPalette();

    palette = push->palette();
    palette.setColor(QPalette::Button, color);
    push->setMinimumWidth(33);
    push->setMaximumHeight(33);
    push->setPalette(palette);
    push->setStyleSheet("text-align: left; font: Bold 13px 'Segoe UI'; qproperty-iconSize: 13px 13px;");
    push->setIcon(icon);
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

    if(globalSettings.get("autoUnit") == "Yes"){
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

        if(surveyType == "BAEA"){
            surveyJsonPolygonList = readGeoJson2(proj->geoJSON);
            //Get All Plot Name List
            proj->loadPlotNameList();
            QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyPolygonJson",
                Q_ARG(QVariant, QVariant::fromValue(surveyJsonPolygonList)));
        }else if(surveyType == "GOEA"){

            surveyJsonSegmentListGOEA = readGeoJson(proj->geoJSON);

            //BCR and Transect Validation
            proj->loadGOEAProperties();

            //loadBcrTrnList();//added 5.22.2020

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

    ui->wavFileList->addItems(names);
    try {
        if (!audio->fileList->isEmpty()){
            audio->player->play();
            setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
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
        QMessageBox alert;
        alert.setText("Project directory has not been configured.\nUnable to import flight data at this time.");
        alert.exec();
        //qDebug() << "Project directory not set. Aborting import.";
        return;
    }

    const QString &sourceFlightDataDirectoryPath = QFileDialog::getExistingDirectory(nullptr, "Select The Flight Directory", startDir);
    //ImportFlightData(sourceFlightDataDirectoryPath, true);

    WorkerThreadImportFlight *workerThread = new WorkerThreadImportFlight();
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

    //Load Finished
    connect(workerThread, &WorkerThreadImportFlight::loadReady, this, [=]() {

        progMessage->okToClose = true;
        progMessage->close();

    });

    workerThread->start();

    progMessage = new progressmessage();

    progMessage->setGeometry(
                            QStyle::alignedRect(
                                Qt::LeftToRight,
                                Qt::AlignCenter,
                                progMessage->size(),
                                qApp->screens().first()->geometry()
                            ));

    progMessage->setText("Importing flight data...");
    progMessage->show();
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
    //qDebug() << "Primary Auto Save function.";

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

       // qDebug()<< "Autosaved to " << autosaveFile;
   }
}

void newmainwindow::ProjectSettingsFinished()
{
    //if (proj->getAdditionalFieldsCount() > 0)
    on_actionAdditional_Fields_triggered();//unremarked 5.7.2020 to load the addnl field values upon saving in settings
    //loadAdditional(); //First time load
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

    QString direct = QFileDialog::getOpenFileName(this, "Select Project", globalSettings.get("projectDirectory"), "Scribe project file (*sproj)");
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

        loadSpeciesHotTable();

        connect(ui->speciesTable, SIGNAL(cellClicked(int,int)), this, SLOT(setSpecies(int,int)));

        CreateLogItemShortcuts();
        BuildBookmarksTableColumns();

        progMessage->okToClose = true;
        progMessage->close();
    });

    progMessage = new progressmessage();
    progMessage->setGeometry(
                            QStyle::alignedRect(
                                Qt::LeftToRight,
                                Qt::AlignCenter,
                                progMessage->size(),
                                qApp->screens().first()->geometry()
                            ));

    progMessage->setText("Importing survey template...");
    progMessage->show();

    workerThread->start();
    /*QDir directory(surveyFile);
    if (proj->Load(directory)){
       qDebug() << "load successful";
    }*/
    //fieldDialog->hide();////remarked 5.6.2020


    //on_actionAdditional_Fields_triggered();
    //PopulateHeaderFields();
}

void newmainwindow::additionalConnect()
{    
    connectionA.clear();
    auto colA = proj->additionalFields;
    //qDebug() << proj->additionalFields;
    const int &aCount = proj->countA;
    for (int t = 0; t < aCount; t++){
        const int &indexAdd = proj->additionalIndex[t];
        DocLayout *docLayoutA = new DocLayout;
        docLayoutA->setIndex(t);
        docLayoutA->setIndexTableW(indexAdd);

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
    //proj->buildAdditionalfieldsTable(ui->additionalfieldsTable);
    const int &rCount = proj->addionalFieldsNamesArray.count();

    proj->additionalFields.clear();
    proj->additionalIndex.clear();
    proj->countA = 0;

    ui->additionalfieldsTable->setRowCount(0); //Clearing it to rebuild
    ui->additionalfieldsTable->setRowCount(rCount);
    ui->additionalfieldsTable->setColumnCount(2);
    ui->additionalfieldsTable->setColumnWidth(1, 184);//edited from 180 6.5.2020

    ui->additionalfieldsTable->setStyleSheet( "font-size: 13px;"
                                              "font-family: 'Segoe UI';"
                                              "background-color: #F7F7F7;"
                                              "border-style:solid;"
                                              "border-width: 1px;"
                                              "border-color: #D8D8D8;"
                                              "border-top:none;");

    for (int y = 0; y < rCount; y++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);//make first column readonly //6.4.2020
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->additionalfieldsTable->setItem(y, 0, item);
        //ui->additionalfieldsTable->setRowHeight(y, 40);
        QFont cellFont;//added 5.25.2020
        cellFont.setFamily("Segoe UI"); //this will set the first column to font to bold
        cellFont.setPixelSize(13); //5.25.2020
        cellFont.setBold(true);// 5.25.2020
        ui->additionalfieldsTable->item(y,0)->setData(Qt::FontRole,QVariant(cellFont)); //added 5.25.2020
        ui->additionalfieldsTable->item(y,0)->setText(proj->getAdditionalFieldName(y) + ": ");
        //Rap - Should add also the value code as well?
        proj->buildAdditionalFieldCell(ui->additionalfieldsTable, y);
    }

    additionalDisconnect();
    additionalConnect();
    /*
    auto colA = proj->additionalFields;
    const int &aCount = proj->countA;
    for (int t = 0; t < aCount; t++){
        const int &indexAdd = proj->additionalIndex[t];
        DocLayout *docLayoutA = new DocLayout;
        docLayoutA->setIndex(t);
        docLayoutA->setIndexTableW(indexAdd);

        //Getting QSize
        connect(colA.at(t)->document()->documentLayout(),
                    &QAbstractTextDocumentLayout::documentSizeChanged,
                    docLayoutA,
                    &DocLayout::resultA);

        //Supply QSize and fit based on content while editing
        connect(docLayoutA,
                &DocLayout::sendSignalA,
                this,
                &newmainwindow::commentDocRect);

    }*/
}

void newmainwindow::refreshHeaderAdditional()
{
    clearHeaderFields();
    clearAdditionalFields();

    WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
    workerThread->setProjectFile(proj);
    workerThread->setAction(2);

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
    connect(workerThread, &WorkerThreadSurvey::refreshDone, this, [=]() {

        //projSettingsDialog->progMessage->okToClose = true;
        //projSettingsDialog->progMessage->close();
        //projSettingsDialog->close();
    });

    workerThread->start();
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
    qDebug() << proj->headerFields.at(index)->maximumHeight();
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

void newmainwindow::focusOutStratum()
{
    const int &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt();
    if (newStratum != 0 &&
        //newStratum != previousWBPS[0] &&
        !proj->getWBPS()->checkStratum(new WBPSObj(newStratum, -1, -1))) {
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Warning! The Stratum is not valid.");
        mb.setStandardButtons(QMessageBox::StandardButton::Ok);
        mb.exec();
        proj->headerFields.at(proj->stratumIndex)->setFocus();
        return;
    }
}

void newmainwindow::focusOutTransect()
{
    const QString &surveyType = proj->getSurveyType();
    if(surveyType == "WBPHS"){
        const int &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt();
        const int &newTransect = proj->headerFields.at(proj->transectIndex)->toPlainText().toInt();
        if (newTransect != 0 &&
            //newTransect != previousWBPS[1] &&
            !proj->getWBPS()->checkTransect(new WBPSObj(newStratum, newTransect, -1))) {
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! The combinations of Stratum and Transect are not valid.");
            mb.setStandardButtons(QMessageBox::StandardButton::Ok);
            mb.exec();
            proj->headerFields.at(proj->transectIndex)->setFocus();
            return;
        }
    }
    else if(surveyType == "GOEA"){
        validateGoeaValue();
    }
}

void newmainwindow::focusOutSegment()
{
    const int &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt();
    const int &newTransect = proj->headerFields.at(proj->transectIndex)->toPlainText().toInt();
    const int &newSegment = proj->headerFields.at(proj->segmentIndex)->toPlainText().toInt();
    if (newSegment != 0 &&
        //newSegment != previousWBPS[2] &&
        !proj->getWBPS()->checkSegment(new WBPSObj(newStratum, newTransect, newSegment))) {
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Warning! The combinations of Stratum, Transect and Segment are not valid.");
        mb.setStandardButtons(QMessageBox::StandardButton::Ok);
        mb.exec();
        proj->headerFields.at(proj->segmentIndex)->setFocus();
        return;
    }
}


//Get Previous Value
void newmainwindow::focusInTransect()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    globalSettings.set("TRNPRe",  proj->headerFields.at(proj->transectIndex)->toPlainText());
    //globalSettings.set("BCRPRe",  proj->headerFieldsC.at(proj->bcrIndex)->currentText());
}

void newmainwindow::focusInBCR()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    //globalSettings.set("TRNPRe",  proj->headerFields.at(proj->transectIndex)->toPlainText());
    globalSettings.set("BCRPRe",  proj->headerFieldsC.at(proj->bcrIndex)->currentText());
}

//Get previous value for checking if it is edited
/*void newmainwindow::collectPreValues()
{
    previousWBPS.clear();
    const int &newStratum = proj->headerFields.at(proj->stratumIndex)->toPlainText().toInt();
    const int &newTransect = proj->headerFields.at(proj->transectIndex)->toPlainText().toInt();
    const int &newSegment = proj->headerFields.at(proj->segmentIndex)->toPlainText().toInt();
    previousWBPS.append(newStratum);
    previousWBPS.append(newTransect);
    previousWBPS.append(newSegment);
}*/

/*
void newmainwindow::focusInStratum()
{
    collectPreValues();
}

void newmainwindow::focusInTransect()
{
    collectPreValues();
}

void newmainwindow::focusInSegment()
{
    collectPreValues();
}

void newmainwindow::focusInPlotName()
{
    const QString &prePlotN = proj->headerFields.at(proj->plotnameIndex)->toPlainText();
    previousPlotName = prePlotN;
}*/

void newmainwindow::focusOutPlotName()
{
    const QString &plotName = proj->headerFields.at(proj->plotnameIndex)->toPlainText();
    /*if (previousPlotName.toLower().trimmed() !=
            plotName.toLower().trimmed() &&
            !plotName.isEmpty())*/
    if (!plotName.isEmpty()){

        const QStringList &r = proj->getAllPlotNameList();
        if (!r.contains(plotName, Qt::CaseInsensitive)) {
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! The Plot Name is not valid.");
            mb.setStandardButtons(QMessageBox::StandardButton::Ok);
            mb.exec();
            proj->headerFields.at(proj->plotnameIndex)->setFocus();
            return;
        }

    }
}

/*
void newmainwindow::focusInAGName()
{
    const QString &preAGName =  proj->headerFields.at(proj->aGNameIndex)->toPlainText();
    previousAGName = preAGName;
}*/

void newmainwindow::focusOutAGName()
{
    const QString &agName = proj->headerFields.at(proj->aGNameIndex)->toPlainText();
   /* if (previousAGName.toLower().trimmed() !=
           agName.toLower().trimmed() &&
            !agName.isEmpty())*/
    if (!agName.isEmpty()){
        const QStringList &r = proj->getAllAGNameList();
        if (!r.contains(agName, Qt::CaseInsensitive)) {
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! The A_G Name is not valid.");
            mb.setStandardButtons(QMessageBox::StandardButton::Ok);
            mb.exec();
            proj->headerFields.at(proj->aGNameIndex)->setFocus();
            return;
        }

    }

}

void newmainwindow::focusOutBCR()
{
    if (proj->getBCRPre() != proj->headerFieldsC.at(proj->bcrIndex)->currentText()) {
        validationBCR();
    }
}


void newmainwindow::adjustSpeciesHotTable()
{

    QString surveyValues = ",";

    ui->HeaderFields->setRowCount(0); //Clear them all
    ui->HeaderFields->setRowCount(proj->getHeaderCount());
    ui->HeaderFields->setColumnCount(2);
    //ui->HeaderFields->setColumnWidth(0, 70);
    ui->HeaderFields->setColumnWidth(1, 180);

    proj->headerFields.clear();
    proj->headerIndex.clear();
    proj->countH = 0;

    proj->headerFieldsC.clear();
    proj->headerIndexC.clear();
    proj->countHC = 0;

    connection.clear();

    proj->allHeaderV.clear();

    for (int y = 0; y < proj->getHeaderCount(); y++) {
        QTableWidgetItem *item = new QTableWidgetItem();//make first column readonly
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->HeaderFields->setItem(y, 0, item);
        ui->HeaderFields->setStyleSheet("font-size: 13px;"
        "font-family: 'Segoe UI'; border: 1px solid rgba(0, 0, 0, 0.2);");
        QFont cellFont;//added 5.25.2020
        cellFont.setFamily("Segoe UI"); //this will set the first column to font to bold
        cellFont.setPixelSize(13); //5.25.2020
        cellFont.setBold(true);// 5.25.2020
        ui->HeaderFields->item(y,0)->setData(Qt::FontRole,QVariant(cellFont)); //added 5.25.2020
        ui->HeaderFields->item(y,0)->setText(proj->getHeaderName(y) + ": ");
        //Rap - Should add also the value code as well?
        proj->buildHeaderCell(ui->HeaderFields, y);
        //ui->HeaderFields->horizontalHeader()->setStretchLastSection(true);
    }

    //Connect Slot for Header
    connectAllHeader();

    //GlobalSettings &globalSettings = GlobalSettings::getInstance();
    const QString &surveyType = proj->getSurveyType();
    if(surveyType == "GOEA" || surveyType == "BAEA"){
        ui->comboBox_3->hide();
        //ui->lockGPS->show();
    }else{
        ui->comboBox_3->show();
        //ui->lockGPS->hide();
    }

    //ui->surveyTypeLabel->setText(surveyType);
    //ui->projectDirectory->setText(globalSettings.get("projectDirectory"));
    //ui->exportFilename->setText(globalSettings.get("exportFilename") + ".asc");

     //qDebug() << "species count: " << species.length();
     //qDebug() << proj->addedSpecies[0];


    QRegExp rx("(\\;)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'

    //const int &mL = (proj->addedSpecies.at(proj->maxSpecies)).length() + 70;
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
            item->setBackgroundColor(QColor(238, 238, 238));
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
    ui->HeaderFields->setColumnWidth(1, 184);
}

void newmainwindow::clearAdditionalFields()
{
    proj->additionalFields.clear();
    proj->additionalIndex.clear();
    proj->countA = 0;

    ui->additionalfieldsTable->setRowCount(0); //Clearing it to rebuild
    ui->additionalfieldsTable->setRowCount(proj->addionalFieldsNamesArray.count());
    ui->additionalfieldsTable->setColumnCount(2);
    ui->additionalfieldsTable->setColumnWidth(1, 184);//edited from 180 6.5.2020
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
            item->setBackgroundColor(QColor(238, 238, 238));

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
            ui->speciesTable->item(rowNum,colNum)->setText( query[0] + "  " + query[1].toLower());
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

void newmainwindow::PopulateHeaderFields()
{
    QString surveyValues = ",";

    ui->HeaderFields->setRowCount(0); //Clear them all
    ui->HeaderFields->setRowCount(proj->getHeaderCount());
    ui->HeaderFields->setColumnCount(2);
    //ui->HeaderFields->setColumnWidth(0, 70);
    ui->HeaderFields->setColumnWidth(1, 184);//6.5.2020 edited from 180

    proj->headerFields.clear();
    proj->headerIndex.clear();
    proj->countH = 0;

    proj->headerFieldsC.clear();
    proj->headerIndexC.clear();
    //tbl->setCellWidget(idx,1,myComboBox2);
    proj->countHC = 0;

    connection.clear();

    proj->allHeaderV.clear();

    ui->HeaderFields->setStyleSheet("font-size: 13px;"
                                    "font-family: 'Segoe UI'; border: 1px solid #D8D8D8; "
                                    "background-color:#F7F7F7; border-top:none;");//rgba(0, 0, 0, 0.2)

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
        //ui->HeaderFields->horizontalHeader()->setStretchLastSection(true);
    }

    //Connect Slot for Header
    connectAllHeader();

    //GlobalSettings &globalSettings = GlobalSettings::getInstance();
    const QString &surveyType = proj->getSurveyType();
    if(surveyType == "GOEA" || surveyType == "BAEA"){
        ui->comboBox_3->hide();
        //ui->lockGPS->show();
    }else{
        ui->comboBox_3->show();
        //ui->lockGPS->hide();
    }

    //ui->surveyTypeLabel->setText(surveyType);
    //ui->projectDirectory->setText(globalSettings.get("projectDirectory"));
    //ui->exportFilename->setText(globalSettings.get("exportFilename") + ".asc");

     //qDebug() << "species count: " << species.length();
     //qDebug() << proj->addedSpecies[0];

    QRegExp rx("(\\;)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'

    //const int &mL = (proj->addedSpecies.at(proj->maxSpecies)).length() + 70;
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
    //ui->speciesTable->setAlternatingRowColor(true);
   // ui->speciesTable->setStyleSheet("alternate-background-color: #E9E9E9; background-color: #FFFFFF;");
    ui->speciesTable->setStyleSheet(QString::fromUtf8("QTableWidget{\n"
                                                      //"	background-color: rgb(255, 255, 255);\n"
                                                      //"alternate-background-color: #EEEEEE \n"
                                                      "background-color: #FFFFFF; \n"
                                                      "	border-style:solid;\n"
                                                      "	border-width:1px;\n"
                                                      //"	border-color: rgb(221, 221, 221);\n"
                                                      "border-color: #DFDFDF; \n"
                                                      "}\n"
                                                      "QTableWidget::item{\n"
                                                      //"	padding: 1px;\n"
                                                      " height: 18px;\n"
                                                      //" max-width: 100px;\n"
                                                      //" min-width: 80px;"
                                                      "}"));//added 4.29.2020


    //ui->speciesTable->sizePolicy().setVerticalStretch(1);
    for( int i=0; i<totalLoops; i++){
        QTableWidgetItem *item = new QTableWidgetItem();
        if (colNum % 2 != 0) {
            item->setBackgroundColor(QColor(238, 238, 238));
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
     }

    connect(ui->speciesTable, SIGNAL(cellClicked(int,int)), this, SLOT(setSpecies(int,int)));


    CreateLogItemShortcuts();
    BuildBookmarksTableColumns();
    //ui->HeaderFields->resizeColumnsToContents();
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

    //CHECK FOR SIMILLAR USER KEYS, COME BACK TO THIS
    QStringList userDefinedKeys;

    //qDebug() << "Creating Shortcuts";

    bool conflictingKeys = false;

    for (int i = 0; i  < species.length(); i ++ ){
        QAction *shortcutAction = new QAction(this);
        shortcutGroup->addAction(shortcutAction);
        this->addAction(shortcutAction);

        QString keyVal = species[i]->shortcutKey.toString();
        if(keyVal.length() > 0){
            if(usedHotkeys.indexOf(keyVal,0) != -1){
                conflictingKeys = true;
                errorMessage += "\r\n - '" + keyVal + "'";
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
        if(keyVal.length() > 0){
            if(usedHotkeys.indexOf(keyVal,0) != -1){
                conflictingKeys = true;
                errorMessage += "\r\n - '" + keyVal + "'";
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
                }
            }
        }
    }

    if (conflictingKeys){
        projSettingsDialog->activeButton->click();
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText(errorMessage);
        mb.exec();
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
                if (keyEvent->key() == shortcutGroup->actions()[i]->shortcut().toString() || keyEvent->key() == Qt::Key_Return) {
                    if (target == ui->quantity && ((keyEvent->key()==Qt::Key_Enter) || (keyEvent->key()==Qt::Key_Return)) ) {
                        //Enter or return was pressed while in the quantity box
                        ui->Observations->selectionModel()->clearSelection();
                        on_insertButton_clicked();
                    }else{
                        //qDebug() << "Ignoring hot key: " << (char)keyEvent->key() << "for" << target;
                    }

                    if(target != ui->HeaderFields && target != ui->Observations){
                        //qDebug() << "Target is not a header field.";
                        event->ignore();
                        return true;
                    }
                }
            }
        }
    }
    if((target == ui->HeaderFields || target == ui->Observations) && event->type() == QEvent::FocusIn){
        //Disconnect Hot Keys
        //qDebug() << "Disconnecting all hot keys.";
        disconnect (specieskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillSpeciesField(int))) ;
        disconnect (groupingskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillGroupingField(int))) ;
        qDeleteAll(shortcutGroup->actions());
        shortcutGroup->setExclusive(false);
    }
    if((target == ui->HeaderFields || target == ui->Observations) && event->type() == QEvent::FocusOut){
        //Re-connect Hot Keys
        //qDebug() << "Re-connecting all hot keys.";
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
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    if( globalSettings.get("surveyType") == "BAEA" ||  globalSettings.get("surveyType") == "GOEA" ){
        allColumnNames << required_columns_eagle;
    }else{
        allColumnNames << required_columns_1;
    }
    allColumnNames << proj->getHeaderNamesList();
    allColumnNames << required_columns_2;
    allColumnNames << proj->getAdditionalFieldsNamesList();

    ui->Observations->setColumnCount(allColumnNames.length());

    for (int i=0; i<allColumnNames.length(); i++) {
        QTableWidgetItem *item = new QTableWidgetItem(allColumnNames[i]);
        item->setTextAlignment(Qt::AlignLeft);
        QFont titleFont;
        titleFont.setFamily("Segoe UI");
        titleFont.setPixelSize(13);
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
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    if( globalSettings.get("surveyType") == "BAEA" ||  globalSettings.get("surveyType") == "GOEA" ){
        const QStringList &otherColumns = {"Audio File", "Latitude", "Longitude", "Altitude", "Course", "Speed", "Time", "# Satellites", "HDOP", "Species", "Quantity"};
        exportHeaders << otherColumns;
    }else{
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
            s->setStyleSheet(css);
            s->setTitle(headerKey);
            s->setMinimumHeight(30);

            //Check fields which are readonly.
            if(proj->getInstanceHeaderPosition(headerKey) != -1) {
                s->setReadOnly(true);
            }

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
            break;
        }
    }
}

void newmainwindow::sendRecords(GPSRecording *records, const QString &matchLine)
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
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
    //GPSRecording *record = new GPSRecording(QStringList(matchLine), wavFilePath);
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
    ui->Play->setChecked(true);

    ui->wavFileList->addItems(names);
    try {
        if (!audio->fileList->isEmpty()){
            audio->player->play();
            setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
        }
    } catch (...) {
        qDebug() << "Invalid to Play";
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

    QTextEdit *s = new QTextEdit();
    s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    s->setStyleSheet(css);
    s->setMinimumHeight(30);
    s->setText(vals[0].toString());

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

    QComboBox* myComboBox2 = new QComboBox();
    myComboBox2->setStyleSheet(css);
    myComboBox2->setMinimumHeight(30);
    myComboBox2->setEditable(false);
    myComboBox2->addItem("");
    for (int i = 0; i < vals.count(); i ++){
        myComboBox2->addItem(vals[i].toString());
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

    QTextEdit *s = new QTextEdit();
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
    s->setText(val);

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
    s->setTitle(headerKey);
    s->setMinimumHeight(30);

    const int &ind = proj->allHeaderV.indexOf(headerKey);
    const QString &val = vals[0].toString();
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
        myComboBox2->addItem(vals[i].toString());
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
    qDebug() << exportLocation << ": " << exportFile;
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
    //if(auto pw = ui->speciesTable->parentWidget()){
      //  ui->speciesTable->resize(pw->size());
   // }
}

void newmainwindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    //refreshMap();//remarked 5.27.2020

    //Refresh Map
    //if (curlocked)
      //  selectWavFileMap(indexLock);
    //else
      //  selectWavFileMap(ui->wavFileList->currentIndex());

    adjustSpeciesHotTable();
}

void newmainwindow::closeEvent(QCloseEvent *event)
{
    //Disable Event
    disconnectAllHeader();
    if (proj->changesMade || ui->Observations->rowCount() > 0){
        QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Scribe",
                                                                    tr("Are you sure you want to exit?\n"),
                                                                    QMessageBox::Cancel | QMessageBox::Close,
                                                                    QMessageBox::Cancel);
        if (resBtn == QMessageBox::Cancel)
        {
            event->ignore();
            //Enable Event
            connectAllHeader();
        }else{
            event->accept();
        }
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
        //nm = OutputFolder + "/" + globalSettings.get("surveyType") + ".surv";
        //QDir dir(nm);
        //proj->Save(dir,false);
        //QMessageBox msgBox;

        WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
        workerThread->setProjectFile(proj);
        workerThread->setDir(OutputFolder);
        workerThread->setAction(1);

        connect(workerThread, &WorkerThreadSurvey::createDone, this, [=]() {

            progMessage->okToClose = true;
            progMessage->close();

            MessageDialog msgBox;
            msgBox.setText("Survey template created successfully.");
            msgBox.setFontSize(13);
            msgBox.exec();

        });

        connect(workerThread, &WorkerThreadSurvey::finished,
                workerThread, &QObject::deleteLater);

        workerThread->start();

        progMessage = new progressmessage();
        progMessage->setGeometry(
                                QStyle::alignedRect(
                                    Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    progMessage->size(),
                                    qApp->screens().first()->geometry()
                                ));

        progMessage->setText("Creating survey template...");
        progMessage->show();

    }
}



void newmainwindow::on_actionSave_triggered()
{
    AutoSave();
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

    projSettingsDialog->PrepHeadFieldValues();
    //projSettingsDialog->PrepAdditionalFieldValues(fieldDialog->table);//remarked 5.6.2020
    //projSettingsDialog->PrepAdditionalFieldValues(ui->additionalfieldsTable);//added 5.6.2020
    projSettingsDialog->sourceWidget = ui->additionalfieldsTable;
    projSettingsDialog->PrepAdditionalFieldValues();
    projSettingsDialog->show();
    projSettingsDialog->setFocus();
}


void newmainwindow::on_actionAbout_Scribe_triggered()
{
    /*aboutScribe = new aboutscribe();
    aboutScribe->show();*///remarked 5.29.2020
    aboutDialog = new AboutDialog();//added 5.29.2020
    aboutDialog->show();
}

void newmainwindow::on_actionUser_Guide_triggered()
{
    userGuide = new userguide();
    userGuide->show();
}

/*
void newmainwindow::on_HeaderFields_cellChanged(int row, int column)
{
    //added 5.13.2020 //set the rowid for statum transect and segment
    if(ui->HeaderFields->item(row, 0)->text().toUpper() == "STRATUM")
         stratumrowid = row;

    if(ui->HeaderFields->item(row, 0)->text().toUpper() == "TRANSECT")
         transectrowid = row;

    if(ui->HeaderFields->item(row, 0)->text().toUpper() == "SEGMENT")
         segmentrowidx = row;


    //orig beyond this point
    if(column < 1 || !proj->isHeaderSingleValue(row)){
        return;
    }
    QTableWidgetItem *item = ui->HeaderFields->item(row, column);
    QString s = item->text();
    s = s.remove(QRegularExpression("[^\\\a-zA-Z0-9\\@\\#\\$\\%\\&\\*\\/\\<\\>\\.\\?\\!\\+\\-\\_\\(\\)]"));
    item->setText(s);

    //added 5.13.2020
    //qDebug() << stratumrowid << ": " << transectrowid << ": " << segmentrowidx;
    if(row == stratumrowid){ // if the stratum value is changed
        if(transectrowid > -1)
            ui->HeaderFields->item(transectrowid, column)->setText("");//set the transect value to empty
        if(segmentrowidx > -1)
            ui->HeaderFields->item(segmentrowidx, column)->setText("");//set the transect value to empty
    }else if(row == transectrowid){
        if(segmentrowidx > -1)
            ui->HeaderFields->item(segmentrowidx, column)->setText("");//set the transect value to empty
    }

}
*/
/*
QStringList newmainwindow::getAllPlotNameList() const
{
    return m_plotNameList;
}*/

/*void newmainwindow::loadBcrTrnList()
{
    QJsonObject jObj = proj->geoJSON;
    QJsonObject::const_iterator i;
    for (i = jObj.begin();  i != jObj.end(); ++i) {
        if(i.key().toLower() == "features"){
            QJsonArray jsonArray = i->toArray();
            foreach (const QJsonValue &value, jsonArray) {
                QJsonObject innerObject = value.toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); j++) {
                    if (j.key().toLower() == "properties"){
                        //qDebug() << j->toObject();
                        QJsonObject innerObject2 = j->toObject();
                        m_bcrTrnList <<  j->toObject().value("BCRTrn").toString();

                    }

                }
            }
        }
    }
    m_bcrTrnList.sort();
}*/

/*QStringList newmainwindow::getAllBcrTrnList() const
{
    return m_bcrTrnList;
}*/

void newmainwindow::validateGoeaValue()
{
    if (proj->getTRNPre() != proj->headerFields.at(proj->transectIndex)->toPlainText()) {
        /*if (proj->getBCRPre() == proj->headerFieldsC.at(proj->bcrIndex)->currentText()) {
            const int &reply = QMessageBox::information(this, tr("Scribe"),
                tr("Do you want to change the BCR?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
            if (reply == QMessageBox::Yes) {
                //proj->headerFieldsC.at(proj->bcrIndex)->setFocus();
                //proj->headerFieldsC.at(proj->transectIndex)->setText("");
            }
            else {
                validationTransectGOEA();
                //GlobalSettings &globalSettings = GlobalSettings::getInstance();
                //globalSettings.set("TRNPRe",  proj->headerFields.at(proj->transectIndex)->toPlainText());
                //globalSettings.set("BCRPRe",  proj->headerFields.at(proj->transectIndex)->toPlainText());
            }
        }
        else {
            validationTransectGOEA();
        }*/
        validationTransectGOEA();
    }
}

void newmainwindow::validationTransectGOEA()
{
    const QString &trnName = proj->headerFields.at(proj->transectIndex)->toPlainText().trimmed();
    //if(trnName == "")
      //  return;

    auto colC = proj->headerFieldsC;
    QComboBox *combo1 = colC[proj->bcrIndex];
    const QString &bcrName = combo1->currentText();

    //if(bcrName == "")
      //  return;

    const QString &brctrnName = bcrName + "." + trnName;

   // if (!brctrnName.isEmpty()){
    const QStringList &r = proj->getBCRTRN();
    if (!r.contains(brctrnName, Qt::CaseInsensitive)) {
        if (proj->getBCRPre() == proj->headerFieldsC.at(proj->bcrIndex)->currentText()) {
            const int &reply = QMessageBox::information(this, tr("Scribe"),
                tr("Do you want to change the BCR?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
            if (reply == QMessageBox::Yes) {
                //proj->headerFieldsC.at(proj->bcrIndex)->setFocus();
                //proj->headerFieldsC.at(proj->transectIndex)->setText("");
            }
            else {
                QMessageBox mb;
                mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                mb.setWindowTitle("Error");
                mb.setText("Warning! The combinations of Transect and BCR are not valid.");
                mb.setStandardButtons(QMessageBox::StandardButton::Ok);
                mb.exec();
                proj->headerFields.at(proj->transectIndex)->setFocus();
                return;
            }
        }
        else {
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! The combinations of Transect and BCR are not valid.");
            mb.setStandardButtons(QMessageBox::StandardButton::Ok);
            mb.exec();
            //proj->headerFieldsC.at(proj->bcrIndex)->setFocus();
            proj->headerFields.at(proj->transectIndex)->setFocus();
            //proj->headerFields.at(proj->transectIndex)->setText("");
            return;
        }
    }
    //}
}

/*
void newmainwindow::loadAirGroundProperties()
{
    QJsonObject jObj = proj->airGround;
    QJsonObject::const_iterator i;
    m_agName.clear();
    for (i = jObj.begin();  i != jObj.end(); ++i) {
        if(i.key().toLower() == "features"){
            QJsonArray jsonArray = i->toArray();
            foreach (const QJsonValue &value, jsonArray) {
                QJsonObject innerObject = value.toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); j++) {
                    if (j.key().toLower() == "properties"){
                        m_agName << j->toObject().value("agcode").toString();
                        //m_plotNameList <<  j->toObject().value("PlotLabel").toString();
                    }

                }
            }
        }
    }
}*/

/*
void newmainwindow::loadWBPSProperties()
{
    QJsonObject jObj = proj->geoJSON;
    QJsonObject::const_iterator i;
    m_wbps = new WBPS();
    for (i = jObj.begin();  i != jObj.end(); ++i) {
        if(i.key().toLower() == "features"){
            QJsonArray jsonArray = i->toArray();
            foreach (const QJsonValue &value, jsonArray) {
                QJsonObject innerObject = value.toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); j++) {
                    if (j.key().toLower() == "properties"){
                        //qDebug() << j->toObject();
                        //QJsonObject innerObject2 = j->toObject();
                        //m_plotNameList <<  j->toObject().value("PlotLabel").toString();
                        const int &newStratum = j->toObject().value("stratum").toInt();
                        const int &newTransect = j->toObject().value("transect").toInt();
                        const int &newSegment = j->toObject().value("segment").toInt();

                        m_wbps->append(new WBPSObj(newStratum, newTransect, newSegment));
                    }

                }
            }
        }
    }
}

WBPS *newmainwindow::getWBPS()
{
    return m_wbps;
}*/

/*
QStringList newmainwindow::getAllAGNameList() const
{
    //Make sure plot name is valid
    if (proj->getSurveyType() == "BAEA") {
        bool isValid = false;
        const QString &plotName = proj->headerFields.at(proj->plotnameIndex)->toPlainText();
        const QStringList &r = getAllPlotNameList();
        //for (int i = 0; i < r.length(); i++) {
        if (r.contains(plotName, Qt::CaseInsensitive)) {
            isValid = true;
        }
        else {
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! The Plot Name is not valid.");
            mb.setStandardButtons(QMessageBox::StandardButton::Ok);
            mb.exec();

        }
    }

    return m_agName;
}*/



void newmainwindow::on_insertButton_clicked()
{
    QString surveyType = proj->getSurveyType();
    if(surveyType == "BAEA"){
        if(returnedValueBAEA.length() > 0 && returnedValueBAEA.contains(",") == true){
            //qDebug() << (returnedValueBAEA);
            QStringList splt = returnedValueBAEA.split(",");
            if(splt[1] == "0"){
                QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Scribe"),
                    tr("This observation is beyond the \n"
                       "maximum distance to the survey boundary. \n"
                       "Do you want to proceed?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
                if (reply == QMessageBox::Yes) {

                }else if (reply == QMessageBox::No) {
                    return;
                }
                QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                    Q_ARG(QVariant,"white"));
            }
        }
    }else if(surveyType == "WBPHS"){
        if(returnedValueWPBHS.length() > 0 && returnedValueWPBHS.contains(",") == true){
            QStringList splt = returnedValueWPBHS.split(",");
            if(splt[5] == "0"){
                QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Scribe"),
                    tr("This observation is beyond the \n"
                       "maximum distance to the survey boundary. \n"
                       "Do you want to proceed?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
                if (reply == QMessageBox::Yes) {

                }else if (reply == QMessageBox::No) {
                    return;
                }
                QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                    Q_ARG(QVariant,"white"));
            }
        }
    }else if(surveyType == "GOEA"){
        //validateGoeaValue();

        if(returnedValueGOEA.length() > 0 && returnedValueGOEA.contains(",") == true){
            QStringList splt = returnedValueGOEA.split(",");
            if(splt[2] == "0"){
                QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Scribe"),
                    tr("This observation is beyond the \n"
                       "maximum distance to the survey boundary. \n"
                       "Do you want to proceed?"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
                if (reply == QMessageBox::Yes) {

                }else if (reply == QMessageBox::No) {
                    return;
                }
                QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                    Q_ARG(QVariant,"white"));
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
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Error! There are no recordings!!");
        mb.exec();
        return;
    }

    // Make sure we have species
    if (species.length()  < 1){
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Error! There are no species defined!");
        mb.exec();
        return;
    }

    //qDebug() <<  ui->comboBox_3->currentText() << " and " <<  ui->quantity->text()  << " right!";
    if( ui->comboBox_3->currentText() == "FLKDRAKE" && ( ui->quantity->text() == "1" ||  ui->quantity->text() == "" ||  ui->quantity->text() == "Quantity" )){
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Are you crazy? You can't have Flock Drakes in quantities of 1");
        mb.setStandardButtons(QMessageBox::StandardButton::Ok);
        mb.exec();
        return;
    }

    // Make sure we have groupings
    if (!currentSpecies->groupingEnabled){
        if (ui->comboBox_3->currentText() != "Open"){
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! Social groupings were not enabled for this species!");
            mb.setStandardButtons(QMessageBox::StandardButton::Ok);
            mb.exec();
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

    //qDebug() << latLongCoordinates;

    QStringList coords = latLongCoordinates.split(",");
    book->species = currentSpecies->itemName;
    book->grouping = ui->comboBox_3->currentText();
    book->latitude = coords[0];
    book->longitude = coords[1];
    book->audioFileName = ui->wavFileList->currentText();
    book->timestamp = gpsEntry.timestamp;
    book->altitude = book->altitude.setNum(gpsEntry.altitude);
    book->speed = book->speed.setNum(gpsEntry.speed);
    book->course = gpsEntry.course;
    book->numSatellites = QString::number(gpsEntry.number_of_satellites);
    book->hdop = book->hdop.setNum(gpsEntry.hdop);


    // Add quantity to Bookmark
    if (quant.isEmpty() || quant == "0" || quant.toInt() == 0){
        if(surveyType == "WBPHS"){
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
    ui->Observations->insertRow( insertAt );
    int currentRow = insertAt;

    QFont cellFont;//added 5.7.2020 unnecessary code
    cellFont.setFamily("Segoe UI"); //this will set the first column to font to bold
    cellFont.setPixelSize(13);//added5.7.2020
    //cellFont.setBold(true);//added5.7.2020
    //tbl->setStyleSheet("text-align: left; font-size: 13px;"
    //"font-family: 'Segoe UI'; border: 1px solid rgba(0, 0, 0, 0.2);");
    //tbl->item(t,0)->setData(Qt::FontRole,QVariant(cellFont)); /


    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Species"), new QTableWidgetItem(book->species));
    ui->Observations->item(currentRow, allColumnNames.indexOf("Species"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *quantityItem = new QTableWidgetItem();
    quantityItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    quantityItem->setText(quantValue);

    //ui->Observations->setItem(currentRow, allColumnNames.indexOf("Quantity"), new QTableWidgetItem(quantValue));
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Quantity"), quantityItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Quantity"))->setData(Qt::FontRole,QVariant(cellFont));

    //hide for eagle surveys (or better yet, hide if no groupings enabled)

    if(surveyType != "BAEA" && surveyType != "GOEA" ){
        ui->Observations->setItem(currentRow, allColumnNames.indexOf("Grouping"), new QTableWidgetItem(book->grouping));
        ui->Observations->item(currentRow, allColumnNames.indexOf("Grouping"))->setData(Qt::FontRole,QVariant(cellFont));
    }

    QTableWidgetItem *latitudeItem = new QTableWidgetItem(book->latitude);
    latitudeItem->setFlags(latitudeItem->flags() ^ Qt::ItemIsEditable);
    latitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Latitude"), latitudeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Latitude"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *longitudeItem = new QTableWidgetItem(book->longitude);
    longitudeItem->setFlags(longitudeItem->flags() ^ Qt::ItemIsEditable);
    longitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Longitude"), longitudeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Longitude"))->setData(Qt::FontRole,QVariant(cellFont));
    //ui->Observations->setItem(currentRow, allColumnNames.indexOf("Longitude"), longitudeItem);

    QTableWidgetItem *timeItem = new QTableWidgetItem(book->timestamp);
    timeItem->setFlags(longitudeItem->flags() ^ Qt::ItemIsEditable);
    timeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Time"), timeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Time"))->setData(Qt::FontRole,QVariant(cellFont));
    //timeItem->setFlags(timeItem->flags() ^ Qt::ItemIsEditable);
    //ui->Observations->setItem(currentRow, allColumnNames.indexOf("Time"), timeItem);

    QTableWidgetItem *altitudeItem = new QTableWidgetItem(book->altitude);
    altitudeItem->setFlags(altitudeItem->flags() ^ Qt::ItemIsEditable);
    altitudeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Altitude"), altitudeItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Altitude"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *speedItem = new QTableWidgetItem(book->speed);
    speedItem->setFlags(speedItem->flags() ^ Qt::ItemIsEditable);
    speedItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Speed"), speedItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Speed"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *courseItem = new QTableWidgetItem(book->course);
    courseItem->setFlags(courseItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Course"), courseItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("Course"))->setData(Qt::FontRole,QVariant(cellFont));

    QTableWidgetItem *HDOPItem = new QTableWidgetItem(book->hdop);
    HDOPItem->setFlags(HDOPItem->flags() ^ Qt::ItemIsEditable);
    HDOPItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("HDOP"), HDOPItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("HDOP"))->setData(Qt::FontRole,QVariant(cellFont));


    QTableWidgetItem *satItem = new QTableWidgetItem(book->numSatellites);
    satItem->setFlags(satItem->flags() ^ Qt::ItemIsEditable);
    satItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("# Satellites"), satItem);
    ui->Observations->item(currentRow, allColumnNames.indexOf("# Satellites"))->setData(Qt::FontRole,QVariant(cellFont));


    QTableWidgetItem *audioItem = new QTableWidgetItem(book->audioFileName);
    audioItem->setFlags(audioItem->flags() ^ Qt::ItemIsEditable);
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
            //Get The value
            QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(ui->HeaderFields->cellWidget(i, 1));
            textEditItem->setText("");
        }
        else if(proj->hAFieldsCb.contains(headerKey)) {
            //Get The value
            QComboBox *comboBoxItem = dynamic_cast<QComboBox*>(ui->HeaderFields->cellWidget(i, 1));
            comboBoxItem->setCurrentIndex(0);
        }

        QTableWidgetItem *headerValue = new QTableWidgetItem(val);
        ui->Observations->setItem(currentRow, columnToInsert, headerValue);
        ui->Observations->item(currentRow, columnToInsert)->setData(Qt::FontRole,QVariant(cellFont));
    }

    // Insert additional fields into Observations Table
    for (int i=0; i<proj->getAdditionalFieldsCount(); i++) {
        QString val;
        QString additionalFieldKey = proj->getAdditionalFieldName(i);
        int columnToInsert = allColumnNames.indexOf(additionalFieldKey);

        val = proj->getAdditionalFieldValue(ui->additionalfieldsTable, i);

        //Check exist in All Fields that unchecked
        if (proj->additionalFieldsChk.contains(additionalFieldKey)) {
            //Get The value
            QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(ui->additionalfieldsTable->cellWidget(i, 1));
            textEditItem->setText("");
            //Reset the value
            //ui->additionalfieldsTable->setCellWidget(i, 1, textEditItem);
        }
        else if(proj->aFieldsCb.contains(additionalFieldKey)) {
            //Get The value
            QComboBox *comboBoxItem = dynamic_cast<QComboBox*>(ui->additionalfieldsTable->cellWidget(i, 1));
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
    //unlock gps
    //LockedGPSIndex = -1;
    //ui->lockGPS->setCheckState(Qt::Unchecked);
    //ui->Observations->resizeColumnsToContents();

    //added 4.24.2020 start //clear the ui->HeaderFields value to null
    //if the settings retain value is not checked
  /*  for (int y = 0; y < proj->getHeaderCount(); y++){
        //QString headername = proj->getHeaderName(y);
        //QStringList headervalue = proj->getHeaderValueList(y);
        QString headercheck = proj->getHeaderValueChk(y);
        if(headercheck.toInt() == 0){
            ui->HeaderFields->item(y,1)->setText("");//set the value to null
        }

        QStringList headercheck = proj->getHeaderValueChk2(y);

        //qDebug() << "headercheck.length: "<< headercheck.length();
        if(headercheck.length() == 1){
            if(headercheck[0].toInt() == 0)
                ui->HeaderFields->item(y,1)->setText("");

                //QWidget *w = ui->HeaderFields->cellWidget(y,1);
                //qDebug() << w->metaObject()->className();
                if(QString::fromUtf8(w->metaObject()->className()) == "QLineEdit"){
                     QLineEdit *l  = new QLineEdit();
                     l->setText("");
                     ui->additionalfieldsTable->setCellWidget(y,1, l);
                    qDebug() << "b1:";
                }else if(QString::fromUtf8(w->metaObject()->className()) == "QComboBox"){
                     QComboBox *cb = qobject_cast<QComboBox*>(w);
                     cb->setCurrentIndex(0);
                    qDebug() << "c1:";
                }else if(QString::fromUtf8(w->metaObject()->className()) == "QPlainTextEdit"){
                     //QPlainTextEdit *pte = qobject_cast<QPlainTextEdit*>(w);
                     QPlainTextEdit  *s  = new QPlainTextEdit ();
                     ui->additionalfieldsTable->setCellWidget(y,1, s);
                    qDebug() << "d1:";
                }
        }else{
            //for array

        }
        auto chkfield = ui->addedHeaderFields->cellWidget(y, 2);
        QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
        if(chkbox->isChecked()){
            intchk = 2;
        }


    }

    //check the proj->getAdditionalFieldsCount()
    for (int y = 0; y < proj->getAdditionalFieldsCount(); y++){
        QStringList addnlvalschk = proj->getAdditionalFieldChk2(y);
        if(addnlvalschk.length() == 1){
            if(addnlvalschk[0].toInt() == 0){
                 QWidget *w = ui->additionalfieldsTable->cellWidget(y,1);
                 //qDebug() << QString::fromUtf8(w->metaObject()->className());//cast const char to QString
                 if(QString::fromUtf8(w->metaObject()->className()) == "QLineEdit"){
                     QLineEdit *l  = new QLineEdit();
                     l->setText("");
                     ui->additionalfieldsTable->setCellWidget(y,1, l);
                 }else if(QString::fromUtf8(w->metaObject()->className()) == "QComboBox"){
                     QComboBox *cb = qobject_cast<QComboBox*>(w);
                     cb->setCurrentIndex(0);
                 }else if(QString::fromUtf8(w->metaObject()->className()) == "QPlainTextEdit"){
                     //QPlainTextEdit *pte = qobject_cast<QPlainTextEdit*>(w);
                     QPlainTextEdit  *s  = new QPlainTextEdit ();
                     ui->additionalfieldsTable->setCellWidget(y,1, s);
                 }
                 else if(QString::fromUtf8(w->metaObject()->className()) == "QTextEdit"){
                      QTextEdit  *s  = new QTextEdit();
                      ui->additionalfieldsTable->setCellWidget(y,1, s);
                  }
            }
        }
    }//added 5.6.2020 end
    //Clean up after insert button
   */
}


void newmainwindow::on_Play_toggled(bool checked)
{
    Q_UNUSED(checked)
    //qDebug() << checked;
    insertWasLastEvent = false;
    if (audio->player->position() == 0){
        int currIndex = ui->wavFileList->currentIndex();
        audio->fileList->setCurrentIndex(currIndex);
    }

   /*if(ui->Play->text() == ">"){
       audio->player.play();
   }else{
       audio->player.pause();
   }setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);*/
    qDebug() << audio->status;
    if (audio->status == 2) {
        audio->player->play();
        //audio->status = 1;
        //Change Icon
        //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
        setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
    }
    else if(audio->status == 0) {
        audio->player->play();
        //audio->status = 1;
        //Change Icon
        //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
        setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
    }
    else {
        audio->player->pause();
        //audio->status = 0;
        setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
    }
}

void newmainwindow::on_Back_clicked()
{
    qDebug() << "on back click";
    insertWasLastEvent = false;
    int currIndex = ui->wavFileList->currentIndex();
    if (currIndex > 0){
     audio->fileList->setCurrentIndex(currIndex - 1);
    }else{
        audio->fileList->setCurrentIndex(audio->fileList->mediaCount() - 1);
    }
    audio->player->play();
    //Change Icon
    setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
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
        setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
    }
}

void newmainwindow::repeatTrack()
{
    qDebug() << "repeat";
    insertWasLastEvent = false;
    audio->player->setPosition(0);
    //ui->Play->setText(">");
    //audio->isPlay = true;
    audio->status = 1;
    //Change Icon
    setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
    //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
    on_Play_toggled(false);
}

void newmainwindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index)
    //refreshMap();//remarked 5.27.2020
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
    const int &currIndex = ui->wavFileList->currentIndex();
    if(currIndex == audio->fileList->mediaCount()-1){
        return;
    }
    if(!insertWasLastEvent){
        this->on_insertButton_clicked();
    }
    this->on_Next_clicked();
}

void newmainwindow::on_comboBox_2_currentTextChanged(const QString &arg1)
{
    for(int i = 0; i < species.length(); i++){
        if (species[i]->itemName == arg1){
            currentSpecies = species[i];
        }
    }
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
    QString s = itm->text();
    if(s.length() < 1){
      return;
    }
    QStringList species_code = s.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    int index = ui->comboBox_2->findText( species_code[0] );
    if ( index != -1 ) { // -1 for not found
       ui->comboBox_2->setCurrentIndex(index);
    }
}

void newmainwindow::selectWavFileMap(const int &index)
{

    audio->fileList->setCurrentIndex(index);
    audio->wavFileList->setItemData( index, QColor("red"), Qt::BackgroundColorRole);

    //added 4.27.2020
    QString wavfile = ui->wavFileList->itemText(index);
    QVariant returnedValue = "";//added 5.17.2020
    if(index >= 0){//updated from = to >= to select the first wave file from the list and zoom to it
        QMetaObject::invokeMethod(gps->wayHandler, "selectWavFile",
            Q_RETURN_ARG(QVariant, returnedValue),
            Q_ARG(QVariant, wavfile),
            Q_ARG(QVariant, proj->getAirGroundMaxD()),
            Q_ARG(QVariant, proj->getObsMaxD()));
        QString surveyType = proj->getSurveyType();
        if(surveyType == "BAEA"){
            setPlotName("");

            if(returnedValue == "")
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
        }
        else if (surveyType == "WBPHS") {
            //Check Air Ground
            QStringList splt = returnedValue.toString().split(",");
            const QString &agnm = splt[1];
            const int &isWithinMaxDist = splt[0].toInt();
            if(isWithinMaxDist == 1){
                setAGName(agnm);
            }else {
                setAGName("N/A");
            }

            //for the stratum transect and segment
            if(returnedValue != ""){
                returnedValueWPBHS = returnedValue.toString();
                const QString &stratum = splt[2];
                const QString &transect = splt[3];
                const QString &segment = splt[4];
                setValuesWBPHS(stratum,transect,segment);

                const int &isWithinMaxDist2 = splt[5].toInt();
                //qDebug() << "returnedValue.toString(): " << returnedValue.toString();
                //qDebug() << "isWithinMaxDist2: " << isWithinMaxDist2;
                if(isWithinMaxDist2 == 0){//over max distance
                        QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                            Q_ARG(QVariant,"yellow"));
                }else{
                    QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                        Q_ARG(QVariant,"white"));
                }
            }
        }
        else if(surveyType == "GOEA"){
            if(returnedValue == "")
                return;
            else{
                returnedValueGOEA = returnedValue.toString();
                QStringList splt = returnedValue.toString().split(",");
                QString bcr = splt[0];
                QString trn = splt[1];
                setValuesGOEA(bcr,trn);

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
    }

}

void newmainwindow::connectAllHeader()
{
    //For Text Edit
    auto col = proj->headerFields;
    const int &hCount = proj->countH;
    for (int t = 0; t < hCount; t++){
        DocLayout *docLayoutH = new DocLayout();
        docLayoutH->setIndex(t);
        docLayoutH->setIndexTableW(proj->headerIndex[t]);
        const int &indexAdd = proj->headerIndex[t];

        //Get custom textEdit index
        if (proj->getHeaderName(indexAdd) == "Stratum") {
            proj->stratumIndex = t;

            connection << connect(col.at(t), &QTextEdit::textChanged,
                    this, &newmainwindow::stratumChanged);

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutStratum);

            //Focus in
            //connect(col.at(t), &CustomEventWidget::focusIn,
              //      this, &newmainwindow::focusInStratum);

        }
        else if(proj->getHeaderName(indexAdd) == "Transect") {
            proj->transectIndex = t;

            if (proj->getSurveyType() == "WBPHS") {
                connection << connect(col.at(t), &QTextEdit::textChanged,
                    this, &newmainwindow::trasectChanged);
            }

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutTransect);

            if (proj->getSurveyType() == "GOEA") {
                //Focus in
                connection << connect(col.at(t), &CustomEventWidget::focusIn,
                    this, &newmainwindow::focusInTransect);
            }
        }
        else if(proj->getHeaderName(indexAdd) == "Segment"){
            proj->segmentIndex = t;

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutSegment);

            //Focus in
           // connect(col.at(t), &CustomEventWidget::focusIn,
             //       this, &newmainwindow::focusInSegment);

        }else if (proj->getHeaderName(indexAdd) == "PlotName") {//added 5.18.2020
            proj->plotnameIndex = t;

            connection << connect(col.at(t), &QTextEdit::textChanged,
                    this, &newmainwindow::plotnameChanged);

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutPlotName);

            //Focus in
            //connect(col.at(t), &CustomEventWidget::focusIn,
              //      this, &newmainwindow::focusInPlotName);

        }else if (proj->getHeaderName(indexAdd) == "A_G Name") {
            proj->aGNameIndex = t;

            //Event for focus out
            connection << connect(col.at(t), &CustomEventWidget::focusOut,
                    this, &newmainwindow::focusOutAGName);

            //Focus in
            //connect(col.at(t), &CustomEventWidget::focusIn,
              //      this, &newmainwindow::focusInAGName);

        }


        //Getting size for resizing for if text edit value is full
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

    //For ComboBox
    auto colC = proj->headerFieldsC;
    const int &hCountC = proj->countHC;
    for(int h = 0; h < hCountC; h++) {
         DocLayout *docLayoutH = new DocLayout();
         docLayoutH->setIndex(h);
         docLayoutH->setIndexTableW(proj->headerIndexC[h]);

         const int &indexAdd = proj->headerIndexC[h];

         if (proj->getHeaderName(indexAdd) == "BCR") {
            proj->bcrIndex = h;
            connection << connect(colC.at(h), &CustomComboBox::focusIn,
                    this, &newmainwindow::focusInBCR);

            connection << connect(colC.at(h), &CustomComboBox::focusOut,
                    this, &newmainwindow::focusOutBCR);

            //added 5.22.2020 //Event for selection change
           // connection << connect(colC.at(h), SIGNAL(currentIndexChanged(int)),
             //       this, SLOT(bcrIndexChange(int)));


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

    //on_Play_toggled(false);

    /*if (audio->status == 2) {
        audio->player.play();
        //Change Icon
        //setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
        setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
    }
    else if(audio->status == 0) {
        audio->player.pause();
        //Change Icon
        setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
        //setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
    }
    else {
        audio->player.stop();
        setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
    }*/

    //else
      //  ui->lockGPS->setChecked(true);
    //Check if wavFile is lock
    /*if (wavfile == wavFileLock)
        ui->lockGPS->setChecked(true);
    else
        ui->lockGPS->setChecked(false);*/

}

QList<QStringList> newmainwindow::readGeoJson(QJsonObject jObj)
{
    QList<QStringList> jsonSegmentList;
    QStringList lineSegment;
    QJsonObject::iterator i;
    for (i = jObj.begin(); i != jObj.end(); ++i) {
        if(i.key().toUpper() == "FEATURES" ){
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
                            if(key.toUpper() == "PROPERTIES" ){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    templist << value2.toVariant().toString(); //QString::number(value2.toDouble());
                                }
                            }else if(key.toUpper() == "GEOMETRY"){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    if(key2.toUpper() == "COORDINATES"){
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

QList<QStringList> newmainwindow::readGeoJsonAir(QJsonObject jObj)
{
    QList<QStringList> jsonSegmentList;
    QStringList lineSegment;
    QJsonObject::iterator i;
    for (i = jObj.begin(); i != jObj.end(); ++i) {
        if(i.key().toUpper() == "FEATURES" ){
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
                            if(key.toUpper() == "PROPERTIES" ){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    templist << value2.toVariant().toString(); //QString::number(value2.toDouble());
                                }
                            }
                            else if(key.toUpper() == "GEOMETRY"){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    if(key2.toUpper() == "COORDINATES"){
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
          Q_ARG(QVariant, ui->wavFileList->currentText()));

        //If there is previous selected lock
        /*if(indexLock >= 0){

            //Remove previous color
            audio->wavFileList->setItemData(indexLock, QColor(255 ,255 ,255), Qt::BackgroundColorRole);

            //Remove lock
            QMetaObject::invokeMethod(gps->wayHandler, "removeLockGPS",
                Q_ARG(QVariant, wavFileLock));


            //New
            indexLock = ui->wavFileList->currentIndex();
            wavFileLock = ui->wavFileList->currentText();
            audio->wavFileList->setItemData(indexLock, QColor("blue"), Qt::BackgroundColorRole);

            //Rename
            //ui->lockGPS->setText(wavFileLock);

        }
        else {

            //New
            indexLock = ui->wavFileList->currentIndex();
            wavFileLock = ui->wavFileList->currentText();
            audio->wavFileList->setItemData(ui->wavFileList->currentIndex(), QColor("blue"), Qt::BackgroundColorRole);
            //Rename
            //ui->lockGPS->setText(wavFileLock);
        }*/

        indexLock = ui->wavFileList->currentIndex();
        wavFileLock = ui->wavFileList->currentText();
        curlocked = true;

        //Locked
        //QMetaObject::invokeMethod(gps->wayHandler, "changeMarkerToLockGPS",
          // Q_ARG(QVariant, wavFileLock));

    }else{

        if (ui->wavFileList->count() > 0) {
            //Remove lock
            QVariant returnedValue;//added 5.17.2020
            QMetaObject::invokeMethod(gps->wayHandler, "selectWavFile",
                Q_RETURN_ARG(QVariant, returnedValue),//added 5.17.2020
                Q_ARG(QVariant, wavFileLock),
                Q_ARG(QVariant, proj->getAirGroundMaxD()),
                Q_ARG(QVariant, proj->getObsMaxD()));
            QString surveyType = proj->getSurveyType();
            if(surveyType == "BAEA"){
                //the returnedValue.toString() format is (BAEA - plotname,1 or 0);
            }

            //Select Current wav file if unchecked the GPS Locked
            selectWavFileMap(ui->wavFileList->currentIndex());

        }

        curlocked = false;
        wavFileLock = "";
        indexLock = -1;

        //audio->wavFileList->setItemData(ui->wavFileList->currentIndex(), QColor(255 ,255 ,255), Qt::BackgroundColorRole);

        //Rename to Original
        //ui->lockGPS->setText("Lock GPS");
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
        if(i.key().toUpper() == "FEATURES" ){
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
                                if(j.key().toUpper() == "GEOMETRY" ){
                                    //qDebug() << '\t' << "GEOMETRY: " << j.key();
                                    QJsonObject innerObject2 = j.value().toObject();
                                    QJsonObject::iterator k;
                                    for (k = innerObject2.begin(); k != innerObject2.end(); ++k) {
                                        if (k.value().isObject()) {
                                            qDebug() << '\t' << "OBJECT" << k.key();
                                            // ...
                                        }else if(k.value().isArray()){
                                            //QJsonObject innerObject3 = k.value().toObject();
                                            QJsonArray jsonArray2 = k.value().toArray();
                                            foreach (const QJsonValue & value2, jsonArray2) {
                                                //QJsonObject obj2 = value2.toObject();
                                                if(k.key().toUpper() == "COORDINATES"){
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
                                }else if(j.key().toUpper() == "PROPERTIES" ){
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
        if(surveyType == "BAEA"){
            //set the maximum observe distance in wayHandler //added 5.15.2020
            QVariant returnedval;
            QMetaObject::invokeMethod(gps->wayHandler, "setMaxDist",
                Q_RETURN_ARG(QVariant,returnedval),
                Q_ARG(QVariant, newObsMaxD),
                Q_ARG(QVariant, "update"),
                Q_ARG(QVariant, "0"));

            //qDebug() << "returnedval2: " << returnedval.toString();
            setPlotName("");

            if(returnedval == ""){

            }else{
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
        }else if(surveyType == "GOEA"){
            //set the maximum observe distance in wayHandler //added 5.15.2020
            QVariant returnedval;
            QMetaObject::invokeMethod(gps->wayHandler, "setMaxDist",
                Q_RETURN_ARG(QVariant,returnedval),
                Q_ARG(QVariant, newObsMaxD),
                Q_ARG(QVariant, "update"),
                Q_ARG(QVariant, proj->getAirGroundMaxD()));

            if(returnedval == ""){

            }else{
                returnedValueGOEA = returnedval.toString();
                QStringList splt = returnedval.toString().split(",");
                QString bcr = splt[0];
                QString trn = splt[1];
                const int &isWithinMaxDist = splt[2].toInt();
                setValuesGOEA(bcr,trn);
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
    }

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

        QStringList splt = returnedval.toString().split(",");
        const QString &agnm = splt[1];
        const int &isWithinMaxDist = splt[0].toInt();
        if(isWithinMaxDist == 1){
            setAGName(agnm);
        }else {
            setAGName("N/A");
        }

        //for the stratum transect and segment
        returnedValueWPBHS = returnedval.toString();
        const QString &stratum = splt[2];
        const QString &transect = splt[3];
        const QString &segment = splt[4];
        const int &isWithinMaxDist2 = splt[5].toInt();

        setValuesWBPHS(stratum,transect,segment);
        if(isWithinMaxDist2 == 0){//over max distance
            //popup messagebox yes no messagebox yellow
            QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                Q_ARG(QVariant,"yellow"));
        }else{
            QMetaObject::invokeMethod(gps->wayHandler, "changeWavFileProperties",
                Q_ARG(QVariant,"white"));
        }
    }

    progMessage->okToClose = true;
    progMessage->close();
    distanceConfig->close();
}

void newmainwindow::plotnameChanged()
{
    //validate value from the list
    //proj->headerFields.at(proj->plotnameIndex)->setText("");
    QString sval = proj->headerFields.at(proj->plotnameIndex)->toPlainText().trimmed();
    //qDebug() << "sval: " << sval;
    bool valexist = false;
    if(sval == "")
        return;

    for(int i = 0; i < surveyJsonPolygonList.length(); i++){
        QStringList slist = surveyJsonPolygonList[i];
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

/*void newmainwindow::bcrIndexChange(int)
{

}*/

void newmainwindow::validationBCR()
{
    const QString &trnName = proj->headerFields.at(proj->transectIndex)->toPlainText().trimmed();
    auto colC = proj->headerFieldsC;
    CustomComboBox *combo1 = colC[proj->bcrIndex];
    const QString &bcrName = combo1->currentText();

    //if(trnName.length() > 0){
    const QString &brctrnName = bcrName + "." + trnName;
      //  if (!brctrnName.isEmpty()){
    const QStringList &r = proj->getBCRTRN();
    if (!r.contains(brctrnName, Qt::CaseInsensitive)) {
        if (proj->getTRNPre() == proj->headerFields.at(proj->transectIndex)->toPlainText()) {
            const int &reply = QMessageBox::information(this, tr("Scribe"),
                tr("Do you want to change the Transect?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
            if (reply == QMessageBox::Yes) {
                proj->headerFields.at(proj->transectIndex)->setFocus();
                proj->headerFields.at(proj->transectIndex)->setText("");
            }
            else {
                QMessageBox mb;
                mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
                mb.setWindowTitle("Error");
                mb.setText("Warning! The combinations of transect and BCR are not valid.");
                mb.setStandardButtons(QMessageBox::StandardButton::Ok);
                mb.exec();
                //proj->headerFieldsC.at(proj->bcrIndex)->setFocus();
                return;
            }
        }
        //proj->headerFields.at(proj->transectIndex)->setFocus();
    }
      // }
    //}
}



void newmainwindow::on_actionProject_Information_triggered()
{
    projectInformation = new ProjectInformation(this);
    projectInformation->show();
}

void newmainwindow::getCurrentStatusAudio(const int &status)
{
    if (status == 2){
        setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
    }
    else if (status == 1){
       setupButton(QIcon(":/img/misc/Icon ionic-md-play.png"), ui->Play);
    }
    else {
        setupButton(QIcon(":/img/misc/Icon ionic-md-pause.png"), ui->Play);
    }
}

void newmainwindow::on_export_2_clicked()
{
    AppendASC();
    ui->Observations->setRowCount(0);
}
