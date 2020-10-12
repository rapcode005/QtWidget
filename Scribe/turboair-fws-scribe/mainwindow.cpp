#include "mainwindow.h"
#include "ui_mainwindow.h"
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
#include "exportdialog.h"
#include "openingscreen.h"
#include "globalsettings.h"


static bool GPS_MAP_ENABLED = false;





/**
 *  mainwindow.cpp is deprecated/replaced by mainwindow2.cpp
 *  per Jason's commit # 3180a50
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    //setting up our gps widget
    if ( GPS_MAP_ENABLED ){
      ui->quickWidget->setSource(QUrl(QStringLiteral("qrc:/gps/main.qml")));
      ui->quickWidget->setResizeMode(ui->quickWidget->SizeRootObjectToView);
      ui->quickWidget->show();
    }
    this->window()->setWindowTitle("Scribe");

    fsiDialog = new FileStructureInfoDialog(this);
    //setting up our audio player.
    audio = new AudioPlayer(ui->pushButton,ui->posLabel,ui->durationlabel, fsiDialog->listWidget, ui->audioLabel_2, ui->audioIndex);
    audio->player.setVolume(ui->horizontalSlider_2->value());
    audio->player.setPlaylist(audio->fileList);

    fsiDialog->audio = audio;
    //fil = new FileHandler; //handling our transfers
    gps = new GpsHandler(ui->quickWidget->rootObject(),  startDir); //handle plotting our gps coordinates

    //settings = new ProjectSettings;
    proj = new ProjectFile;
    projectFile = new ProjectSaveFile;

    shortcutGroup = new QActionGroup(this);
    specieskeyMapper = new QSignalMapper(this);
    groupingskeyMapper = new QSignalMapper(this);

    proDialog = new ProjectSettingsDialog(this,proj,settings);
    fieldDialog = new AdditionalFieldsDialog(this);
    exportDialog = new ExportDialog(this);
    openingScreen = new OpeningScreenDialog(this,proj,settings);
    openingScreen->show();
    openingScreen->setGeometry(
                               QStyle::alignedRect(
                                   Qt::LeftToRight,
                                   Qt::AlignCenter,
                                   openingScreen->size(),
                                   qApp->desktop()->availableGeometry()
                               ));



    proDialog->projectFile = projectFile;

    connect(proDialog, SIGNAL(receivedLogDialogItems()),this,SLOT(RefreshItems()));

    if ( GPS_MAP_ENABLED ){
      connect (gps->wayHandler, SIGNAL(removeLog(QVariant)),
               this, SLOT(removeLog(QVariant)));
    }

    connect(audio->fileList, SIGNAL(currentIndexChanged(int)), this, SLOT(RecordingIndexChanged(int)));
    proDialog->setGeometry(    QStyle::alignedRect(
                                   Qt::LeftToRight,
                                   Qt::AlignCenter,
                                   proDialog->size(),
                                   qApp->desktop()->availableGeometry()
                               ));
    connect(exportDialog,SIGNAL(accepted()),this,SLOT(ExportASC()));
    connect(exportDialog,SIGNAL(Append()),this,SLOT(AppendASC()));
    connect(proj,SIGNAL(Loaded()),proDialog,SLOT(ProjectOpened()));
    connect(projectFile,SIGNAL(fileLoaded(QList<BookMark*>)),this,SLOT(ItemsLoaded(QList<BookMark*>)));
    connect(projectFile, SIGNAL(fileLoaded(QList<BookMark*>)),proDialog,SLOT(ProjectFileOpened()));
    connect(proDialog, SIGNAL(finished(int)),this, SLOT(ProjectSettingsFinished()));
    ui->lineEdit_2->installEventFilter(this);
    startDir = QDir::homePath();
    proDialog->startDir = startDir;
    //proDialog->show();
    autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, SIGNAL(timeout()), this, SLOT(AutoSave()));
    autoSaveTimer->start(20000);
    //for our problems with ` moving to the next audio file
    new QShortcut(QKeySequence(Qt::Key_QuoteLeft), this, SLOT(on_next_clicked()));
    //stupid roundabout bullshit below, above should've worked but it didn't
    //QAction *next = new QAction(this);
    //next->setShortcut(Qt::Key_acute);

    //connect(next, SIGNAL(triggered()), this, SLOT(on_next_clicked()));
    //this->addAction(next);

    qDebug() << "MainWindow loaded without errors.";
}





MainWindow::~MainWindow()
{
    audio->player.stop();
    QApplication::closeAllWindows();
    delete ui;
}





void MainWindow::closeEvent (QCloseEvent *event)
{
    if (projectFile->changesMade){
        QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Scribe",
                                                                    tr("Exit without saving changes?\n"),
                                                                    QMessageBox::Cancel | QMessageBox::Save | QMessageBox::Close,
                                                                    QMessageBox::Save);
        if (resBtn == QMessageBox::Cancel)
        {
            event->ignore();
        }
        else if (resBtn == QMessageBox::Save)
        {
            QString direct = projectFile->filePath;
            if (direct.isEmpty())
                direct = QFileDialog::getSaveFileName(this, "Select Project", startDir, "Scribe project file (*sproj)");
            if (direct.isEmpty()){
                event->ignore();
                return;
            }
            if (!direct.endsWith(".sproj"))
                direct += ".sproj";

            projectFile->Save(direct, true);
            event->accept();
        }else{
            event->accept();
        }
    }else{
        event->accept();
    }
}





void MainWindow::AutoSave(){
    if (projectFile->changesMade) {
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

        // Remove older autosave files
        while (taggedFiles.length() > 4) {
            qDebug() << "Removing autosave file at " << taggedFiles[0].trimmed();
            QFile curFile(taggedFiles[0]);
            curFile.remove();
            taggedFiles.removeAt(0);
        }

        QString autosaveFile =
                autosaveDirectory +
                "/" +
                QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") +
                "_autosave.sproj";

        projectFile->Save(autosaveFile, false);
        qDebug()<< "Autosaved to " << autosaveFile;
    }
}





void MainWindow::on_actionSave_As_triggered()
{
    QString direct = QFileDialog::getSaveFileName(this,"Select Survey",startDir,"Scribe survey file (*.surv)");
    if (direct.isEmpty()){
        return;
    }
    if (!direct.endsWith(".surv")){
        direct += ".surv";
    }
    QDir directory(direct);
    if (proj->Save(directory)){
        qDebug() << "Saved successfully";
        startDir = direct;
    }
    //this->setWindowTitle("Scribe - " + direct);
}


void MainWindow::on_actionOpen_survey_triggered()
{
     QString direct = QFileDialog::getOpenFileName(this,"Select Survey",startDir,"Scribe survey file (*.surv)");
     if (direct.isEmpty()){
         return;
     }
     QDir directory(direct);
     if (proj->Load(directory)){
        //emit fileOpened();
         qDebug() << "load successful";
         //startDir = direct;
     }
     fieldDialog->hide();
     on_actionAdditional_Fields_triggered();
     //this->setWindowTitle("Scribe - " + direct);
}


void MainWindow::on_actionSave_project_triggered()
{
     QString direct = proj->fileName;
     if (direct.isEmpty()){
         on_actionSave_As_triggered();
         return;
     }
     QDir directory(direct);
     if (proj->Save(directory)){
         qDebug() << "Saved successfully";
         //startDir = direct;
     }
     //this->setWindowTitle("Scribe - " + direct);
}





void MainWindow::on_pushButton_clicked()
{
    //making sure we can mess with the audio
    if (audio->player.isAudioAvailable()){
        if(audio->player.state() != QMediaPlayer::PlayingState){
            audio->player.play();
        }else{
            audio->player.pause();
        }
    }
}





//our volume bar
void MainWindow::on_horizontalSlider_2_sliderMoved(int position)
{
    audio->player.setVolume(position);
}



void MainWindow::on_next_clicked()
{
    //if we have no more files to move to next, set ourselves back to the firse
    if(audio->fileList->currentIndex() < audio->fileList->mediaCount() - 1){
        audio->fileList->next();
    }else{
        audio->fileList->setCurrentIndex(0);
    }
    if (audio->player.state() != QMediaPlayer::PlayingState){
        audio->player.play();
    }
}


void MainWindow::on_previous_clicked()
{
    //if we're more than 5 seconds in, just rewind
    if (audio->player.position() > 5000){
         audio->player.setPosition(0);
    }else{
        //we loop back to the end of the list if theres no previous track
        if (audio->fileList->currentIndex() > 0){
         audio->fileList->previous();
        }else{
            audio->fileList->setCurrentIndex(audio->fileList->mediaCount() - 1);
        }
    }
}





void MainWindow::on_pushButton_10_clicked()
{
    if (recordings.length() < 1){
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Error! There are no recordings!!");
        mb.exec();
        return;
    }
    if (species.length()  < 1){
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText("Error! There are no species defined!");
        mb.exec();
        return;
    }
    if (!currentSpecies->groupingEnabled){
        if (ui->comboBox_3->currentText() != "Open"){
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Error");
            mb.setText("Warning! Social groupings were not enabled for this species!");
            mb.setStandardButtons(QMessageBox::StandardButton::Ok);
            mb.exec();
            return; //not too happy that I had to add this line
        }
    }
    projectFile->changesMade = true;
    BookMark *book = new BookMark();
    book->species = currentSpecies->itemName;

    //solving the no quantity of species problem
    QString quant = ui->lineEdit_2->text().trimmed();
    if (quant.isEmpty() || quant == "0"){
        book->quantity = 1;
    }else{
        book->quantity = quant.toInt();
    }
    //for defaulting back to a value, will change to phils liking
    ui->lineEdit_2->setText("");

    book->grouping = ui->comboBox_3->currentText();
    book->transect = ui->lineEdit_3->text();

    for (int i = 0; i < fieldDialog->table->rowCount(); i ++){
        QLineEdit *s = dynamic_cast<QLineEdit*>(fieldDialog->table->cellWidget(i,1));
        book->extraValues << s->text();
        s->setText(fieldDialog->defaultVals[i]);
    }
    QString c = recordings[currentGpsRecording]->coords[curCoord];
    QStringList coords = c.split(",");
    book->lattitude = coords[0];
    book->longitude = coords[1];
    //get strings from field dialog, then reset its values to the default in project settings
    bookmarks.append(book);
    //bookmarks
    QVariant wayDesc;
    wayDesc = book->species + " " + QString::number(book->quantity) + " " + book->grouping;
    ui->bookmarkbox->addItem(book->species + " " + QString::number(book->quantity) + " " + book->grouping);
    qDebug() << book->species << " " << book->quantity << " " << book->grouping << " " << book->transect;
    QMetaObject::invokeMethod(gps->wayHandler, "addBookMarkAtCurrent",
            Q_ARG(QVariant, wayDesc));
    qDebug() << "bookmarks length is now " << bookmarks.length();
    projectFile->entries = bookmarks;
    //projectFile->AddSighting(book);
    ui->lineEdit_2->setFocus();
}





void MainWindow::removeLog(QVariant index){
    int ind = index.toInt();
    bookmarks.removeAt(ind);
    projectFile->entries = bookmarks;
    //projectFile->RemoveSighting(ind);
    qDebug() << "index we received was " << ind << "bookmarks length is now " << bookmarks.length();
    QListWidgetItem *item = ui->bookmarkbox->item(ind);
    ui->bookmarkbox->takeItem(ind);
    delete item;
    projectFile->changesMade = true;
}





void MainWindow::ClearBookmarks(){
    QMetaObject::invokeMethod(gps->wayHandler, "clearAllBookmarks");
}





void MainWindow::RecordingIndexChanged(int position){
        currentGpsRecording = position;
        curCoord = 0;
        QVariant start = recordings[currentGpsRecording]->coords[0];
        //qDebug() << "RecordingIndexChanged()";
        if ( GPS_MAP_ENABLED ){
          QMetaObject::invokeMethod(gps->wayHandler, "updatePilotCoordinates",
                 Q_ARG(QVariant, start));
        }
}





void MainWindow::ItemsLoaded(QList<BookMark*> items){
    //bookmarks.append(book);
    //ui->bookmarkbox->clear();
    //qDeleteAll(bookmarks);
    ClearBookmarks();
    bookmarks = items;
    for(int i = 0; i < bookmarks.length(); i++){
        BookMark *book = bookmarks[i];
        QVariant wayDesc;
        QVariant lattitude;
        QVariant longitude;
        wayDesc = book->species + " " + QString::number(book->quantity) + " " + book->grouping;
        lattitude = book->lattitude;
        longitude = book->longitude;
        ui->bookmarkbox->addItem(book->species + " " + QString::number(book->quantity) + " " + book->grouping);
        qDebug() << book->species << " " << book->quantity << " " << book->grouping << " " << book->transect;
        if ( GPS_MAP_ENABLED ){
          QMetaObject::invokeMethod(gps->wayHandler, "addBookMarkWithCoord",
                  Q_ARG(QVariant, wayDesc),
                  Q_ARG(QVariant, lattitude),
                  Q_ARG(QVariant, longitude)
                  );
        }
        //bookmarks.append(book);
    }
    qDebug() << "bookmarks length is now " << bookmarks.length();
    //some cheekiness here
    fieldDialog->hide();
    on_actionAdditional_Fields_triggered();
}





QString MainWindow::BookmarksToAsc(){
    QString textToWrite = "";
    for (int i = 0; i < bookmarks.length(); i++){
        QString line = "";
        BookMark* current = bookmarks[i];
        QString surveyValues = ",";
        for (int t = 0; t < projectFile->surveyHeaderKeys.length(); t++){
            surveyValues += projectFile->surveyHeaderValues[t] + ",";
        }
        QString extraValues = ",";
        for (int s = 0; s < current->extraValues.length(); s++){
            if (s == current->extraValues.length()){
                extraValues += current->extraValues[s];
            }else{
                extraValues += current->extraValues[s] + ",";
            }
        }
        QString str = "(na)";
        if (!current->transect.isEmpty()){
            str = current->transect;
        }
        line =
                projectFile->initials +
                "," + projectFile->seat +
                "," + projectFile->flightDate +
                    surveyValues
                 + current->lattitude +
                "," + current->longitude +
                //"," + current.timestamp +
                "," + current->species +
                "," + QString::number(current->quantity) +
                "," + current->grouping +
                "," + str +
                extraValues;
        line += "\r\n";
        textToWrite += line;
    }
    return textToWrite;
}





void MainWindow::on_actionExport_triggered()
{
    exportDialog->Setup(BookmarksToAsc());
    exportDialog->show();
}





void MainWindow::ExportASC(){
    if (bookmarks.length() < 1){
        return;
    }
    QString exportLocation = QFileDialog::getSaveFileName(0,"Export ASC File", startDir, "ASC files (*asc)");
    if (exportLocation.isEmpty()){
        qDebug() << "No export directory specified";
        return;
    }
    if (!exportLocation.endsWith(".asc",Qt::CaseInsensitive)){
        exportLocation.append(".asc");
    }
    QFile exportFile(exportLocation);
    if (!exportFile.open(QIODevice::WriteOnly|QIODevice::Truncate)){
          qDebug() << "Couldn't open file for writing";
          return;
    }
    QTextStream stream (&exportFile);
    QString ascString = BookmarksToAsc();
    stream << ascString;
    exportFile.close();
    QMessageBox mb;
    mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    mb.setWindowTitle("Export");
    mb.setText("Exported bookmarks successfully!");
    mb.exec();
    ascFilename = exportLocation;
    //setWindowTitle("Scribe - " + ascFilename);
    ClearBookmarks();
    qDebug() << "Successfully exported " << exportLocation;
}





void MainWindow::AppendASC(){
    if (bookmarks.length() < 1){
        return;
    }
    QString exportLocation = "";
    if (ascFilename.isEmpty()){
        exportLocation = QFileDialog::getOpenFileName(this, "Append all bookmarks to existing ASC",startDir,"ASC files (*asc)");
    }else{
        exportLocation = ascFilename;
    }
    if (exportLocation.isEmpty()){
        qDebug() << "No export directory specified";
        return;
    }
    /*if (!exportLocation.endsWith(".asc",Qt::CaseInsensitive)){
        exportLocation.append(".asc");
    }*/
    QFile exportFile(exportLocation);
    if (!exportFile.open(QIODevice::ReadWrite|QIODevice::Append)){
          qDebug() << "Couldn't open file for writing";
          return;
    }
    QTextStream stream (&exportFile);
    QString ascString = BookmarksToAsc();
    stream << ascString;
    exportFile.close();
    QMessageBox mb;
    mb. setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    mb.setWindowTitle("Export");
    mb.setText("Appended bookmarks successfully!");
    mb.exec();
    ascFilename = exportLocation;
    //setWindowTitle("Scribe - " + ascFilename);
    ClearBookmarks();
    qDebug() << "Successfully exported " << exportLocation;
}





