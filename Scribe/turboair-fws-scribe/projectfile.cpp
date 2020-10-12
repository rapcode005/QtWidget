#include "projectfile.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit >
#include "globalsettings.h"
#include <QDirIterator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <algorithm>


ProjectFile::ProjectFile()
{
    instancehList = { "Year", "Month", "Day" ,"Observer", "Seat", "Distance"};//added distance 7.17.2020
    changesMade = false;
    //commentTxt = new QTextEdit();
}

bool ProjectFile::Save(QDir dir, bool checkChanges){
    if(checkChanges && !changesMade){
        return false;
    }else if(checkChanges){
        changesMade = false;
    }

    QJsonObject obj;

    //Write the standards
    buildStandardSave(obj);

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();
    QFile fil (dir.absolutePath());
    if (!fil.open(QFile::WriteOnly | QIODevice::Truncate)){
        return false;
    }

    fil.write(data);
    fil.close();
    fileName = fil.fileName();

    return true;
}
/*
void ProjectFile::buildStandardSave(QJsonObject &obj){

    //Groupings
    obj.insert("GroupingsNumber", addedGroupings.length());
    for (int t = 0; t < addedGroupings.length(); t++){
        //qDebug() << addedGroupings[t];
        obj.insert("GroupingsValue" + QString::number(t), addedGroupings[t]);
    }

    //Species
    obj.insert("SpeciesNumber", addedSpecies.length());
    for (int i = 0; i < addedSpecies.length(); i++){
        //qDebug() << addedSpecies[i];
        obj.insert("SpeciesValue" + QString::number(i), addedSpecies[i]);
    }

    QJsonObject h;
    h.insert("FieldNames",headerNamesArray);
    h.insert("FieldValues",headerValuesArray);
    h.insert("FieldValuesChecked",headerValuesChkArray);//added 4.22.2020
    obj.insert("Headers",h);

    QJsonObject a;
    a.insert("FieldNames",addionalFieldsNamesArray);
    a.insert("FieldValues",addionalFieldsValuesArray);
    a.insert("FieldValuesChecked",addionalFieldsValuesChkArray);//added 4.22.2020
    obj.insert("AdditionalFields",a);

    //Distance Configuration
    obj.insert("ObsMaxD", getObsMaxD());

    if(getSurveyType() == QLatin1String("WBPHS"))
        obj.insert("AirMaxD", getAirGroundMaxD());
}*/

void ProjectFile::buildStandardSave(QJsonObject &obj){

    QJsonObject a;
    a.insert("FieldNames",addionalFieldsNamesArray);
    a.insert("FieldValues",addionalFieldsValuesArray);
    a.insert("FieldValuesChecked",addionalFieldsValuesChkArray);//added 4.22.2020
    obj.insert("AdditionalFields",a);

    QJsonObject h;
    h.insert("FieldNames",headerNamesArray);
    h.insert("FieldValues",headerValuesArray);
    h.insert("FieldValuesChecked",headerValuesChkArray);//added 4.22.2020
    obj.insert("Headers",h);

    //Groupings
    obj.insert("GroupingsNumber", addedGroupings.length());
    for (int t = 0; t < addedGroupings.length(); t++){
        obj.insert("GroupingsValue" + QString::number(t), addedGroupings[t]);
    }

    //Species
    obj.insert("SpeciesNumber", addedSpecies.length());
    for (int i = 0; i < addedSpecies.length(); i++){
        obj.insert("SpeciesValue" + QString::number(i), addedSpecies[i]);
    }

    //Distance Configuration
    obj.insert("ObsMaxD", getObsMaxD());

    if(getSurveyType() == QLatin1String("WBPHS"))
        obj.insert("AirMaxD", getAirGroundMaxD());

    //If there is new Changes
    obj.insert("Changes", getChanges());

    //add conditionalfields added 8.13.2020
    //qDebug() << "getConditionalValueFieldsArraycount: " << getConditionalValueFieldsArray().count();
    if(getConditionalValueFieldsArray().count() > 0)
        obj.insert("ConditionalValues",getConditionalValueFieldsArray());

}

