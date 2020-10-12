#ifndef DISTANCECONFIGOBJ_H
#define DISTANCECONFIGOBJ_H

#include <QObject>

class DistanceConfigObj : public QObject
{
public:
    DistanceConfigObj(const double &obsMaxD, const double &airMaxD, const bool &status);
    DistanceConfigObj(const double &obsMaxD, const bool &status);

    double getObsMaxD() const;
    double getAirMaxD() const;
    bool getStatus() const;


private:
    bool m_status;
    double m_obsMaxD;
    double m_airMaxD;

    void setObsMaxD(const double &newVal);
    void setAirMaxD(const double &newVal);
    void setStatus(const bool &newVal);
};


#endif // DISTANCECONFIGOBJ_H
