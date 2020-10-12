#include "workerthread2nd.h"


void WorkerThread2nd::run()
{
    CreateExportFile();
    emit load1();
    PopulateHeaderFields();
    if (getHasFlightData()) {
        //qDebug() << "importing files";
        ImportFlightData(getFlightData(), false);
    }
    emit LoadReady(getProjectFile());
}

ProjectFile *WorkerThread2nd::getProjectFile()
{
    return m_proj;
}

void WorkerThread2nd::setProjectFile(ProjectFile *newProj)
{
    m_proj = newProj;
}

AudioPlayer *WorkerThread2nd::getAudioPlayer()
{
    return m_audio;
}

void WorkerThread2nd::setAudioPlayer(AudioPlayer *newAudio)
{
    m_audio = newAudio;
}

bool WorkerThread2nd::getHasFlightData() const
{
    return m_hasFlightData;
}

void WorkerThread2nd::setHasFlightData(const bool &newHas)
{
    m_hasFlightData = newHas;
}

QString WorkerThread2nd::getFlightData() const
{
    return m_flightData;
}

void WorkerThread2nd::setFlightData(const QString &newFlight)
{
    m_flightData = newFlight;
}

GpsHandler *WorkerThread2nd::getGPS()
{
    return m_gps;
}

void WorkerThread2nd::setGPS(GpsHandler *newGPS)
{
    m_gps = newGPS;
}


void WorkerThread2nd::PopulateHeaderFields()
{
    QString surveyValues = ",";

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
                emit sendHeaderFields(item, y, 0);
            }
            else {
                emit sendHeaderFields(item, y, 1);
            }

        }else if (vals.count() >= 1) { //Multiple values. Make it a select box
            emit sendHeaderFields(item, y, 2);
        }else{ //Empty but editable.
            emit sendHeaderFields(item, y, 3);
        }
    }
}

void WorkerThread2nd::CreateExportFile()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    const QString &exportLocation = globalSettings.get("projectDirectory") + "/" + globalSettings.get("exportFilename") + ".asc";
    QFile exportFile(exportLocation);
    const int &export_count = globalSettings.get("exportCount").toInt();
    if( export_count > 0 ||  exportFile.size() > 0 ){
        globalSettings.set( "exportCount", QString( export_count ) );
        //Disable Add field in Project Setting Dialog
        emit disbleAddField();
    }
    if (!exportFile.open(QIODevice::WriteOnly|QIODevice::Append)){
          //qDebug() << "Couldn't open file for writing";
          return;
    }

    exportFile.close();
}

void WorkerThread2nd::ImportFlightData(const QString &sourceFlightDataDirectoryPath, const bool &copyfiles)
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
            emit unexpecteFinished();
            return;
        }

        getAudioPlayer()->fileList->clear();
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
                //qDebug() << "Unable to copy " << sourceFile << " to " << targetFilePath;
            }
        }
    }else{
        targetFlightDataDirectoryPath = sourceFlightDataDirectoryPath;
    }

    emit load2();

    QDirIterator wavFileIterator(targetFlightDataDirectoryPath, QStringList() << "*.WAV", QDir::Files, QDirIterator::Subdirectories);

    QRegExp rx("-");// edit 4.25.2020 rx("(-)")
    QRegExp notNums("[^0-9]");

    //qDebug() << "getFlightData: " << flightFilePath;

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
        emit unexpecteFinished();
        return;
    }

    emit load3();

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

    emit load4();

    //Looping the wav files.
    while (wavFileIterator.hasNext()) {
        QString wavFilePath = wavFileIterator.next();
        QFileInfo wavFileInfo(wavFileIterator.filePath());
        QString wavFileName = wavFileInfo.fileName();

        //Break the name into a string list with [HH,MM,SS]
        QStringList query = wavFileName.split(rx);
        query.replaceInStrings(notNums, "");

        QString compareLine = "1" + query[0].right(2) + query[1] + query[2];
        QString matchLine = "";
        QString wavfile = "";//added 4.25.2020

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
        //  this is where the import data is parsed*/
        GPSRecording *record = new GPSRecording(QStringList(matchLine), wavFilePath);
        //recordings.append(record);

        //Add record in MainWindow
        emit sendRecords(record, matchLine);

        names.append(wavFileName);
        //qDebug() << "with times " << record->times;

        //-- need to do someting about this..
        getAudioPlayer()->fileList->addMedia(QUrl::fromLocalFile(wavFilePath));
    }

    emit load5();

    if(globalSettings.get("autoUnit") == QLatin1String("yes")){

        //qDebug() << "Map enabled";

        emit plotGPS(true, flightLines, wavFileItems);
         const QString &surveyType = getProjectFile()->getSurveyType();
        if(surveyType.toLower() == QLatin1String("baea")){
            //surveyJsonPolygonList = readGeoJson2(getProjectFile()->geoJSON);
            //Get All Plot Name List
            getProjectFile()->loadPlotNameList();

            emit load6();

            emit plotBAEA(readGeoJson2(getProjectFile()->geoJSON));
        }else if(surveyType.toLower() == QLatin1String("goea")){

            //surveyJsonSegmentListGOEA = readGeoJson(getProjectFile()->geoJSON);
            const QList<QStringList> &surveyJsonSegmentListGOEA =
                    readGeoJson(getProjectFile()->geoJSON);

            //BCR and Transect Validation
            getProjectFile()->loadGOEAProperties();

            /*qDebug() << "surveyJsonSegmentListGOEA: " << surveyJsonSegmentListGOEA.length();

            for(int i = 0; i < surveyJsonSegmentListGOEA.length(); i++){
                qDebug() << surveyJsonSegmentListGOEA[i];
            }*/

            //emit plotGOEA(readGeoJson(getProjectFile()->geoJSON));
            emit plotGOEA(surveyJsonSegmentListGOEA);
            emit load6();
        }else if(surveyType.toLower() == QLatin1String("other")) {
            //No code yet
            emit load6();
        }else{
            //surveyJsonSegmentList = readGeoJson(getProjectFile()->geoJSON);

            //Load Properties for Transect Stratum Segment
            getProjectFile()->loadWBPSProperties();

            //Load Properties for AGCode
            getProjectFile()->loadAirGroundProperties();

            emit load6();

            //for reading the air ground json
            //airGroundSurveyJsonSegmentList = readGeoJson(getProjectFile()->airGround);
            emit plotWPHS(readGeoJson(getProjectFile()->geoJSON), readGeoJson(getProjectFile()->airGround));
        }
    }

    emit load7();

    names.sort();
    //qDebug() << "WorkerThread2nd";
    emit playAudio(names);
}

QString WorkerThread2nd::renameFile(const QString &fileInfo, const QString &directoryInfo)
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

QList<QStringList> WorkerThread2nd::readGeoJson2(QJsonObject jObj)
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
                                            qDebug() << '\t' << "OBJECT" << k.key();
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
    return jsonGeomList;
}

QList<QStringList> WorkerThread2nd::readGeoJson(QJsonObject jObj)
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
                            }else if(key.toUpper() == QLatin1String("GEOMETRY") ){
                                QJsonObject obj2 = value.toObject();
                                foreach(const QString& key2, obj2.keys()) {
                                    QJsonValue value2 = obj2.value(key2);
                                    if(key2.toUpper() == QLatin1String("COORDINATES") ){
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

bool WorkerThread2nd::getLoadCount() const
{
    return m_firstRun;
}

void WorkerThread2nd::setLoadCount(const bool &newLoad)
{
    m_firstRun = newLoad;
}