bool ProjectFile::HasAutoSave(){
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    const QString &projectDirectory = globalSettings.get("projectDirectory");

    if (projectDirectory == nullptr || projectDirectory.isEmpty()){
        return false;
    }

    const QString &autosaveDirectory = projectDirectory + "/AutoSaves";

    if (!QDir(autosaveDirectory).exists()) {
        QDir dir(projectDirectory);
        return false;
    }

    QDirIterator it(QDir(autosaveDirectory).absolutePath(), QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        if (it.filePath().endsWith("_autosave.sproj")){
            return true;
        }
        it.next();
    }

    return false;
}


/*bool ProjectFile::HasAutoSave(){
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    QString projectDirectory = globalSettings.get("projectDirectory");

    if (projectDirectory == nullptr || projectDirectory.isEmpty()){
        return false;
    }

    QString autosaveDirectory = projectDirectory + "/AutoSaves";

    if (!QDir(autosaveDirectory).exists()) {
        QDir dir(projectDirectory);
        return false;
    }

    QDirIterator it(QDir(autosaveDirectory).absolutePath(), QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        if (it.filePath().endsWith("_autosave.sproj")){
            return true;
        }
        it.next();
    }

    return false;
}*/

QString ProjectFile::getMostRecentAutoSaveFile(){
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    QString projectDirectory = globalSettings.get("projectDirectory");
    QString autosaveDirectory = projectDirectory + "/AutoSaves";
    QStringList taggedFiles;

    QDirIterator it(QDir(autosaveDirectory).absolutePath(), QStringList() << "*_autosave.sproj", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        taggedFiles << it.next();
    }

    taggedFiles.sort();
    return taggedFiles.last();
}

bool ProjectFile::Load(QDir dir){
    QFile fil (dir.absolutePath());
    if (!fil.open(QFile::ReadOnly)){
        qDebug()<< "Could not Open file : " << dir.absolutePath();
        return false;
    }
    QJsonParseError err;
    QByteArray data = fil.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data,&err);
    if (!doc.isObject()){
        qDebug() << "Opened file is not a valid doc.";
        return false;
    }

    fil.close();
    QJsonObject obj = doc.object();

    //Load headers from JSON
    QJsonValue headerJSONObject = obj.value(QString("Headers"));
    QJsonObject headerObject = headerJSONObject.toObject();
    headerNamesArray = headerObject["FieldNames"].toArray();
    headerValuesArray = headerObject["FieldValues"].toArray();
    headerValuesChkArray = headerObject["FieldValuesChecked"].toArray();//added 4.22.2020

    //Load additional fields from JSON
    QJsonValue additionalFieldsJSONObject = obj.value(QString("AdditionalFields"));
    QJsonObject additionalFieldsObject = additionalFieldsJSONObject.toObject();
    addionalFieldsNamesArray = additionalFieldsObject["FieldNames"].toArray();
    addionalFieldsValuesArray = additionalFieldsObject["FieldValues"].toArray();
    addionalFieldsValuesChkArray = additionalFieldsObject["FieldValuesChecked"].toArray();//added 4.22.2020

    //Load Species
    addedSpecies.clear();
    const int &surSpeciesCount = obj.value("SpeciesNumber").toInt();
    for (int i = 0; i < surSpeciesCount; i++){
        addedSpecies.append(obj.value("SpeciesValue" + QString::number(i)).toString());
    }

    //Load Groupings
    addedGroupings.clear();
    const int &surGroupingCount = obj.value("GroupingsNumber").toInt();
    for (int t = 0; t < surGroupingCount; t++){
        addedGroupings.append(obj.value("GroupingsValue" + QString::number(t)).toString());
    }

    //Load Distance Configuration
    QJsonValue obsMaxDJSONObject = obj.value("ObsMaxD");
    setObsMaxD(obsMaxDJSONObject.toDouble());

    if(getSurveyType() == QLatin1String("WBPHS")) {
        QJsonValue airMaxDJSONObject = obj.value("AirMaxD");
        setAirGroundMaxD(airMaxDJSONObject.toDouble());
    }

    //Load Changes
    QJsonValue obsChJSONObject = obj.value("Changes");
    setChanges(obsChJSONObject.toBool());

    //Load ConditionalValues //added 8.13.2020
    QJsonValue obsConditionalJSONObject = obj.value("ConditionalValues");
    QJsonArray conditionalValuesObjectArray =  obsConditionalJSONObject.toArray();
    setConditionalValueFields(conditionalValuesObjectArray);

    fileName = fil.fileName();
    emit Loaded();
    emit LoadedDistanceConfig();
    return true;
}

