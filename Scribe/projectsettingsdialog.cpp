#include "projectsettingsdialog.h"
#include "ui_projectsettingsdialog.h"
#include "calenderpopup.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QDirIterator>
#include <QMessageBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDebug>
#include <QHBoxLayout>
#include <QCheckBox>
#include "globalsettings.h"
#include <QJsonObject>
#include "workerThreadSurvey.h"

ProjectSettingsDialog::ProjectSettingsDialog(QWidget *parent, ProjectFile *pro) :
    QDialog(parent),
    ui(new Ui::ProjectSettingsDialog)
{
    setModal(true);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));

    ui->setupUi(this);

    proj = pro;
    activeButton = ui->pushButton;

    speciesDialog = new LogItemEditDialog(this, "Species/Items Edit", true);
    speciesDialog->setModal(true);

    groupingDialog = new LogItemEditDialog(this, "Edit Grouping", false);
    groupingDialog->setModal(true);

    //regular expression that says we can only use alpha numeric characters, important for later in the dialog
    rx.setPattern("([A-Z]|[a-z]|[0-9]| )*");

    //connect(speciesDialog,SIGNAL(Done()),this,SLOT(GetSpeciesAndGrouping()));
    //connect(groupingDialog,SIGNAL(Done()),this,SLOT(GetSpeciesAndGrouping()));

    connect(speciesDialog,SIGNAL(Done()),this,SLOT(GetSpeciesAndGroupingWithoutSlot()));
    connect(groupingDialog,SIGNAL(Done()),this,SLOT(GetSpeciesAndGroupingWithoutSlot()));

    //connect(speciesDialog,SIGNAL(Rejected()),this,SLOT(GetSpeciesAndGroupingReject()));
    //connect(groupingDialog,SIGNAL(Rejected()),this,SLOT(GetSpeciesAndGroupingReject()));

    connect(speciesDialog,SIGNAL(Loaded()),this,SLOT(GetSpeciesAndGrouping()));
    //connect(speciesDialog,SIGNAL(Loaded()),this,SLOT(GetSpeciesAndGroupingWithoutSlot()));

    connect(this,SIGNAL(speciesDialogUpdate(QStringList)),speciesDialog,SLOT(ReceiveItems(QStringList)));
    connect(this,SIGNAL(groupingsDialogUpdate(QStringList)),groupingDialog,SLOT(ReceiveItems(QStringList)));

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->addField->setToolTip(QString("Insert value to additional field."));
    ui->addField_2->setToolTip(QString("Insert value to header field."));
    ui->removeField->setToolTip(QString("Delete selected additional field."));
    ui->removeField_2->setToolTip(QString("Delete selected header field."));
    ui->buttonBox->setToolTip(QString("Save values of header and additional fields."));
    ui->pushButton_2->setToolTip(QString("Open a dialog to edit grouping."));
    ui->pushButton->setToolTip(QString("Open a dialog to edit species/items."));
}

ProjectSettingsDialog::~ProjectSettingsDialog()
{
    delete ui;
}

/**
 * @brief ProjectSettingsDialog::on_addField_pressed
 * Adds a custom field when the user presses the 'plus' button.
 */
void ProjectSettingsDialog::on_addField_pressed()
{
    ui->addedAdditionalFields->setRowCount(ui->addedAdditionalFields->rowCount() + 1);

    QTableWidgetItem *f1 = new QTableWidgetItem("");
    QTableWidgetItem *f2 = new QTableWidgetItem("");

    /*if(ui->addedAdditionalFields->rowCount() + 1 % 2 == 0)
        f1->setData(Qt::BackgroundRole, QVariant(QColor(Qt::#FFFFFF)));
    else
        f1->setData(Qt::BackgroundRole, QVariant(QColor(Qt::#F5F5F5)));*/

    ui->addedAdditionalFields->setItem(ui->addedAdditionalFields->rowCount()-1, 0, f1);
    ui->addedAdditionalFields->setItem(ui->addedAdditionalFields->rowCount()-1, 1, f2);
    //ui->addedAdditionalFields->setItem(ui->addedAdditionalFields->rowCount()-1, 2, f3);//remarked 4.23.2020

    //added 4.22.2020 //add checkbox item
    addCheckBoxAt(ui->addedAdditionalFields->rowCount()-1,2,1,ui->addedAdditionalFields);
    ui->addedAdditionalFields->scrollToBottom();//added 5.1.2020
    proj->changesMade = true;
}

/**
 * @brief ProjectSettingsDialog::on_removeField_pressed
 * Removes a row from added fields.
 */
void ProjectSettingsDialog::on_removeField_pressed()
{
    int rowToKill = ui->addedAdditionalFields->currentRow();
    ui->addedAdditionalFields->removeRow(rowToKill);
    proj->changesMade = true;
}

