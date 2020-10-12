#include "workerThreadImportFlight.h"


void WorkerThreadImportFlight::run()
{
    ImportFlightData();
    emit loadReady();
}

ProjectFile *WorkerThreadImportFlight::getProjectFile()
{
    return m_proj;
}

void WorkerThreadImportFlight::setProjectFile(ProjectFile *newProj)
{
    m_proj = newProj;
}

AudioPlayer *WorkerThreadImportFlight::getAudioPlayer()
{
    return m_play;
}

void WorkerThreadImportFlight::setAudioPlayer(AudioPlayer *newAudio)
{
    m_play = newAudio;
}

QString WorkerThreadImportFlight::getSourceFlightData() const
{
    return m_sourceFlightData;
}

void WorkerThreadImportFlight::setSourceFlightData(const QString &newData)
{
    m_sourceFlightData = newData;
}

QString WorkerThreadImportFlight::getFlightPath() const
{
    return m_flightPath;
}

void WorkerThreadImportFlight::setFlightPath(const QString &newData)
{
    m_flightPath = newData;
}

bool WorkerThreadImportFlight::getCopyFiles() const
{
    return m_copyFiles;
}

void WorkerThreadImportFlight::setCopyFiles(const bool &newCopy)
{
    m_copyFiles = newCopy;
}

QStringList WorkerThreadImportFlight::getFlightData() const
{
    return m_flightData;
}

void WorkerThreadImportFlight::setFlightData(const QStringList &data)
{
    m_flightData = data;
}

void WorkerThreadImportFlight::ImportFlightData()
{

    QDirIterator gpsFileIterator(getSourceFlightData(),
                                 QStringList() << "*.GPS", QDir::Files, QDirIterator::Subdirectories);
    QString flightFilePath = "";
    while(gpsFileIterator.hasNext()) {
        const QString &sourceFilePath = gpsFileIterator.next();
        flightFilePath = sourceFilePath;
    }

    QStringList flightdata;
    QFile gpsFile(flightFilePath);

    if (!gpsFile.exists()) {
        qDebug() << "No FLIGHT.GPS file. Exiting. Import failed.";
        emit unexpectedFinished();
        return;
    }

   /* if (gpsFile.open(QIODevice::ReadOnly)){
        QTextStream stream(&gpsFile);
        while(!stream.atEnd()){
            QString currentFlightLine = stream.readLine();
            flightdata << currentFlightLine;
        }
        gpsFile.close();
    }*/

    /*emit load0();

    if(getSourceFlightData().length() > 0){
        emit calculateMissingSec(flightFilePath, flightdata);
    }*/

    QStringList names;

    // Make sure the project's FlightData directory exists
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    QString directoryName;
    QString projectDirectory = globalSettings.get("projectDirectory");
    QString observer = globalSettings.get("observerInitials");
    QString targetFlightDataDirectoryPath("");
    if(getCopyFiles()){
        if (getSourceFlightData().isEmpty()){
            emit unexpectedFinished();
            return;
        }

        getAudioPlayer()->fileList->clear();
        QDir sourceDirectory(getSourceFlightData());
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
                //qDebug() << "Unable to copy " << sourceFilePath << " to " << targetFilePath;
                if(sourceFileInfo.fileName().contains("_TMP.GPS")){//added7.6.2020 for the new interpolated gps file
                    if(QFile::exists(targetFilePath))
                        QFile::remove(targetFilePath);
                    if (!QFile::copy(sourceFilePath, targetFilePath)) {
                        qDebug() << "Unable to copy " << sourceFilePath << " to " << targetFilePath;
                    }
                }
            }
        }
    }else{
        targetFlightDataDirectoryPath = getSourceFlightData();
    }

    emit load1();
    QDirIterator wavFileIterator(targetFlightDataDirectoryPath, QStringList() << "*.WAV", QDir::Files, QDirIterator::Subdirectories);

    QRegExp rx("-");// edit 4.25.2020 rx("(-)")
    QRegExp notNums("[^0-9]");

    //qDebug() << flightFilePath;

    //Load GPS File
    if(!directoryName.isEmpty()) {
        //qDebug() << "A";
        flightFilePath = targetFlightDataDirectoryPath + "/" + observer + "_" + directoryName + ".GPS";
    }
    else {
        //qDebug() << "B";
         QDirIterator gpsFileIterator(targetFlightDataDirectoryPath, QStringList() << "*.GPS", QDir::Files, QDirIterator::Subdirectories);
         while(gpsFileIterator.hasNext()) {
             const QString &sourceFilePath = gpsFileIterator.next();
             flightFilePath = sourceFilePath;
         }
    }
    //qDebug() << "flightFilePath: " << flightFilePath;

   // QFile gpsFile(flightFilePath);


    emit load2();

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

    emit load3();

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

    emit load4();

    if(globalSettings.get("autoUnit") == QLatin1String("yes") ){

        //qDebug() << "Map enabled";

        emit plotGPS(true, flightLines, wavFileItems);
         const QString &surveyType = getProjectFile()->getSurveyType();
        if(surveyType == QLatin1String("BAEA") ){
            const QList<QStringList> &surveyJsonPolygonList =
                    readGeoJson2(getProjectFile()->geoJSON);

            //Get All Plot Name List
            getProjectFile()->loadPlotNameList();

            emit plotBAEA(surveyJsonPolygonList);
        }else if(surveyType == QLatin1String("GOEA") ){

             const QList<QStringList> &surveyJsonSegmentListGOEA =
                     readGeoJson(getProjectFile()->geoJSON);

            //BCR and Transect Validation
            getProjectFile()->loadGOEAProperties();

            /*qDebug() << "surveyJsonSegmentListGOEA: " << surveyJsonSegmentListGOEA.length();

            for(int i = 0; i < surveyJsonSegmentListGOEA.length(); i++){
                qDebug() << surveyJsonSegmentListGOEA[i];
            }*/

            emit plotGOEA(surveyJsonSegmentListGOEA);
        }else if(surveyType == QLatin1String("Other") ) {
            //No code yet
        }else{
             const QList<QStringList> &surveyJsonSegmentList =
                     readGeoJson(getProjectFile()->geoJSON);

            //Load Properties for Transect Stratum Segment
            getProjectFile()->loadWBPSProperties();

            //Load Properties for AGCode
            getProjectFile()->loadAirGroundProperties();

            //for reading the air ground json
             const QList<QStringList> &airGroundSurveyJsonSegmentList =
                     readGeoJson(getProjectFile()->airGround);

            emit plotWPHS(surveyJsonSegmentList, airGroundSurveyJsonSegmentList);
        }
    }

    emit load5();

    names.sort();
    //qDebug() << "WorkerThreadImportFlight";
    emit playAudio(names);
}

QString WorkerThreadImportFlight::renameFile(const QString &fileInfo, const QString &directoryInfo)
{
    std::string fileStd = fileInfo.toStdString();
    //Get Last period
    std::size_t lastIndex = fileInfo.toStdString().find_last_of(".");
    //Get File Name
    std::string fileName = fileInfo.toStdString().substr(0, lastIndex);
    //Get extension file
    std::string extFile = fileInfo.toStdString().substr(lastIndex, fileStd.length());
    if (extFile == ".GPS" )
        return directoryInfo + QString::fromUtf8(extFile.c_str());
    else
        return directoryInfo + "_" + QString::fromUtf8(fileName.c_str()) + QString::fromUtf8(extFile.c_str());
}

QList<QStringList> WorkerThreadImportFlight::readGeoJson2(QJsonObject jObj)
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

QList<QStringList> WorkerThreadImportFlight::readGeoJson(QJsonObject jObj)
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
