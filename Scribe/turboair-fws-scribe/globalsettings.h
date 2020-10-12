#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

#include <QObject>
#include <QMap>

class GlobalSettings : public QObject
{
    Q_OBJECT
public:
    static GlobalSettings& getInstance();

    QMap<QString, QString> settings;

private:
    explicit GlobalSettings(QObject *parent = nullptr);

signals:

public slots:
    void set(QString key, QString value);
    QString get(QString key);
    QList<QString> getKeys();
};

#endif // GLOBALSETTINGS_H
