//#include "mainwindow.h"
//#include "mainwindow2.h"
#include <QApplication>
#include "newmainwindow.h"
#include "testmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MainWindow w;
    //MainWindow2 w;

    qRegisterMetaType<QList<QStringList> >("QList<QStringList>");
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);//added 8.11.2020
    newmainwindow w;
    w.show();

    /*TestMainWindow w;
    w.show();*/

    return a.exec();
}
