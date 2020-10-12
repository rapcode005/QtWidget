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
#include "messagebox.h"


ProjectSettingsDialog::ProjectSettingsDialog(QWidget *parent, ProjectFile *pro) :
    QDialog(parent),
    ui(new Ui::ProjectSettingsDialog)
{
    setModal(true);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));

    loadHeaderAdditional = false;

    ui->setupUi(this);

    proj = pro;
    activeButton = ui->pushButton;

    message = false;

    speciesDialog = new LogItemEditDialog(this, "Species/Items Edit", true,
                                          "Specimen and Hotkeys");
    speciesDialog->setModal(true);

    groupingDialog = new LogItemEditDialog(this, "Edit Grouping", false,
                                           "Groupings");
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
    //proj->changesMade = true;
}

/**
 * @brief ProjectSettingsDialog::on_removeField_pressed
 * Removes a row from added fields.
 */
void ProjectSettingsDialog::on_removeField_pressed()
{
    int rowToKill = ui->addedAdditionalFields->currentRow();
    ui->addedAdditionalFields->removeRow(rowToKill);
    //proj->changesMade = true;
}

bool ProjectSettingsDialog::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() != QEvent::Paint)
        return false;

    //Paint by hand (borrowed from QTableCornerButton)
    if (target == ui->addedHeaderFields->findChild<QAbstractButton *>()) {

        QStyleOptionHeader opt;
        opt.init(ui->addedHeaderFields->findChild<QAbstractButton *>());
        QFlags<QStyle::StateFlag> styleState = QStyle::State_None;

        if (ui->addedHeaderFields->findChild<QAbstractButton *>()->isEnabled())
            styleState |= QStyle::State_Enabled;
        if (ui->addedHeaderFields->findChild<QAbstractButton *>()->isActiveWindow())
            styleState |= QStyle::State_Active;
        if (ui->addedHeaderFields->findChild<QAbstractButton *>()->isDown())
            styleState |=QStyle::State_Sunken;

        opt.state = styleState;
        opt.rect = ui->addedHeaderFields->findChild<QAbstractButton *>()->rect();

        //This line is the only difference to QTableCornerButton
        opt.text = ui->addedHeaderFields->findChild<QAbstractButton *>()->text();
        opt.position = QStyleOptionHeader::OnlyOneSection;
        QStylePainter painter(ui->addedHeaderFields->findChild<QAbstractButton *>());
        painter.drawControl(QStyle::CE_Header, opt);
        return true; // eat event
    }
    else if(target == ui->addedAdditionalFields->findChild<QAbstractButton *>()) {
        QStyleOptionHeader opt;
        opt.init(ui->addedAdditionalFields->findChild<QAbstractButton *>());
        QFlags<QStyle::StateFlag> styleState = QStyle::State_None;

        if (ui->addedAdditionalFields->findChild<QAbstractButton *>()->isEnabled())
            styleState |= QStyle::State_Enabled;
        if (ui->addedAdditionalFields->findChild<QAbstractButton *>()->isActiveWindow())
            styleState |= QStyle::State_Active;
        if (ui->addedAdditionalFields->findChild<QAbstractButton *>()->isDown())
           styleState |=QStyle::State_Sunken;

        opt.state = styleState;
        opt.rect = ui->addedAdditionalFields->findChild<QAbstractButton *>()->rect();

        //This line is the only difference to QTableCornerButton
        opt.text = ui->addedAdditionalFields->findChild<QAbstractButton *>()->text();
        opt.position = QStyleOptionHeader::OnlyOneSection;
        QStylePainter painter(ui->addedAdditionalFields->findChild<QAbstractButton *>());
        painter.drawControl(QStyle::CE_Header, opt);
        return true; // eat event
    }

    return ProjectSettingsDialog::eventFilter(target, event);
}

