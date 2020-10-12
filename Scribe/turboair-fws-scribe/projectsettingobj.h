#ifndef PROJECTSETTINGOBJ_H
#define PROJECTSETTINGOBJ_H

#include <QObject>

class ProjectSettingObj : public QObject
{

public:
    ProjectSettingObj(const QString &newTitle, const QString &newValue);

    QString getTitle() const;
    QString getValue() const;

private:
    void setTitle(const QString &newVal);
    void setValue(const QString &newVal);

    QString m_title;
    QString m_value;
};

#endif // PROJECTSETTINGOBJ_H
