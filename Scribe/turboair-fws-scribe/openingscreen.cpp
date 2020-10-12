#include "openingscreen.h"
#include "ui_openingscreen.h"
#include "calenderpopup.h"
#include "globalsettings.h"
#include <QFileDialog>
#include <QDir>
#include <QInputDialog>
#include <QDirIterator>
#include <QMessageBox>
#include <QErrorMessage>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDebug>
#include <QCloseEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QQmlComponent>


OpeningScreenDialog::OpeningScreenDialog( QWidget *parent, ProjectFile *pro):
    QMainWindow(parent), ui(new Ui::OpeningScreenDialog)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    ui->setupUi(this);

    //The empty project object
    proj = pro;

    //Add the survey types to the drop down
    /*ui->surveyType->addItem("WBPHS");
    ui->surveyType->addItem("GOEA");
    ui->surveyType->addItem("BAEA");
    ui->surveyType->addItem("Other");*/
    ui->surveyType->addItem("wbphs");
    ui->surveyType->addItem("goea");
    ui->surveyType->addItem("baea");
    ui->surveyType->addItem("other");

    //Add Observer Seat List
    QStringList list;
    list << "lf" << "rf" << "lm" << "lr" << "rr";
    ui->observerSeat->addItems(list);

    //Readonly
    ui->projectDirVal->setReadOnly(true);
    ui->geoFileText->setReadOnly(true);
    ui->geoFileText2->setReadOnly(true);
    ui->label_3->setText("Obs. Initials / Seat:");

    ui->observerInitials->setPlaceholderText("Max 3 Letters");
    ui->projectDirVal->setPlaceholderText("Choose");
    //Event for mouse double click
    ui->projectDirVal->setCursor(Qt::PointingHandCursor);
    ui->geoFileText->setCursor(Qt::ArrowCursor);
    ui->geoFileText2->setCursor(Qt::ArrowCursor);
    customEvent = new CustomEvent(ui->projectDirVal, 0);
    //Disable
    directoryText(false);

    //surveyInt.clear();
    //surveyInt << 5 << 2 << 1;

    QFont font;
    font.setPixelSize(12);
    font.setFamily("Segoe UI");
    QString styleSheet = QString("font-size:%1px;").arg(font.pixelSize());
    styleSheet = styleSheet + QString("\n font-family:'" + font.family() + "';");
    this->setStyleSheet(styleSheet);

    ui->lblGeo2->setFont(font);

    //Add icon
    QIcon icon;
    icon.addFile(":/img/misc/Icon awesome-folder.png", QSize(13, 13));
    QAction *act = ui->projectDirVal->addAction(icon, QLineEdit::TrailingPosition);
    connect(act, &QAction::triggered, this, &OpeningScreenDialog::action_openProjectDir);
}

void OpeningScreenDialog::closeEvent (QCloseEvent *event)
{
    Q_UNUSED(event)
    if (clearToClose){
        // done; form has validated
        return;
    } else {
        //Close Scribe entirely.
        emit closeProgram();
    }
}

void OpeningScreenDialog::frmInit(bool hasSetting, QString settingFile)
{
    ui->rdoRecent->setEnabled(hasSetting);
    if(hasSetting){
        scribeSettings = new QSettings(settingFile, QSettings::IniFormat);
    }
}



void OpeningScreenDialog::clearControls()
{
    foreach(QLineEdit* le, this->findChildren<QLineEdit*>()){
        le->clear();
    }

    foreach(QComboBox* cmb, this->findChildren<QComboBox*>()){
        cmb->setCurrentIndex(0);
    }
}

void OpeningScreenDialog::setSystemStatusNew(const bool &boolStatus)
{
    m_statusNew = boolStatus;
}

bool OpeningScreenDialog::getSystemStatusNew() const
{
    return m_statusNew;
}

void OpeningScreenDialog::refreshControls()
{
    clearControls();
    if(ui->rdoNew->isChecked()){
        ui->rdoNo->setChecked(true);
        //on_rdoNo_clicked();
    }else{
        ui->rdoYes->setChecked(true);
        //on_rdoYes_clicked();
    }
}

