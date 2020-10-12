#ifndef TRANSECTINFO_H
#define TRANSECTINFO_H

#include <QObject>

class TransectInfo : public QObject
{
    Q_OBJECT
public:
    explicit TransectInfo(QObject *parent = nullptr, QString tranName = "", double latMax = 0, double latMin = 0, double lonMax = 0, double lonMin = 0);
    QString transectName;
    double maximumLat;
    double maximumLon;
    double minimumLat;
    double minimumLon;
signals:

public slots:
};

#endif // TRANSECTINFO_H
