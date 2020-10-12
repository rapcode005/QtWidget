#include "workingThread.h"

void WorkerThread::run()
{
   firstLoad();
}

ProjectFile *WorkerThread::getProjectFile()
{
    return m_proj;
}

void WorkerThread::setProjectFile(ProjectFile *newProj)
{
    m_proj = newProj;
}

QString WorkerThread::getSurveyType() const
{
    return m_survey;
}

void WorkerThread::setSurveyType(const QString &newST)
{
    m_survey = newST;
}

int WorkerThread::getSurveyIndex() const
{
    return m_surveyI;
}

void WorkerThread::setSurveyIndex(const int &newSI)
{
    m_surveyI = newSI;
}

QString WorkerThread::getProjectDirVal() const
{
    return m_proDirVal;
}

void WorkerThread::setProjectDirVal(const QString &newST)
{
    m_proDirVal = newST;
}

QString WorkerThread::getObserverInitials() const
{
    return m_obI;
}

void WorkerThread::setObserverInitials(const QString &newSI)
{
    m_obI = newSI;
}

QString WorkerThread::getObserverSeat() const
{
    return m_oSeat;
}

void WorkerThread::setObserverSeat(const QString &newOS)
{
    m_oSeat = newOS;
}

QString WorkerThread::getExportFilename() const
{
    return m_eF;
}

void WorkerThread::setExportFilename(const QString &newEF)
{
    m_eF = newEF;
}

QString WorkerThread::getAutoUnit() const
{
    return m_au;
}

void WorkerThread::setAutoUnit(const QString &newAU)
{
    m_au = newAU;
}

QString WorkerThread::getGeoFileText() const
{
    return m_geoFT;
}

void WorkerThread::setGeoFileText(const QString &newGFT)
{
    m_geoFT = newGFT;
}

QString WorkerThread::getAirGround() const
{
    return m_aG;
}

void WorkerThread::setAirGround(const QString &newAG)
{
    m_aG = newAG;
}

QString WorkerThread::getSurveyFileName() const
{
    return m_surveyFlNm;
}

void WorkerThread::setSurveyFileName(const QString &newFilenm)
{
    m_surveyFlNm = newFilenm;
}

