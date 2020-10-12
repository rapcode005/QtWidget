#include "customcombobox.h"


CustomComboBox::CustomComboBox(QWidget *parent) :
    QComboBox(parent)
{
    QFont font;
    font.setFamily("Segoe UI");
    font.setPixelSize(13);
    this->setFont(font);
}

void CustomComboBox::focusOutEvent(QFocusEvent *event)
{
    QComboBox::focusOutEvent(event);
    emit focusOut();
}

void CustomComboBox::focusInEvent(QFocusEvent *event)
{
    QComboBox::focusInEvent(event);
    emit focusIn();
}

void CustomComboBox::keyPressEvent(QKeyEvent *event)
{
    QComboBox::keyPressEvent(event);
    if (event->key() == 16777220)
        emit enterKey();
}
