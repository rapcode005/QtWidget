#include "distanceconfig.h"
#include "ui_distanceconfig.h"
#include <QRegularExpressionMatch>
#include <QMessageBox>
#include "messagebox.h"

distanceconfig::distanceconfig(QWidget *parent, ProjectFile *pro) :
    QDialog(parent),
    ui(new Ui::distanceconfig)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    ui->setupUi(this);

    proj = pro;
}

distanceconfig::~distanceconfig()
{
    delete ui;
}


void distanceconfig::on_lineEdit_textChanged(const QString &arg1)
{
    QString s = arg1;
    s = s.remove(QRegularExpression("[^0-9.]"));
    ui->lineEdit->setText(s);
}

void distanceconfig::on_saveButton_clicked()
{
    //emit showProgressMessage();
    DistanceConfigObj *current;
    if (proj->getSurveyType() == QLatin1String("WBPHS")) {
        current = getDistanceConfigDataWBPHS();
        //Check if there is error
        if(!current->getStatus()) {
           // const QString &air = QString::number(current->getObsMaxD());
           // const QString &obs = QString::number(current->getAirMaxD());

           // if (hasChangesMade(obs, air)) {
             //   msgBox.show();
               // if(msgBox.isAccept())
                    emit showProgressMessage(current, true);
                //else
                  //  this->close();
            //}
            //else {
              //  this->close();
            //}

        }
    }
    else {
        current = getDistanceConfigData();
        //Check if there is error
        if(!current->getStatus()) {
            //const QString &obs = QString::number(current->getObsMaxD());

            //if (hasChangesMade(obs)) {
              //  msgBox.exec();
                //if(msgBox.isAccept())
                    emit showProgressMessage(current, false);
                //else
                  //  this->close();
            //}
            //else {
            //    this->close();
            //}

            //this->close();
        }
    }
}

void distanceconfig::on_cancelButton_clicked()
{
    this->close();
}

void distanceconfig::distanceConfigOpened()
{
    //Copy Default Value
    ui->lineEdit->setText(QString::number(proj->getObsMaxD()));
    preObs = QString::number(proj->getObsMaxD());

    //Check Survey Type
    if (proj->getSurveyType() == QLatin1String("WBPHS")) {
        statusQWidget(true);
        //Copy Default Value
        ui->airEdit->setText(QString::number(proj->getAirGroundMaxD()));
        preAir = QString::number(proj->getAirGroundMaxD());
    }
    else
        statusQWidget(false);

}

void distanceconfig::closeEvent(QCloseEvent *event)
{
    //Retrieve current value
    //Check Survey Type
    if (proj->getSurveyType() == QLatin1String("WBPHS")) {
        ui->airEdit->setText(QString::number(proj->getAirGroundMaxD()));
    }
    ui->lineEdit->setText(QString::number(proj->getObsMaxD()));
    event->accept();
}

void distanceconfig::statusQWidget(const bool &airGroundIncluded)
{
    ui->lblAir->setVisible(airGroundIncluded);
    ui->airEdit->setVisible(airGroundIncluded);
    ui->airEditkm->setVisible(airGroundIncluded);
}

DistanceConfigObj *distanceconfig::getDistanceConfigData()
{
    bool result;
    QRegExp re("([0-9]*\\.[0-9]+|[0-9]+)");
    const QString &newValue = ui->lineEdit->text();

    //Check for valid Input
    if(newValue.length() < 1){
        QMessageBox::warning(this, "Distance Configuration", "You must specify a distance(km).");
        result = true;
    }
    else if (!re.exactMatch(newValue)) {
        QMessageBox::warning(this, "Distance Configuration", "You must specify a valid number.");
        result = true;
    }
    else {
        result = false;
    }

    DistanceConfigObj *newDConfig = new DistanceConfigObj(newValue.toDouble(), result);
    return newDConfig;
}

DistanceConfigObj *distanceconfig::getDistanceConfigDataWBPHS()
{
    bool result;
    QRegExp re("([0-9]*\\.[0-9]+|[0-9]+)");
    const QString &newValue = ui->lineEdit->text();
    const QString &newValue2 = ui->airEdit->text();

    //Check for valid Input
    if(newValue.length() < 1 && newValue2.length() < 1){
        QMessageBox::warning(this, "Distance Configuration", "You must specify a distance(km).");
        result = true;
    }
    else if (!re.exactMatch(newValue) && !re.exactMatch(newValue2)) {
        QMessageBox::warning(this, "Distance Configuration", "You must specify a valid number.");
        result = true;
    }
    else {
        result = false;
    }
    DistanceConfigObj *newDConfig = new DistanceConfigObj(newValue.toDouble(), newValue2.toDouble(), result);
    return newDConfig;
}

bool distanceconfig::hasChangesMade(const QString &obs)
{
    qDebug() << preObs;
    qDebug() << obs;
    if (obs != preObs)
        return true;

    return false;
}

bool distanceconfig::hasChangesMade(const QString &obs, const QString &air)
{
    if (obs != preObs || air != preAir)
        return true;

    return false;
}


void distanceconfig::on_airEdit_textChanged(const QString &arg1)
{
    QString s = arg1;
    s = s.remove(QRegularExpression("[^0-9.]"));
    ui->airEdit->setText(s);
}
