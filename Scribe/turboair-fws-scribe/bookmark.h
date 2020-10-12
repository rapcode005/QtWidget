#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <QObject>
#include <QList>
class BookMark
{
public:
    BookMark();

    int quantity;
    QString species;
    QString grouping;
    //QString transect;
    QString audioFileName;
    QString hdop;
    QString course;
    QString speed;
    QString numSatellites;
    QString altitude;
    QString latitude;
    QString longitude;
    QString timestamp;
    QStringList additionalFields; //This is additional fields
    QStringList headerValues;
signals:

public slots:
};

#endif // BOOKMARK_H
