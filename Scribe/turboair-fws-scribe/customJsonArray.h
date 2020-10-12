#ifndef CUSTOMJSONARRAY_H
#define CUSTOMJSONARRAY_H

#include <QJsonArray>

class CustomJsonArray : public QJsonArray
{

public:
    CustomJsonArray(QJsonArray *pre = nullptr);
    CustomJsonArray(CustomJsonArray const &pre);

    //bool hasChangesMade() const;

    //QList<CustomJsonArray> getPreviousArray() const;
    //oid setPreviousArray(CustomJsonArray obj);

private:
    //QList<CustomJsonArray> previousJsonArray;

};

#endif // CUSTOMJSONARRAY_H