//Header Utility Functions******************************************************

int ProjectFile::getHeaderCount(){
    return headerNamesArray.count();
}

QString ProjectFile::getHeaderName(int idx){
    return headerNamesArray[idx].toString();
}

void ProjectFile::buildHeaderCell(QTableWidget *tbl, int idx){
    if (!(headerValuesArray.count() > idx)) {
        return;
    }
    QJsonArray vals = headerValuesArray[idx].toArray();
    QString headerKey = getHeaderName(idx);
    QString css = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#FFFFFF; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    QString cssreadonly = "font-size:  13px; "
                "font-family: 'Segoe UI'; "
                "background-color:#999999; "
                "margin:1px;"
                "border: 1px solid rgba(0, 0, 0, 0.2);"
                "padding-left:4px";

    if(vals.count() == 1){ //If only one val, make it an editable text box
        CustomEventWidget *s = new CustomEventWidget();
        s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        //s->setStyleSheet(css);
        s->setTitle(headerKey);
        s->setMinimumHeight(30);
        //Check fields which are readonly.
        if(getInstanceHeaderPosition(headerKey) != -1) {
            s->setReadOnly(true);
            s->setStyleSheet(cssreadonly);
        }else
            s->setStyleSheet(css);

        //if there are same fields choose the second if there is no value for the first one
        if (allHeaderV.contains(headerKey)) {
            //Get What fields it is
            const int &index = allHeaderV.indexOf(headerKey);
            const QString &val = vals[0].toString();
            headerFields.at(index)->setText(val);
        }
        else {
            //If not
            const QString &val = vals[0].toString();
            s->setText(val);

            headerFields.append(s);
            headerIndex.append(idx);
            tbl->setCellWidget(idx, 1, headerFields.at(countH));
            countH += 1;
            allHeaderV.append(headerKey);
        }

    }else if (vals.count() >= 1) { //Multiple values. Make it a select box
        CustomComboBox *myComboBox2  = new CustomComboBox();
        //QComboBox* myComboBox2 = new QComboBox();
        myComboBox2->setEditable(false);
        myComboBox2->addItem("");
        myComboBox2->setMinimumHeight(30);
        for (int i = 0; i < vals.count(); i++){
            myComboBox2->addItem(vals[i].toString());
        }
        myComboBox2->setStyleSheet(css);
        headerFieldsC.append(myComboBox2);
        headerIndexC.append(idx);
        //tbl->setCellWidget(idx,1,myComboBox2);
        tbl->setCellWidget(idx,1, headerFieldsC.at(countHC));
        countHC += 1;
    }else{ //Empty but editable.
        CustomEventWidget *s = new CustomEventWidget();
        s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        //s->setStyleSheet(css);
        s->setMinimumHeight(30);
        if(getInstanceHeaderPosition(headerKey) != -1) {
            s->setStyleSheet(cssreadonly);
            s->setReadOnly(true);
            s->setText(getInstanceHeaderDataValue(headerKey));
        }else
            s->setStyleSheet(css);
        s->setTitle(headerKey);

        headerFields.append(s);
        headerIndex.append(idx);
        tbl->setCellWidget(idx, 1, headerFields.at(countH));
        countH += 1;
        allHeaderV.append(headerKey);
    }
}