void ProjectSettingsDialog::PrepHeadFieldValues() {
    try {
        ui->addedHeaderFields->setRowCount(0);
        ui->addedHeaderFields->setColumnCount(3);

        int intwidth = ui->addedHeaderFields->width() - 25;
        ui->addedHeaderFields->setShowGrid(true);

        ui->addedHeaderFields->setColumnWidth(0,intwidth * 0.35);
        ui->addedHeaderFields->setColumnWidth(1,intwidth * 0.35);
        ui->addedHeaderFields->setColumnWidth(2,intwidth * 0.25);

        QStringList labels;
        labels << tr("  Title") << tr("  Values") << tr("  Retain Values");
        ui->addedHeaderFields->setHorizontalHeaderLabels(labels);//border: solid; border-width: 1px;
        ui->addedHeaderFields->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);//added 4.30.2020
        ui->addedHeaderFields->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter);//added 4.30.2020
        ui->addedHeaderFields->setAlternatingRowColors(true);//added 4.30.2020
        ui->addedHeaderFields->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui->addedHeaderFields->verticalHeader()->setDefaultSectionSize(22);
        ui->addedHeaderFields->horizontalHeader()->show();

        ui->addedHeaderFields->horizontalHeader()->setStretchLastSection(true);
        QAbstractButton *button =  ui->addedHeaderFields->findChild<QAbstractButton *>();
        if(button){
            QVBoxLayout *lay = new QVBoxLayout(button);
            //lay->setAlignment(Qt::AlignCenter);
            lay->setContentsMargins(0, 0, 0, 0);
            QLabel *label = new QLabel("#");
            label->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            label->setContentsMargins(0, 0, 0, 0);
            lay->addWidget(label);
        }

        //Text Edit
        //proj->headerFieldsChk.clear();
        //ComboBox
        //proj->hAFieldsCb.clear();

        WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
        workerThread->setProjectFile(proj);
        workerThread->setAction(4);

        connect(workerThread, &WorkerThreadSurvey::finished,
                workerThread, &QObject::deleteLater);

        connect(workerThread, &WorkerThreadSurvey::preHeaderValue, workerThread, [=](const int &rowHeight,
                const int &checkValue,
                const int &currentRow,
                QTableWidgetItem *item1,
                QTableWidgetItem *item2) {

                ui->addedHeaderFields->setRowHeight(currentRow, rowHeight);
                ui->addedHeaderFields->setRowCount(ui->addedHeaderFields->rowCount() + 1);
                ui->addedHeaderFields->setItem(currentRow, 0, item1);
                ui->addedHeaderFields->setItem(currentRow, 1, item2);

                if(checkValue == 2) {
                    addCheckBoxAt(currentRow,2,1,ui->addedHeaderFields);
                }
                else {
                    addCheckBoxAt(currentRow,2,0,ui->addedHeaderFields);
                }
        });

        workerThread->start();
    }
    catch(...) {

    }
}

/* Old Code
void ProjectSettingsDialog::PrepHeadFieldValues(){
    try{
        ui->addedHeaderFields->setRowCount(0);
        ui->addedHeaderFields->setColumnCount(3);// 4.21.2020 changed setColumnCount(2)

        //added 4.21.2020 start
        //int intwidth = (ui->addedHeaderFields->width() - 45) / 3;
        int intwidth = ui->addedHeaderFields->width() - 25;
        ui->addedHeaderFields->setShowGrid(true);

        //for(int i =0; i < ui->addedHeaderFields->columnCount(); i++){
        ui->addedHeaderFields->setColumnWidth(0,intwidth * 0.35);
        ui->addedHeaderFields->setColumnWidth(1,intwidth * 0.35);
        ui->addedHeaderFields->setColumnWidth(2,intwidth * 0.25);
        //}

        QStringList labels;
        //labels << tr("  Title") << tr("  Values") << tr("  Retain Values"); //column headers name
        labels << tr("  Title") << tr("  Values") << tr("  Retain Values");
        ui->addedHeaderFields->setHorizontalHeaderLabels(labels);//border: solid; border-width: 1px;
        ui->addedHeaderFields->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);//added 4.30.2020
        ui->addedHeaderFields->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter);//added 4.30.2020
        ui->addedHeaderFields->setAlternatingRowColors(true);//added 4.30.2020
        ui->addedHeaderFields->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui->addedHeaderFields->verticalHeader()->setDefaultSectionSize(22);
        ui->addedHeaderFields->horizontalHeader()->show();
        //added 4.21.2020 end

        ui->addedHeaderFields->horizontalHeader()->setStretchLastSection(true);
        QAbstractButton *button =  ui->addedHeaderFields->findChild<QAbstractButton *>();
        if(button){
            QVBoxLayout *lay = new QVBoxLayout(button);
            //lay->setAlignment(Qt::AlignCenter);
            lay->setContentsMargins(0, 0, 0, 0);
            QLabel *label = new QLabel("#");
            label->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            label->setContentsMargins(0, 0, 0, 0);
            lay->addWidget(label);
        }

        //Text Edit
        proj->headerFieldsChk.clear();
        //ComboBox
        proj->hAFieldsCb.clear();
        //int cnt = proj->headerFieldsChk.count();
        //The headers field names and values for the display widget are set here.
        int currentRow = 0;
        //qDebug() << "projcount: " << proj->headerValuesChkArray.count() ;
        for (int y = 0; y < proj->getHeaderCount(); y++){
            //Here we'll actually loop the values and create a row for each one and repeat the header title for each.
            QString str = proj->getHeaderName(y);
            QStringList vals = proj->getHeaderValueList(y);

            //qDebug() << vals;

            //QString valchk = proj->getHeaderValueChk(y);//added 4.23.2020//for header as qstring
            QStringList valchk = proj->getHeaderValueChk2(y);

            bool isinstanceHeader = proj->getInstanceHeaderPosition(str) != -1;

            //qDebug() << str;
            //qDebug() << isinstanceHeader;

            if(vals.count() == 0){
                ui->addedHeaderFields->setRowHeight(currentRow,22);
                ui->addedHeaderFields->setRowCount(ui->addedHeaderFields->rowCount()+1);
                ui->addedHeaderFields->setItem(currentRow, 0, new QTableWidgetItem);
                ui->addedHeaderFields->item(currentRow,0)->setText(str);
                ui->addedHeaderFields->setItem(currentRow, 1, new QTableWidgetItem);

                //New Field
                //ui->addedHeaderFields->setItem(currentRow, 2, new QTableWidgetItem("Item3"));
                //ui->addedHeaderFields->item(currentRow, 2)->setCheckState(Qt::Checked);

                //Prefill the instance header data value.
                if(isinstanceHeader){

                    //qDebug() << proj->getInstanceHeaderDataValue(str);

                    ui->addedHeaderFields->item(currentRow,1)->setText(proj->getInstanceHeaderDataValue(str));
                }

                if(valchk[0].toInt() == 2) {
                    addCheckBoxAt(currentRow,2,1,ui->addedHeaderFields);
                }//added 4.23.2020 //set  state checked
                else {
                    addCheckBoxAt(currentRow,2,0,ui->addedHeaderFields);
                    proj->headerFieldsChk <<  str;
                }//added 4.23.2020 //set  state unchecked

                currentRow++;
            }else{
                for (int v = 0; v < vals.count(); v++){
                    ui->addedHeaderFields->setRowHeight(currentRow,22);
                    ui->addedHeaderFields->setRowCount(ui->addedHeaderFields->rowCount()+1);
                    ui->addedHeaderFields->setItem(currentRow, 0, new QTableWidgetItem);
                    ui->addedHeaderFields->item(currentRow,0)->setText(str);

                    ui->addedHeaderFields->setItem(currentRow, 1, new QTableWidgetItem);
                    ui->addedHeaderFields->item(currentRow,1)->setText(vals[v]);

                    //qDebug() << str;
                    //qDebug() << valchk[0].toInt();

                    if(valchk[0].toInt() == 2) {
                        addCheckBoxAt(currentRow,2,1,ui->addedHeaderFields);
                    }//added 4.23.2020 //set  state checked
                    else {
                        addCheckBoxAt(currentRow,2,0,ui->addedHeaderFields);
                        if (vals.count() > 1) {
                            if (!proj->hAFieldsCb.contains(str))
                                proj->hAFieldsCb << str;
                        }
                        else {
                            proj->headerFieldsChk << str;
                        }
                    }//added 4.23.2020 //set  state unchecked

                    currentRow++;
                }
            }
        }

        //qDebug() << proj->headerFieldsChk.at(0);
    }catch(...){
        qDebug() << "error";
    }
}*/

