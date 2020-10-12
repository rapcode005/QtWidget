#include "mainwindow2.h"
#include "ui_mainwindow2.h"
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
#include "openingscreen.h"
#include "globalsettings.h"
#include <io.h>
#include <QScreen>

using namespace std;

static bool GPS_MAP_ENABLED = false;

MainWindow2::MainWindow2(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow2)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));

    this->window()->setWindowTitle("Scribe");
    ui->Play->setCheckable(true);

    audio = new AudioPlayer(ui->Play, ui->wavFileList);
    connect(audio->fileList, SIGNAL(currentIndexChanged(int)), this, SLOT(RecordingIndexChanged(int)));
    audio->player.setVolume(ui->Volume->value());
    audio->player.setPlaylist(audio->fileList);

    proj = new ProjectFile;

    shortcutGroup = new QActionGroup(this);
    specieskeyMapper = new QSignalMapper(this);
    groupingskeyMapper = new QSignalMapper(this);

    projSettingsDialog = new ProjectSettingsDialog(this, proj);
    fieldDialog = new AdditionalFieldsDialog(this);
    exportDialog = new ExportDialog(this);
    openingScreen = new OpeningScreenDialog(this, proj);
    openingScreen->show();

    openingScreen->setGeometry(
                               QStyle::alignedRect(
                                   Qt::LeftToRight,
                                   Qt::AlignCenter,
                                   openingScreen->size(),
                                   qApp->screens().first()->geometry()
                                    //qApp->desktop()->availableGeometry()
                               ));


    connect(projSettingsDialog, SIGNAL(receivedLogDialogItems()),this,SLOT(RefreshItems()));

    /* The removeLog function was deleted because it wasn't being used. Jon 7/24/19
    if ( GPS_MAP_ENABLED ){
      connect (gps->wayHandler, SIGNAL(removeLog(QVariant)),
               this, SLOT(removeLog(QVariant)));
    }
    */



    projSettingsDialog->setGeometry(    QStyle::alignedRect(
                                   Qt::LeftToRight,
                                   Qt::AlignCenter,
                                   projSettingsDialog->size(),
                                   qApp->screens().first()->geometry()
                                            //qApp->desktop()->availableGeometry()
                               ));


    connect(exportDialog,SIGNAL(Append()),this,SLOT(AppendASC()));
    connect(proj,SIGNAL(Loaded()),projSettingsDialog,SLOT(ProjectOpened()));
    connect(projSettingsDialog, SIGNAL(finished(int)),this, SLOT(ProjectSettingsFinished()));
    //This is called when Opening Screen is done. BuildBookmarksTableColumns is called at the end.
    connect(openingScreen, SIGNAL(PopulateHeaderFields()),this,SLOT(PopulateHeaderFields()));
    connect(openingScreen, SIGNAL(CreateExportFile()),this, SLOT(CreateExportFile()));
    connect(openingScreen, SIGNAL(ImportFlightData(QString, bool )),this,SLOT(ImportFlightData(QString, bool)));
    connect(openingScreen, SIGNAL(closeProgram()),this,SLOT(closeProgram()));

    ui->quantity->installEventFilter(this);
    ui->HeaderFields->installEventFilter(this);
    ui->Observations->installEventFilter(this);

    startDir = QDir::homePath();
    projSettingsDialog->startDir = startDir;

    autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(AutoSave()));
    autoSaveTimer->start(20000);

    //for our problems with ` moving to the next audio file
    new QShortcut(QKeySequence(Qt::Key_QuoteLeft), this, SLOT(handleBackTick()));
    //Alt-R
    new QShortcut(QKeySequence(tr("Alt+r")), this, SLOT(repeatTrack()));
    //Alt-B
    new QShortcut(QKeySequence(tr("Alt+b")), this, SLOT(on_Back_clicked()));

    ui->insertButton->setToolTip(QString("Insert an observation above the selected observation"));
    ui->deleteButton->setToolTip(QString("Delete the selected observation"));
    ui->exportButton->setToolTip(QString("Save/Export values to ASC file"));

    //Unlock GPS
    on_lockGPS_stateChanged(0);

    //qDebug() << "MainWindow loaded without errors.";

    //added 4.29.2020
    //initializong the map (code comes from ImportFlightData)
    ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/gps/main_copy.qml")));
    ui->quickWidget->setResizeMode(ui->quickWidget->SizeRootObjectToView);
    ui->quickWidget->show();

}