void MainWindow::RefreshItems( ){
    qDebug() << "Refreshing Items";
    species = proDialog->speciesItems;
    groupings = proDialog->groupingItems;

    FillSpeciesTemplates(species);
    FillGroupingTemplates(groupings);
    CreateLogItemShortcuts();
    //some cheekiness here
    fieldDialog->hide();
    on_actionAdditional_Fields_triggered();
}





void MainWindow::FillSpeciesTemplates(QList<LogTemplateItem*> items){
    ui->comboBox_2->clear();
    QStringList s;
    for (int i = 0; i  < species.length(); i ++ ){
        s << items[i]->itemName;
    }
    s.sort();
    ui->comboBox_2->addItems(s);
}





void MainWindow::FillGroupingTemplates(QList<LogTemplateItem*> items){
    ui->comboBox_3->clear();
    QStringList g;
    for (int i = 0; i  < groupings.length(); i ++ ){
        g << items[i]->itemName;
    }
    g.sort();
    ui->comboBox_3->addItems(g);
}





void MainWindow::FillSpeciesField(int item){
        qDebug() << "Filled a Species field";
        currentSpecies = species[item];
        ui->comboBox_2->setCurrentText(currentSpecies->itemName);
}


void MainWindow::FillGroupingField(int item){
        qDebug() << "Filled a grouping field";
    ui->comboBox_3->setCurrentText(groupings[item]->itemName);
    on_pushButton_10_clicked();

}





