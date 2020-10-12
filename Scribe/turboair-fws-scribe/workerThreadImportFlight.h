#ifndef WORKERTHREADIMPORTFLIGHT_H
#define WORKERTHREADIMPORTFLIGHT_H

#include <QThread>
#include <QObject>
#include "projectfile.h"
#include "audioplayer.h"
#include "gpshandler.h"
#include "gpsrecording.h"
#include "globalsettings.h"

class WorkerThreadImportFlight : public QThread
{
    Q_OBJECT
    void run() override;

public:
    ProjectFile *getProjectFile();
    void setProjectFile(ProjectFile *newProj);

    AudioPlayer *getAudioPlayer();
    void setAudioPlayer(AudioPlayer *newAudio);

    QString getSourceFlightData() const;
    void setSourceFlightData(const QString &newData);

    QString getFlightPath() const;
    void setFlightPath(const QString &newData);

    bool getCopyFiles() const;
    void setCopyFiles(const bool &newCopy);

    QStringList getFlightData() const;
    void setFlightData(const QStringList &data);

    void ImportFlightData();

    QString renameFile(const QString &fileInfo, const QString &directoryInfo);

    QList<QStringList> readGeoJson2(QJsonObject jObj);
    QList<QStringList> readGeoJson(QJsonObject jObj);


signals:
    //Save GPS
    void calculateMissingSec(QString flightpath, QStringList flighdataD);
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
    void loadReady();

    //Load
    void load0();
    void load1();
    void load2();
    void load3();
    void load4();
    void load5();
    void unexpectedFinished();

private:
    ProjectFile *m_proj;
    AudioPlayer *m_play;
    bool m_copyFiles;
    QString m_sourceFlightData;
    QString m_flightPath;
    QStringList  m_flightData;
};

#endif // WORKERTHREADIMPORTFLIGHT_H