bool ProjectSettingsDialog::hasChangesMade()
{
    //Validation

    qDebug() << ui->addedHeaderFields->rowCount();
    qDebug() << preHeaderCount;
    qDebug() << ui->addedAdditionalFields->rowCount();
    qDebug() << preAdditionalCount;

    if(ui->addedHeaderFields->rowCount() != preHeaderCount){
        changeFields = "Header Fields";
        return true;
    }

    if (ui->addedAdditionalFields->rowCount() != preAdditionalCount) {
        changeFields = "Additional Header Fields";
        return true;
    }

    qDebug() << "Header";
    for(int i = 0; i < headerValues.count(); i++) {
        const QString &str = proj->getHeaderName(i);
        const QJsonValue &curValV = headerValues.at(i);
        const QJsonValue &preValV = preHeaderValues.at(i);
        const QJsonValue &curValC = headerValuesChk.at(i);
        const QJsonValue &preValC = preHeaderValuesChk.at(i);

        if ((proj->getSurveyType() == QLatin1String("BAEA") ||
             proj->getSurveyType() == QLatin1String("WBPHS")) &&
                proj->getInstanceHeaderPosition(str) != -1){

            //If Blank from
            if (preValV.toArray().count() != 0) {
                if (curValV.toString().toLower() != preValV.toString().toLower() ||
                        curValC != preValC) {
                    changeFields = "Header Fields";
                    return true;
                }
            }

        }
        else {

            if (preValV.toArray().count() == 0) {
                if (curValV.toArray().at(0).toString() != "") {
                    changeFields = "Header Fields";
                    return true;
                }

                if (curValC != preValC) {
                    changeFields = "Header Fields";
                    return true;
                }
            }
            else if (curValV.toString().toLower() != preValV.toString().toLower() ||
                    curValC != preValC) {
                changeFields = "Header Fields";
                return true;
            }
        }
    }

    //Additional
    qDebug() << "Additional";
    for(int i = 0; i < additionalFieldValues.count(); i++) {
        const QJsonValue &curValV = additionalFieldValues.at(i);
        const QJsonValue &preValV = preAdditionalFieldValues.at(i);
        const QJsonValue &curValC = additionalFieldValuesChk.at(i);
        const QJsonValue &preValC = preAdditionalFieldValuesChk.at(i);

        //Should be equal QJsonValue(array, QJsonArray([""])) == QJsonValue(array, QJsonArray())
        if (preValV.toArray().count() == 0) {
            if (!curValV.toArray().at(0).toString().isEmpty()) {
                changeFields = "Additional Header Fields";
                return true;
            }

            if (curValC != preValC) {
                changeFields = "Additional Header Fields";
                return true;
            }
        }
        else if (curValV != preValV ||
                curValC != preValC) {
            changeFields = "Additional Header Fields";
            return true;
        }
    }

    for (int i = 0; i < headerNames.count(); i++) {
        const QJsonValue &curValN = headerNames.at(i);
        const QJsonValue &preValN = preHeaderNames.at(i);

        if (curValN.toString().toLower() != preValN.toString().toLower())  {
            changeFields = "Header Fields";
            return true;
        }
    }

    for (int i = 0; i < additionalFieldNames.count(); i++) {
        const QJsonValue &curValN = additionalFieldNames.at(i);
        const QJsonValue &preValN = preAdditionalFieldNames.at(i);

        if (curValN.toString().toLower() != preValN.toString().toLower()) {
            changeFields = "Additional Header Fields";
            return true;
        }
    }

    return false;
}

QJsonValue ProjectSettingsDialog::getValues(const QJsonValue &val) const
{
    if (val.toArray().count() > 1) {
        foreach(QJsonValue j, val.toArray()) {
            QJsonObject jO;
            QJsonArray jA;
            jA.insert(0, j.toString());
            jO.insert("value", jA);
            return jO.find("value").value();
        }
    }
    return val;
}

