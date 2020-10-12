#ifndef DOCLAYOUT_H
#define DOCLAYOUT_H

#include <QObject>
//#include <QWidget>

class DocLayout: public QObject {

    Q_OBJECT
    /*Q_PROPERTY(int getIndex
               READ getIndex
               WRITE setIndex
               NOTIFY newValue)*/

public:
    int getIndex() const;
    void setIndex(const int &newindex);
    int getIndexTableW() const;
    void setIndexTableW(const int &newIndex);

signals:
    void sendSignal(const QSizeF &r, const int &index, const int &tIndex);
    void sendSignalA(const QSizeF &r, const int &index, const int &tIndex);
    //void sendCheckBox(const int &index);
    //void sendText(const int &index);
    //void sendSignalH(const QSizeF &r, const int &index, const int &tIndex);


public slots:
    void result(const QSizeF &newSize);
    void resultA(const QSizeF &newSize);
    //void checkBoxResult(bool chcked = false);
    //void resultTextEdit();
    //void resultH(const QSizeF &newSize);

private:
    int m_index;
    int m_indexTableW;

};

#endif // DOCLAYOUT_H



