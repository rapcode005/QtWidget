#ifndef LOGTEMPLATEITEM_H
#define LOGTEMPLATEITEM_H

#include <QObject>
#include <QKeySequence>
#include <QAction>
#include <QKeySequenceEdit>

class LogTemplateItem
{
public:
    LogTemplateItem(QString name);
    ~LogTemplateItem();
    QString itemName;
    bool    groupingEnabled;
    QKeySequence shortcutKey;
};

#endif // LOGTEMPLATEITEM_H
