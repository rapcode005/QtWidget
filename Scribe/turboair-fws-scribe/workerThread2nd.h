#ifndef WORKERTHREAD2ND_H
#define WORKERTHREAD2ND_H

#include <QThread>
#include <QObject>
#include "projectfile.h"
#include "audioplayer.h"
#include "gpshandler.h"
#include "gpsrecording.h"
#include "globalsettings.h"

class WorkerThread2nd : public QThread
{
    Q_OBJECT
    void run() override;

public:
    ProjectFile *getProjectFile();
    void setProjectFile(ProjectFile *newProj);

    AudioPlayer *getAudioPlayer();
    void setAudioPlayer(AudioPlayer *newAudio);

    bool getHasFlightData() const;
    void setHasFlightData(const bool &newHas);

    QString getFlightData() const;
    void setFlightData(const QString &newFlight);

    GpsHandler *getGPS();
    void setGPS(GpsHandler *newGPS);

    void PopulateHeaderFields();
    void CreateExportFile();
    void ImportFlightData(const QString &sourceFlightDataDirectoryPath, const bool &copyfiles);
    QString renameFile(const QString &fileInfo, const QString &directoryInfo);

    QList<QStringList> readGeoJson2(QJsonObject jObj);
    QList<QStringList> readGeoJson(QJsonObject jObj);

    bool getLoadCount() const;
    void setLoadCount(const bool &newLoad);

signals:
    void LoadReady(ProjectFile *newProj);
    void load0(QString flightFilePath);
    void load1();
    void load2();
    void load3();
    void load4();
    void load5();
    void load6();
    void load7();
    void unexpecteFinished();

    void sendHeaderFields(QTableWidgetItem *item, const int &index, const int &condition);
    void disbleAddField();
    /** Import Flight Data
     */
    //Send Record for Audio
    void sendRecords(GPSRecording *records, const QString &matchLine);
    //Plot GPS
    void plotGPS(const bool &autoUnit, const QStringList &flightLines, const QStringList &wavFileItems);
    //Plot GPSSurvey
    void plotBAEA(const QList<QStringList> &surveyJsonPolygonList);
    void plotGOEA(const QList<QStringList> &surveyJsonSegmentListGOEA);
    void plotWPHS(const QList<QStringList> &surveyJsonSegmentList,
                  const QList<QStringList> &airGroundSurveyJsonSegmentList);
    void playAudio(const QStringList &names);

    void calculateMissingSec(QString flightpath, QStringList flighdataD);

private:
    ProjectFile *m_proj;
    AudioPlayer *m_audio;
    QString m_flightData;
    bool m_hasFlightData;
    GpsHandler *m_gps;
    bool m_firstRun;
    //QList<QStringList> surveyJsonPolygonList, surveyJsonSegmentListGOEA,
    //surveyJsonSegmentList, airGroundSurveyJsonSegmentList;
};

#endif // WORKERTHREAD2ND_H
