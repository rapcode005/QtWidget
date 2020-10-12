#include "species_widget.h"
#include "ui_mainwindow2.h"
#include <QObject>
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include "logtemplateitem.h"
#include <QTimer>





/**
 *  Species Widget on main screen
 *  the UI includes hotkeys
 *
 */
SpeciesWidget::SpeciesWidget(QWidget *parent, QString windowTitle, bool useCheck) :
    QDialog(parent),
    ui(new Ui::SpeciesWidget)
{
    ui->setupUi(this);
    this->setWindowTitle(windowTitle);
    ui->label->setText(windowTitle);
    ui->label->setStyleSheet("font-size: 14px; font-weight: bold;");
    ui->label->setAlignment(Qt::AlignCenter);
    useCheckBox = useCheck;
    ui->pushButton->setFocus();
    rx.setPattern("([A-Z]|[a-z]|[0-9]| )*");
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setColumnWidth(0,80);
    if (!useCheckBox){
        ui->tableWidget->setColumnCount(2);
        AddItem("Open");
    }else{
         ui->tableWidget->setColumnWidth(2,120);
    }
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    this->setUpdatesEnabled(true);
    hotKeyTimer = new QTimer(this);
    connect(hotKeyTimer, SIGNAL(timeout()),this, SLOT(CheckEdititing()));
    hotKeyTimer->start(100);
}





SpeciesWidget::~SpeciesWidget()
{
    delete ui;
}





void SpeciesWidget::CheckEdititing(){
    for (int i = 0; i < edits.length(); i++){
        //if (edits[i] != nullptr){
            if(edits[i]->hasFocus()){
                edits[i]->setStyleSheet("background-color: yellow;");
            }else{
               edits[i]->setStyleSheet("background-color: white;");
            }
        //}
        //edits[i]->setKeySequence(edits[i]->keySequence()[0]);
    }
}





void SpeciesWidget::AddItem(QString name, QString shortcut , bool useGrouping){
    int rowcount = ui->tableWidget->rowCount();

    ui->tableWidget->setRowCount(rowcount + 1);
    ui->tableWidget->scrollToTop();
    //ui->tableWidget->setColumnWidth(3,35);
    //QPushButton *p = new QPushButton("X", this);
    //ui->tableWidget->setCellWidget(rowcount,3,p);
    //connect(s,SIGNAL(keySequenceChanged(QKeySequence)),this,SLOT(StopRecording()));
    if (useCheckBox){
        QCheckBox *c = new QCheckBox(ui->tableWidget);
        c->setText("Use Grouping");
        c->setStyleSheet("margin-left:10%; margin-right:10%;");
        //if (useGrouping)
        c->setChecked(useGrouping);
        ui->tableWidget->setCellWidget(rowcount,2,c);
    }

    QKeySequenceEdit *s =  new QKeySequenceEdit(ui->tableWidget);
    edits.append(s);
    connect(s,SIGNAL(keySequenceChanged(QKeySequence)),this,SLOT(StopRecording(QKeySequence)));
    //connect(s,SIGNAL())
    ui->tableWidget->setCellWidget(rowcount,1, s);
    QKeySequence k = QKeySequence::fromString(shortcut);
    s->setKeySequence(k);

    ui->tableWidget->setItem(rowcount, 0, new QTableWidgetItem(name));
    LogTemplateItem *t = new LogTemplateItem(name);


    t->groupingEnabled = useGrouping;
    t->shortcutKey = k;
    logItems.append(t);
    //this->update();
    //ui->tableWidget->scrollToTop();
    ui->tableWidget->resizeRowsToContents();
    ui->tableWidget->scrollToBottom();
}





void SpeciesWidget::RemoveRowItem(int row){
   // LogTemplateItem *logItem = logItems[row];

    ui->tableWidget->removeCellWidget(row, 1);
    if (useCheckBox)
        ui->tableWidget->removeCellWidget(row, 2);
    ui->tableWidget->removeRow(row);
    logItems.removeAt(row);
    edits.removeAt(row);
    //delete edits[row];
}





