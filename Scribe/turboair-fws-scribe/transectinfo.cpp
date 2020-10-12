#include "transectinfo.h"

TransectInfo::TransectInfo(QObject *parent, QString tranName, double latMax, double latMin, double lonMax, double lonMin) : QObject(parent)
{
    transectName = tranName;
    maximumLat = latMax;
    maximumLon = lonMax;
    minimumLat = latMin;
    minimumLon = lonMin;

}
