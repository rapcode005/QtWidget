#include "docLayout.h"
#include <QDebug>

int DocLayout::getIndex() const
{
    return m_index;
}

void DocLayout::setIndex(const int &newindex)
{
    m_index = newindex;
    //emit newValue();
}

int DocLayout::getIndexTableW() const
{
    return m_indexTableW;
}

void DocLayout::setIndexTableW(const int &newIndex)
{
    m_indexTableW = newIndex;
}

void DocLayout::result(const QSizeF &newSize)
{
    emit sendSignal(newSize, getIndex(), getIndexTableW());
}

void DocLayout::resultA(const QSizeF &newSize)
{
     emit sendSignalA(newSize, getIndex(), getIndexTableW());
}

/*void DocLayout::checkBoxResult(bool chcked)
{
    emit sendCheckBox(getIndex());
}*/

/*void DocLayout::resultTextEdit()
{
    emit sendText(getIndex());
}*/

/*void DocLayout::resultH(const QSizeF &newSize)
{
    emit sendSignalH(newSize, getIndex(), getIndexTableW());
}*/