void OpeningScreenDialog::firstLoadDone(const bool &hasFlightData, const QString &existingFlightDataPath)
{
    clearToClose = true;

    //GPS Lock
    emit widgetStatus(status);
    emit secondLoadReady(hasFlightData, existingFlightDataPath);
}

/**
 * @brief OpeningScreenDialog::~OpeningScreenDialog
 * Destructor
 */
OpeningScreenDialog::~OpeningScreenDialog()
{
    delete ui;
}

/**
 * @brief OpeningScreenDialog::on_saveButton_clicked
 * @see GlobalSettings
 * Validates data and adds it to Global Settings
 */
void OpeningScreenDialog::on_saveButton_clicked()
{
    clearToClose = false;
    bool hasError = false;

    MessageDialog msgBox;

    //Check for valid Project Directory
    if(ui->projectDirVal->text().length() < 1){
         msgBox.setText("You must specify a Project Directory.");
         //ui->chooseProjectDir->setFocus();
         hasError = true;
    }

    //Check for valid Initials
    else if(ui->observerInitials->text().length() < 2){
         msgBox.setText("You must provide Observer's Full Initials.");
         ui->observerInitials->setFocus();
         hasError = true;
    }

    //Check for valid Export File Name
    else if(ui->exportFilename->text().length() < 3){
         msgBox.setText("You must provide an Export File Name.");
         ui->exportFilename->setFocus();
         hasError = true;
    }

    //Check that a Survey JSON file was associatedif needed
    if( ui->rdoYes->isChecked() && ui->geoFileText->text().length() < 1){
        msgBox.setText("You must specify a GEO JSON file.");
        //ui->chooseGeoFile->setFocus();
        hasError = true;
    }

    if(hasError){
        clearToClose = true;
        msgBox.exec();
        return;
    }

    //Check for current Survey
    if(!currentSurvey().isEmpty() &&
            currentSurvey() != ui->surveyType->currentText().toUpper() + ".surv") {
        msgBox.setText("Project directory has different\n"
                       "survey type to currently choose.\n"
                       "Do you still want to use?");
        msgBox.setTwoButton("Yes", "No, do not use");
        msgBox.resizeHeight(116);
        msgBox.exec();
        if (!msgBox.isAccept())
            return;
    }

    WorkerThread *workerThread = new WorkerThread();

    workerThread->setProjectFile(proj);
    workerThread->setSurveyType(ui->surveyType->currentText().toUpper());
    workerThread->setSurveyIndex(ui->surveyType->currentIndex());
    workerThread->setProjectDirVal(ui->projectDirVal->text());
    workerThread->setObserverInitials(ui->observerInitials->text());
    workerThread->setObserverSeat(ui->observerSeat->currentText());
    workerThread->setExportFilename(ui->exportFilename->text());
    workerThread->setAutoUnit(autoUnit);
    workerThread->setGeoFileText(ui->geoFileText->text());
    workerThread->setAirGround(ui->geoFileText2->text());
    workerThread->setSurveyFileName(currentSurvey());//added 8.12.2020

    connect(workerThread, &WorkerThread::firstLoadReady, this, &OpeningScreenDialog::firstLoadDone);
    connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);

    connect(workerThread, &WorkerThread::load, this, [=](){

        progMessage->setValue(10);

    });

    connect(workerThread, &WorkerThread::loads, this, [=](){

        progMessage->setValue(28);

    });

    connect(workerThread, &WorkerThread::load1, this, [=](){

        progMessage->setValue(43);

    });

    connect(workerThread, &WorkerThread::load2, this, [=]() {

        progMessage->setValue(49);

    });

    connect(workerThread, &WorkerThread::load3, this, [=]() {

        progMessage->setValue(60);

    });

    workerThread->start();

    this->hide();

    if(ui->rdoRecent->isChecked())
        setSystemStatusNew(false);
    else if(ui->rdoNew->isChecked())
        setSystemStatusNew(true);

    progMessage = new progressmessage();
    progMessage->setMax(160);
    progMessage->show();

    /*GlobalSettings &globalSettings = GlobalSettings::getInstance();
    globalSettings.set("surveyType", ui->surveyType->currentText());
    globalSettings.set("projectDirectory", ui->projectDirVal->text());
    globalSettings.set("observerInitials", ui->observerInitials->text());
    //globalSettings.set("observerSeat", ui->observerSeat->currentText());
    //Check if Eagle survery
    switch(ui->surveyType->currentIndex()) {
        //Eagle
        case 1:
        case 2: {
            globalSettings.set("observerSeat", "");
            break;
        }
        default: {
            globalSettings.set("observerSeat", ui->observerSeat->currentText());
            break;
        }
    }

    globalSettings.set("exportFilename", ui->exportFilename->text());
    globalSettings.set("autoUnit", autoUnit);
    globalSettings.set("geoFile", ui->geoFileText->text());
    globalSettings.set("exportCount", "0" );

    if(ui->surveyType->currentIndex() == 0)
        globalSettings.set("airGroundFile", ui->geoFileText2->text());

    if(globalSettings.get("surveyType") != "Other"){
        // Try to copy the base survey file into the directory. If it already exists, load it.
        QString filename = "", jsonFile = "";
        if(globalSettings.get("surveyType") == QLatin1String("WBPHS") ){
            filename = ":/json/WBPHS.0411.surv";
            //jsonFile = ":/json/WBPHS.airGround.surv";
            //addJsonFile(filename, jsonFile);
        }else{
            filename =":/json/" +  globalSettings.get("surveyType") + ".surv";
        }

        addJsonFile(fileName);

        QDir survey_file(filename);

        if( !QFile::copy(survey_file.absolutePath(), ui->projectDirVal->text() + "/" + globalSettings.get("surveyType") +  ".surv") ){
            //Look for auto saves.  If there are auto saves, load the most recent auto save
            if(proj->HasAutoSave()){
                qDebug() << proj->getMostRecentAutoSaveFile();
                if (proj->Load(  proj->getMostRecentAutoSaveFile() )){
                    qDebug() << "loaded save file successfully";
                }

            }else{
                if (proj->Load(  ui->projectDirVal->text() + "/" +  globalSettings.get("surveyType") + ".surv" )){
                    qDebug() << "loaded existing survey successfully";
                }
            }
        }else{
            if (proj->Load(ui->projectDirVal->text() + "/" + globalSettings.get("surveyType") + ".surv")){
                qDebug() << "load copy of survey file into project directory successfully at " <<
                            ui->projectDirVal->text() + "/" + globalSettings.get("surveyType") + ".surv";
            }
        }
    }
    //-- end of import survey


    //Import Survey JSON File
    if(ui->geoFileText->text().length() > 0){
        QFile fil (ui->geoFileText->text());
        if (!fil.open(QFile::ReadOnly)){
            qDebug()<< "Could not Opens Survey JSON file : " << ui->geoFileText->text();
        }else{
            QJsonParseError err;
            QByteArray data = fil.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data,&err);
            fil.close();

            if (!doc.isObject()){
                qDebug() << "Opened Survey JSON file is not a valid JSON document.";
            }else{
                //qDebug() << "A: " << ui->geoFileText->text();
                QJsonObject obj = doc.object();
                //Put the JSON object into settings.
                proj->geoJSON = obj;
            }
        }
    }
    //End import Survey JSON file


    //Import Air Ground File
    if(ui->surveyType->currentIndex() == 0 &&
            ui->geoFileText2->text().length() > 0) {
        QFile air(ui->geoFileText2->text());
        if (!air.open(QFile::ReadOnly)) {
            qDebug()<< "Could not Opens Air Ground JSON file : " << ui->geoFileText2->text();
        }
        else {
            QJsonParseError err;
            QByteArray data = air.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data,&err);
            air.close();

            if (!doc.isObject()){
                qDebug() << "Opened Air Ground JSON file is not a valid JSON document.";
            }else{
                QJsonObject obj = doc.object();
                //Put the JSON object into settings.
                proj->airGround = obj;
            }
        }
    }
    //End import Air Ground File


    //--Check for existing flight data in project directory, and import
    bool hasFlightData = false;
    QString existingFlightDataPath("");
    QStringList all_dirs;
    all_dirs <<  ui->projectDirVal->text();
    QDirIterator directories( ui->projectDirVal->text(), QDir::Dirs | QDir::NoDotAndDotDot);

    while(directories.hasNext()){
        directories.next();
        all_dirs << directories.filePath();
         if(directories.filePath().contains( "FlightData_") & !hasFlightData ){
            hasFlightData = true;
            existingFlightDataPath = directories.filePath();
         }
    }
    //--end of existing flight data check

    emit CreateExportFile();
    emit PopulateHeaderFields();
    emit widgetStatus(status);
    if(hasFlightData){
        emit ImportFlightData( existingFlightDataPath, false);
    }

    clearToClose = true;
    close();*/
}


