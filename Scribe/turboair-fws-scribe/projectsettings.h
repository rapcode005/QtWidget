#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include <QObject>
#include <QList>
class ProjectSettings
{
public:
    ProjectSettings();

    QString                       filename;

    QStringList                addedHeaderFields;
    QList<QStringList>  addedItems;

    bool SaveCurrent();
    bool Load(QString file);
};

#endif // PROJECTSETTINGS_H
