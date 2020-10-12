#include "distanceConfigObj.h"

DistanceConfigObj::DistanceConfigObj(const double &obsMaxD, const double &airMaxD, const bool &status)
{
    setObsMaxD(obsMaxD);
    setAirMaxD(airMaxD);
    setStatus(status);
}

DistanceConfigObj::DistanceConfigObj(const double &obsMaxD, const bool &status)
{
    setObsMaxD(obsMaxD);
    setStatus(status);
}

double DistanceConfigObj::getObsMaxD() const
{
    return m_obsMaxD;
}

void DistanceConfigObj::setObsMaxD(const double &newVal)
{
    m_obsMaxD = newVal;
}


double DistanceConfigObj::getAirMaxD() const
{
    return m_airMaxD;
}

void DistanceConfigObj::setAirMaxD(const double &newVal)
{
    m_airMaxD = newVal;
}


bool DistanceConfigObj::getStatus() const
{
    return m_status;
}

void DistanceConfigObj::setStatus(const bool &newVal)
{
    m_status = newVal;
}