/**
 * @brief OpeningScreenDialog::addJsonFile
 * @param survey
 */
void OpeningScreenDialog::addJsonFile(const QString &survey)
{
    const QString &surveyType = ui->surveyType->currentText() + ".surv";
    QDir survey_file(survey);
    //qDebug() << "surveyType" << surveyType;
    //qDebug() << "survey" << survey;

    if(!QFile::copy(survey_file.absolutePath(), ui->projectDirVal->text() + "/" + surveyType) ){
        //Look for auto saves.  If there are auto saves, load the most recent auto save
        if(proj->HasAutoSave()){
            if (proj->Load(proj->getMostRecentAutoSaveFile() )){
                qDebug() << "loaded save file successfully";
            }
        }else{
            if (proj->Load(ui->projectDirVal->text() + "/" + surveyType)){
                qDebug() << "loaded existing survey successfully";
            }
        }
    }else{
        if (proj->Load(ui->projectDirVal->text() + "/" + surveyType)){
            qDebug() << "load copy of survey file into project directory successfully at " <<
                        ui->projectDirVal->text() + "/" + surveyType;
        }
    }

}

/**
 * @brief OpeningScreenDialog::addJsonFile
 * @param survey
 * @param airGround
 */
void OpeningScreenDialog::addJsonFile(const QString &survey, const QString airGround)
{
    //qDebug() << survey;
    //qDebug() << airGround;
    const QString &airType = ui->surveyType->currentText() + ".AIRGROUND.surv";
    //Add Survey Type
    addJsonFile(survey);

    //Add Air Ground
    QDir survey_file(airGround);
    if(!QFile::copy(survey_file.absolutePath(), ui->projectDirVal->text() + "/" + airType) ){
        //Look for auto saves.  If there are auto saves, load the most recent auto save
        if(proj->HasAutoSave()){
            if (proj->Load(proj->getMostRecentAutoSaveFile() )){
                qDebug() << "loaded save file successfully";
            }
        }else{
            if (proj->Load(ui->projectDirVal->text() + "/" + airType)){
                qDebug() << "loaded existing survey successfully";
            }
        }
    }else{
        //qDebug() << "test";
        if (proj->Load(ui->projectDirVal->text() + "/" + airType)){
            qDebug() << "load copy of survey file into project directory successfully at " <<
                        ui->projectDirVal->text() + "/" + airType;
        }
    }
}