void WorkerThread::firstLoad()
{
    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    globalSettings.set("surveyType", getSurveyType());
    globalSettings.set("projectDirectory", getProjectDirVal());
    globalSettings.set("observerInitials", getObserverInitials());
    //Check if Eagle survery
    switch(getSurveyIndex()) {
        //Eagle
        case 1:
        case 2: {
            globalSettings.set("observerSeat", "");
            break;
        }
        default: {
            globalSettings.set("observerSeat", getObserverSeat());
            break;
        }
    }

    globalSettings.set("exportFilename", getExportFilename());
    globalSettings.set("autoUnit", getAutoUnit());
    globalSettings.set("geoFile", getGeoFileText());
    globalSettings.set("exportCount", "0");

    if(getSurveyIndex() == 0)
        globalSettings.set("airGroundFile", getAirGround());

    //if(globalSettings.get("surveyType") != "Other"){
        // Try to copy the base survey file into the directory. If it already exists, load it.
        QString filename = "", jsonFile = "";
        //if(globalSettings.get("surveyType") == QLatin1String("WBPHS") ){
          //  filename = ":/json/WBPHS.0411.surv";
        //}else{
        //filename =":/json/" + globalSettings.get("surveyType") + ".surv";
        //}

        //added 8.12.2020
        if(getSurveyFileName().isEmpty())
            filename =":/json/" + globalSettings.get("surveyType") + ".surv";
        else
            filename = getProjectDirVal() + "/" + getSurveyFileName();

        const QString &survey = filename;

        QDir survey_file(survey);

        if(!QFile::copy(survey_file.absolutePath(), getProjectDirVal() + "/" + getSurveyType() +  ".surv") ){
            //Look for auto saves.  If there are auto saves, load the most recent auto save
            if(getProjectFile()->HasAutoSave()){
                if (getProjectFile()->Load(getProjectFile()->getMostRecentAutoSaveFile())){
                    qDebug() << "loaded auto save file successfully";
                }
                getProjectFile()->newOpen = false;
                //Load to get if changes made
                //getProjectFile()->loadChanges();
                /*if (getProjectFile()->getChanges()) {
                    if (getProjectFile()->Load(getProjectFile()->getMostRecentAutoSaveFile())){
                        qDebug() << "loaded auto save file successfully";
                    }
                }
                else {
                    if (getProjectFile()->Load(getProjectDirVal() + "/" + getSurveyType() +  ".surv")){
                        qDebug() << "loaded existing survey successfully. 1";
                    }
                }*/
            }else{
                if (getProjectFile()->Load(getProjectDirVal() + "/" + getSurveyType() +  ".surv")){
                    qDebug() << "loaded existing survey successfully";
                    getProjectFile()->newOpen = false;
                }
            }
        }else{
            if (getProjectFile()->Load(getProjectDirVal() + "/" + getSurveyType() +  ".surv")){
                qDebug() << "load copy of survey file into project directory successfully at " <<
                           getProjectDirVal() + "/" + getSurveyType();
                getProjectFile()->newOpen = true;
            }
        }

        emit load();
        /*
        if( !QFile::copy(survey_file.absolutePath(), getProjectDirVal() + "/" + globalSettings.get("surveyType") +  ".surv") ){
            //Look for auto saves.  If there are auto saves, load the most recent auto save
            if(getProjectFile()->HasAutoSave()){
                qDebug() << getProjectFile()->getMostRecentAutoSaveFile();
                if (getProjectFile()->Load(getProjectFile()->getMostRecentAutoSaveFile() )){
                    qDebug() << "loaded save file successfully";
                }

            }else{
                if (getProjectFile()->Load(getProjectDirVal() + "/" +  globalSettings.get("surveyType") + ".surv" )){
                    qDebug() << "loaded existing survey successfully";
                }
            }
        }else{
            if (getProjectFile()->Load(getProjectDirVal() + "/" + globalSettings.get("surveyType") + ".surv")){
                qDebug() << "load copy of survey file into project directory successfully at " <<
                            getProjectDirVal() + "/" + globalSettings.get("surveyType") + ".surv";
            }
        }*/

        //emit loads();
    //}
    //-- end of import survey

    emit load1();

    //Import Survey JSON File
    if(getGeoFileText().length() > 0){
        QFile fil (getGeoFileText());
        if (!fil.open(QFile::ReadOnly)){
            qDebug()<< "Could not Opens Survey JSON file : " << getGeoFileText();
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
                getProjectFile()->geoJSON = obj;
            }
        }
    }
    //End import Survey JSON file


    //Import Air Ground File
    if(getSurveyIndex() == 0 &&
            getAirGround().length() > 0) {
        QFile air(getAirGround());
        if (!air.open(QFile::ReadOnly)) {
            qDebug()<< "Could not Opens Air Ground JSON file : " << getAirGround();
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
                getProjectFile()->airGround = obj;
            }
        }
    }
    //End import Air Ground File


    emit load2();

    //--Check for existing flight data in project directory, and import
    bool hasFlightData = false;
    QString existingFlightDataPath("");
    QStringList all_dirs;
    all_dirs <<  getProjectDirVal();
    QDirIterator directories(getProjectDirVal(), QDir::Dirs | QDir::NoDotAndDotDot);

    while(directories.hasNext()){
        directories.next();
        all_dirs << directories.filePath();
         if(directories.filePath().contains("FlightData_") & !hasFlightData ){
            hasFlightData = true;
            existingFlightDataPath = directories.filePath();
         }
    }
    //--end of existing flight data check

    emit load3();

    emit firstLoadReady(hasFlightData, existingFlightDataPath);
}
