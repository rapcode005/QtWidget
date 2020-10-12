#ifndef GPSHANDLER_H
#define GPSHANDLER_H

#include <QObject>
#include <QFile>
#include <QQuickItem>
#include <QDir>
#include "transectinfo.h"
class GpsHandler : public QObject
{
    Q_OBJECT

public:
    GpsHandler(QQuickItem *map, QString localDirectory);
    void ParseGpsStream(QStringList flightLines);
    void ReadTransects(QDir fileLocation);

    QQuickItem *mapobj;
    QObject        *wayHandler;
    QObject        *transectHandler;
    QString         localDir;
    QList<TransectInfo*> transects;
};

#endif // GPSHANDLER_H
