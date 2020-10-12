#ifndef PROGRESSMESSAGE_H
#define PROGRESSMESSAGE_H

#include <QMainWindow>
#include <QCloseEvent>

namespace Ui {
class progressmessage;
}

class progressmessage : public QMainWindow
{
    Q_OBJECT

public:
    explicit progressmessage(QWidget *parent = nullptr);
    ~progressmessage();

    void setText(const QString &newText);
    bool okToClose = false;
    void setMax(const int &max);
    void setValue(const int &value);

private slots:
    void closeEvent(QCloseEvent *event);

signals:
    void saveProjectSet();

private:
    Ui::progressmessage *ui;
};

#endif // PROGRESSMESSAGE_H