void OpeningScreenDialog::on_observerInitials_textChanged(const QString &arg1)
{
    QString s = arg1;
    s = s.remove(QRegularExpression("[^A-Za-z]"));
    //ui->observerInitials->setText(s.toUpper());
    ui->observerInitials->setText(s.toLower());
}

void OpeningScreenDialog::on_exportFilename_textChanged(const QString &arg1)
{
    QString s = arg1;
    //s = s.remove(QRegularExpression(""));
    s = s.remove(QRegularExpression("[^A-Za-z_0-9\\-]"));
    //ui->exportFilename->setText(s);
    ui->exportFilename->setText(s.toLower());
    if(s.length() > 0){
        saveTextCursor = saveTextCursor + (s.length() - saveTextValueLength);
        saveTextValueLength = s.length();
        ui->exportFilename->setCursorPosition(saveTextCursor);
    }
}

void OpeningScreenDialog::on_exportFilename_cursorPositionChanged(int arg1, int arg2)
{
    if(saveTextCursor == 0 && saveTextValueLength != ui->exportFilename->text().length()){
        saveTextCursor = arg1 + (ui->exportFilename->text().length() - saveTextValueLength);
        saveTextValueLength = ui->exportFilename->text().length();
    }else{
        if(saveTextValueLength != ui->exportFilename->text().length()){
            ui->exportFilename->setCursorPosition(saveTextCursor);
        }else{
            saveTextValueLength = ui->exportFilename->text().length();
            saveTextCursor = arg2;
        }
    }
}

