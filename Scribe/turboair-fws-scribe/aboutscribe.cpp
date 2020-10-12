#include "aboutscribe.h"
//#include "ui_aboutscribe.h"


aboutscribe::aboutscribe(QWidget *parent) :
    QWidget(parent)
    //ui(new Ui::aboutscribe)
{
    //ui->setupUi(this);
    //view->setUrl();
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    window()->setWindowTitle("About Scribe");
    this->resize(1280, 720);

    QWebEngineView *view = new QWebEngineView();
    view->setHtml("<!DOCTYPE html>"
                  "<html>"
                  "<body>"
                  " "
                  "<h1>About Sribe</h1>"
                  " "
                  "</body>"
                  "</html>");
    view->setVisible(true);
    //view->resize(1280, 720);
    QGridLayout *ex = new QGridLayout();
    ex->addWidget(view);
    setLayout(ex);
    //view->show();
}

//aboutscribe::~aboutscribe()
//{
    //delete ui;
//}