MainWindow2::~MainWindow2()
{
    delete ui;
}

bool MainWindow2::eventFilter(QObject *target, QEvent *event)
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

void MainWindow2::refreshMap()
{
    if (auto pw = ui->quickWidget->parentWidget()){
        ui->quickWidget->resize(pw->size());
    }
    if(auto pw = ui->speciesTable->parentWidget()){
        ui->speciesTable->resize(pw->size());
        /*ui->speciesTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui->speciesTable->verticalHeader()->setDefaultSectionSize(8);*/
    }
}

//added 4.29.2020 make map and species widget reponsive
void MainWindow2::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    refreshMap();
}


/**
 * @brief MainWindow2::closeEvent
 * @param event
 * Event handler for closing the main window. Warns user and asks for confirmation.
 */
void MainWindow2::closeEvent (QCloseEvent *event)
{
    if (proj->changesMade || ui->Observations->rowCount() > 0){
        QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Scribe",
                                                                    tr("Are you sure you want to exit?\n"),
                                                                    QMessageBox::Cancel | QMessageBox::Close,
                                                                    QMessageBox::Cancel);
        if (resBtn == QMessageBox::Cancel)
        {
            event->ignore();
        }else{
            event->accept();
        }
    }else{
        event->accept();
    }
}

void MainWindow2::closeProgram(){
    this->close();
}



/**
 * @brief MainWindow2::AutoSave
 * Autosave functionality adds new autosave files and removes old ones.
 */
void MainWindow2::AutoSave()
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

/**
 * @brief MainWindow2::on_actionImport_Flight_Data_triggered
 * Handler for the event triggered by importing flight data.
 * Adds GPS and WAV files.
 */
void MainWindow2::on_actionImport_Flight_Data_triggered()
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

    QString sourceFlightDataDirectoryPath = QFileDialog::getExistingDirectory(nullptr, "Select The Flight Directory", startDir);
    ImportFlightData(sourceFlightDataDirectoryPath, true);

}


/**
 * @brief MainWindow2::renameFile
 * @param data
 * @return
 * Rename the GPS and WAV Files
 */
QString MainWindow2::renameFile(const QString &fileInfo, const QString &directoryInfo)
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

//Move the contents of this function to project------------------------------------------------------------
void MainWindow2::ImportFlightData(QString sourceFlightDataDirectoryPath, bool copyfiles){
    //qDebug() << "Importing flightdata";

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
            //QString targetFilePath = targetFlightDataDirectoryPath + "/" + observer + "_" + sourceFileInfo.fileName();
            QString targetFilePath = targetFlightDataDirectoryPath + "/" + observer + "_" +
                   renameFile(sourceFileInfo.fileName(), sourceDirectory.dirName());
            //qDebug() << "Copying " << sourceFilePath << " to " << targetFilePath;

            if (!QFile::copy(sourceFilePath, targetFilePath)) {
                //qDebug() << "Unable to copy " << sourceFile << " to " << targetFilePath;
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
                //qDebug() << "query[0] " + timePieces[0].right(2);
                //qDebug() << "query[1] " + timePieces[1];
                //qDebug() << "query[2] " + timePieces[2];
                gpsTimes.append("1" + timePieces[0] + timePieces[1] + timePieces[2]);
                //qDebug() << currentFlightLine;
                //return;
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
            globalSettings.set( "Year",  column[2] );
            globalSettings.set( "Month", column[3] );
            globalSettings.set( "Day",   column[4] );

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

    /*qDebug() << "wavFileItems" << wavFileItems.length();//check the list
    for(int i=0; i< wavFileItems.length(); i++){
        qDebug() << "wavFileItems" << i << ": " << wavFileItems[i];
    }
    //return;*/

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

        //Then I assume we're going to try to draw the transects
        //That just creates a rectangle from the geo json file.  It appears there are lots of areas.  Thus lots
        //Of transects.

        //Then are we supposed to have a button to show/hide transects?
//        QString send = "true";
//        if (arg1 == 0){
//            send = "false";
//        }
//        QVariant cur = send;
//        QMetaObject::invokeMethod(gps->wayHandler, "showTransects",
//            Q_ARG(QVariant, cur));

        //added 5.25.2020 populateWavList
        int intval = 0;
        if(ui->lockGPS->checkState() == Qt::Checked)
            intval = 1;
        else if(ui->lockGPS->checkState() == Qt::Unchecked)
            intval = 0;

        QMetaObject::invokeMethod(gps->wayHandler, "plotWavFile",
            Q_ARG(QVariant, wavFileItems),Q_ARG(QVariant, intval));

        //add survey json 5.4.2020
        readSurveyJson();
    }

    names.sort();
    ui->Play->setChecked(true);

    ui->wavFileList->addItems(names);

    if (!audio->fileList->isEmpty()){
        audio->player.play();
    }

    //Unlock GPS
    on_lockGPS_stateChanged(0);

    //qDebug() << "Flight data imported without errors.";
}

