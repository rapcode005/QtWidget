#ifndef CUSTOMEVENTWIDGET_H
#define CUSTOMEVENTWIDGET_H

#include <QtWidgets>

class CustomEventWidget : public QTextEdit
{
    Q_OBJECT

public:
    CustomEventWidget(QWidget *parent = nullptr);
    QString getTitleID() const;
    void setTitle(const QString &newVal);

protected:
     void focusOutEvent(QFocusEvent *event);
     void focusInEvent(QFocusEvent *event);
     void keyPressEvent(QKeyEvent *event);

signals:
    void focusOut();
    void focusIn();
    void enterKey();

private:
    QString m_titleID;

};

#endif // CUSTOMEVENTWIDGET_H
