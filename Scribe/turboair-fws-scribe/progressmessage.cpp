#include "progressmessage.h"
#include "ui_progressmessage.h"

progressmessage::progressmessage(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::progressmessage)
{
    this->window()->setWindowTitle("Scribe");

    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowCloseButtonHint);
    this->setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));

    ui->setupUi(this);

    QFont cellFont;
    cellFont.setFamily("Segoe UI");
    cellFont.setPixelSize(13);

    ui->message->setFont(cellFont);

    this->setStyleSheet("QProgressBar {"
                        "border: 1px solid #707070;"
                        //"border-radius: 5px;"
                        "Color: #000000;"
                        "background-color: #F7F7F7;"
                        "text-align: center;"
                        "height: 18px; } "
                        "QProgressBar::chunk {"
                        "background-color: #707070;}");

    /*QLinearGradient buttonGradient(0, 0, 0, ui->progressBar->maximumHeight());
    buttonGradient.setColorAt(0, QColor(112, 112, 112));
    buttonGradient.setColorAt(1, QColor(112, 112, 112));

    QPalette palette = ui->progressBar->palette();
    palette.setBrush(QPalette::Base, buttonGradient);

    ui->progressBar->setPalette(palette);*/

    okToClose = false;
}

progressmessage::~progressmessage()
{
    delete ui;
}

void progressmessage::setText(const QString &newText)
{
    ui->message->setText(newText);
    ui->progressBar->setVisible(false);
}

void progressmessage::setMax(const int &max)
{
    ui->progressBar->setMaximum(max);
    ui->message->setVisible(false);
    ui->progressBar->setValue(0);
}

void progressmessage::setValue(const int &value)
{
    ui->progressBar->setValue(value);
}

void progressmessage::closeEvent(QCloseEvent *event)
{
    if (okToClose) {
        event->accept();
    }
    else {
        event->ignore();
    }
}
