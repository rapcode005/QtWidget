#include "workerThreadSurvey.h"


void WorkerThreadSurvey::run()
{
    switch (getAction()) {
        case 0: { // Import Survey
            ImportSurvey();
            break;
        }
        case 1: { //Create Survey
            CreateSurvey();
            break;
        }
        case 2: { //Refresh Header
            refreshHeaderFields();
            emit refreshDone();
            break;
        }
        case 3: { //Distance Configuration after click save button
            if (getIsSurveyWBPHS()) {
                getProjectFile()->setObsMaxD(getDitanceConfig()->getObsMaxD());
                emit load1();
                getProjectFile()->setAirGroundMaxD(getDitanceConfig()->getAirMaxD());
                //getProjectFile()->setChanges(true);
                getProjectFile()->changesMade = true;
                emit updateProjectFile(getDitanceConfig()->getObsMaxD(), getDitanceConfig()->getAirMaxD());
            }
            else {
                getProjectFile()->setObsMaxD(getDitanceConfig()->getObsMaxD());
                emit load1();
                //getProjectFile()->setChanges(true);
                getProjectFile()->changesMade = true;
                emit updateProjectFile(getDitanceConfig()->getObsMaxD());
            }
            break;
        }
        case 4: { //PrepHeaderValue in Project Setting
            preHeaderValues();
            emit refreshDone();
            break;
        }
        case 5: { //PrepAdditionalValue in Project Setting
            preAdditionalValues();
            break;
        }
        case 6: { //Project Setting save header name
            saveHeaderNames();
            //emit saveHeaderNameDone();
            break;
        }
        case 7: {//Project Setting save additional name
            saveAddedNames();
            //emit saveAddedNameDone();
            break;
        }
        case 8:{//Project Setting save header values and retain values
            saveHeaderValueChk();
            break;
        }
        case 9: {//Project Setting save additional values and retain values
            saveAddedValueChk();
            break;
        }
        case 10: {//Auto Save
            AutoSave();
            emit saveDone();
            break;
        }
        case 11: {//Refresh Additional Fields
            refreshAdditionalFields();
            emit refreshDone();
            break;
        }
    }
}

ProjectFile *WorkerThreadSurvey::getProjectFile()
{
    return m_proj;
}

void WorkerThreadSurvey::setProjectFile(ProjectFile *proj)
{
    m_proj = proj;
}

int WorkerThreadSurvey::getAction() const
{
    return m_action;
}

void WorkerThreadSurvey::setAction(const int &newAction)
{
    m_action = newAction;
}

QString WorkerThreadSurvey::getSurveyFile() const
{
    return m_survey;
}

void WorkerThreadSurvey::setSurveyFile(const QString &survey)
{
    m_survey = survey;
}

QString WorkerThreadSurvey::getDir() const
{
    return m_dir;
}

void WorkerThreadSurvey::setDir(const QString &dir)
{
    m_dir = dir;
}

DistanceConfigObj *WorkerThreadSurvey::getDitanceConfig()
{
    return m_ditanceConfig;
}

void WorkerThreadSurvey::setDitanceConfig(DistanceConfigObj *dc)
{
    m_ditanceConfig = dc;
}

bool WorkerThreadSurvey::getIsSurveyWBPHS() const
{
    return m_isSurvey;
}

void WorkerThreadSurvey::setIsSurveyWBPHS(const bool &newV)
{
    m_isSurvey = newV;
}

int WorkerThreadSurvey::getHeaderCount() const
{
    return m_headerC;
}

void WorkerThreadSurvey::setHeaderCount(const int &newC)
{
    m_headerC = newC;
}

int WorkerThreadSurvey::getAddedCount() const
{
    return m_addedC;
}

void WorkerThreadSurvey::setAddedCount(const int &newC)
{
    m_addedC = newC;
}

void WorkerThreadSurvey::ImportSurvey()
{
    QDir directory(getSurveyFile());
    if (getProjectFile()->Load(directory)){
       qDebug() << "load successful";
    }
    emit load1();
    refreshHeaderFields();
    emit load2();
    refreshAdditionalFields();
    emit load3();
    emit importDone();
}

