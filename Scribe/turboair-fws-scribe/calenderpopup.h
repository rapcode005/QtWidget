#ifndef CALENDERPOPUP_H
#define CALENDERPOPUP_H

#include <QWidget>

namespace Ui {
class CalenderPopup;
}

class CalenderPopup : public QWidget
{
    Q_OBJECT

public:
    explicit CalenderPopup(QWidget *parent = 0);
    ~CalenderPopup();

private:
    Ui::CalenderPopup *ui;
};

#endif // CALENDERPOPUP_H
