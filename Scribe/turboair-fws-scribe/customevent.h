#ifndef CUSTOMEVENT_H
#define CUSTOMEVENT_H
#include <QtWidgets>
#include <QFileDialog>
#include <QMainWindow>


class CustomEvent : public QWidget {

    Q_OBJECT

public:
    explicit CustomEvent(QObject * parent = nullptr, int newMethod = 0);
    void addTo(QObject * obj);
    void removeFilter();

private:
    bool eventFilter(QObject *watched, QEvent *event);
    //void on_selectedAccepted(const QString &file, QObject *watched);
    void openFileDialog(const QString &title, const QString &filter, const QString &fileTitle, QObject *watched);

    int method;
    QObject *objectEvent;
};

#endif // CUSTOMEVENT_H
