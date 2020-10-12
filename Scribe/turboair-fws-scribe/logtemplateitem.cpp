#include "logtemplateitem.h"

LogTemplateItem::LogTemplateItem(QString name)
{
    itemName = name;
}
LogTemplateItem::~LogTemplateItem(){
    delete this;
}
