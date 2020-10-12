#include "gpshandler.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>


GpsHandler::GpsHandler(QQuickItem *map, QString localDirectory)
{
    //setting our map object
    mapobj = map;
    localDir = localDirectory;
    //making sure we have components from that map object to mess with
    //qDebug() << "Gps Module Initialized";
    wayHandler = mapobj->findChild<QObject*>("waypointHandler");
}


//reading our transects from our transects.json
void GpsHandler::ReadTransects(QDir fileLocation){
    QStringList textStream; //each string in here defines a transect box
    //grabbing our file
    QFile fil (fileLocation.absolutePath());
    if (!fil.open(QFile::ReadOnly)){
        //qDebug()<< "Could not Open file : " << fileLocation.absolutePath();
        fil.close();
        return;
    }
    //getting our json from the file and making sure it has no errors
    QJsonParseError err;
    QByteArray data = fil.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data,&err);
    if (!doc.isObject()){
        //qDebug() << "Opened file is not a valid doc.";
        fil.close();
        return;
    }
    fil.close();   //after we have our json as an object, close the file
    qDeleteAll(transects); //protecting against memory leaks
    transects.clear(); //clearing the list so we don't accidentaly access a null pointer
    QJsonObject obj = doc.object();
    //creating strings to push to our textStream
    for (int i = 0; i < obj.keys().length(); i ++){
        QString appendage = ""; //start with nothing
        QJsonObject transect = obj[obj.keys()[i]].toObject(); //access our object one level
        QList<QJsonArray> *arrays = new QList<QJsonArray>(); //grab some plotting arrays
        //nasty if block, unfortunately I'm not sure how to case and switch something like this
        if (transect["TL"].isArray()){
        arrays->append(transect["TL"].toArray());
        }
        if (transect["TR"].isArray()){
        arrays->append(transect["TR"].toArray());
        }
        if (transect["BR"].isArray()){
        arrays->append(transect["BR"].toArray());
        }
        if (transect["BL"].isArray()){
        arrays->append(transect["BL"].toArray());
        }
        //setting the limits of our transect box
        double maxLat = 0;
        double maxLon = 0;
        double minLat = 0;
        double minLon = 0;
        QString tranName = obj.keys().at(i); //giving our box a title
        //qDebug() << tranName;
        //for every array of coordinates in our transect's object, split it and figure out where it goes
        for(int s = 0; s < arrays->length(); s++){
                QJsonArray arr  = arrays->at(s);
                QString line = "";
                //getting our lat and lon with a lot of accuracy, i.e. the 9 in the second number
                line += QString::number(arr.at(0).toDouble()) + "," +  QString::number(arr.at(1).toDouble(),'f',9) + ";";
                if (s == 1){
                    maxLat = arr.at(0).toDouble();
                    maxLon = arr.at(1).toDouble();
                }else if (s == 3){
                    minLat = arr.at(0).toDouble();
                    minLon = arr.at(1).toDouble();
                }
                appendage += line;
        }
        //qDebug() << "top left" << minLat << maxLon;
        //qDebug() << "bottom right" << maxLat << minLon;
        //creating a new object on the cpp side that defines some transect info
        TransectInfo *t = new TransectInfo(nullptr,tranName,maxLat,minLat,maxLon,minLon);
        transects.append(t);
        delete arrays; //getting rid of our list of arrays
        textStream.append(appendage);
    }
    //qDebug() << transects.length();

    //cart this off to our map for plotting, that magic happens in qml
    QVariant tStream = textStream;
    QMetaObject::invokeMethod(wayHandler, "plotTransects",
            Q_ARG(QVariant, tStream));

}

void GpsHandler::ParseGpsStream(QStringList flightLines){
    QMetaObject::invokeMethod(wayHandler, "plotCoordinates",
                    Q_ARG(QVariant, flightLines));
}





