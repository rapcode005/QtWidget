#ifndef WBPS_H
#define WBPS_H

#include <wbpsObj.h>
#include <QObject>
#include <QList>

class WBPS : public QObject
{

public:
    void append(WBPSObj *obj);
    bool checkTransect(WBPSObj *allObj);
    bool checkSegment(WBPSObj *allObj);
    bool checkStratum(WBPSObj *allObj);

private:
    QList<WBPSObj*> lists;
};

#endif // WBPS_H
