#include "customeventwidget.h"


CustomEventWidget::CustomEventWidget(QWidget *parent):
    QTextEdit(parent)
{
    QFont font;
    font.setFamily("Segoe UI");
    font.setPixelSize(13);
    //font.setCapitalization(QFont::AllLowercase);
    this->setFont(font);
}

QString CustomEventWidget::getTitleID() const
{
    return m_titleID;
}

void CustomEventWidget::focusOutEvent(QFocusEvent *event)
{
    QTextEdit::focusOutEvent(event);
    emit focusOut();
}

void CustomEventWidget::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);
    emit focusIn();
}

void CustomEventWidget::keyPressEvent(QKeyEvent *event)
{
    QTextEdit::keyPressEvent(event);
    if (event->key() == 16777220)
        emit enterKey();
}

void CustomEventWidget::setTitle(const QString &newVal)
{
    m_titleID = newVal;
}