QString ProjectFile::getInstanceHeaderDataValue(QString headerName){
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    switch(getInstanceHeaderPosition(headerName)){
      case 0:
        // Year
        return globalSettings.get("Year");

      case 1:
        // Month
        return globalSettings.get("Month");

      case 2:
        //Day
        return globalSettings.get("Day");

      case 3:
      //Observer
        return globalSettings.get("observerInitials");

      case 4:
      //Seat
        return globalSettings.get("observerSeat");

      default:
        return "Not Found";
    }
}

int ProjectFile::getInstanceHeaderPosition(QString headerName){
    return instancehList.indexOf(headerName);
}

int ProjectFile::getHeaderIndex(QString headerName){
    if(headerNamesArray.contains(headerName)){
        for(int i=0; i<headerNamesArray.count(); i++){
            if(headerNamesArray[i].toString() == headerName){
                return i;
            }
        }
    }
    return -1;
}

bool ProjectFile::isHeaderSingleValue(int idx){
    QJsonArray vals = headerValuesArray[idx].toArray();
    if (vals.count() > 1) {
        return false;
    }
    return true;
}

QStringList ProjectFile::getHeaderNamesList(){
    QStringList rtn;
    for (int t = 0; t < headerNamesArray.count(); t++){
        rtn.append(headerNamesArray[t].toString());
    }
    return rtn;
}

QString ProjectFile::getHeaderValue(QTableWidget *tbl, int idx){
    if(isHeaderSingleValue(idx)){
        //return tbl->item(idx,1)->text();
        QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(tbl->cellWidget(idx,1));
        const QString &headerValueText = textEditItem->toPlainText();
        return headerValueText;
    }else{
        QComboBox *w = dynamic_cast<QComboBox*>(tbl->cellWidget(idx,1));
        return w->currentText();
    }
}

//Multi Line
QString ProjectFile::getHeaderValue(QTableWidget *tbl, int idx, QString title)
{
    Q_UNUSED(title)
    QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(tbl->cellWidget(idx,1));
    const QString &headerValueText = textEditItem->toPlainText();
    return headerValueText;

}

QStringList ProjectFile::getHeaderValueList(int idx){
    QJsonArray vals = headerValuesArray[idx].toArray();
    QStringList rtn;
    for (int y = 0; y < vals.count(); y++){
        rtn.append(vals[y].toString());
    }
    return rtn;
}

void ProjectFile::setHeaderNames(QJsonArray h){
    headerNamesArray = h;
}

void ProjectFile::setHeaderValues(QJsonArray h){
    headerValuesArray = h;
}

void ProjectFile::setHeaderValuesChk(QJsonArray h)
{
    headerValuesChkArray = h;
}

QString ProjectFile::getHeaderValueChk(int idx)
{
    return headerValuesChkArray[idx].toString();
}

QStringList ProjectFile::getHeaderValueChk2(int idx)//added 4.26.2020
{
    QJsonArray vals = headerValuesChkArray[idx].toArray();
    QStringList rtn;
    for (int y = 0; y < vals.count(); y++){
        rtn.append(vals[y].toString());
    }
    return rtn;
}

//Additional Fields Utility Functions********************************************

QStringList ProjectFile::getAdditionalFieldsNamesList(){
    QStringList rtn;
    for (int t = 0; t < addionalFieldsNamesArray.count(); t++){
        rtn.append(addionalFieldsNamesArray[t].toString());
    }
    return rtn;
}

