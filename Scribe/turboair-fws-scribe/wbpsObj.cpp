#include "wbpsObj.h"

WBPSObj::WBPSObj(const int &stratum, const int &transect, const int &segment)
{
    setStratum(stratum);
    setSegment(segment);
    setTransect(transect);
}

int WBPSObj::getStratum() const
{
    return m_stratum;
}

void WBPSObj::setStratum(const int &newVal)
{
    m_stratum = newVal;
}

int WBPSObj::getTransect() const
{
    return m_transect;
}

void WBPSObj::setTransect(const int &newVal)
{
    m_transect = newVal;
}

int WBPSObj::getSegment() const
{
    return m_segment;
}

void WBPSObj::setSegment(const int &newVal)
{
    m_segment = newVal;
}
