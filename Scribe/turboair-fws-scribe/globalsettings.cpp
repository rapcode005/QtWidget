#include "globalsettings.h"

GlobalSettings& GlobalSettings::getInstance() {
    static GlobalSettings* _instance = nullptr;
    if (_instance == nullptr) {
        _instance = new GlobalSettings();
    }
    return *_instance;
}

GlobalSettings::GlobalSettings(QObject *parent) : QObject(parent)
{
    settings = QMap<QString, QString>();
}

void GlobalSettings::set(QString key, QString value) {
    settings.insert(key, value);
}

QString GlobalSettings::get(QString key) {
    return settings.value(key);
}

QList<QString> GlobalSettings::getKeys() {
    return settings.keys();
}
