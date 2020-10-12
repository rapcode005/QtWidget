#include "projectsettingobj.h"

ProjectSettingObj::ProjectSettingObj(const QString &newTitle, const QString &newValue)
{
    setTitle(newTitle);
    setValue(newValue);
}

QString ProjectSettingObj::getTitle() const
{
    return m_title;
}

QString ProjectSettingObj::getValue() const
{
    return m_value;
}

void ProjectSettingObj::setTitle(const QString &newVal)
{
    m_title = newVal;
}

void ProjectSettingObj::setValue(const QString &newVal)
{
    m_value = newVal;
}