void MainWindow::CreateLogItemShortcuts(){
   disconnect (specieskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillSpeciesField(int))) ;
   disconnect (groupingskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillGroupingField(int))) ;
   qDebug() << "Deleting Old Shortcuts";
   for (int s = 0; s < shortcutGroup->actions().length(); s++){
       QAction *shortcutAction = shortcutGroup->actions()[s];
       this->removeAction(shortcutAction);
       qDebug()<<"Action Removed" << shortcutAction->shortcut().toString();
    }
    qDeleteAll(shortcutGroup->actions());
    shortcutGroup->setExclusive(false);

    bool conflictingKeys = false;
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
    //some cheese here
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

    //CHECK FOR SIMILLAR USER KEYS, COME BACK TO THIS
    QStringList userDefinedKeys;
    qDebug() << "Creating Shortcuts";
    for (int i = 0; i  < species.length(); i ++ ){
        QAction *shortcutAction = new QAction(this);
        shortcutGroup->addAction(shortcutAction);
        this->addAction(shortcutAction);
        for (int u = 0; u < usedHotkeys.length(); u++){
            if (usedHotkeys[u] == species[i]->shortcutKey.toString()){
                conflictingKeys = true;
                errorMessage += "\r\n - '" + usedHotkeys[u] + "'";
            }else{
                 //usedHotkeys.append(species[i]->shortcutKey.toString());
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
        for (int u = 0; u < usedHotkeys.length(); u++){
            if (usedHotkeys[u] == groupings[i]->shortcutKey.toString()){
                conflictingKeys = true;
                errorMessage += "\r\n - '" + usedHotkeys[u] + "(Already defined by program)";
            }else{
                shortcutAction->setShortcut(groupings[i]->shortcutKey);
                shortcutAction->setShortcutContext(Qt::WindowShortcut);
            }
        }
        userDefinedKeys.append(shortcutAction->shortcut().toString());

        connect (shortcutAction, SIGNAL(triggered()), groupingskeyMapper, SLOT(map())) ;
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
        proDialog->activeButton->click();
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText(errorMessage);
        mb.exec();
    }
   connect (groupingskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillGroupingField(int))) ;
   connect (specieskeyMapper, SIGNAL(mapped(int)), this, SLOT(FillSpeciesField(int))) ;
}