/*void ProjectSettingsDialog::on_observationHeader_clicked(const int &index)
{
    qDebug() << "click";
}*/

void ProjectSettingsDialog::PrepAdditionalFieldValues(){
    try {
        ui->addedAdditionalFields->setRowCount(0);
        ui->addedAdditionalFields->setColumnCount(3);// 4.21.2020 edited 2

        //added 4.22.2020 start
        int intwidth = ui->addedAdditionalFields->width() - 25;
        ui->addedAdditionalFields->setShowGrid(true);

        ui->addedAdditionalFields->setColumnWidth(0,intwidth * 0.35);
        ui->addedAdditionalFields->setColumnWidth(1,intwidth * 0.35);
        ui->addedAdditionalFields->setColumnWidth(2,intwidth * 0.25);

        QStringList labels;
        labels << tr("  Title") << tr("  Values") << tr("  Retain Values"); //column headers name
        ui->addedAdditionalFields->setHorizontalHeaderLabels(labels);
        ui->addedAdditionalFields->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        ui->addedAdditionalFields->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter);
        ui->addedAdditionalFields->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui->addedAdditionalFields->verticalHeader()->setDefaultSectionSize(22);
        ui->addedAdditionalFields->horizontalHeader()->show();
        //added 4.22.2020 end

        //added 6.1.2020 start
        ui->addedAdditionalFields->horizontalHeader()->setStretchLastSection(true);
        QAbstractButton *button =  ui->addedAdditionalFields->findChild<QAbstractButton *>();
        if(button){
            QVBoxLayout *lay = new QVBoxLayout(button);
            //lay->setAlignment(Qt::AlignCenter);
            lay->setContentsMargins(0, 0, 0, 0);
            QLabel *label = new QLabel("#");
            label->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            label->setContentsMargins(0, 0, 0, 0);
            lay->addWidget(label);
        }//added 6.1.2020 end

        proj->additionalFieldsChk.clear();
        proj->aFieldsCb.clear();

        WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
        workerThread->setProjectFile(proj);
        workerThread->setAction(5);

        connect(workerThread, &WorkerThreadSurvey::preAddedValue, workerThread,
                [=](const int &rowHeight,
                const int &checkValue,
                const int &currentRow,
                QTableWidgetItem *item1,
                QTableWidgetItem *item2,
                const int &index) {

                ui->addedAdditionalFields->setRowHeight(currentRow, rowHeight);
                ui->addedAdditionalFields->setRowCount(ui->addedAdditionalFields->rowCount() + 1);
                ui->addedAdditionalFields->setItem(currentRow, 0, item1);
                ui->addedAdditionalFields->setItem(currentRow, 1, item2);

                if (sourceWidget != nullptr) {
                    QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(sourceWidget->cellWidget(index, 1));
                    //Available to textEdit only
                    if (textEditItem != nullptr) {
                        const QString &additionalFieldValueText = textEditItem->toPlainText();
                        ui->addedAdditionalFields->item(currentRow,1)->setText(additionalFieldValueText);
                    }
                }
                else {
                    ui->addedAdditionalFields->item(currentRow,1)->setText("");
                }

                if(checkValue == 2)
                    addCheckBoxAt(currentRow,2,1,ui->addedAdditionalFields);//added 4.22.2020 //checked
                else {
                    addCheckBoxAt(currentRow,2,0,ui->addedAdditionalFields);//added 4.22.2020 //unchecked
                }

        });

        workerThread->start();
    }
    catch(...) {

    }
}

