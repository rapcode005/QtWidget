#include "messagebox.h"
#include "ui_messagebox.h"

MessageDialog::MessageDialog(QWidget *parent, QString text) :
    QDialog(parent),
    ui(new Ui::MessageBox)
{
    ui->setupUi(this);
    window()->setWindowTitle("Scribe");
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QFont cellFont, buttonFont;
    cellFont.setFamily("Segoe UI");
    cellFont.setPixelSize(13);
    buttonFont.setFamily("Segoe UI");
    buttonFont.setPixelSize(12);
    buttonFont.setBold(true);

    ui->message->setText(text);
    ui->message->setFont(cellFont);
    ui->ok->setFont(buttonFont);
    ui->No->setFont(buttonFont);
    ui->No->setVisible(false);
    ui->cancel->setVisible(false);
}

MessageDialog::~MessageDialog()
{
    delete ui;
}

void MessageDialog::setText(const QString &str)
{
    ui->message->setText(str);
}

void MessageDialog::setFontSize(const int &size)
{
    QFont cellFont;
    cellFont.setFamily("Segoe UI");
    cellFont.setPixelSize(size);
    ui->message->setFont(cellFont);
}

void MessageDialog::setTwoButton(const QString &acceptText, const QString &rejectText)
{
    ui->ok->setText(acceptText);
    ui->No->setText(rejectText);
    ui->No->setVisible(true);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
}

void MessageDialog::setTwoButton(const QString &acceptText, const int &acceptWidth,
                                 const QString &rejectText, const int &rejectWidth)
{
    ui->ok->setText(acceptText);
    ui->ok->setMinimumWidth(acceptWidth);
    ui->No->setText(rejectText);
    ui->No->setMinimumWidth(rejectWidth);
    ui->No->setVisible(true);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
}

void MessageDialog::setThreeButton(const QString &acceptText, const QString &rejectText, const QString &cancelText)
{
    ui->ok->setText(acceptText);
    ui->No->setText(rejectText);
    ui->cancel->setText(cancelText);
    ui->No->setVisible(true);
    ui->cancel->setVisible(true);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
}

void MessageDialog::resizeHeight(const int &size)
{
    this->setMinimumHeight(size);
}

void MessageDialog::resizeWidth(const int &size)
{
    this->setMinimumWidth(size);
}

void MessageDialog::textAlign()
{
    ui->message->setAlignment(Qt::AlignLeft);
}

void MessageDialog::changeLayout()
{
    ui->groupBox_2->unsetLayoutDirection();
}

bool MessageDialog::isAccept() const
{
    return m_accept;
}

bool MessageDialog::isReject() const
{
    return m_reject;
}

void MessageDialog::setTitle(const QString &str)
{
    this->setWindowTitle(str);
}

void MessageDialog::on_ok_clicked()
{
    if (ui->No->isVisible())
        m_accept = true;
    this->close();
}

void MessageDialog::on_No_clicked()
{
    m_accept = false;
    if(ui->cancel->isVisible())
        m_reject = true;
    this->close();
}

void MessageDialog::on_cancel_clicked()
{
    m_accept = false;
    m_reject = false;
    this->close();
}
