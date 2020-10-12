#ifndef WBPSOBJ_H
#define WBPSOBJ_H

#include <QObject>

class WBPSObj : public QObject
{

public:
    WBPSObj(const int &stratum, const int &transect, const int &segment);

   int getStratum() const;
   int getTransect() const;
   int getSegment() const;

private:
    void setStratum(const int &newVal);
    void setTransect(const int &newVal);
    void setSegment(const int &newVal);

    int m_stratum;
    int m_transect;
    int m_segment;
};

#endif // WBPSOBJ_H