void MainWindow::on_pushButton_3_clicked()
{
    QDir dir(QCoreApplication::applicationDirPath() + "/UserData/Transects.json");
    gps->ReadTransects(dir);
    ui->checkBox_4->setChecked(true);
}





void MainWindow::on_actionRecordings_List_triggered()
{
    fsiDialog->setParent(this);
    fsiDialog->show();
    int yCoord = this->y() + 30;
    int xCoord = this->x() - 185;
    fsiDialog->setGeometry(xCoord, yCoord,fsiDialog->width(),fsiDialog->height());
}





// data import
void MainWindow::on_actionImport_Flight_Data_triggered()
{
    // Make sure the project's FlightData directory exists
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    QString projectDirectory = globalSettings.get("projectDirectory");
    if (projectDirectory == nullptr || projectDirectory.isEmpty()){
        QMessageBox alert;
        alert.setText("Project directory has not been configured.\nUnable to import flight data at this time.");
        alert.exec();
        qDebug() << "Project directory not set. Aborting import.";
        return;
    }

    QString sourceFlightDataDirectoryPath = QFileDialog::getExistingDirectory(nullptr, "Select The Flight Directory", startDir);
    if (sourceFlightDataDirectoryPath.isEmpty()){
        return;
    }
    fsiDialog->listWidget->clear();
    audio->fileList->clear();
    qDeleteAll(recordings);
    recordings.clear();
    fsiDialog->setParent(this);
    fsiDialog->show();
    int yCoord = this->y() + 30;
    int xCoord = this->x() - 185;
    fsiDialog->setGeometry(xCoord, yCoord, fsiDialog->width(), fsiDialog->height());

    QString targetFlightDataDirectoryPath = projectDirectory + "/FlightData";
    if (!QDir(targetFlightDataDirectoryPath).exists()) {
        QDir projDir(projectDirectory);
        projDir.mkdir("./FlightData");
    }

    QDir sourceDirectory(sourceFlightDataDirectoryPath);
    QDirIterator projectFiles(sourceDirectory.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (projectFiles.hasNext()) {
        QString sourceFilePath = projectFiles.next();
        QFileInfo sourceFileInfo(sourceFilePath);
        QString targetFilePath = targetFlightDataDirectoryPath + "/" + sourceFileInfo.fileName();
        qDebug() << "Copying " << sourceFilePath << " to " << targetFilePath;
        if (!QFile::copy(sourceFilePath, targetFilePath)) {
            qDebug() << "Unable to copy " << sourceFile << " to " << targetFilePath;
        }
    }

    QDirIterator wavFileIterator(targetFlightDataDirectoryPath, QStringList() << "*.WAV", QDir::Files, QDirIterator::Subdirectories);
    QStringList names;
    setWindowTitle("Scribe - Current Flight (" + targetFlightDataDirectoryPath + ")");
    while (wavFileIterator.hasNext()) {
        QString wavFilePath = wavFileIterator.next();
        QFileInfo wavFileInfo(wavFileIterator.filePath());
        QString wavFileName = wavFileInfo.fileName();
        QString gpsFileName = wavFileInfo.baseName() + ".GPS";
        QString gpsFilePath = targetFlightDataDirectoryPath + "/" + gpsFileName;

        qDebug() << "reading " << wavFilePath << " as " << gpsFilePath;
        QFile gpsFile(gpsFilePath);

        if (gpsFile.exists()) {
            if (gpsFile.open(QIODevice::ReadOnly)){
             QTextStream stream(&gpsFile);
             QStringList coordsToAdd;
             while(!stream.atEnd()){
                 coordsToAdd << stream.readLine();
             }
             qDebug() << "Added coordinates " << coordsToAdd;
             gpsFile.close();
             //
             //  this is where the import data is parsed
             //
             GPSRecording *record = new GPSRecording(coordsToAdd, wavFilePath);
             recordings.append(record);
             names.append(wavFileName);
             //qDebug() << "with times " << record->times;

             audio->fileList->addMedia(QUrl::fromLocalFile(wavFilePath));
             //adding the media from our directory
             int f = audio->fileList->currentIndex() + 1;
             int l = audio->fileList->mediaCount();
             ui->audioIndex->setText("(" + QString::number(f) + "/" + QString::number(l) + ")");

             qDebug()<< "Loaded GPS Recording - " << record->audioPath << " with " << record->coords.length() << "lines of GPS Data";
            }
        } else {
            qDebug() << "Associated GPS File - " << gpsFileName << "does not exist";
        }
        //Adding an item with the newly created name
    }
    QStringList gpsPaths;
    QDirIterator iterator (QDir(targetFlightDataDirectoryPath).absolutePath(), QStringList() << "*.GPS", QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()){
        qDebug() <<
          iterator.next();
        gpsPaths.append(iterator.filePath());
    }
    gps->ParseGpsStream(&gpsPaths);
    fsiDialog->FillList(names);
    if (!audio->fileList->isEmpty()){
        audio->player.play();
    }
    qDebug() << "Flight data imported without errors.";
}





void MainWindow::on_actionAdditional_Fields_triggered()
{
    fieldDialog->show();
    int yCoord = this->y() + 30;
    int xCoord = this->x() + this->width() + 24;
    fieldDialog->setGeometry(xCoord, yCoord, fieldDialog->width(), fieldDialog->height());
    fieldDialog->AddFields(projectFile->addedHeaderKeys, projectFile->addedHeaderValues);
}





void MainWindow::on_actionProject_Settings_triggered()
{
    proDialog->show();
    proDialog->setFocus();
}





void MainWindow::on_checkBox_4_stateChanged(int arg1)
{
    //qDebug()<< "on_checkBox_4_stateChanged " << arg1;
    QString send = "true";
    if (arg1 == 0){
        send = "false";
    }
    QVariant cur = send;
    QMetaObject::invokeMethod(gps->wayHandler, "showTransects",
        Q_ARG(QVariant, cur));
}




bool MainWindow::eventFilter(QObject *watched, QEvent *event){
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        for(int i = 0; i < shortcutGroup->actions().length(); i++){
            if (shortcutGroup->actions()[i]->shortcut().toString().length() == 1){
                if (keyEvent->key() == shortcutGroup->actions()[i]->shortcut().toString() || keyEvent->key() == Qt::Key_Return) {
                    qDebug() << "Ignoring" << (char)keyEvent->key() << "for" << watched;
                    event->ignore();
                    return true;
                }
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}





void MainWindow::on_pushButton_2_clicked()
{
    if (ascFilename.isEmpty()){
        exportDialog->Setup(BookmarksToAsc());
        exportDialog->show();
        return;
    }
    AppendASC();
}





void MainWindow::on_actionExit_triggered()
{
    this->close();
}




void MainWindow::on_actionOpen_Project_triggered()
{
    QString direct = QFileDialog::getOpenFileName(this, "Select Project", startDir, "Scribe project file (*sproj)");
    if (direct.isEmpty()){
        return;
    }
    projectFile->Load(direct);
}


void MainWindow::on_actionSave_project_2_triggered()
{
    QString direct = projectFile->filePath;
    if (direct.isEmpty())
        direct = QFileDialog::getSaveFileName(this, "Select Project", startDir, "Scribe project file (*sproj)");
    if (direct.isEmpty()){
        return;
    }
    if (!direct.endsWith(".sproj"))
        direct += ".sproj";

    projectFile->Save(direct, true);
}


void MainWindow::on_actionSave_project_as_triggered()
{
    QString direct = QFileDialog::getSaveFileName(this, "Select Project", startDir, "Scribe project file (*sproj)");
    if (direct.isEmpty()){
        return;
    }
    if (!direct.endsWith(".sproj"))
        direct += ".sproj";
    projectFile->Save(direct, true);
}





void MainWindow::on_verticalSlider_valueChanged(int value)
{
    QMetaObject::invokeMethod(gps->wayHandler, "zoomToValue",
        Q_ARG(QVariant, (float)value/10));
}





void MainWindow::ProjectSettingsFinished(){
    fieldDialog->hide();
    if (projectFile->addedHeaderKeys.length() > 0)
        on_actionAdditional_Fields_triggered();
}





void MainWindow::on_lineEdit_2_textChanged(const QString &arg1)
{
    if (arg1.contains('`'))
        this->on_next_clicked();
    QString s = arg1;
    s = s.remove("`");
    s = s.remove(QRegularExpression("[A-Z]"));
    s = s.remove(QRegularExpression("[a-z]"));
    ui->lineEdit_2->setText(s);
}






void MainWindow::on_comboBox_2_currentTextChanged(const QString &arg1)
{
    for(int i = 0; i < species.length(); i++){
        if (species[i]->itemName == arg1){
            currentSpecies = species[i];
        }
    }
   qDebug()<< currentSpecies->itemName;
}
