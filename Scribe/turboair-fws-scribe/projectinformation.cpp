#include "globalsettings.h"
#include "projectinformation.h"
#include "ui_projectinformation.h"

ProjectInformation::ProjectInformation(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectInformation)
{
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    //clear values
    ui->lblFileName->setText("");
    ui->lblSurveyType->setText("");
    ui->lblDirectory->setText("");

    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    int export_count = globalSettings.get("exportCount").toInt();
    globalSettings.set( "exportCount", QString( export_count++ ) );
    QString exportLocation = globalSettings.get("projectDirectory") + "/";
    QString exportFileNm = globalSettings.get("exportFilename") + ".asc";
    QString surveyType = globalSettings.get("surveyType");

    ui->lblFileName->setText(exportFileNm);
    ui->lblSurveyType->setText(surveyType);
    ui->lblDirectory->setText(exportLocation);
}

ProjectInformation::~ProjectInformation()
{
    delete ui;
}

void ProjectInformation::on_buttonBox_clicked()
{
    this->close();
}
