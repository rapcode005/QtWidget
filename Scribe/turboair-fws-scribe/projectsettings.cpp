#include "projectsettings.h"
#include <QSettings>
#include <QFile>
#include <QDebug>
ProjectSettings::ProjectSettings()
{
}
bool ProjectSettings::SaveCurrent(){
    QSettings settings (filename, QSettings::IniFormat);
    //if we can't save our project, throw an error and return false
    if (settings.status() == settings.AccessError){
        qWarning("Could not get this project settings file.");
        return false;
    }
    //this is where it gets a little bananas. For our list of columns, x, take the value of each row and serialize it, y
    QString val;
    settings.setValue("columnCount", addedItems.length());
    for(int i = 0; i < addedItems.length(); i++){
        settings.setValue("rowCountn" + QString::number(i), addedItems.length());
        for(int n = 0; n < addedItems[i].length(); n++){
            val = addedItems[i][n];
            settings.setValue("addedn" + QString::number(i) + "n" + QString::number(n),val);
        }
    }
    //then we just serialize our header field as a list of strings
    settings.setValue("headerfieldCount", addedHeaderFields.length());
    for(int t = 0; t < addedHeaderFields.length(); t++){
        settings.setValue("addedHFieldn" + QString::number(t),addedHeaderFields[t]);
    }
    return true;
}
bool ProjectSettings::Load(QString file){
    //do the same thing as before, but the other way around.
    QSettings settings (file, QSettings::IniFormat);
    if (settings.status() == settings.AccessError){
        qWarning("Could not open this project settings file.");
        return false;
    }
        qWarning() << "we're right before the load is good";
    filename = file;
    addedItems.clear();
    addedHeaderFields.clear();
    //this time we use presaved column counts and row counts, so we'd know how many of rows of each column we need to iterate through and grab
    for(int i = 0; i < settings.value("columnCount").toInt(); i++){
        int rowCeiling = settings.value("rowCountn" + QString::number(i)).toInt();
        QStringList rows;
        qWarning() << "column is good";
        for (int n = 0; n < rowCeiling; n++){
            rows.clear();
            rows.append(settings.value("addedn" + QString::number(i) + "n"+ QString::number(n)).toString());
            qWarning() << "row is good";
            //we slap all of the stringlists into our QList variable
            addedItems.append(rows);
        }

    }
    int headerCeiling = settings.value("headerfieldCount").toInt();
    //iterating through our header fields, this is easier since it's a 1 dimensional array
    for(int t = 0; t < headerCeiling; t++){
        addedHeaderFields << settings.value("addedHFieldn" + QString::number(t)).toString();
    }
    return true;
}