/*void OpeningScreenDialog::on_chooseProjectDir_clicked()
{
    QString defaultDir = QDir::homePath();
    if( ui->projectDirVal->text().length() > 2  ){
        defaultDir = ui->projectDirVal->text();
    }
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Project Directory"),
                                                defaultDir,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    if(dir.length() > 0){
        ui->projectDirVal->setText(dir);
    }
}*/

/*void OpeningScreenDialog::on_autoUnitChoice_currentIndexChanged(const QString &arg1)
{
    if(arg1 == QLatin1String("No")){
        //ui->chooseGeoFile->setEnabled(0);
        ui->geoFileText->setText("");
        surveryEvent = nullptr;
        ui->geoFileText->setCursor(Qt::ArrowCursor);
        //Check if WBPHS
        if(ui->surveyType->currentIndex() == 0) {
            ui->geoFileText2->setText("");
            airGroundEvent = nullptr;
            ui->geoFileText2->setCursor(Qt::ArrowCursor);
        }

    }else{
        surveryEvent = new CustomEvent(ui->geoFileText, 1);
        ui->geoFileText->setCursor(Qt::PointingHandCursor);
        //ui->chooseGeoFile->setEnabled(1);
        //Check if WBPHS
        if(ui->surveyType->currentIndex() == 0) {
            airGroundEvent = new CustomEvent(ui->geoFileText2, 2);
            ui->geoFileText2->setCursor(Qt::PointingHandCursor);
        }
    }

}

void OpeningScreenDialog::on_chooseGeoFile_clicked()
{
    QString defaultDir = QDir::homePath();
    if(ui->projectDirVal->text().length() > 2)
        defaultDir = ui->projectDirVal->text();

    const QString &filter = "Survey JSON (*.geojson)";
    QFileDialog *fd = new QFileDialog();

    connect(fd, &QFileDialog::fileSelected, this, &OpeningScreenDialog::on_selectedAccepted);

    fd->setDirectory(defaultDir);
    fd->setWindowTitle("Select Survey JSON");
    fd->setNameFilter(filter);
    fd->exec();
}

void OpeningScreenDialog::on_selectedAccepted(const QString &file)
{
    QDir directory(file);
    ui->geoFileText->setText(directory.path());
}
*/

/*
void OpeningScreenDialog::on_selectedAccepted(const QString &file)
{
    //Get Current Surv file.
    QString currentFileSurv;
    QDirIterator survFileIterator(ui->projectDirVal->text(), QStringList() << "*.surv", QDir::Files, QDirIterator::Subdirectories);

    while (survFileIterator.hasNext()) {
        const QString &currentFilePath = survFileIterator.next();
        const QFileInfo &currentFile(currentFilePath);
        currentFileSurv = currentFile.fileName();
    }

    qDebug() << ui->surveyType->currentText();

    if(currentFileSurv == ui->surveyType->currentText() + ".surv") {
        QDir directory(file);
        ui->geoFileText->setText(directory.path());
    }
    else {
        const int &reply = QMessageBox::warning(this, tr("Scribe"),
                                                tr("Current surv file is different to new surv file. \n"
                                                   "Do you want still to continue your process?"),
                                                QMessageBox::Yes | QMessageBox::No
                                                );
        if (reply == QMessageBox::Yes) {
            QDir directory(file);
            ui->geoFileText->setText(directory.path());
        }
        else
            return;
    }

}
*/
/**
 * @brief OpeningScreenDialog::on_surveyType_currentIndexChanged
 * @param index
 * Validates if Eagle or not
 */
