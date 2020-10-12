#ifndef DISTANCECONFIG_H
#define DISTANCECONFIG_H

#include <QDialog>
#include "globalsettings.h"
#include <QDebug>
#include <QLineEdit>
#include "projectfile.h"
#include <QCloseEvent>
#include "distanceConfigObj.h"
#include "progressmessage.h"

namespace Ui {
class distanceconfig;
}

class distanceconfig : public QDialog
{
    Q_OBJECT

public:
    explicit distanceconfig(QWidget *parent = nullptr, ProjectFile *pro = nullptr);
    ~distanceconfig();
    QLineEdit *res;
    ProjectFile *proj;
    bool retainValue = false;
    QString preObs, preAir;

signals:
    void saveNewValues(const double &newObsMaxD);
    void saveNewValues(const double &newObsMaxD, const double &newAirMaxD);
    void showProgressMessage(DistanceConfigObj *dc, const bool &WBPHS);
    //void closeProgressMessage();

public slots:
    void distanceConfigOpened();

private slots:

    void on_lineEdit_textChanged(const QString &arg1);

    void on_saveButton_clicked();

    void on_cancelButton_clicked();

    void closeEvent(QCloseEvent *event);

    void on_airEdit_textChanged(const QString &arg1);

private:
    Ui::distanceconfig *ui;
    void statusQWidget(const bool &airGroundIncluded); // With/out Air Ground
    DistanceConfigObj *getDistanceConfigData();
    DistanceConfigObj *getDistanceConfigDataWBPHS();
    progressmessage *progMessage;
    bool hasChangesMade(const QString &obs);
    bool hasChangesMade(const QString &obs, const QString &air);
    //DistanceConfigObj *distanceConfig;
};

#endif // DISTANCECONFIG_H