void ProjectFile::buildAdditionalfieldsTable(QTableWidget *tbl){
    const int &rCount = addionalFieldsNamesArray.count();
    tbl->setRowCount(0); //Clearing it to rebuild
    tbl->setRowCount(rCount);
    tbl->setColumnCount(2);

    QHeaderView *verticalHeader = tbl->verticalHeader();
    //verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(30);

    tbl->setShowGrid(false);//added 5.7.2020
    //int intwidth = tbl->width() / 3;
    //tbl->setColumnWidth(0, 85);
    tbl->setColumnWidth(1, 180);
    additionalFields.clear();
    additionalIndex.clear();
    countA = 0;

    /*qDebug() << "A";
    tbl->setStyleSheet( "font-size: 13px;"
                        "font-family: 'Segoe UI'; border: 1px solid #D8D8D8; "
                        "background-color:#F7F7F7; border-top:none;");
    qDebug() << "B";*/
    for (int t = 0; t < rCount; t++){
        QTableWidgetItem *w = new QTableWidgetItem();//added 5.7.2020
        const QString &val = addionalFieldsNamesArray[t].toString();
        w->setFlags(w->flags() ^ Qt::ItemIsEditable);//set the first column read only
        w->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        tbl->setItem(t,0, w);
        tbl->setRowHeight(t, 22);
        //tbl->item(t,0)->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);//added5.7.2020
        QFont cellFont;//added 5.7.2020 unnecessary code
        cellFont.setFamily("Segoe UI"); //this will set the first column to font to bold
        cellFont.setPixelSize(13);//added5.7.2020
        cellFont.setBold(true);//added5.7.2020

        tbl->item(t,0)->setData(Qt::FontRole,QVariant(cellFont)); //added5.7.2020
        tbl->item(t,0)->setText(val + ": ");//1st column field updated 5.7.2020

        buildAdditionalFieldCell(tbl, t);//2nd column value
    }
}

void ProjectFile::buildAdditionalFieldCell(QTableWidget *tbl, int idx){
    if(!(addionalFieldsValuesArray.count() > idx)) {
        return;
    }
    const QJsonArray &vals = addionalFieldsValuesArray[idx].toArray();
    //QString additionalFieldsKey = getAdditionalFieldName(idx);

    QString css = "font-size:  13px;"
                  "font-family: 'Segoe UI';"
                  "background-color:#FFFFFF;"
                  "margin:1px;"
                  "border-style:solid;"
                  "border-width:1px;"
                  "border-color:rgba(0, 0, 0, 0.2);"
                  "padding-left:4px;";

    if(vals.count() == 1){ //If only one val, make it an editable text box
        CustomEventWidget *s = new CustomEventWidget();
        s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        s->setStyleSheet(css);
        s->setMinimumHeight(30);
        //s->setText(vals[0].toString());
        s->setText(vals[0].toString().toLower());

        additionalFields.append(s);
        additionalIndex.append(idx);
        tbl->setCellWidget(idx, 1, additionalFields.at(countA));
        countA = countA + 1;
    }else if (vals.count() >= 1) { //Multiple values. Make it a select box
        CustomComboBox* myComboBox2 = new CustomComboBox();
        myComboBox2->setStyleSheet(css);
        myComboBox2->setMinimumHeight(30);
        myComboBox2->setEditable(false);
        myComboBox2->addItem("");
        for (int i = 0; i < vals.count(); i ++){
            //myComboBox2->addItem(vals[i].toString());
            myComboBox2->addItem(vals[i].toString().toLower());
        }
        tbl->setCellWidget(idx,1,myComboBox2);
    }else{ //Empty but editable.
        CustomEventWidget *s = new CustomEventWidget();
        s->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        s->setStyleSheet(css);

        s->setMinimumHeight(30);
        additionalFields.append(s);
        additionalIndex.append(idx);
        tbl->setCellWidget(idx, 1, additionalFields.at(countA));
        countA += 1;
    }
}

QString ProjectFile::getAdditionalFieldName(int idx){
    return addionalFieldsNamesArray[idx].toString();
}

int ProjectFile::getAdditionalFieldsCount(){
    return addionalFieldsNamesArray.count();
}

bool ProjectFile::isAdditionalFieldSingleValue(int idx){
    const QJsonArray &vals = addionalFieldsValuesArray[idx].toArray();
    if (vals.count() > 1) {
        return false;
    }
    return true;
}

