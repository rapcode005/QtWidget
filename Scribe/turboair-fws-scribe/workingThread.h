#ifndef WORKINGTHREAD_H
#define WORKINGTHREAD_H

#include <QThread>
#include <QObject>
#include "projectfile.h"
#include "newmainwindow.h"

class WorkerThread : public QThread
{

    Q_OBJECT
    void run() override;

public:

    ProjectFile *getProjectFile();
    void setProjectFile(ProjectFile *newProj);

    QString getSurveyType() const;
    void setSurveyType(const QString &newST);

    int getSurveyIndex() const;
    void setSurveyIndex(const int &newSI);

    QString getProjectDirVal() const;
    void setProjectDirVal(const QString &newST);

    QString getObserverInitials() const;
    void setObserverInitials(const QString &newSI);

    QString getObserverSeat() const;
    void setObserverSeat(const QString &newOS);

    QString getExportFilename() const;
    void setExportFilename(const QString &newEF);

    QString getAutoUnit() const;
    void setAutoUnit(const QString &newAU);

    QString getGeoFileText() const;
    void setGeoFileText(const QString &newGFT);

    QString getAirGround() const;
    void setAirGround(const QString &newAG);

    //QString getExistingFlightData() const;
    //void setExistingFlightData(const QString &newEFD);

    //bool getHasFlightData() const;
    //void setHasFlightData(const bool &val);

    QString getSurveyFileName() const;//added 8.12.2020
    void setSurveyFileName(const QString &newFilenm);//added 8.12.2020

    //Opening Screen
    void firstLoad();

signals:
    void firstLoadReady(const bool &hasFlightData, const QString &existingFlightDataPath);
    void load();
    void loads();
    void load1();
    void load2();
    void load3();

private:
    //MessageDialog *msgBox;
    ProjectFile *m_proj;
    //QTableWidget *m_tblField;
    int m_surveyI;
    //bool m_hasFlightData;
    QString m_survey, m_proDirVal, m_obI, m_oSeat, m_eF, m_au, m_geoFT, m_aG, m_surveyFlNm;
};

#endif // WORKINGTHREAD_H