void OpeningScreenDialog::on_surveyType_currentIndexChanged(int index)
{
    switch(index) {
        case 0: {
            selectSurveyWBPHS();
            break;
        }
        case 1:
        case 2: {
            selectSurveyEagle();
            break;
        }
        default: {
            selectSurveyOther();
            break;
        }
    }
}

/**
 * @brief OpeningScreenDialog::on_chooseGeoFile2_clicked
 * File picker for Air Ground JSON

void OpeningScreenDialog::on_chooseGeoFile2_clicked()
{
    QString defaultDir = QDir::homePath();
    if(ui->projectDirVal->text().length() > 2)
        defaultDir = ui->projectDirVal->text();

    const QString &filter = "Air Ground JSON (*.geojson)";
    QFileDialog *fd = new QFileDialog();

    connect(fd, &QFileDialog::fileSelected, this, &OpeningScreenDialog::on_selectedGeoFile2);

    fd->setDirectory(defaultDir);
    fd->setWindowTitle("Select Air Ground JSON");
    fd->setNameFilter(filter);
    fd->exec();
}
*/

/**
 * @brief OpeningScreenDialog::on_selectedGeoFile2
 * @param file
 * run the function after select from file picker

void OpeningScreenDialog::on_selectedGeoFile2(const QString &file)
{
    QDir directory(file);
    ui->geoFileText2->setText(directory.path());
}
*/
/**
 * @brief OpeningScreenDialog::selectSurveyWBPHS
*/
void OpeningScreenDialog::selectSurveyWBPHS()
{
    ui->geoFileText2->setVisible(true);
    ui->lblGeo2->setVisible(true);
    ui->observerInitials->setMaxLength(3);
    ui->observerSeat->setVisible(true);
    ui->label_3->setText("Obs. Initials / Seat:");
    ui->observerInitials->setPlaceholderText("Max 3 Letters");
    //GPS Lock Visibility
    status = false;
}

/**
 * @brief OpeningScreenDialog::selectSurveyEagle
 */
void OpeningScreenDialog::selectSurveyEagle()
{
    ui->geoFileText2->setVisible(false);
    ui->lblGeo2->setVisible(false);
    ui->observerInitials->setMaxLength(5);
    ui->observerSeat->setVisible(false);
    ui->label_3->setText("Obs. Initials:");
    ui->observerInitials->setPlaceholderText("Max 5 Letters");
    ui->geoFileText2->setText("");
    //GPS Lock Visibility
    status = true;
}

/**
 * @brief OpeningScreenDialog::selectSurveyOther
 */
void OpeningScreenDialog::selectSurveyOther()
{
    ui->geoFileText2->setVisible(false);
    ui->lblGeo2->setVisible(false);
    ui->observerInitials->setMaxLength(3);
    ui->observerSeat->setVisible(true);
    ui->label_3->setText("Obs. Initials / Seat:");
    ui->observerInitials->setPlaceholderText("Max 3 Letters");
    ui->geoFileText2->setText("");
    //GPS Lock Visibility
    status = true;
}

void OpeningScreenDialog::directoryText(const bool &status)
{
    //If enable
    if (status) {
        ui->geoFileText->setStyleSheet("");
        ui->geoFileText2->setStyleSheet("");
        ui->geoFileText->setPlaceholderText("Choose");
        ui->geoFileText2->setPlaceholderText("Choose");
    }
    else {
        ui->geoFileText->setStyleSheet("QLineEdit { background-color:  rgb(225, 225, 225); }");
        ui->geoFileText2->setStyleSheet("QLineEdit { background-color:  rgb(225, 225, 225); }");
        ui->geoFileText->setPlaceholderText("");
        ui->geoFileText2->setPlaceholderText("");
    }
}

QString OpeningScreenDialog::currentSurvey()
{
    QStringList nameFilter("*.surv");
    QDir directory(ui->projectDirVal->text());
    const QStringList &filesAndDirectories = directory.entryList(nameFilter);

    foreach (QString chr, filesAndDirectories) {
        return chr;
    }

    return "";
}