QString ProjectFile::getAdditionalFieldValue(QTableWidget *tbl, int idx){
    if(isAdditionalFieldSingleValue(idx)){
        QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(tbl->cellWidget(idx,1));
        const QString &additionalFieldValueText = textEditItem->toPlainText();
        return additionalFieldValueText;
    }else{
        QComboBox *w = dynamic_cast<QComboBox*>(tbl->cellWidget(idx,1));
        return w->currentText();
    }
}

//Multi-Line text such as comment field
QString ProjectFile::getAdditionalFieldValue(QTableWidget *tbl, int idx, QString title)
{
    Q_UNUSED(title)
    QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(tbl->cellWidget(idx,1));
    QString additionalFieldValueText = textEditItem->toPlainText();
    return additionalFieldValueText;
}

QStringList ProjectFile::getAdditionalFieldValueList(int idx){
    QJsonArray vals = addionalFieldsValuesArray[idx].toArray();
    QStringList rtn;
    for (int y = 0; y < vals.count(); y++){
        rtn.append(vals[y].toString());
    }
    return rtn;
}

void ProjectFile::setAdditionalFieldNames(QJsonArray h){
    addionalFieldsNamesArray = h;
}

void ProjectFile::setAdditionalFieldValues(QJsonArray h){
    addionalFieldsValuesArray = h;
}

void ProjectFile::setAdditionalFieldValuesChk(QJsonArray h)
{
    addionalFieldsValuesChkArray = h;
}

int ProjectFile::getAdditionalFieldIndex(QString additionalFieldName){
    if(addionalFieldsNamesArray.contains(additionalFieldName)){
        for(int i=0; i<addionalFieldsNamesArray.count(); i++){
            if(addionalFieldsNamesArray[i].toString() == additionalFieldName){
                return i;
            }
        }
    }
    return -1;
}

QString ProjectFile::getAdditionalFieldChk(int idx)
{
    return addionalFieldsValuesChkArray[idx].toString();
}

QStringList ProjectFile::getAdditionalFieldChk2(int idx)
{
    const QJsonArray &vals = addionalFieldsValuesChkArray[idx].toArray();
    QStringList rtn;
    for (int y = 0; y < vals.count(); y++){
        rtn.append(vals[y].toString());
    }
    return rtn;
}

// General Utility Functions
QString ProjectFile::getSurveyType(){
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    if (globalSettings.get("surveyType") == QLatin1String("other"))
        return "Other";
    else
        return globalSettings.get("surveyType").toUpper();
}


void ProjectFile::setCurMainWindows(QObject *newObj)
{
    m_curMainWindow = newObj;
    m_curMainWindow->installEventFilter(this);
}

//Distance Configuration
void ProjectFile::setObsMaxD(const double &newVal)
{
    m_obsMaxD = newVal;
}

double ProjectFile::getObsMaxD() const
{
    return m_obsMaxD;
}

void ProjectFile::setAirGroundMaxD(const double &newVal)
{
    m_airGroundMaxD = newVal;
}

double ProjectFile::getAirGroundMaxD() const
{
    return m_airGroundMaxD;
}

void ProjectFile::setConditionalValueFields(QJsonArray &newArray)
{
    m_conditionalvaluefieldsarray = newArray;
}

QJsonArray ProjectFile::getConditionalValueFieldsArray()
{
    return m_conditionalvaluefieldsarray;
}

void ProjectFile::loadGOEAProperties()
{
    QJsonObject jObj = geoJSON;
    QJsonObject::const_iterator i;
    m_bcr.clear();
    m_trn.clear();
    for (i = jObj.begin();  i != jObj.end(); ++i) {
        if(i.key().toLower() == QLatin1String("features")){
            QJsonArray jsonArray = i->toArray();
            foreach (const QJsonValue &value, jsonArray) {
                QJsonObject innerObject = value.toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); j++) {
                    if (j.key().toLower() == QLatin1String("properties")){
                        m_trn.append(j->toObject().value("Trn").toString());
                        m_bcr.append(j->toObject().value("BCR").toString());
                        m_bcrTrn.append(j->toObject().value("BCRTrn").toString());
                    }

                }
            }
        }
    }
}

