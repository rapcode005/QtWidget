#include "gpsrecording.h"
#include <QFile>
#include "audioplayer.h"
#include <QTime>
#include <QJsonObject>



GPSRecording::GPSRecording(QStringList coordinates, QString audioFilePath)
{
    //ties coordinates to an audiofiles
    qint64 firstMs = 0;
    for (int i = 0; i < coordinates.length(); i++)
    {
        GPSRecordingEntry entry = this->parseLine( (QString)coordinates[i] );
        this->entries.append(entry);

        QStringList listRowData = coordinates[i].split(","); //splitting up the coordinates we're given
        coords.append(listRowData[0] +"," + listRowData[1]); //our lats and longs
        if(i == 0){
            QTime first = QTime::fromString(listRowData[5], "h:m:s"); //find what time we start
            firstMs = first.msecsSinceStartOfDay();
            times.append(0);
        }else{
        QString s = listRowData[5];
        QTime t = QTime::fromString(s, "h:m:s"); //getting the time for each coord
        qint64 ms = t.msecsSinceStartOfDay();
        ms = ms - firstMs;
        times.append(ms);
        }
    }
    audioPath = audioFilePath;
}





GPSRecordingEntry  GPSRecording::parseLine( QString line )
{
    GPSRecordingEntry* entry = new GPSRecordingEntry();
    QStringList column = line.split(",");
    QDateTime* timestamp;
    QDate* date =  new QDate(column[2].toInt(), column[3].toInt(), column[4].toInt());
    QStringList timeStringValues = column[5].split(":");

    int hour = timeStringValues[0].toInt();
    int minute = timeStringValues[1].toInt();
    int second = timeStringValues[2].toInt();

    QTime* time = new QTime (hour, minute, second);
    QTimeZone* timezone = new QTimeZone(-6 * 3600);

    timestamp = new QDateTime(*date, *time, *timezone);

    entry->latitude                 =   column[0].toDouble();
    entry->longitutde               =   column[1].toDouble();
    entry->timestamp                =   timestamp->toString();
    entry->altitude                 =   column[6].toFloat();
    entry->speed                    =   column[7].toFloat();
    entry->course                   =   column[8];
    entry->number_of_satellites     =   column[9].toInt();
    entry->hdop                     =   column[10].toInt();

    return *entry;
}