void ProjectSettingsDialog::PrepHeadFieldValues() {
    try {

        if (openingWindow) {
            progMessage = new progressmessage(this); //this 7.30.2020
            progMessage->setMax(50);
            progMessage->show();
        }

        ui->addedHeaderFields->setRowCount(0);
        ui->addedHeaderFields->setColumnCount(3);

        const int &intwidth = ui->addedHeaderFields->width() - 25;
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

        QAbstractButton *button = ui->addedHeaderFields->findChild<QAbstractButton *>();
        if(button) {
            button->setText(tr("#"));
            button->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            button->setContentsMargins(0, 0, 0, 0);
            button->installEventFilter(this);

            QStyleOptionHeader *opt = new QStyleOptionHeader();
            opt->text = button->text();
            QSize *s = new QSize(button->style()->sizeFromContents(QStyle::CT_HeaderSection, opt,
                                                                   QSize(), button).expandedTo(QApplication::globalStrut()));
            if (s->isValid())
                ui->addedHeaderFields->verticalHeader()->setMinimumWidth(s->width());
        }

        //Text Edit
        proj->headerFieldsChk.clear();
        //ComboBox
        proj->hAFieldsCb.clear();

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


/*
void ProjectSettingsDialog::updateHeaders(const QString &item1, const QString &item2, const int &checkvalue)
{
    //Header Fields
    if (headerNames.count() == 0) {
        QJsonArray tmp;
        tmp.append(item1);
        headerNames.append(tmp);
        QJsonArray tmp1;
        tmp1.append(QJsonValue::Array);
        headerValues.append(tmp1);
        QJsonArray tmp2;
        tmp2.append(QJsonValue::Array);
        headerValuesChk.append(tmp2);
    }
    else if (!headerNames.at(headerNames.count() - 1).contains(item1)) {
        //const int &id = proj->getHeaderIndex(item1);
        QJsonArray tmp = headerNames.at(headerNames.count() - 1);
        tmp.append(item1);
        headerNames[headerNames.count() - 1] = tmp;
       QJsonArray tmp1 = headerValues.at(headerValues.count() - 1);
        tmp1.insert(id, QJsonValue::Array);
        headerValues[headerValues.count() - 1] = tmp1;
        QJsonArray tmp2 = headerValuesChk.at(headerValuesChk.count() - 1);
        tmp2.insert(id, QJsonValue::Array);
        headerValuesChk[headerValuesChk.count() - 1] = tmp2;
    }


}*/

/*
void ProjectSettingsDialog::PrepHeadFieldValues() {
    try {

        if (openingWindow) {
            progMessage = new progressmessage();
            progMessage->setMax(50);
            progMessage->show();

            headerNames.clear();
            headerValues.clear();
            headerValuesChk.clear();
            additionalFieldNames.clear();
            additionalFieldValues.clear();
            additionalFieldValuesChk.clear();
        }

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


        QAbstractButton *button = ui->addedHeaderFields->findChild<QAbstractButton *>();
        if(button) {
            button->setText(tr("#"));
            button->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            button->setContentsMargins(0, 0, 0, 0);
            button->installEventFilter(this);

            QStyleOptionHeader *opt = new QStyleOptionHeader();
            opt->text = button->text();
            QSize *s = new QSize(button->style()->sizeFromContents(QStyle::CT_HeaderSection, opt,
                                                                   QSize(), button).expandedTo(QApplication::globalStrut()));
            if (s->isValid())
                ui->addedHeaderFields->verticalHeader()->setMinimumWidth(s->width());
        }

        //Text Edit
        proj->headerFieldsChk.clear();
        //ComboBox
        proj->hAFieldsCb.clear();

        WorkerThreadSurvey *workerThread = new WorkerThreadSurvey();
        workerThread->setProjectFile(proj);
        workerThread->setAction(4);

        connect(workerThread, &WorkerThreadSurvey::finished,
                workerThread, &QObject::deleteLater);

        connect(workerThread, &WorkerThreadSurvey::refreshDone, this, [=]() {

            QJsonArray tmp;
            int pre = -1;

            for (int y = 0; y < ui->addedHeaderFields->rowCount(); y++){
                if (ui->addedHeaderFields->item(y,0) != nullptr){
                    const QString &h = ui->addedHeaderFields->item(y,0)->text();
                    const QString &val = ui->addedHeaderFields->item(y,1)->text();

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
                        //headerValuesChk[headerValuesChk.count() - 1].
                          //      at(idx0).toArray() << QString::number(intchk);

                        if(proj->getHeaderIndex(h)) {

                        }
                    }

                    //added 4.22.2020 end

                    if(h.length() < 1 || val.length() < 1){
                        continue;
                    }
                    //Find the index of the header
                    int idx = proj->getHeaderIndex(h);
                    if(idx != -1){
                        auto hv = headerValues[headerValues.count() - 1];
                        QJsonArray tmp = hv[idx].toArray();
                        tmp.append(val);
                        hv[idx] = tmp;
                    }

                }
            }

            qDebug() << headerNames;
            qDebug() << headerValues;
            qDebug() << headerValuesChk;

        });

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

                //Use for Saving
                updateHeaders(item1->text(), item2->text(), checkValue);

        });

        workerThread->start();
    }
    catch(...) {

    }
}*/

/*void ProjectSettingsDialog::updateAdditional(const QString &item1, const QString &item2, const int &checkvalue)
{
    //Additional Fields
    if (additionalFieldNames.count() == 0) {
        QJsonArray tmp;
        tmp.append(item1);
        additionalFieldNames.append(tmp);
        QJsonArray tmp1;
        tmp1.append(QJsonArray());
        additionalFieldValues.append(tmp1);
        QJsonArray tmp2;
        tmp2.append(QJsonArray());
        additionalFieldValuesChk.append(tmp2);
    }
    else if (!additionalFieldNames.at(additionalFieldNames.count() - 1).contains(item1)) {
        QJsonArray tmp = additionalFieldNames.at(additionalFieldNames.count() - 1);
        tmp.append(item1);
        additionalFieldNames[additionalFieldNames.count() - 1] = tmp;
        QJsonArray tmp1 = additionalFieldValues.at(additionalFieldValues.count() - 1);
        tmp1.append(QJsonArray());
        additionalFieldValues[additionalFieldValues.count() - 1] = tmp1;
        QJsonArray tmp2 = additionalFieldValuesChk.at(additionalFieldValuesChk.count() - 1);
        tmp2.append(QJsonArray());
        additionalFieldValuesChk[additionalFieldValuesChk.count() - 1] = tmp2;
    }
}*/
    /*const int &id = proj->getAdditionalFieldIndex(item1);
    if(id != -1){
        auto hvc = additionalFieldValuesChk.at(additionalFieldValuesChk.count() - 1);
        QJsonArray tmp = hvc[id].toArray();
        tmp.append(QString::number(checkvalue));
        hvc[id] = tmp;

        auto hv = additionalFieldValues.at(additionalFieldValues.count() - 1);
        QJsonArray tmp1 = hv[id].toArray();
        tmp1.append(item2);
        hv[id] = tmp1;
    }
}

//Old Code
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

/*
void ProjectSettingsDialog::PrepAdditionalFieldValues(QTableWidget *sourceWidget){
    try {

        if (openingWindow) {
            progMessage->setValue(40);
        }

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

        QAbstractButton *button = ui->addedAdditionalFields->findChild<QAbstractButton *>();
        if(button) {
            button->setText(tr("#"));
            button->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            button->setContentsMargins(0, 0, 0, 0);
            button->installEventFilter(this);

            QStyleOptionHeader *opt = new QStyleOptionHeader();
            opt->text = button->text();
            QSize *s = new QSize(button->style()->sizeFromContents(QStyle::CT_HeaderSection, opt,
                                                                   QSize(), button).expandedTo(QApplication::globalStrut()));
            if (s->isValid())
                ui->addedAdditionalFields->verticalHeader()->setMinimumWidth(s->width());
        }

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
                    //If textBoxEdit only
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

                updateAdditional(item1->text(), item2->text(), checkValue);

                //tAdditionalFieldValues.append(item2->text());
                //tAdditionalFieldValuesChk.append(QString::number(checkValue));
        });

        connect(workerThread, &WorkerThreadSurvey::saveSettingDone, this,
                [=]() {

            additionalFieldNames.append(additionalFieldNames.at(additionalFieldNames.count() - 1));
            additionalFieldValues.append(additionalFieldValues.at(additionalFieldValues.count() - 1));
            additionalFieldValuesChk.append(additionalFieldValuesChk.at(additionalFieldValuesChk.count() - 1));
            //For Opening the window progress message
            if (openingWindow) {
                progMessage->okToClose = true;
                openingWindow = false;

                progMessage->setValue(50);
                progMessage->close();
            }
            //For Saving progress message
            else if (loadHeaderAdditional) {
                if(hasChangesMade(headerNames) || hasChangesMade(headerValues) || hasChangesMade(headerValuesChk) ||
                        hasChangesMade(additionalFieldNames) || hasChangesMade(additionalFieldValues)
                        || hasChangesMade(additionalFieldValuesChk)) {

                    MessageDialog msgBox;
                    msgBox.setText("Do you want to save the changes?");
                    msgBox.setTwoButton("Yes", "No");
                    msgBox.exec();

                    if (msgBox.isAccept()) {
                        //If there is changes
                        proj->setHeaderNames(headerNames.at(headerNames.count() - 1));
                        proj->setHeaderValues(headerValues.at(headerValues.count() - 1));
                        proj->setHeaderValuesChk(headerValuesChk.at(headerValuesChk.count() - 1));
                        proj->setAdditionalFieldNames(additionalFieldNames.at(additionalFieldNames.count() - 1));
                        proj->setAdditionalFieldValues(additionalFieldValues.at(additionalFieldValues.count() - 1));
                        proj->setAdditionalFieldValuesChk(additionalFieldValuesChk.at(additionalFieldValuesChk.count() - 1));

                        proj->setChanges(true);
                        proj->changesMade = true;
                        emit receivedLogDialogItems();
                    }
                    else {

                        //Revert the previous data
                        proj->setHeaderNames(headerNames.at(headerNames.count() - 2));
                        proj->setHeaderValues(headerValues.at(headerValues.count() - 2));
                        proj->setHeaderValuesChk(headerValuesChk.at(headerValuesChk.count() - 2));
                        proj->setAdditionalFieldNames(additionalFieldNames.at(additionalFieldNames.count() - 2));
                        proj->setAdditionalFieldValues(additionalFieldValues.at(additionalFieldValues.count() - 2));
                        proj->setAdditionalFieldValuesChk(additionalFieldValuesChk.at(additionalFieldValuesChk.count() - 2));

                        progMessage->okToClose = true;
                        progMessage->close();

                        this->close();
                    }

                }
            }

        });

        workerThread->start();
    }
    catch(...) {

    }
}*/


void ProjectSettingsDialog::PrepAdditionalFieldValues(QTableWidget *sourceWidget){
    try {

        if (openingWindow) {
            progMessage->setValue(40);
        }

        ui->addedAdditionalFields->setRowCount(0);
        ui->addedAdditionalFields->setColumnCount(3);// 4.21.2020 edited 2

        //added 4.22.2020 start
        const int &intwidth = ui->addedAdditionalFields->width() - 25;
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

        QAbstractButton *button = ui->addedAdditionalFields->findChild<QAbstractButton *>();
        if(button) {
            button->setText(tr("#"));
            button->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
            button->setContentsMargins(0, 0, 0, 0);
            button->installEventFilter(this);

            QStyleOptionHeader *opt = new QStyleOptionHeader();
            opt->text = button->text();
            QSize *s = new QSize(button->style()->sizeFromContents(QStyle::CT_HeaderSection, opt,
                                                                   QSize(), button).expandedTo(QApplication::globalStrut()));
            if (s->isValid())
                ui->addedAdditionalFields->verticalHeader()->setMinimumWidth(s->width());
        }

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
                    //If textBoxEdit only
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

        connect(workerThread, &WorkerThreadSurvey::saveSettingDone, this,
                [=]() {

            //For Opening the window progress message
            if (openingWindow) {
                progMessage->okToClose = true;
                openingWindow = false;
                progMessage->setValue(50);
                progMessage->close();
                preHeaderCount = ui->addedHeaderFields->rowCount();
                preAdditionalCount = ui->addedAdditionalFields->rowCount();
            }
            //For Saving progress message
            else if (loadHeaderAdditional) {

                if (!proj->newOpen) {

                    if (hasChangesMade()) {
                        MessageDialog msgBox;
                        msgBox.setText("Survey template file found in Project Directory\n"
                                       "and will make changes on " + changeFields + ".\n\n"
                                       "Do you want to apply these changes?");
                        msgBox.setTwoButton("Yes", "No, do not apply");
                        msgBox.resizeHeight((154 + 26) - 38);
                        if (changeFields == QLatin1String("Header Fields"))
                            msgBox.resizeWidth(316);
                        else
                            msgBox.resizeWidth(341);
                        msgBox.exec();

                        if (msgBox.isAccept()) {

                            proj->changesMade = true;
                            emit receivedLogDialogItems();

                        }
                        else {

                            //Retain Previous Value
                            proj->headerFieldsChk = preHeaderFieldsChk;
                            proj->hAFieldsCb = preHAFieldsCb;
                            proj->additionalFieldsChk = preAdditionalFieldsChk;
                            proj->aFieldsCb = preAFieldsCb;

                            proj->setHeaderNames(preHeaderNames);
                            proj->setHeaderValues(preHeaderValues);
                            proj->setHeaderValuesChk(preHeaderValuesChk);
                            proj->setAdditionalFieldNames(preAdditionalFieldNames);
                            proj->setAdditionalFieldValues(preAdditionalFieldValues);
                            proj->setAdditionalFieldValuesChk(preAdditionalFieldValuesChk);

                            progMessage->okToClose = true;
                            progMessage->close();

                            this->close();

                        }
                    }
                    else {
                        progMessage->okToClose = true;
                        progMessage->close();
                        this->close();
                    }

                }
                else {

                    proj->changesMade = true;
                    emit receivedLogDialogItems();

                }

            }

        });

        workerThread->start();
    }
    catch(...) {

    }
}


// Old Code
/*
void ProjectSettingsDialog::PrepAdditionalFieldValues(QTableWidget *sourceWidget){

    try{
        ui->addedAdditionalFields->setRowCount(0);
        ui->addedAdditiodonalFields->setColumnCount(3);// 4.21.2020 edited 2

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

                if (sourceWidget != nullptr) {
                    QTextEdit *textEditItem = dynamic_cast<QTextEdit*>(sourceWidget->cellWidget(y,1));
                    if (textEditItem != nullptr){
                        QString additionalFieldValueText = textEditItem->toPlainText();
                        ui->addedAdditionalFields->item(currentRow,1)->setText(additionalFieldValueText);
                    }
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

    //proj->changesMade = true;
}

void ProjectSettingsDialog::on_removeField_2_pressed()
{
    int rowToKill = ui->addedHeaderFields->currentRow();
    ui->addedHeaderFields->removeRow(rowToKill);

    //proj->changesMade = true;
}


void ProjectSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)//This is the save button
{
    Q_UNUSED(button)
    doSave();
}

void ProjectSettingsDialog::on_pushButton_pressed()
{
    speciesDialog->openNew = proj->newOpen;
    speciesDialog->loadCurrentList(proj->addedSpecies);
    speciesDialog->defaultUsedHotkeys = defaultUsedHotkeys;
    speciesDialog->loadOtherItems(proj->addedGroupings);
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
    groupingDialog->openNew = proj->newOpen;
    groupingDialog->loadCurrentList(proj->addedGroupings);
    groupingDialog->defaultUsedHotkeys = defaultUsedHotkeys;
    groupingDialog->loadOtherItems(proj->addedSpecies);
    groupingDialog->show();
    activeButton = ui->pushButton_2;
}

void ProjectSettingsDialog::GetSpeciesAndGrouping(){

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
       //qDebug() << speciesItems[i]->itemName + ";" + speciesItems[i]->shortcutKey.toString() + ";" + useGrouping;
       proj->addedSpecies.append(speciesItems[i]->itemName + ";" + speciesItems[i]->shortcutKey.toString() + ";" + useGrouping);
    }

    proj->setChanges(true);

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
    const QString &css = QString("border: solid; border-width: thin; height: 22px; border-color: #C6C6C680; background-color: %1").arg(backcolor);
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
}

void ProjectSettingsDialog::on_buttonBox_clicked()
{
    progMessage = new progressmessage(this); //added this 7.30.2020
    progMessage->setMax(120);
    message = true;
    hide();
    progMessage->setFocus();
    progMessage->show();
    doSave();
}

void ProjectSettingsDialog::closeEvent(QCloseEvent *event)
{
    //emit refreshSignals();
    event->accept();
}

/*
void ProjectSettingsDialog::doSave()
{
    WorkerThreadSurvey *workerThread1 = new WorkerThreadSurvey();
    workerThread1->setAction(6);
    workerThread1->setHeaderCount(ui->addedHeaderFields->rowCount());
    workerThread1->setProjectFile(proj);

    connect(workerThread1, &WorkerThreadSurvey::finished, workerThread1, &QObject::deleteLater);

    //Loop all header Names
    connect(workerThread1, &WorkerThreadSurvey::saveHeaderNameLoop, this,
            [=](const int &index) {

        if (ui->addedHeaderFields->item(index,0) != nullptr){
            const QString &val = ui->addedHeaderFields->item(index,0)->text();
            if(val.length() > 0){
                if (headerNames.count() == 1) {
                    QJsonArray tmp;
                    tmp.append(val);
                    headerNames.append(tmp);
                    QJsonArray tmp1;
                    tmp1.append(QJsonArray());
                    headerValues.append(tmp1);
                    QJsonArray tmp2;
                    tmp2.append(QJsonArray());
                    headerValuesChk.append(tmp2);
                }
                else if(!headerNames.at(headerNames.count() - 1).contains(val)) {
                    QJsonArray tmp = headerNames.at(headerNames.count() - 1);
                    tmp.append(val);
                    headerNames.append(tmp);
                    QJsonArray tmp1 = headerValues.at(headerValues.count() - 1);
                    tmp1.append(QJsonArray());
                    headerValues.append(tmp1);
                    QJsonArray tmp2 = headerValuesChk.at(headerValuesChk.count() - 1);
                    tmp2.append(QJsonArray());
                    headerValuesChk.append(tmp2);
                }
            }
        }

    });

    //Done loop all header Names
    connect(workerThread1, &WorkerThreadSurvey::saveHeaderNameDone, this,
             [=]() {

        progMessage->setValue(12);

        //proj->setHeaderNames(headerNames.at(headerNames.count() - 1));
        //headerNames.append(tHeaderName);

        WorkerThreadSurvey *workerThread2 = new WorkerThreadSurvey();
        workerThread2->setAction(7);
        workerThread2->setAddedCount(ui->addedAdditionalFields->rowCount());
        workerThread2->setProjectFile(proj);

        connect(workerThread2, &WorkerThreadSurvey::finished, workerThread2, &QObject::deleteLater);

        //Loop all additional Names
        connect(workerThread2, &WorkerThreadSurvey::saveAddedNameLoop, this,
                [=](const int &index) {


            if (ui->addedAdditionalFields->item(index,1) != nullptr) {
                QString val = ui->addedAdditionalFields->item(index,0)->text();
                if(val.length() > 0){
                    if (additionalFieldNames.count() == 1) {
                        QJsonArray tmp;
                        tmp.append(val);
                        additionalFieldNames.append(tmp);
                        QJsonArray tmp1;
                        tmp1.append(QJsonArray());
                        additionalFieldValues.append(tmp1);
                        QJsonArray tmp2;
                        tmp2.append(QJsonArray());
                        additionalFieldValuesChk.append(tmp2);
                    }
                    else if(!additionalFieldNames.at(additionalFieldNames.count() - 1).contains(val)) {
                        QJsonArray tmp = additionalFieldNames.at(additionalFieldNames.count() - 1);
                        tmp.append(val);
                        additionalFieldNames.append(tmp);
                        QJsonArray tmp1 = additionalFieldValues.at(additionalFieldValues.count() - 1);
                        tmp1.append(QJsonArray());
                        additionalFieldValues.append(tmp1);
                        QJsonArray tmp2 = additionalFieldValuesChk.at(additionalFieldValuesChk.count() - 1);
                        tmp2.append(QJsonArray());
                        additionalFieldValuesChk.append(tmp2);
                    }
                }
            }


        });

        //Done loop additional Names
        connect(workerThread2, &WorkerThreadSurvey::saveAddedNameDone, this,
                [=]() {

                progMessage->setValue(35);

                //proj->setAdditionalFieldNames(additionalFieldNames.at(additionalFieldNames.count() - 1));
                //additionalFieldNames.append(tAdditionalFieldNames);

                WorkerThreadSurvey *workerThread3 = new WorkerThreadSurvey();
                workerThread3->setAction(8);
                workerThread3->setHeaderCount(ui->addedHeaderFields->rowCount());
                workerThread3->setProjectFile(proj);

                connect(workerThread3, &WorkerThreadSurvey::finished, workerThread3, &QObject::deleteLater);

                //Loop Header Values and checked
                connect(workerThread3, &WorkerThreadSurvey::saveHeaderValueChkLoop, this,
                        [=](const int &index) {

                    if (ui->addedHeaderFields->item(index, 0) != nullptr){
                        const QString &h = ui->addedHeaderFields->item(index, 0)->text();
                        const QString &val = ui->addedHeaderFields->item(index, 1)->text();

                        //added 4.22.2020 start
                        int intchk = 0;
                        auto chkfield = ui->addedHeaderFields->cellWidget(index, 2);
                        QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
                        if(chkbox->isChecked()){
                            intchk = 2;
                        }
                        //headerValuesChk.append(QString::number(intchk));//working for single checked value
                        int idx0 = proj->getHeaderIndex(h);
                        if(idx0 != -1){
                            auto hvc = headerValuesChk.at(headerValuesChk.count() - 1);
                            QJsonArray tmp = hvc[idx0].toArray();
                            tmp.append(QString::number(intchk));
                            hvc[idx0] = tmp;
                        }
                        //added 4.22.2020 end

                        if(h.length() < 1 || val.length() < 1){

                        }
                        else {

                            //Find the index of the header
                            int idx = proj->getHeaderIndex(h);
                            if(idx != -1){
                                auto hv = headerValues.at(headerValues.count() - 1);
                                QJsonArray tmp = hv[idx].toArray();
                                tmp.append(val);
                                hv[idx] = tmp;
                            }

                        }
                    }

                });

                //Done loop Header values and check status
                connect(workerThread3, &WorkerThreadSurvey::saveHeaderValueChkDone, this,
                        [=]() {

                    progMessage->setValue(47);

                    //proj->setHeaderValues(tHeaderValues);
                    //proj->setHeaderValuesChk(tHeaderValuesChk);

                    //headerValues.append(tHeaderValues);
                    //headerValuesChk.append(tHeaderValuesChk);

                    //Refresh header fields
                    PrepHeadFieldValues();

                    progMessage->setValue(59);

                    WorkerThreadSurvey *workerThread4 = new WorkerThreadSurvey();
                    workerThread4->setAction(9);
                    workerThread4->setAddedCount(ui->addedAdditionalFields->rowCount());
                    workerThread4->setProjectFile(proj);

                    connect(workerThread4, &WorkerThreadSurvey::finished, workerThread4, &QObject::deleteLater);

                    //Loop Additional values and check status
                    connect(workerThread4, &WorkerThreadSurvey::saveAddedValueChkLoop, this,
                            [=](const int &index) {

                        if (ui->addedAdditionalFields->item(index, 0) != nullptr){
                            const QString &h = ui->addedAdditionalFields->item(index, 0)->text();
                            const QString &val = ui->addedAdditionalFields->item(index, 1)->text();

                            if(h.length() < 1){

                            }
                            else {

                                //added 4.22.2020 start
                                int intchk = 0;
                                auto chkfield = ui->addedAdditionalFields->cellWidget(index, 2);
                                QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
                                //qDebug() << h << ": " <<  chkbox->isChecked();
                                if(chkbox->isChecked()){
                                    intchk = 2;
                                }
                                //additionalFieldValuesChk.append(QString::number(intchk));//remarked woring for single checked value
                                int idx0 = proj->getAdditionalFieldIndex(h);
                                //qDebug() << h << ": " <<  chkbox->isChecked();
                                if(idx0 != -1){
                                    auto avc = additionalFieldValuesChk.at(additionalFieldValuesChk.count() - 1);
                                    QJsonArray tmp = avc[idx0].toArray();
                                    tmp.append(QString::number(intchk));
                                    avc[idx0] = tmp;
                                }
                               //added 4.22.2020 end//added 4.22.2020 end

                                //Find the index of the additional field
                                int idx = proj->getAdditionalFieldIndex(h);
                                if(idx != -1){
                                    auto av = additionalFieldValues.at(additionalFieldValues.count() - 1);
                                    QJsonArray tmp = av[idx].toArray();
                                    tmp.append(val);
                                    av[idx] = tmp;
                                }

                            }

                        }

                    });

                    //Done loop additional values and check status
                    connect(workerThread4, &WorkerThreadSurvey::saveAddedValueChkDone, this,
                            [=]() {

                        progMessage->setValue(75);

                        //proj->setAdditionalFieldValues(tAdditionalFieldValues);
                        //proj->setAdditionalFieldValuesChk(tAdditionalFieldValuesChk);

                        //additionalFieldValues.append(tAdditionalFieldValues);
                        //additionalFieldValuesChk.append(tAdditionalFieldValuesChk);

                        loadHeaderAdditional = true;
                        sourceWidget = nullptr;
                        PrepAdditionalFieldValues(nullptr);

                        progMessage->setValue(87);
                    });

                    workerThread4->start();

                });
                workerThread3->start();


        });
        workerThread2->start();

    });

    workerThread1->start();
}
*/

//Old Version 2
void ProjectSettingsDialog::doSave()
{
/*
    preHeaderNames = *new QJsonArray();
    foreach(QJsonValue str, proj->headerNamesArray) {
        preHeaderNames.append(str);
    }

    preHeaderValues = *new QJsonArray();
    foreach(QJsonValue str,  proj->headerValuesArray) {
        preHeaderValues.append(str);
    }

    preHeaderValuesChk = *new QJsonArray();
    foreach(QJsonValue str,  proj->headerValuesChkArray) {
        preHeaderValuesChk.append(str);
    }

    preAdditionalFieldNames = *new QJsonArray();
    foreach(QJsonValue str,  proj->addionalFieldsNamesArray) {
        preAdditionalFieldNames.append(str);
    }

    preAdditionalFieldValues = *new QJsonArray();
    foreach(QJsonValue str,  proj->addionalFieldsValuesArray) {
        preAdditionalFieldValues.append(str);
    }

    preAdditionalFieldValuesChk = *new QJsonArray();
    foreach(QJsonValue str,  proj->addionalFieldsValuesChkArray) {
        preAdditionalFieldValuesChk.append(str);
    }
*/

    preHeaderNames = proj->headerNamesArray;
    preHeaderValues = proj->headerValuesArray;
    preHeaderValuesChk = proj->headerValuesChkArray;
    preAdditionalFieldNames = proj->addionalFieldsNamesArray;
    preAdditionalFieldValues = proj->addionalFieldsValuesArray;
    preAdditionalFieldValuesChk = proj->addionalFieldsValuesChkArray;


    preHeaderFieldsChk.clear();
    foreach(QString str, proj->headerFieldsChk) {
        preHeaderFieldsChk.append(str);
    }

    preHAFieldsCb.clear();
    foreach(QString str, proj->hAFieldsCb) {
        preHAFieldsCb.append(str);
    }

    preAdditionalFieldsChk.clear();
    foreach(QString str, proj->additionalFieldsChk) {
        preAdditionalFieldsChk.append(str);
    }

    preAFieldsCb.clear();
    foreach(QString str, proj->aFieldsCb) {
        preAFieldsCb.append(str);
    }

    headerNames = *new QJsonArray();
    headerValues = *new QJsonArray();
    headerValuesChk = *new QJsonArray();
    additionalFieldNames = *new QJsonArray();
    additionalFieldValues = *new QJsonArray();
    additionalFieldValuesChk =*new QJsonArray();

    WorkerThreadSurvey *workerThread1 = new WorkerThreadSurvey();
    workerThread1->setAction(6);
    workerThread1->setHeaderCount(ui->addedHeaderFields->rowCount());
    workerThread1->setProjectFile(proj);

    connect(workerThread1, &WorkerThreadSurvey::finished, workerThread1, &QObject::deleteLater);

    //Loop all header Names
    connect(workerThread1, &WorkerThreadSurvey::saveHeaderNameLoop, this,
            [=](const int &index) {

        if (ui->addedHeaderFields->item(index,0) != nullptr){
            const QString &val = ui->addedHeaderFields->item(index,0)->text();
            if(val.length() > 0 && !headerNames.contains(val)){
                headerNames.append(val);
                headerValues.append(QJsonArray());
                headerValuesChk.append(QJsonArray());//added 4.26.2020
            }
        }
    });

    //Done loop all header Names
    connect(workerThread1, &WorkerThreadSurvey::saveHeaderNameDone, this,
             [=]() {

        progMessage->setValue(12);

        proj->setHeaderNames(headerNames);

        WorkerThreadSurvey *workerThread2 = new WorkerThreadSurvey();
        workerThread2->setAction(7);
        workerThread2->setAddedCount(ui->addedAdditionalFields->rowCount());
        workerThread2->setProjectFile(proj);

        connect(workerThread2, &WorkerThreadSurvey::finished, workerThread2, &QObject::deleteLater);

        //Loop all additional Names
        connect(workerThread2, &WorkerThreadSurvey::saveAddedNameLoop, this,
                [=](const int &index) {


            if (ui->addedAdditionalFields->item(index,1) != nullptr) {
                const QString &val = ui->addedAdditionalFields->item(index,0)->text();
                if(val.length() > 0 && !additionalFieldNames.contains(val)){
                    additionalFieldNames.append(val);
                    additionalFieldValues.append(QJsonArray());
                    additionalFieldValuesChk.append(QJsonArray());//added 4.26.2020
                }
            }


        });

        //Done loop additional Names
        connect(workerThread2, &WorkerThreadSurvey::saveAddedNameDone, this,
                [=]() {

                progMessage->setValue(35);

                proj->setAdditionalFieldNames(additionalFieldNames);

                WorkerThreadSurvey *workerThread3 = new WorkerThreadSurvey();
                workerThread3->setAction(8);
                workerThread3->setHeaderCount(ui->addedHeaderFields->rowCount());
                workerThread3->setProjectFile(proj);

                connect(workerThread3, &WorkerThreadSurvey::finished, workerThread3, &QObject::deleteLater);

                //Loop Header Values and checked
                connect(workerThread3, &WorkerThreadSurvey::saveHeaderValueChkLoop, this,
                        [=](const int &index) {

                    if (ui->addedHeaderFields->item(index, 0) != nullptr){
                        const QString &h = ui->addedHeaderFields->item(index, 0)->text();
                        const QString &val = ui->addedHeaderFields->item(index, 1)->text();

                        //added 4.22.2020 start
                        int intchk = 0;
                        auto chkfield = ui->addedHeaderFields->cellWidget(index, 2);
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

                        }
                        else {

                            //Find the index of the header
                            int idx = proj->getHeaderIndex(h);
                            if(idx != -1){
                                QJsonArray tmp = headerValues[idx].toArray();
                                tmp.append(val);
                                headerValues[idx] = tmp;
                            }

                        }
                    }

                });

                //Done loop Header values and check status
                connect(workerThread3, &WorkerThreadSurvey::saveHeaderValueChkDone, this,
                        [=]() {

                    progMessage->setValue(47);

                    proj->setHeaderValues(headerValues);
                    proj->setHeaderValuesChk(headerValuesChk);

                    //Refresh header fields
                    PrepHeadFieldValues();

                    progMessage->setValue(59);

                    WorkerThreadSurvey *workerThread4 = new WorkerThreadSurvey();
                    workerThread4->setAction(9);
                    workerThread4->setAddedCount(ui->addedAdditionalFields->rowCount());
                    workerThread4->setProjectFile(proj);

                    connect(workerThread4, &WorkerThreadSurvey::finished, workerThread4, &QObject::deleteLater);

                    //Loop Additional values and check status
                    connect(workerThread4, &WorkerThreadSurvey::saveAddedValueChkLoop, this,
                            [=](const int &index) {

                        if (ui->addedAdditionalFields->item(index, 0) != nullptr){
                            const QString &h = ui->addedAdditionalFields->item(index, 0)->text();
                            const QString &val = ui->addedAdditionalFields->item(index, 1)->text();

                            if(h.length() < 1){

                            }
                            else {

                                //added 4.22.2020 start
                                int intchk = 0;
                                auto chkfield = ui->addedAdditionalFields->cellWidget(index, 2);
                                QCheckBox *chkbox = qobject_cast <QCheckBox*> (chkfield->layout()->itemAt(0)->widget());
                                //qDebug() << h << ": " <<  chkbox->isChecked();
                                if(chkbox->isChecked()){
                                    intchk = 2;
                                }
                                //additionalFieldValuesChk.append(QString::number(intchk));//remarked woring for single checked value
                                const int &idx0 = proj->getAdditionalFieldIndex(h);
                                //qDebug() << h << ": " <<  chkbox->isChecked();
                                if(idx0 != -1){
                                    QJsonArray tmp = additionalFieldValuesChk[idx0].toArray();
                                    tmp.append(QString::number(intchk));
                                    additionalFieldValuesChk[idx0] = tmp;
                                }
                               //added 4.22.2020 end//added 4.22.2020 end

                                //Find the index of the additional field
                                const int &idx = proj->getAdditionalFieldIndex(h);
                                if(idx != -1){
                                    QJsonArray tmp = additionalFieldValues[idx].toArray();
                                    tmp.append(val);
                                    additionalFieldValues[idx] = tmp;
                                }

                            }

                        }

                    });

                    //Done loop additional values and check status
                    connect(workerThread4, &WorkerThreadSurvey::saveAddedValueChkDone, this,
                            [=]() {

                        progMessage->setValue(75);

                        proj->setAdditionalFieldValues(additionalFieldValues);
                        proj->setAdditionalFieldValuesChk(additionalFieldValuesChk);

                        loadHeaderAdditional = true;
                        sourceWidget = nullptr;
                        PrepAdditionalFieldValues(nullptr);

                        progMessage->setValue(87);
                    });

                    workerThread4->start();

                });
                workerThread3->start();


        });
        workerThread2->start();

    });

    workerThread1->start();
}


void ProjectSettingsDialog::on_pushButton_clicked()
{

}