/* Old Code
void ProjectSettingsDialog::PrepAdditionalFieldValues(QTableWidget *sourceWidget){

    try{
        ui->addedAdditionalFields->setRowCount(0);
        ui->addedAdditionalFields->setColumnCount(3);// 4.21.2020 edited 2

        //added 4.22.2020 start
        //int intwidth = (ui->addedAdditionalFields->width() - 45) / 3;
        int intwidth = ui->addedAdditionalFields->width() - 25;
        ui->addedAdditionalFields->setShowGrid(true);

        //for(int i =0; i < ui->addedAdditionalFields->columnCount(); i++){
            ui->addedAdditionalFields->setColumnWidth(0,intwidth * 0.35);
            ui->addedAdditionalFields->setColumnWidth(1,intwidth * 0.35);
            ui->addedAdditionalFields->setColumnWidth(2,intwidth * 0.25);
        //}

        QStringList labels;
        labels << tr("  Title") << tr("  Values") << tr("  Retain Values"); //column headers name

        ui->addedAdditionalFields->setHorizontalHeaderLabels(labels);
        ui->addedAdditionalFields->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        ui->addedAdditionalFields->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter);
        ui->addedAdditionalFields->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui->addedAdditionalFields->verticalHeader()->setDefaultSectionSize(22);
        ui->addedAdditionalFields->horizontalHeader()->show();
        //added 4.22.2020 end

        //added 6.1.2020 start
        ui->addedAdditionalFields->horizontalHeader()->setStretchLastSection(true);
        QAbstractButton *button =  ui->addedAdditionalFields->findChild<QAbstractButton *>();
        if(button){
            QVBoxLayout *lay = new QVBoxLayout(button);
            //lay->setAlignment(Qt::AlignCenter);
            lay->setContentsMargins(0, 0, 0, 0);
            QLabel *label = new QLabel("#");
            label->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            label->setContentsMargins(0, 0, 0, 0);
            lay->addWidget(label);
        }//added 6.1.2020 end

        proj->additionalFieldsChk.clear();
        proj->aFieldsCb.clear();

        //The additional fields names and values for the display widget are set here.
        int currentRow = 0;
        for (int y = 0; y < proj->getAdditionalFieldsCount(); y++){
            //Here we'll actually loop the values and create a row for each one and repeat the header title for each.
            QString str = proj->getAdditionalFieldName(y);
            QStringList vals = proj->getAdditionalFieldValueList(y);
            //QString valchk = proj->getAdditionalFieldChk(y);//remarked woeking for qstring checked
            QStringList valchk = proj->getAdditionalFieldChk2(y);

            if(vals.count() <= 1){
                ui->addedAdditionalFields->setRowHeight(currentRow,22);
                ui->addedAdditionalFields->setRowCount(ui->addedAdditionalFields->rowCount()+1);
                ui->addedAdditionalFields->setItem(currentRow, 0, new QTableWidgetItem);
                ui->addedAdditionalFields->item(currentRow,0)->setText(str);
                ui->addedAdditionalFields->setItem(currentRow, 1, new QTableWidgetItem);

                QLineEdit *lineEditItem = dynamic_cast<QLineEdit*>(sourceWidget->cellWidget(y,1));
                QString additionalFieldValueText = lineEditItem->text();
                ui->addedAdditionalFields->item(currentRow,1)->setText(additionalFieldValueText);

                if (sourceWidget != nullptr) {
                    QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(sourceWidget->cellWidget(y,1));
                    QString additionalFieldValueText = textEditItem->toPlainText();
                    ui->addedAdditionalFields->item(currentRow,1)->setText(additionalFieldValueText);
                }
                else {
                    ui->addedAdditionalFields->item(currentRow,1)->setText("");
                }

                //qDebug() << "A: " << str << vals.count() << valchk[0].toInt() << currentRow;
                if(valchk[0].toInt() == 2)
                    addCheckBoxAt(currentRow,2,1,ui->addedAdditionalFields);//added 4.22.2020 //checked
                else {
                    addCheckBoxAt(currentRow,2,0,ui->addedAdditionalFields);//added 4.22.2020 //unchecked
                    proj->additionalFieldsChk <<  str;
                }

                currentRow++;
            }else{
                for (int v = 0; v < vals.count(); v++){
                    ui->addedAdditionalFields->setRowHeight(currentRow,22);
                    ui->addedAdditionalFields->setRowCount(ui->addedAdditionalFields->rowCount()+1);
                    ui->addedAdditionalFields->setItem(currentRow, 0, new QTableWidgetItem);
                    ui->addedAdditionalFields->item(currentRow,0)->setText(str);

                    ui->addedAdditionalFields->setItem(currentRow, 1, new QTableWidgetItem);
                    ui->addedAdditionalFields->item(currentRow,1)->setText(vals[v]);

                    //qDebug() << "B" << str << vals.count();
                    if(valchk[0].toInt() == 2)
                        addCheckBoxAt(currentRow,2,1,ui->addedAdditionalFields);//added 4.22.2020 //checked
                    else {
                        addCheckBoxAt(currentRow,2,0,ui->addedAdditionalFields);//added 4.22.2020 //unchecked
                        if (vals.count() > 1) {
                            if (!proj->aFieldsCb.contains(str))
                                proj->aFieldsCb << str;
                        }
                        else {
                            proj->additionalFieldsChk <<  str;
                        }
                    }

                    currentRow++;
                }
            }
        }
    }catch(...)
    {

    }
}*/