void OpeningScreenDialog::on_rdoYes_clicked()
{
    autoUnit = "yes";
    //Add Mouse DblClick event
    surveryEvent = new CustomEvent(ui->geoFileText, 1);
    ui->geoFileText->setCursor(Qt::PointingHandCursor);
    //Check if WBPHS
    if(ui->surveyType->currentIndex() == 0) {
        //Add Mouse DblClick event
        airGroundEvent = new CustomEvent(ui->geoFileText2, 2);
        ui->geoFileText2->setCursor(Qt::PointingHandCursor);
    }
    //Enable
    directoryText(true);
}

void OpeningScreenDialog::on_rdoNo_clicked()
{
    try{
        autoUnit = "no";
        ui->geoFileText->setText("");
        //Remove Mouse DblClick event
        surveryEvent->removeFilter();
        ui->geoFileText->setCursor(Qt::ArrowCursor);
        //Check if WBPHS
        if(ui->surveyType->currentIndex() == 0) {
            ui->geoFileText2->setText("");
            //Remove Mouse DblClick event
            airGroundEvent->removeFilter();
            ui->geoFileText2->setCursor(Qt::ArrowCursor);
        }
        //Disable
        directoryText(false);
    }catch(...){
        qDebug() << "something went wrong on the yes/no radio button";
    }
}

void OpeningScreenDialog::action_openProjectDir(bool checked)
{
    Q_UNUSED(checked)
    QString curDir = QDir::homePath();
    const QString &dirPath = "C:/Users/Public/Downloads";
    //Get current directory path
    QFile file(dirPath + "/projectDir");
    if (!file.open(QFile::ReadOnly)){

    }
    else {
        //qDebug() << file.readAll();
        curDir = file.readAll();
    }


    const QString &dir = QFileDialog::getExistingDirectory(this,
                                                    tr("Choose Project Directory"),
                                                    curDir,
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    if(dir.length() > 0){
        ui->projectDirVal->setText(dir);
            //Save new directory path
        QFile savedFile(dirPath + "/projectDir");
        if(!savedFile.open(QFile::WriteOnly)) {
            qDebug() << "Unable to read";
        }
        else {
            qDebug() << savedFile.write(dir.toUtf8());
            savedFile.close();
        }
    }
    else {
        qDebug() << "Error";
    }
}

void OpeningScreenDialog::on_rdoRecent_clicked()
{
    //qDebug() << "autounit: " << scribeSettings->value("RecentOpeningScreen/autoUnit").toString();
    if(scribeSettings->value("RecentOpeningScreen/autoUnit").toString() == ""){
        MessageDialog msgBox;
        msgBox.setWindowTitle("Warning");
        msgBox.setText("Your scribe settings information is invalid.");
        msgBox.exec();

        clearControls();
        ui->rdoNew->setChecked(true);
        ui->rdoNo->setChecked(true);
        ui->surveyType->setFocus();
        return;
    }else if(scribeSettings->value("RecentOpeningScreen/autoUnit").toString() == "yes"){
        ui->rdoYes->setChecked(true);
        on_rdoYes_clicked();
    }else{
        ui->rdoNo->setChecked(true);
        on_rdoNo_clicked();
    }
    ui->surveyType->setCurrentText(scribeSettings->value("RecentOpeningScreen/surveyType").toString());
    ui->exportFilename->setText(scribeSettings->value("RecentOpeningScreen/saveFilename").toString());
    ui->projectDirVal->setText(scribeSettings->value("RecentOpeningScreen/folderDirectory").toString());
    ui->observerInitials->setText(scribeSettings->value("RecentOpeningScreen/obsInit").toString());
    ui->observerSeat->setCurrentText(scribeSettings->value("RecentOpeningScreen/seat").toString());
    ui->geoFileText->setText(scribeSettings->value("RecentOpeningScreen/surveyJson").toString());
    ui->geoFileText2->setText(scribeSettings->value("RecentOpeningScreen/airGroundJson").toString());
}

void OpeningScreenDialog::on_rdoNew_clicked()
{
    clearControls();
    refreshControls();
}