void SpeciesWidget::on_pushButton_pressed()
{
    QString submitted = ui->lineEdit->text();
    if (submitted.isEmpty()){
        return;
    }
    if (!rx.exactMatch(submitted)){
        QMessageBox mb;
        mb.setText("Added item cannot contain a special character!");
        mb.exec();
        return;
    }
    AddItem(submitted);
    ui->lineEdit->setText("");
}






void SpeciesWidget::on_buttonBox_accepted()
{
    for(int i = 0; i < logItems.length(); i++){
        QString k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
        if (k != NULL){
            QKeySequence s (k);
            logItems[i]->shortcutKey = s;
            //qDebug()<< logItems[i]->shortcutKey.toString();

        }
        if (useCheckBox){
            QString c = ui->tableWidget->cellWidget(i,2)->property("checked").toString();
            qDebug() << c;
            if (c == "true"){
                logItems[i]->groupingEnabled = true;
            }else{
                logItems[i]->groupingEnabled = false;
            }
            //qDebug()<< logItems[i]->groupingEnabled;
        }
    }
    emit Done();
}






void SpeciesWidget::on_pushButton_2_pressed()
{
    if (!useCheckBox){
        if (ui->tableWidget->currentRow() == 0){
            QMessageBox mb;
            mb.setText("Cannot delete the 'Open' field!");
            mb.exec();
            return;
        }
    }
    RemoveRowItem(ui->tableWidget->currentRow());
}





void SpeciesWidget::StopRecording(QKeySequence key){
    for (int i = 0; i < edits.length(); i++){
        edits[i]->setKeySequence(edits[i]->keySequence()[0]);
        edits[i]->setStyleSheet("background-color: white;");
        edits[i]->clearFocus();
    }
}





void SpeciesWidget::ReceiveItems(QStringList items){
    int lengthy = logItems.length();
    for (int i = 0; i < lengthy; i ++){
        logItems.removeAt(0);
    }
    qDeleteAll(edits);
    edits.clear();
    ui->tableWidget->setRowCount(0);
    //ui->tableWidget->clear();
    qDebug()<< logItems.length();
    QString lineToTranslate;
    for(int s = 0; s < items.length(); s++){
        lineToTranslate = "";
        lineToTranslate = items.at(s);
        qDebug() << lineToTranslate << "Line to translate";
        QStringList cur = lineToTranslate.split(';');
        bool shouldGroup = false;
        if (useCheckBox){
            if (cur[2] == "true"){
                shouldGroup = true;
            }
        }
        AddItem(cur[0],cur[1],shouldGroup);
    }
    //add logic for loading stuff
    //qDebug() << items.length();
    emit Loaded();
}





void SpeciesWidget::reject(){
    //ffffff, whatever I guess warnings scare people
    for(int i = 0; i < logItems.length(); i++){
        QString k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
        if (k != NULL){
            QKeySequence s (k);
            logItems[i]->shortcutKey = s;
            //qDebug()<< logItems[i]->shortcutKey.toString();

        }
        if (useCheckBox){
            QString c = ui->tableWidget->cellWidget(i,2)->property("checked").toString();
            qDebug() << c;
            if (c == "true"){
                logItems[i]->groupingEnabled = true;
            }else{
                logItems[i]->groupingEnabled = false;
            }
            //qDebug()<< logItems[i]->groupingEnabled;
        }
    }
    emit Done();
    hide();
    /*QMessageBox mb;
    mb.setWindowTitle("Warning");
    QString descriptor = "groupings";
    if (useCheckBox)
        descriptor = "species";
    mb.setText("Warning! Your added " + descriptor + " will not be saved!" );
    mb.exec();
    hide();*/
}





void SpeciesWidget::on_tableWidget_viewportEntered()
{

}




