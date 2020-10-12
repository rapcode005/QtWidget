#include "wbps.h"


void WBPS::append(WBPSObj *obj)
{
    lists.append(obj);
}

bool WBPS::checkTransect(WBPSObj *allObj)
{
    foreach(WBPSObj *obj, lists) {
        if (allObj->getTransect() == obj->getTransect() &&
                allObj->getStratum() == obj->getStratum()){
            return true;
        }
    }
    return false;
}

bool WBPS::checkSegment(WBPSObj *allObj)
{
    foreach(WBPSObj *obj, lists) {
        if (allObj->getSegment() == obj->getSegment() &&
                allObj->getStratum() == obj->getStratum() &&
                allObj->getTransect() == obj->getTransect()) {
            return true;
        }
    }
    return false;
}

bool WBPS::checkStratum(WBPSObj *allObj)
{
    foreach(WBPSObj *obj, lists) {
        if (allObj->getStratum() == obj->getStratum()){
            return true;
        }
    }
    return false;
}
