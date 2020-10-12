#include "userguide.h"
#include "ui_userguide.h"

userguide::userguide(QWidget *parent) :
    QWidget(parent)
    //ui(new Ui::userguide)
{
    //ui->setupUi(this);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    window()->setWindowTitle("User Guide");
    this->resize(1280, 720);

    QWebEngineView *view = new QWebEngineView();
    view->setHtml("<!DOCTYPE html>"
                  "<html>"
                  "<body>"
                  " "
                  "<h1>User Guide</h1>"
                  " "
                  "</body>"
                  "</html>");
    view->setVisible(true);
    //view->resize(1280, 720);
    QGridLayout *ex = new QGridLayout();
    ex->addWidget(view);
    setLayout(ex);
}

//userguide::~userguide()
//{
  //  delete ui;
//}