QStringList ProjectFile::getTRN() const
{
     return m_trn;
}

QStringList ProjectFile::getBCR() const
{
    return m_bcr;
}

QStringList ProjectFile::getBCRTRN() const
{
    return m_bcrTrn;
}

void ProjectFile::loadWBPSProperties()
{
    QJsonObject jObj = geoJSON;
    QJsonObject::const_iterator i;
    m_wbps = new WBPS();
    for (i = jObj.begin();  i != jObj.end(); ++i) {
        if(i.key().toLower() == QLatin1String("features")){
            QJsonArray jsonArray = i->toArray();
            foreach (const QJsonValue &value, jsonArray) {
                QJsonObject innerObject = value.toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); j++) {
                    if (j.key().toLower() == QLatin1String("properties")){
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

WBPS *ProjectFile::getWBPS()
{
    return m_wbps;
}

void ProjectFile::loadAirGroundProperties()
{
    QJsonObject jObj = airGround;
    QJsonObject::const_iterator i;
    m_agName.clear();
    for (i = jObj.begin();  i != jObj.end(); ++i) {
        if(i.key().toLower() == QLatin1String("features")){
            QJsonArray jsonArray = i->toArray();
            foreach (const QJsonValue &value, jsonArray) {
                QJsonObject innerObject = value.toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); j++) {
                    if (j.key().toLower() == QLatin1String("properties")){
                        m_agName << j->toObject().value("agcode").toString();
                    }
                }
            }
        }
    }
}

QStringList ProjectFile::getAllAGNameList() const
{
    return m_agName;
}

void ProjectFile::loadPlotNameList()
{
    QJsonObject jObj = geoJSON;
    QJsonObject::const_iterator i;
    for (i = jObj.begin();  i != jObj.end(); ++i) {
        if(i.key().toLower() == QLatin1String("features")){
            QJsonArray jsonArray = i->toArray();
            foreach (const QJsonValue &value, jsonArray) {
                QJsonObject innerObject = value.toObject();
                QJsonObject::iterator j;
                for (j = innerObject.begin(); j != innerObject.end(); j++) {
                    if (j.key().toLower() == QLatin1String("properties")){
                        m_plotNameList <<  j->toObject().value("PlotLabel").toString();
                    }

                }
            }
        }
    }
}

QStringList ProjectFile::getAllPlotNameList() const
{
    return m_plotNameList;
}

QString ProjectFile::getTRNPre() const
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    return globalSettings.get("TRNPRe");
}

QString ProjectFile::getBCRPre() const
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    return globalSettings.get("BCRPRe");
}

QFont ProjectFile::getFontItem(bool bold)
{
    m_font.setFamily("Segoe UI"); //this will set the first column font to bold
    m_font.setPixelSize(13); //5.25.2020
    m_font.setBold(bold);// 5.25.2020
    return m_font;
}

//Use for first Load
void ProjectFile::loadChanges()
{
    QFile fil(getMostRecentAutoSaveFile());
    if (!fil.open(QFile::ReadOnly)){
        qDebug()<< "Could not Open file : " << fil;
        return;
    }
    QJsonParseError err;
    QByteArray data = fil.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data,&err);
    if (!doc.isObject()){
        qDebug() << "Opened file is not a valid doc.";
        return;
    }

    fil.close();
    QJsonObject obj = doc.object();
    QJsonValue obsChJSONObject = obj.value("Changes");
    if (!obsChJSONObject.isNull())
        m_changes = obsChJSONObject.toBool();
    else
        m_changes = false;
}

bool ProjectFile::getChanges() const
{
    return m_changes;
}

void ProjectFile::setChanges(const bool &newC)
{
    m_changes = newC;
}

QString ProjectFile::cssReadOnly() const
{
    return  "font-size:  13px;"
            "font-family: 'Segoe UI';"
            "margin:3px;"
            "border: none";
}