void ProjectSettingsDialog::on_addField_2_pressed()
{
    //adding a custom field when they press plus
    ui->addedHeaderFields->setRowCount(ui->addedHeaderFields->rowCount() + 1);

    QTableWidgetItem *f1 = new QTableWidgetItem("");
    QTableWidgetItem *f2 = new QTableWidgetItem("");
    //QTableWidgetItem *f3 = new QTableWidgetItem("item3");//remarked 4.23.2020

    //f3->setCheckState(Qt::Checked);//remarked 4.23.2020

    ui->addedHeaderFields->setItem(ui->addedHeaderFields->rowCount()-1, 0, f1);
    ui->addedHeaderFields->setItem(ui->addedHeaderFields->rowCount()-1, 1, f2);
    //ui->addedHeaderFields->setItem(ui->addedHeaderFields->rowCount()-1, 2, f3);//remarked 4.23.2020

    //added 4.22.2020 //add checkbox item
    addCheckBoxAt(ui->addedHeaderFields->rowCount()-1,2,1,ui->addedHeaderFields);
    ui->addedHeaderFields->scrollToBottom();//added 5.1.2020

    proj->changesMade = true;
}

void ProjectSettingsDialog::on_removeField_2_pressed()
{
    int rowToKill = ui->addedHeaderFields->currentRow();
    ui->addedHeaderFields->removeRow(rowToKill);
    proj->changesMade = true;
}


void ProjectSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)//This is the save button
{
    Q_UNUSED(button)

    doSave();
}


void ProjectSettingsDialog::on_pushButton_pressed()
{
    speciesDialog->show();
    activeButton = ui->pushButton;
}


void ProjectSettingsDialog::ProjectOpened()
{
    emit groupingsDialogUpdate(proj->addedGroupings);
    emit speciesDialogUpdate(proj->addedSpecies);

    PrepHeadFieldValues();
}

void ProjectSettingsDialog::on_pushButton_2_pressed()
{
    groupingDialog->show();
    activeButton = ui->pushButton_2;
}

void ProjectSettingsDialog::GetSpeciesAndGrouping(){
     //qDebug() << "Got species and grouping";

     proj->addedSpecies.clear();
     proj->addedGroupings.clear();

     speciesItems = speciesDialog->logItems;
     groupingItems = groupingDialog->logItems;

    for (int i = 0; i < groupingItems.length(); i++){
       proj->addedGroupings.append(groupingItems[i]->itemName + ";" + groupingItems[i]->shortcutKey.toString());
    }

    for (int i = 0; i < speciesItems.length(); i++){
        QString useGrouping = "false";
        if (speciesItems[i]->groupingEnabled){
            useGrouping = "true";
        }
        proj->addedSpecies.append(speciesItems[i]->itemName + ";" + speciesItems[i]->shortcutKey.toString() + ";" + useGrouping);
    }
    proj->changesMade = true;

    emit receivedLogDialogItems();
}

void ProjectSettingsDialog::GetSpeciesAndGroupingWithoutSlot()
{
    proj->addedSpecies.clear();
    proj->addedGroupings.clear();

    speciesItems = speciesDialog->logItems;
    groupingItems = groupingDialog->logItems;

   for (int i = 0; i < groupingItems.length(); i++){
      proj->addedGroupings.append(groupingItems[i]->itemName + ";" + groupingItems[i]->shortcutKey.toString());
   }

   for (int i = 0; i < speciesItems.length(); i++){
       QString useGrouping = "false";
       if (speciesItems[i]->groupingEnabled){
           useGrouping = "true";
       }
       proj->addedSpecies.append(speciesItems[i]->itemName + ";" + speciesItems[i]->shortcutKey.toString() + ";" + useGrouping);
   }

    proj->changesMade = true;
    emit receivedLogDialogItemsWithoutSlot();
}

void ProjectSettingsDialog::disable_field_buttons(){

   ui->addField->setVisible(false);
   ui->addField_2->setVisible(false);

   ui->removeField->setVisible(false);
   ui->removeField_2->setVisible(false);

}

//added 2.21.2020 add checkbox at tablewidget
void ProjectSettingsDialog::addCheckBoxAt(int row_number, int column_number,int state, QTableWidget *qtwidget)
{    
    // Create a widget that will contain a checkbox
    QWidget *checkBoxWidget = new QWidget();
    QCheckBox *checkBox = new QCheckBox();
    checkBox->setTristate(false);// We declare and initialize the checkbox
    QHBoxLayout *layoutCheckBox = new QHBoxLayout(checkBoxWidget); // create a layer with reference to the widget
    layoutCheckBox->addWidget(checkBox);            // Set the checkbox in the layer
    layoutCheckBox->setAlignment(Qt::AlignCenter);  // Center the checkbox
    layoutCheckBox->setContentsMargins(0,0,0,0);    // Set the zero padding

    if(state == 1){
        checkBox->setChecked(true);
    } else {
        checkBox->setChecked(false);
    }

    QString backcolor = "";
    if(row_number % 2 == 0){
        backcolor = "#FFFFFF";
    }else{
        backcolor = "#F5F5F5";
    }

    //qtwidget->setRowHeight(row_number,22);//added 4.29.2020
    checkBoxWidget->resize(qtwidget->width(),qtwidget->height());
    QString css = QString("border: solid; border-width: thin; height: 22px; border-color: #C6C6C680; background-color: %1").arg(backcolor);
    checkBoxWidget->setStyleSheet({css});
    qtwidget->setCellWidget(row_number,column_number, checkBoxWidget);

    //Connect index 3 column which is observation column
    /*if (column_number == 2) {

        chkOberservation.append(checkBox);

        DocLayout *doclayout = new DocLayout();
        doclayout->setIndex(row_number);

        //Header Fields
        if (headerFields) {
            //On Click event for Check Box
            connect(chkOberservation.at(row_number), &QCheckBox::clicked, doclayout, &DocLayout::checkBoxResult);
            connect(doclayout, &DocLayout::sendCheckBox, this, &ProjectSettingsDialog::on_observationHeader_clicked);
        } //Additional Fields
        else {

        }

    }*/

    /*//ver2
    QTableWidgetItem *checkBoxItem = new QTableWidgetItem;
    checkBoxItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    if(state == 1)
        checkBoxItem->setCheckState(Qt::Checked);
    else
        checkBoxItem->setCheckState(Qt::Unchecked);

    qtwidget->setItem(row_number,column_number,checkBoxItem);*/

}

