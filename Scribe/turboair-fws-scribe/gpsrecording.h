#ifndef GPSRECORDING_H
#define GPSRECORDING_H
#include <QObject>
#include <QTime>





class GPSRecordingEntry
{
public:
    double                            latitude;
    double                          longitutde;
    QString                          timestamp;
    float                             altitude;
    float                                speed;
    QString                             course;
    int                   number_of_satellites;
    float                                 hdop; // horizontal dilution of precision
};





class GPSRecording
{

public:

    GPSRecording( QStringList coordinates, QString audioFilePath );

    QStringList                         coords;
    QList<qint64>                        times;
    QString                          audioPath;

    QList<GPSRecordingEntry>           entries;


private:

    GPSRecordingEntry parseLine( QString line );


};





#endif // GPSRECORDING_H
