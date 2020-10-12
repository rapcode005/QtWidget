#include "customJsonArray.h"
#include "qdebug.h"

CustomJsonArray::CustomJsonArray(QJsonArray *pre) :
    QJsonArray(*pre)
{

}

CustomJsonArray::CustomJsonArray(const CustomJsonArray &pre)
{
    qDebug() << pre;
}