void ProjectSettingsDialog::on_buttonBox_clicked()
{

    progMessage = new progressmessage();
    progMessage->setText("Updating project settings...");
    progMessage->show();
    message = true;
    doSave();
    //this->close();
}

void ProjectSettingsDialog::closeEvent(QCloseEvent *event)
{
    //emit refreshSignals();
    event->accept();
}




void ProjectSettingsDialog::doSave()
{
    /* Old Code
     * QJsonArray headerNames;
    QJsonArray headerValues;
    QJsonArray headerValuesChk;//added 4.22.2020

    //Set the header names
    for (int y = 0; y < ui->addedHeaderFields->rowCount(); y++){
        if (ui->addedHeaderFields->item(y,0) != nullptr){
            QString val = ui->addedHeaderFields->item(y,0)->text();
            if(val.length() > 0 && !headerNames.contains(val)){
                headerNames.append(val);
                headerValues.append(QJsonArray());
                headerValuesChk.append(QJsonArray());//added 4.26.2020
            }
        }
    }
    proj->setHeaderNames(headerNames);

    //Set the header values
    for (int y = 0; y < ui->addedHeaderFields->rowCount(); y++){
        if (ui->addedHeaderFields->item(y,0) != nullptr){
            QString h = ui->addedHeaderFields->item(y,0)->text();
            QString val = ui->addedHeaderFields->item(y,1)->text();

            //added 4.22.2020 start
            int intchk = 0;
            auto chkfield = ui->addedHeaderFields->cellWidget(y, 2);
            QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
            if(chkbox->isChecked()){
                intchk = 2;
            }
            //headerValuesChk.append(QString::number(intchk));//working for single checked value
            int idx0 = proj->getHeaderIndex(h);
            if(idx0 != -1){
                QJsonArray tmp = headerValuesChk[idx0].toArray();
                tmp.append(QString::number(intchk));
                headerValuesChk[idx0] = tmp;
            }
            //added 4.22.2020 end

            if(h.length() < 1 || val.length() < 1){
                continue;
            }
            //Find the index of the header
            int idx = proj->getHeaderIndex(h);
            if(idx != -1){
                QJsonArray tmp = headerValues[idx].toArray();
                tmp.append(val);
                headerValues[idx] = tmp;
            }
        }
    }


    proj->setHeaderValues(headerValues);
    proj->setHeaderValuesChk(headerValuesChk);//added 4.22.2020

    //Refresh heade fields
    PrepHeadFieldValues();*/

    sourceWidget = nullptr;

    WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
    workerThread->setProjectFile(proj);
    workerThread->setAction(6);
    workerThread->setHeaderCount(ui->addedHeaderFields->rowCount());
    workerThread->setAddedCount(ui->addedAdditionalFields->rowCount());

    connect(workerThread, &WorkerThreadSurvey::headerItemByIndex, this,
            [=](const int &index) {

        const bool &valid = (ui->addedHeaderFields->item(index,0) != nullptr);
        QStringList strList;
        if (valid) {
            strList << ui->addedHeaderFields->item(index,0)->text();
            strList << ui->addedHeaderFields->item(index,1)->text();
            auto chkfield = ui->addedHeaderFields->cellWidget(index, 2);
            QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
            if(chkbox->isChecked()){
                strList << "2";
            }
            else
                strList << "0";

            emit sendHeaderValues(strList, index);
        }
    });

    connect(workerThread, &WorkerThreadSurvey::addedItemByIndex, this,
            [=](const int &index) {

        const bool &valid = (ui->addedAdditionalFields->item(index,0) != nullptr);
        QStringList strList;
        if (valid) {
          strList << ui->addedAdditionalFields->item(index,0)->text();
          strList << ui->addedAdditionalFields->item(index,1)->text();
          auto chkfield = ui->addedAdditionalFields->cellWidget(index, 2);
          QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
          if(chkbox->isChecked()){
              strList << "2";
          }
          else
              strList << "0";
        }

        emit sendAddedValues(strList, index);
    });

    connect(workerThread, &WorkerThreadSurvey::finished,
            workerThread, &QObject::deleteLater);

    connect(this, &ProjectSettingsDialog::sendHeaderValues,
            workerThread, &WorkerThreadSurvey::setHeaderValues);

    connect(this, &ProjectSettingsDialog::sendAddedValues,
            workerThread, &WorkerThreadSurvey::setAddedValues);

    connect(workerThread, &WorkerThreadSurvey::refreshDone,
            this, [=]() {

        proj->changesMade = true;

        emit receivedLogDialogItems();
        //progMessage->okToClose = true;
        //progMessage->close();

    });

    workerThread->start();


    //Set the additional field values
    /*QJsonArray additionalFieldNames;
    QJsonArray additionalFieldValues;
    QJsonArray additionalFieldValuesChk;//added 5.6.2020

    for (int n = 0; n < ui->addedAdditionalFields->rowCount(); n++) {
        if (ui->addedAdditionalFields->item(n,1) != nullptr) {
            QString val = ui->addedAdditionalFields->item(n,0)->text();
            if(val.length() > 0 && !additionalFieldNames.contains(val)){
                additionalFieldNames.append(val);
                additionalFieldValues.append(QJsonArray());
                additionalFieldValuesChk.append(QJsonArray());//added 4.26.2020
            }
        }
    }
    proj->setAdditionalFieldNames(additionalFieldNames);


    proj->additionalFieldsChk.clear();
    proj->aFieldsCb.clear();

    for (int y = 0; y < ui->addedAdditionalFields->rowCount(); y++){
        if (ui->addedAdditionalFields->item(y,0) != nullptr){
            QString h = ui->addedAdditionalFields->item(y,0)->text();
            QString val = ui->addedAdditionalFields->item(y,1)->text();

            if(h.length() < 1){
                continue;
            }

            //added 4.22.2020 start
            int intchk = 0;
            auto chkfield = ui->addedAdditionalFields->cellWidget(y, 2);
            QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
            //qDebug() << h << ": " <<  chkbox->isChecked();
            if(chkbox->isChecked()){
                intchk = 2;
            }
            //additionalFieldValuesChk.append(QString::number(intchk));//remarked woring for single checked value
            int idx0 = proj->getAdditionalFieldIndex(h);
            //qDebug() << h << ": " <<  chkbox->isChecked();
            if(idx0 != -1){
                QJsonArray tmp = additionalFieldValuesChk[idx0].toArray();
                tmp.append(QString::number(intchk));
                additionalFieldValuesChk[idx0] = tmp;
            }
           //added 4.22.2020 end//added 4.22.2020 end

            //Find the index of the additional field
            int idx = proj->getAdditionalFieldIndex(h);
            if(idx != -1){
                QJsonArray tmp = additionalFieldValues[idx].toArray();
                tmp.append(val);
                additionalFieldValues[idx] = tmp;
            }
        }
    }
    proj->setAdditionalFieldValues(additionalFieldValues);
    proj->setAdditionalFieldValuesChk(additionalFieldValuesChk);

    //PrepAdditionalFieldValues(nullptr);
    //sourceWidget = nullptr;
    //PrepAdditionalFieldValues();
    */

    /*QJsonArray headerNames;
    QJsonArray headerValues;
    QJsonArray headerValuesChk;//added 4.22.2020

    //Set the header names
    for (int y = 0; y < ui->addedHeaderFields->rowCount(); y++){
        if (ui->addedHeaderFields->item(y,0) != nullptr){
            QString val = ui->addedHeaderFields->item(y,0)->text();
            if(val.length() > 0 && !headerNames.contains(val)){
                headerNames.append(val);
                headerValues.append(QJsonArray());
                headerValuesChk.append(QJsonArray());//added 4.26.2020
            }
        }
    }
    proj->setHeaderNames(headerNames);

    //Set the header values
    for (int y = 0; y < ui->addedHeaderFields->rowCount(); y++){
        if (ui->addedHeaderFields->item(y,0) != nullptr){
            QString h = ui->addedHeaderFields->item(y,0)->text();
            QString val = ui->addedHeaderFields->item(y,1)->text();

            //added 4.22.2020 start
            int intchk = 0;
            auto chkfield = ui->addedHeaderFields->cellWidget(y, 2);
            QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
            if(chkbox->isChecked()){
                intchk = 2;
            }
            //headerValuesChk.append(QString::number(intchk));//working for single checked value
            int idx0 = proj->getHeaderIndex(h);
            if(idx0 != -1){
                QJsonArray tmp = headerValuesChk[idx0].toArray();
                tmp.append(QString::number(intchk));
                headerValuesChk[idx0] = tmp;
            }
            //added 4.22.2020 end

            if(h.length() < 1 || val.length() < 1){
                continue;
            }
            //Find the index of the header
            int idx = proj->getHeaderIndex(h);
            if(idx != -1){
                QJsonArray tmp = headerValues[idx].toArray();
                tmp.append(val);
                headerValues[idx] = tmp;
            }

        }
    }
    proj->setHeaderValues(headerValues);
    proj->setHeaderValuesChk(headerValuesChk);//added 4.22.2020

    //Set the additional field values

    QJsonArray additionalFieldNames;
    QJsonArray additionalFieldValues;
    QJsonArray additionalFieldValuesChk;//added 5.6.2020

    for (int n = 0; n < ui->addedAdditionalFields->rowCount(); n++) {
        if (ui->addedAdditionalFields->item(n,1) != nullptr) {
            QString val = ui->addedAdditionalFields->item(n,0)->text();
            if(val.length() > 0 && !additionalFieldNames.contains(val)){
                additionalFieldNames.append(val);
                additionalFieldValues.append(QJsonArray());
                additionalFieldValuesChk.append(QJsonArray());//added 4.26.2020
            }
        }
    }
    proj->setAdditionalFieldNames(additionalFieldNames);

    for (int y = 0; y < ui->addedAdditionalFields->rowCount(); y++){
        if (ui->addedAdditionalFields->item(y,0) != nullptr){
            QString h = ui->addedAdditionalFields->item(y,0)->text();
            QString val = ui->addedAdditionalFields->item(y,1)->text();

            if(h.length() < 1){
                continue;
            }

            //added 4.22.2020 start
            int intchk = 0;
            auto chkfield = ui->addedAdditionalFields->cellWidget(y, 2);
            QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
            //qDebug() << h << ": " <<  chkbox->isChecked();
            if(chkbox->isChecked()){
                intchk = 2;
            }
            //additionalFieldValuesChk.append(QString::number(intchk));//remarked woring for single checked value
            int idx0 = proj->getAdditionalFieldIndex(h);
            //qDebug() << h << ": " <<  chkbox->isChecked();
            if(idx0 != -1){
                QJsonArray tmp = additionalFieldValuesChk[idx0].toArray();
                tmp.append(QString::number(intchk));
                additionalFieldValuesChk[idx0] = tmp;
            }
           //added 4.22.2020 end//added 4.22.2020 end

            //Find the index of the additional field
            int idx = proj->getAdditionalFieldIndex(h);
            if(idx != -1){
                QJsonArray tmp = additionalFieldValues[idx].toArray();
                tmp.append(val);
                additionalFieldValues[idx] = tmp;
            }
        }
    }
    proj->setAdditionalFieldValues(additionalFieldValues);
    proj->setAdditionalFieldValuesChk(additionalFieldValuesChk);*/


    /*QJsonArray headerNames;
    QJsonArray headerValues;
    QJsonArray headerValuesChk;//added 4.22.2020

    //Set the header names
    for (int y = 0; y < ui->addedHeaderFields->rowCount(); y++){
        if (ui->addedHeaderFields->item(y,0) != nullptr){
            QString val = ui->addedHeaderFields->item(y,0)->text();
            if(val.length() > 0 && !headerNames.contains(val)){
                headerNames.append(val);
                headerValues.append(QJsonArray());
                headerValuesChk.append(QJsonArray());//added 4.26.2020
            }
        }
    }
    proj->setHeaderNames(headerNames);

    //Set the header values
    for (int y = 0; y < ui->addedHeaderFields->rowCount(); y++){
        if (ui->addedHeaderFields->item(y,0) != nullptr){
            QString h = ui->addedHeaderFields->item(y,0)->text();
            QString val = ui->addedHeaderFields->item(y,1)->text();

            //added 4.22.2020 start
            int intchk = 0;
            auto chkfield = ui->addedHeaderFields->cellWidget(y, 2);
            QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
            if(chkbox->isChecked()){
                intchk = 2;
            }
            //headerValuesChk.append(QString::number(intchk));//working for single checked value
            int idx0 = proj->getHeaderIndex(h);
            if(idx0 != -1){
                QJsonArray tmp = headerValuesChk[idx0].toArray();
                tmp.append(QString::number(intchk));
                headerValuesChk[idx0] = tmp;
            }
            //added 4.22.2020 end

            if(h.length() < 1 || val.length() < 1){
                continue;
            }
            //Find the index of the header
            int idx = proj->getHeaderIndex(h);
            if(idx != -1){
                QJsonArray tmp = headerValues[idx].toArray();
                tmp.append(val);
                headerValues[idx] = tmp;
            }

        }
    }
    proj->setHeaderValues(headerValues);
    proj->setHeaderValuesChk(headerValuesChk);//added 4.22.2020

    //qDebug() << "additional1";
    //Set the additional field values
    QJsonArray additionalFieldNames;
    QJsonArray additionalFieldValues;
    QJsonArray additionalFieldValuesChk;//added 5.6.2020

    for (int n = 0; n < ui->addedAdditionalFields->rowCount(); n++) {
        if (ui->addedAdditionalFields->item(n,1) != nullptr) {
            QString val = ui->addedAdditionalFields->item(n,0)->text();
            if(val.length() > 0 && !additionalFieldNames.contains(val)){
                additionalFieldNames.append(val);
                additionalFieldValues.append(QJsonArray());
                additionalFieldValuesChk.append(QJsonArray());//added 4.26.2020
            }
        }
    }
    proj->setAdditionalFieldNames(additionalFieldNames);

    //qDebug() << "additional2";
    for (int y = 0; y < ui->addedAdditionalFields->rowCount(); y++){
        if (ui->addedAdditionalFields->item(y,0) != nullptr){
            QString h = ui->addedAdditionalFields->item(y,0)->text();
            QString val = ui->addedAdditionalFields->item(y,1)->text();

            if(h.length() < 1){
                continue;
            }

            //added 4.22.2020 start
            int intchk = 0;
            auto chkfield = ui->addedAdditionalFields->cellWidget(y, 2);
            QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
            //qDebug() << h << ": " <<  chkbox->isChecked();
            if(chkbox->isChecked()){
                intchk = 2;
            }
            //additionalFieldValuesChk.append(QString::number(intchk));//remarked woring for single checked value
            int idx0 = proj->getAdditionalFieldIndex(h);
            //qDebug() << h << ": " <<  chkbox->isChecked();
            if(idx0 != -1){
                QJsonArray tmp = additionalFieldValuesChk[idx0].toArray();
                tmp.append(QString::number(intchk));
                additionalFieldValuesChk[idx0] = tmp;
            }
           //added 4.22.2020 end//added 4.22.2020 end

            //Find the index of the additional field
            int idx = proj->getAdditionalFieldIndex(h);
            if(idx != -1){
                QJsonArray tmp = additionalFieldValues[idx].toArray();
                tmp.append(val);
                additionalFieldValues[idx] = tmp;
            }
        }
    }
    proj->setAdditionalFieldValues(additionalFieldValues);
    proj->setAdditionalFieldValuesChk(additionalFieldValuesChk);*/

//    qDebug() << "---------------";
//    qDebug() << proj->headerNamesArray;
//    qDebug() << proj->headerValuesArray;
//    qDebug() << proj->addionalFieldsNamesArray;
//    qDebug() << proj->addionalFieldsValuesArray;
//    qDebug() << proj->headerNamesArray << proj->headerValuesChkArray;
//    qDebug() << proj->addionalFieldsNamesArray << proj->addionalFieldsValuesChkArray;


    //proj->changesMade = true;
    //emit receivedLogDialogItems();
    //proj->HasAutoSave();
}
