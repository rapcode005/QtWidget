#ifndef CUSTOMCOMBOBOX_H
#define CUSTOMCOMBOBOX_H

#include <QtWidgets>

class CustomComboBox : public QComboBox
{
    Q_OBJECT

public:
    CustomComboBox(QWidget *parent = nullptr);

protected:
    void focusOutEvent(QFocusEvent *event);
    void focusInEvent(QFocusEvent *event);
    void keyPressEvent(QKeyEvent *event);

signals:
    void focusOut();
    void focusIn();
    void enterKey();

};

#endif // CUSTOMCOMBOBOX_H