void WorkerThreadSurvey::CreateSurvey()
{
    const QString &nm = getDir() + "/" + getProjectFile()->getSurveyType() + ".surv";
    QDir dir(nm);
    emit load1();
    getProjectFile()->Save(dir,false);
    emit load2();
    emit createDone();
}

void WorkerThreadSurvey::refreshHeaderFields()
{
    for (int y = 0; y < getProjectFile()->getHeaderCount(); y++) {
        //First Column
        QTableWidgetItem *item = new QTableWidgetItem();//make first column readonly added 6.4.2020
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);//make first column readonly added 6.4.2020
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        item->setData(Qt::FontRole,QVariant(getProjectFile()->getFontItem())); //added 5.25.2020
        item->setText(getProjectFile()->getHeaderName(y) + ": ");

        //Values
        const QJsonArray &vals = getProjectFile()->headerValuesArray[y].toArray();
        const QString &headerKey = getProjectFile()->getHeaderName(y);

        if(vals.count() == 1){ //If only one val, make it an editable text box

            //if there are same fields choose the second if there is no value for the first one
            if (getProjectFile()->allHeaderV.contains(headerKey)) {
                emit sendHeaderFieldsOneValNoAddded(item, y);
            }
            else {
                emit sendHeaderFieldsOneVal(item, y);
            }

        }else if (vals.count() >= 1) { //Multiple values. Make it a select box
            emit sendHeaderFieldsValues(item, y);
        }else{ //Empty but editable.
            emit sendHeaderFieldsNoVal(item, y);
        }
    }
}

void WorkerThreadSurvey::refreshAdditionalFields()
{
    const int &rCount = getProjectFile()->addionalFieldsNamesArray.count();
    for (int y = 0; y < rCount; y++) {
        //First Column
        QTableWidgetItem *item = new QTableWidgetItem();//make first column readonly added 6.4.2020
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);//make first column readonly added 6.4.2020
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        item->setData(Qt::FontRole,QVariant(getProjectFile()->getFontItem())); //added 5.25.2020
        item->setText(getProjectFile()->addionalFieldsNamesArray[y].toString() + ": ");

        //Values
        const QJsonArray &vals = getProjectFile()->addionalFieldsValuesArray[y].toArray();
        const int &count = vals.count();

        if (count == 1){
            emit sendAdditionalFieldsOneVal(item, y);
        }
        else if(count > 1) {
            emit sendAdditionalFieldsValues(item, y);
        }
        else
            emit sendAdditionalFieldsNoVal(item, y);
    }
}

void WorkerThreadSurvey::saveHeaderNames()
{
    for (int y = 0; y < getHeaderCount(); y++){
        emit saveHeaderNameLoop(y);
    }
    emit saveHeaderNameDone();
}

void WorkerThreadSurvey::saveAddedNames()
{
    for (int y = 0; y < getAddedCount(); y++){
        emit saveAddedNameLoop(y);
    }
    emit saveAddedNameDone();
}

void WorkerThreadSurvey::saveHeaderValueChk()
{
    //Set the header values
    for (int y = 0; y < getHeaderCount(); y++){
        emit saveHeaderValueChkLoop(y);
    }
    emit saveHeaderValueChkDone();
}

void WorkerThreadSurvey::saveAddedValueChk()
{
    //Set the additional values
    for (int y = 0; y < getAddedCount(); y++){
        emit saveAddedValueChkLoop(y);
    }
    emit saveAddedValueChkDone();
}

