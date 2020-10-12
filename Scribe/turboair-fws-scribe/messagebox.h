#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QDialog>
#include <QMainWindow>
#include <QObject>

namespace Ui {
class MessageBox;
}

class MessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MessageDialog(QWidget *parent = nullptr, QString text = "");
    ~MessageDialog();
    void setText(const QString &str);
    void setFontSize(const int &size);
    void setTwoButton(const QString &acceptText, const QString &rejectText);
    void setTwoButton(const QString &acceptText, const int &acceptWidth,
                       const QString &rejectText, const int &rejectWidth);
    void setThreeButton(const QString &acceptText, const QString &rejectText, const QString &cancelText);
    void resizeHeight(const int &size);
    void resizeWidth(const int &size);
    void textAlign();
    void changeLayout();
    bool isAccept() const;
    bool isReject() const;
    void setTitle(const QString &str);

private slots:
    void on_ok_clicked();

    void on_No_clicked();

    void on_cancel_clicked();

private:
    Ui::MessageBox *ui;
    bool m_accept;
    bool m_reject;
};

#endif // MESSAGEBOX_H