/**
 * @brief MainWindow2::ClearBookmarks
 * Clears all bookmarks
 */
//void MainWindow2::ClearBookmarks(){
//    QMetaObject::invokeMethod(gps->wayHandler, "clearAllBookmarks");
//}

/**
 * @brief MainWindow2::RecordingIndexChanged
 * @param position
 * Handler for updating a change in GPS position.
 */
void MainWindow2::RecordingIndexChanged(int position){
    if(position == -1){
        return;
    }
    currentGpsRecording = position;
    curCoord = 0;
    QVariant start = recordings[currentGpsRecording]->coords[0];



    //qDebug() << "RecordingIndexChanged()" << position;

   // if ( GPS_MAP_ENABLED ){
     //   QMetaObject::invokeMethod(gps->wayHandler, "updatePilotCoordinates",
       // Q_ARG(QVariant, start));
    //}

}

void MainWindow2::CreateExportFile()
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

/**
 * @brief MainWindow2::AppendASC
 * Appends bookmarks to existing text file
 */
void MainWindow2::AppendASC(){

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

void MainWindow2::RefreshItems( ){
    //qDebug() << "Refreshing Items";
    species = projSettingsDialog->speciesItems;
    groupings = projSettingsDialog->groupingItems;

    FillSpeciesTemplates(species);
    FillGroupingTemplates(groupings);
    CreateLogItemShortcuts();
    //some cheekiness here
    fieldDialog->hide();
    on_actionAdditional_Fields_triggered();
}

/**
 * @brief MainWindow2::FillSpeciesTemplates
 * @param items
 * Populates species dropdown.
 */
void MainWindow2::FillSpeciesTemplates(QList<LogTemplateItem*> items){
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
void MainWindow2::FillGroupingTemplates(QList<LogTemplateItem*> items){
//--    ui->comboBox_3->clear();
    QStringList g;
    for (int i = 0; i  < groupings.length(); i ++ ){
        g << items[i]->itemName;
    }
  //  g.sort();
    ui->comboBox_3->addItems(g);
}

void MainWindow2::FillSpeciesField(int item){
    insertWasLastEvent = false;
    //qDebug() << "Filled a Species field";
    currentSpecies = species[item];
    ui->comboBox_2->setCurrentText(currentSpecies->itemName);
}

void MainWindow2::FillGroupingField(int item){
    //qDebug() << "Filled a grouping field";
    ui->comboBox_3->setCurrentText(groupings[item]->itemName);
    ui->Observations->selectionModel()->clearSelection();
    on_insertButton_clicked();
}

void MainWindow2::CreateLogItemShortcuts(){
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
        connect (shortcutAction, SIGNAL(triggered()), specieskeyMapper, SLOT(map())) ;
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
        connect (shortcutAction, SIGNAL(triggered()), groupingskeyMapper, SLOT(map()));
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

void MainWindow2::ProjectSettingsFinished(){
    //qDebug() << "Settings finished called";
    on_actionAdditional_Fields_triggered();
    PopulateHeaderFields();
}

/**
 * @brief MainWindow2::PopulateHeaderFields
 * Adds header fields to the header fields key/value table.
 */
void MainWindow2::PopulateHeaderFields(){
    //qDebug() << "in populating header fields";

    QString surveyValues = ",";

    ui->HeaderFields->setRowCount(0); //Clear them all
    ui->HeaderFields->setRowCount(proj->getHeaderCount());
    ui->HeaderFields->setColumnCount(2);

    for (int y = 0; y < proj->getHeaderCount(); y++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);

        //qDebug() << proj->getHeaderName(y);

        ui->HeaderFields->setItem(y, 0, item);
        ui->HeaderFields->item(y,0)->setText(proj->getHeaderName(y));
        //Rap - Should add also the value code as well?
        proj->buildHeaderCell(ui->HeaderFields, y);
    }

    //ui->HeaderFields->verticalHeader()->setDefaultSectionSize(10);
    //ui->HeaderFields->verticalHeader()->hide();
    //ui->HeaderFields->horizontalHeader()->hide();

    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    QString surveyType = proj->getSurveyType();
    if(surveyType == "GOEA" || surveyType == "BAEA"){
        ui->comboBox_3->hide();
        ui->lockGPS->show();
    }else{
        ui->comboBox_3->show();
         ui->lockGPS->hide();
    }

    ui->surveyTypeLabel->setText(surveyType);
    ui->projectDirectory->setText(globalSettings.get("projectDirectory"));
    ui->exportFilename->setText(globalSettings.get("exportFilename") + ".asc");

     //qDebug() << "species count: " << species.length();
     //qDebug() << proj->addedSpecies[0];

    QRegExp rx("(\\;)"); //RegEx for ' ' or ',' or '.' or ':' or '\t'

    //These lines were added by someone just for debug
    //QStringList query = proj->addedSpecies[0].split(rx);
    // qDebug() << query[0] << " - " << query[1] << " - " << query[2];

    int columnCount = 5;
    int numSpecies = proj->addedSpecies.length();
    int numPerCol = numSpecies / columnCount;
    if( numSpecies % columnCount > 0 ){
        numPerCol++;
    }
    int colNum = 0;
    int rowNum = 0;
    int totalLoops = numPerCol * columnCount;

    ui->speciesTable->setRowCount( 0 );//To clear the table.
    ui->speciesTable->setColumnCount( 0 );

    ui->speciesTable->setRowCount( numPerCol );
    ui->speciesTable->setColumnCount( columnCount );
    ui->speciesTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);//added 4.29.2020
    ui->speciesTable->verticalHeader()->setDefaultSectionSize(7);
    ui->speciesTable->verticalHeader()->hide();
    ui->speciesTable->horizontalHeader()->hide();
    ui->speciesTable->setStyleSheet(QString::fromUtf8("QTableWidget{\n"
                                                      "	background-color: rgb(255, 255, 255);\n"
                                                      "	border-style:solid;\n"
                                                      "	border-width:1px;\n"
                                                      "	border-color: rgb(221, 221, 221);\n"
                                                      "}\n"
                                                      "QTableWidget::item{\n"
                                                      "	padding: 1px;\n"
                                                      "}"));//added 4.29.2020

    QFont fnt;//added 4.29.2020
    fnt.setPointSize(8);//added 4.29.2020
    fnt.setFamily("Segoe UI");//added 4.29.2020

    for( int i=0; i<totalLoops; i++){
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setFont(fnt);//added 4.29.2020
        ui->speciesTable->setRowHeight(rowNum,8);//added 4.29.2020 set the rowheight
        ui->speciesTable->setItem(rowNum, colNum, item);

        if(i < numSpecies){
            QStringList query = proj->addedSpecies[i].split(rx);
            ui->speciesTable->item(rowNum,colNum)->setText( query[0] + "  " + query[1].toLower());
        }else{
            ui->speciesTable->item(rowNum,colNum)->setText("");
        }

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

void MainWindow2::setSpecies( int row, int column ){
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

/**
 * @brief MainWindow2::BuildBookmarksTableColumns
 * Generate Observations Table Columns from headers, additional fields, and the standard observation columns
 */
void MainWindow2::BuildBookmarksTableColumns(){

    if (ui->Observations->rowCount() > 0) {
        return;
    }

    ui->Observations->clear();
    allColumnNames.clear();

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
        ui->Observations->setHorizontalHeaderItem(i, item);
    }

    QString styleSheet = "::section {"
                         "spacing: 10px;"
                         "background-color: #CCCCCC;"
                         "color: #333333;"
                         "border: 1px solid #333333;"
                         "margin: 1px;"
                         "text-align: right;"
                         "font-family: arial;"
                         "font-size: 12px; }";

    ui->Observations->horizontalHeader()->setStyleSheet(styleSheet);

    updateExportHeaders();

    //qDebug() << "Observations table columns re-built.";
}

/**
 * @brief MainWindow2::updateExportHeaders
 * Creates the list of export column headers in the correct order
 */
void MainWindow2::updateExportHeaders() {
    exportHeaders.clear();
    exportHeaders << proj->getHeaderNamesList();
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    if( globalSettings.get("surveyType") == "BAEA" ||  globalSettings.get("surveyType") == "GOEA" ){
        QStringList otherColumns = {"Audio File", "Latitude", "Longitude", "Altitude", "Course", "Speed", "Time", "# Satellites", "HDOP", "Species", "Quantity"};
        exportHeaders << otherColumns;
    }else{
        QStringList otherColumns = {"Audio File", "Latitude", "Longitude", "Altitude", "Course", "Speed", "Time", "# Satellites", "HDOP", "Species", "Quantity", "Grouping"};
        exportHeaders << otherColumns;
    }
    exportHeaders << proj->getAdditionalFieldsNamesList();
}

void MainWindow2::on_actionExport_Survey_Template_triggered()
{
    //qDebug() << "Export Survey Template Clicked.";
    QFileDialog dialog(this);
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    dialog.setFileMode(QFileDialog::Directory);
    QString nm;
    QString OutputFolder = dialog.getExistingDirectory(this, tr("Select a folder to place the survey template .surv file in"), globalSettings.get("projectDirectory"));
    if(OutputFolder.length() > 0){
        nm = OutputFolder + "/" + globalSettings.get("surveyType") + ".surv";
        QDir dir(nm);
        proj->Save(dir,false);
        QMessageBox msgBox;
        msgBox.setText("Survey Template created successfully.");
        msgBox.exec();
    }
}

void MainWindow2::on_actionExit_triggered()
{
    //qDebug() << "Exit Clicked.";
    this->close();
}

void MainWindow2::on_actionExport_triggered()
{
    //qDebug() << "on actionExport Triggered.";
    AppendASC();
    ui->Observations->setRowCount(0);
}

void MainWindow2::on_actionOpen_Project_triggered()
{
    //qDebug() << "Open Project Clicked.";
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    QString direct = QFileDialog::getOpenFileName(this, "Select Project", globalSettings.get("projectDirectory"), "Scribe project file (*sproj)");
    if (direct.isEmpty()){
        return;
    }
    qDebug() << "Load Called";
    proj->Load(direct);
    fieldDialog->hide();
    on_actionAdditional_Fields_triggered();
    PopulateHeaderFields();
}

void MainWindow2::on_actionImport_Survey_Template_triggered()
{
    //qDebug() << "Import Survey Template Clicked.";
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    QString surveyFile = QFileDialog::getOpenFileName(this,"Select Survey Template",globalSettings.get("projectDirectory"),"Scribe survey file (*.surv)");
    if (surveyFile.isEmpty()){
        return;
    }
    QDir directory(surveyFile);
    if (proj->Load(directory)){
       qDebug() << "load successful";
    }
    fieldDialog->hide();
    on_actionAdditional_Fields_triggered();
    PopulateHeaderFields();
}

void MainWindow2::on_actionAdditional_Fields_triggered()
{
    //qDebug() << "View - Additional Fields Clicked.";
    fieldDialog->show();
    int yCoord = this->y() + 30;
    int xCoord = this->x() + this->width() + 4;
    fieldDialog->setGeometry(xCoord, yCoord, fieldDialog->width(), this->height());
    proj->buildAdditionalfieldsTable(fieldDialog->table);

}

void MainWindow2::on_actionSpecies_Items_triggered()
{
    //qDebug() << "Settings - Species/Items Clicked.";
}

void MainWindow2::on_actionSocial_Groupings_triggered()
{
    //qDebug() << "Settings - Social Groupings Clicked.";
}

void MainWindow2::on_actionAbout_Scribe_triggered()
{
    //qDebug() << "About Clicked.";

    about = new AboutDialog(this);
    about->show();
}

void MainWindow2::on_actionSave_triggered()
{
    //qDebug() << "Save Clicked.";
    AutoSave();
}

void MainWindow2::on_Next_clicked()
{
    insertWasLastEvent = false;
    int currIndex = ui->wavFileList->currentIndex();

    //qDebug()<< "Current index" << currIndex;
    //qDebug()<< "Media count" << audio->fileList->mediaCount();

    if(currIndex < audio->fileList->mediaCount()-1){
        audio->fileList->setCurrentIndex(currIndex + 1);
    }else{
        return;
    }

    if (audio->player.state() != QMediaPlayer::PlayingState){
        audio->player.play();
    }
}

void MainWindow2::on_Back_clicked()
{
    insertWasLastEvent = false;
    int currIndex = ui->wavFileList->currentIndex();
    if (currIndex > 0){
     audio->fileList->setCurrentIndex(currIndex - 1);
    }else{
        audio->fileList->setCurrentIndex(audio->fileList->mediaCount() - 1);
    }
    audio->player.play();
}

void MainWindow2::repeatTrack(){
    insertWasLastEvent = false;
    audio->player.setPosition(0);
    ui->Play->setText(">");
    on_Play_toggled(false);
}

void MainWindow2::on_Play_toggled(bool checked)
{
    Q_UNUSED(checked)
    insertWasLastEvent = false;
    if (audio->player.position() == 0){
        int currIndex = ui->wavFileList->currentIndex();
        audio->fileList->setCurrentIndex(currIndex);
    }

   if(ui->Play->text() == ">"){
       audio->player.play();
   }else{
       audio->player.pause();
   }
}

void MainWindow2::on_actionLegacy_Project_Settings_triggered()
{
    //qDebug() << "Project Settings Clicked.";
    GlobalSettings &globalSettings = GlobalSettings::getInstance();

    int export_count = globalSettings.get("exportCount").toInt();
    if( export_count > 0 || ui->Observations->rowCount() > 0 ){
       projSettingsDialog->disable_field_buttons();
    }

    projSettingsDialog->PrepHeadFieldValues();
    projSettingsDialog->PrepAdditionalFieldValues(fieldDialog->table);

    projSettingsDialog->show();
    projSettingsDialog->setFocus();
}

void MainWindow2::on_quantity_textChanged(const QString &arg1)
{
    QString s = arg1;
    s = s.remove(QRegularExpression("[^0-9]"));
    ui->quantity->setText(s);

    if (arg1.contains('`')){
        handleBackTick();
    }
}

void MainWindow2::handleBackTick(){
    int currIndex = ui->wavFileList->currentIndex();
    if(currIndex == audio->fileList->mediaCount()-1){
        return;
    }
    if(!insertWasLastEvent){
        this->on_insertButton_clicked();
    }
    this->on_Next_clicked();
}

void MainWindow2::on_comboBox_2_currentTextChanged(const QString &arg1)
{
    for(int i = 0; i < species.length(); i++){
        if (species[i]->itemName == arg1){
            currentSpecies = species[i];
        }
    }
    //qDebug()<< currentSpecies->itemName;

    ui->quantity->setFocus();
}

void MainWindow2::on_Volume_sliderMoved(int position)
{
    audio->player.setVolume(position);
}

void MainWindow2::on_wavFileList_currentIndexChanged(int index)
{
    //qDebug() << index;

    audio->fileList->setCurrentIndex(index);
    audio->wavFileList->setItemData( index, QColor("red"), Qt::BackgroundColorRole);

    //added 4.27.2020
    QString wavfile = ui->wavFileList->itemText(index);
    if(index >= 0){//updated from = to >= to select the first wave file from the list and zoom to it
        QMetaObject::invokeMethod(gps->wayHandler, "selectWavFile",
            Q_ARG(QVariant, wavfile));
    }
    //qDebug() << wavfile;*/
}

void MainWindow2::on_insertButton_clicked()
{
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
    if(LockedGPSIndex == -1){
        latLongCoordinates = recordings[currentGpsRecording]->coords[curCoord];
        gpsEntry = recordings[currentGpsRecording]->entries[0];
    }else{
        latLongCoordinates = recordings[LockedGPSIndex]->coords[curCoord];
        gpsEntry = recordings[LockedGPSIndex]->entries[0];
    }

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

    QString surveyType = proj->getSurveyType();

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

    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Species"), new QTableWidgetItem(book->species));
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Quantity"), new QTableWidgetItem(quantValue));

    //hide for eagle surveys (or better yet, hide if no groupings enabled)

    if(surveyType != "BAEA" && surveyType != "GOEA" ){
     ui->Observations->setItem(currentRow, allColumnNames.indexOf("Grouping"), new QTableWidgetItem(book->grouping));
    }

    QTableWidgetItem *latitudeItem = new QTableWidgetItem(book->latitude);
    latitudeItem->setFlags(latitudeItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Latitude"), latitudeItem);

    QTableWidgetItem *longitudeItem = new QTableWidgetItem(book->longitude);
    longitudeItem->setFlags(longitudeItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Longitude"), longitudeItem);

    QTableWidgetItem *timeItem = new QTableWidgetItem(book->timestamp);
    timeItem->setFlags(timeItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Time"), timeItem);

    QTableWidgetItem *altitudeItem = new QTableWidgetItem(book->altitude);
    altitudeItem->setFlags(altitudeItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Altitude"), altitudeItem);

    QTableWidgetItem *speedItem = new QTableWidgetItem(book->speed);
    speedItem->setFlags(speedItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Speed"), speedItem);

    QTableWidgetItem *courseItem = new QTableWidgetItem(book->course);
    courseItem->setFlags(courseItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Course"), courseItem);

    QTableWidgetItem *HDOPItem = new QTableWidgetItem(book->hdop);
    HDOPItem->setFlags(HDOPItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("HDOP"), HDOPItem);

    QTableWidgetItem *satItem = new QTableWidgetItem(book->numSatellites);
    satItem->setFlags(satItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("# Satellites"), satItem);

    QTableWidgetItem *audioItem = new QTableWidgetItem(book->audioFileName);
    audioItem->setFlags(audioItem->flags() ^ Qt::ItemIsEditable);
    ui->Observations->setItem(currentRow, allColumnNames.indexOf("Audio File"), audioItem);

    // Insert headers into Observations Table
    for (int i=0; i<proj->getHeaderCount(); i++) {
        QString headerKey = proj->getHeaderName(i);
        int columnToInsert = allColumnNames.indexOf(headerKey);
        QString val = proj->getHeaderValue(ui->HeaderFields,i);
        QTableWidgetItem *headerValue = new QTableWidgetItem(val);
        ui->Observations->setItem(currentRow, columnToInsert, headerValue);
    }

    // Insert additional fields into Observations Table
    for (int i=0; i<proj->getAdditionalFieldsCount(); i++) {
        QString additionalFieldKey = proj->getAdditionalFieldName(i);
        int columnToInsert = allColumnNames.indexOf(additionalFieldKey);
        QString val = proj->getAdditionalFieldValue(fieldDialog->table,i);
        QTableWidgetItem *additionalFieldValue = new QTableWidgetItem(val);
        ui->Observations->setItem(currentRow, columnToInsert, additionalFieldValue);
    }

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
    LockedGPSIndex = -1;
    ui->lockGPS->setCheckState(Qt::Unchecked);
    ui->Observations->resizeColumnsToContents();

    //added 4.24.2020 start //clear the ui->HeaderFields value to null
    //if the settings retain value is not checked
    for (int y = 0; y < proj->getHeaderCount(); y++){
        QString headername = proj->getHeaderName(y);
        QStringList headervalue = proj->getHeaderValueList(y);
        /*QString headercheck = proj->getHeaderValueChk(y);
        if(headercheck.toInt() == 0){
            ui->HeaderFields->item(y,1)->setText("");//set the value to null
        }*/

        QStringList headercheck = proj->getHeaderValueChk2(y);
        //qDebug() << headername << "; " << headercheck.toInt();

        //qDebug() << "headercheck.length: "<< headercheck.length();
        if(headercheck.length() == 1){
            if(headercheck[0].toInt() == 0)
                ui->HeaderFields->item(y,1)->setText("");
        }else{
            //for array

        }
        /*auto chkfield = ui->addedHeaderFields->cellWidget(y, 2);
        QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
        if(chkbox->isChecked()){
            intchk = 2;
        }*/


    }

    /*//check the proj->getAdditionalFieldsCount()
    for (int y = 0; y < proj->getAdditionalFieldsCount(); y++){
        QString str = proj->getAdditionalFieldName(y);
        QStringList addnlvals = proj->getAdditionalFieldValueList(y);
        //QString valchk = proj->getAdditionalFieldChk(y);
        QStringList addnlvalschk = proj->getAdditionalFieldChk2(y);
        qDebug() << "addnlvalschk.length: "<< addnlvalschk.length();
        if(addnlvalschk.length() == 1){
            if(addnlvalschk[0].toInt() == 0){
                //for the addnl field
            }
        }else{
            //for array

        }
    }//added 4.24.2020 end*/

    //check the proj->getAdditionalFieldsCount()
    for (int y = 0; y < proj->getAdditionalFieldsCount(); y++){
        QStringList addnlvalschk = proj->getAdditionalFieldChk2(y);
        if(addnlvalschk.length() == 1){
            if(addnlvalschk[0].toInt() == 0){
                 QWidget *w = fieldDialog->table->cellWidget(y,1);
                 qDebug() << QString::fromUtf8(w->metaObject()->className());//cast const char to QString
                 if(QString::fromUtf8(w->metaObject()->className()) == "QLineEdit"){
                     QLineEdit *l  = new QLineEdit();
                     l->setText("");
                     fieldDialog->table->setCellWidget(y,1, l);
                 }else if(QString::fromUtf8(w->metaObject()->className()) == "QComboBox"){
                     QComboBox *cb = qobject_cast<QComboBox*>(w);
                     cb->setCurrentIndex(0);
                 }else if(QString::fromUtf8(w->metaObject()->className()) == "QPlainTextEdit"){
                     //QPlainTextEdit *pte = qobject_cast<QPlainTextEdit*>(w);
                     QPlainTextEdit  *s  = new QPlainTextEdit ();
                     fieldDialog->table->setCellWidget(y,1, s);
                 }
            }
        }
    }//added 5.6.2020 end
}

void MainWindow2::on_HeaderFields_cellChanged(int row, int column)
{
    if(column < 1 || !proj->isHeaderSingleValue(row)){
        return;
    }
    QTableWidgetItem *item = ui->HeaderFields->item(row, column);
    QString s = item->text();
    s = s.remove(QRegularExpression("[^\\\a-zA-Z0-9\\@\\#\\$\\%\\&\\*\\/\\<\\>\\.\\?\\!\\+\\-\\_\\(\\)]"));
    item->setText(s);
}

void MainWindow2::on_deleteButton_clicked()
{
    //check selected row in table, and remove that thing
    ui->Observations->removeRow(ui->Observations->currentRow());
    //also need to remove it from application data structure----------------??
}

void MainWindow2::on_Observations_cellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    //Stub
}

void MainWindow2::on_exportButton_clicked()
{
    AppendASC();
}

void MainWindow2::on_lockGPS_stateChanged(int arg1)
{
    //qDebug() << "Lock gps = " << arg1;
    if(arg1 == 2 && ui->wavFileList->count() > 0){
        LockedGPSIndex = ui->wavFileList->currentIndex();
        ui->lockGPS->setCheckState(Qt::Checked);
        ui->lockGPS->setText( ui->wavFileList->currentText() );
    }else{
        LockedGPSIndex = -1;
        ui->lockGPS->setCheckState(Qt::Unchecked);
          ui->lockGPS->setText("Lock GPS" );
    }
    //qDebug() << "Lock gps value = " << LockedGPSIndex;
}

void MainWindow2::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index)
    refreshMap();
}

void MainWindow2::readSurveyJson()
{
    surveyJsonSegmentList.clear();
    QJsonObject::iterator i;
    for (i = proj->geoJSON.begin(); i != proj->geoJSON.end(); ++i) {
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
                    surveyJsonSegment.clear();//clears the list

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
                                    templist << QString::number(value2.toDouble());
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
                        surveyJsonSegment.append(templist2);
                        if(templist.length() > 0){
                            for(int i = 0; i < templist.length(); i++){
                                surveyJsonSegment.insert(i,templist[i]);
                            }
                        }
                    }
                    surveyJsonSegmentList.append(surveyJsonSegment);
                }
            }
        }
    }

    /*for(int i = 0; i < surveyJsonSegmentList.length(); i++){
        qDebug() << surveyJsonSegmentList[i];
    }*/

    qDebug() << surveyJsonSegmentList.length();

    QMetaObject::invokeMethod(gps->wayHandler, "plotSurveyJson",
        Q_ARG(QVariant, QVariant::fromValue(surveyJsonSegmentList)));

}