void WorkerThreadSurvey::preHeaderValues()
{
    //Text Edit
    //getProjectFile()->headerFieldsChk.clear();
    //ComboBox
    //getProjectFile()->hAFieldsCb.clear();

    int currentRow = 0;
    for (int y = 0; y < getProjectFile()->getHeaderCount(); y++){
        const QString &str = getProjectFile()->getHeaderName(y);
        const QStringList &vals = getProjectFile()->getHeaderValueList(y);
        const QStringList &valchk = getProjectFile()->getHeaderValueChk2(y);
        const bool &isinstanceHeader = getProjectFile()->getInstanceHeaderPosition(str) != -1;

        if(vals.count() == 0){
            QTableWidgetItem *item1 = new QTableWidgetItem(),
                    *item2 = new QTableWidgetItem();

            item1->setText(str);
            if (isinstanceHeader)
                item2->setText(getProjectFile()->getInstanceHeaderDataValue(str));

            if (valchk.count() >= 1 && valchk[0].toInt() != 2)
                getProjectFile()->headerFieldsChk.append(str);

            emit preHeaderValue(22, valchk[0].toInt(), currentRow, item1, item2);

            currentRow++;
        }
        else{
            for (int v = 0; v < vals.count(); v++){
                QTableWidgetItem *item1 = new QTableWidgetItem(),
                        *item2 = new QTableWidgetItem();

                item1->setText(str);
                item2->setText(vals[v]);

                if (valchk[0].toInt() != 2) {
                    if (!getProjectFile()->hAFieldsCb.contains(str))
                        getProjectFile()->hAFieldsCb << str;
                }

                emit preHeaderValue(22, valchk[0].toInt(), currentRow, item1, item2);

                currentRow++;
            }
        }
    }
}

void WorkerThreadSurvey::preAdditionalValues()
{
    //Text Edit
    //getProjectFile()->additionalFieldsChk.clear();
    //ComboBox
    //getProjectFile()->aFieldsCb.clear();

    //The additional fields names and values for the display widget are set here.
    int currentRow = 0;
    for (int y = 0; y < getProjectFile()->getAdditionalFieldsCount(); y++){
        const QString &str = getProjectFile()->getAdditionalFieldName(y);
        const QStringList &vals = getProjectFile()->getAdditionalFieldValueList(y);
        const QStringList &valchk = getProjectFile()->getAdditionalFieldChk2(y);

        if(vals.count() <= 1){
            QTableWidgetItem *item1 = new QTableWidgetItem(),
                    *item2 = new QTableWidgetItem();

            item1->setText(str);

            if (valchk[0].toInt() != 2)
                getProjectFile()->additionalFieldsChk.append(str);

            emit preAddedValue(22, valchk[0].toInt(), currentRow, item1, item2,
                    y);

            currentRow ++;
        }
        else {
            for (int v = 0; v < vals.count(); v++){
                QTableWidgetItem *item1 = new QTableWidgetItem(),
                    *item2 = new QTableWidgetItem();

                item1->setText(str);
                item2->setText(vals[v]);

                if (valchk[0].toInt() != 2) {
                    if (!getProjectFile()->aFieldsCb.contains(str))
                        getProjectFile()->aFieldsCb << str;
                }

                emit preAddedValue(22, valchk[0].toInt(), currentRow, item1, item2,
                        y);

                currentRow ++;
            }
        }
    }

    //Use signal to check if saving setting done.
    emit saveSettingDone();
}


void WorkerThreadSurvey::AutoSave()
{
    if (getProjectFile()->changesMade) {
        GlobalSettings &globalSettings = GlobalSettings::getInstance();
        QString projectDirectory = globalSettings.get("projectDirectory");

        if (projectDirectory == nullptr || projectDirectory.isEmpty()){
            qDebug() << "Project directory not set. Aborting autosave.";
            emit unexpectedFinish();
            return;
        }

        QString autosaveDirectory = projectDirectory + "/AutoSaves";

        if (!QDir(autosaveDirectory).exists()) {
            QDir dir(projectDirectory);
            dir.mkdir("./AutoSaves");
        }

        emit load1();

        QDirIterator it(QDir(autosaveDirectory).absolutePath(), QDir::Files, QDirIterator::Subdirectories);
        QStringList taggedFiles;

        while (it.hasNext()) {
            if (it.filePath().endsWith("_autosave.sproj")){
             taggedFiles << it.filePath();
            }
            it.next();
        }

        taggedFiles.sort();

        emit load2();

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

        getProjectFile()->Save(autosaveFile, true);

        emit load3();
    }
    else {
        emit unexpectedFinish();
    }
}
