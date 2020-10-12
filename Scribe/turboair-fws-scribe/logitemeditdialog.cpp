#include "logitemeditdialog.h"
#include "ui_logitemeditdialog.h"
#include <QObject>
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMenu>
#include "logtemplateitem.h"
#include <QTimer>
#include "messagebox.h"

/**
 *  Let's deprecate this file and its header file
 *
 */
LogItemEditDialog::LogItemEditDialog(QWidget *parent, QString windowTitle, bool useCheck,
                                     QString title) :
    QDialog(parent),
    ui(new Ui::LogItemEditDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(windowTitle);
    useCheckBox = useCheck;
    ui->pushButton->setFocus();
    rx.setPattern("([A-Z]|[a-z]|[0-9]| )*");
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setColumnWidth(0,80);

    fields = title;
    //openNew = isOpenNew;
    //preItems = preList;

    QStringList labels;//added 5.1.2020
    if (!useCheckBox){
        labels << tr(" Group Name") << tr(" Keyboard Shortcut");//added 5.1.2020
        int intwidth = (ui->tableWidget->width()- 2) / 2;
        for(int i =0; i < ui->tableWidget->columnCount(); i++){
            ui->tableWidget->setColumnWidth(i,intwidth);
        }
        ui->tableWidget->setColumnCount(2);
        AddItem("Open");
        this->resize(299,338);//added 5.1.2020 299
    }else{
        labels << tr(" Name") << tr(" Keyboard Shortcut") << tr(" Use Grouping");//added 5.1.2020
        ui->tableWidget->setColumnWidth(0,110);
        ui->tableWidget->setColumnWidth(1,132);
        ui->tableWidget->setColumnWidth(2,90);
        this->resize(380,556);//added 5.1.2020 368
    }

    ui->tableWidget->setShowGrid(true);//added 5.3.2020
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);//added 5.1.2020
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter);//added 5.1.2020
    ui->tableWidget->setHorizontalHeaderLabels(labels);//added 5.1.2020
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);//added 5.3.2020
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(22);//added 5.3.2020
    ui->tableWidget->horizontalHeader()->show();//added 5.3.2020

    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    this->setUpdatesEnabled(true);
    hotKeyTimer = new QTimer(this);
    connect(hotKeyTimer, SIGNAL(timeout()),this, SLOT(CheckEdititing()));
    hotKeyTimer->start(100);
}

LogItemEditDialog::~LogItemEditDialog()
{
    delete ui;
}

void LogItemEditDialog::loadCurrentList(const QStringList &n)
{
    preItems.clear();
    foreach(QString str, n) {
        preItems << str;
    }
}



void LogItemEditDialog::RefreshItems(const QStringList &items)
{
    try{
        const int &lengthy = logItems.length();
        for (int i = 0; i < lengthy; i ++){
            logItems.removeAt(0);
        }
        edits.clear();
        ui->tableWidget->setRowCount(0);
        for(int s = 0; s < items.length(); s++){
            const QString &lineToTranslate = items.at(s);
            const QStringList &cur = lineToTranslate.split(';');
            bool shouldGroup = false;
            if (useCheckBox){
                if (cur[2] == QLatin1String("true")){
                    shouldGroup = true;
                }
            }
            AddItem(cur[0],cur[1],shouldGroup);
        }
    }catch(...){//added 5.15.2020

    }
}

void LogItemEditDialog::retainPreItems(const QStringList &items)
{
    try{

        const int &lengthy = logItems.length();
        for (int i = 0; i < lengthy; i ++){
            logItems.removeAt(0);
        }
        edits.clear();
        ui->tableWidget->setRowCount(0);
        for(int s = 0; s < items.length(); s++){
            const QString &lineToTranslate = items.at(s);
            const QStringList &cur = lineToTranslate.split(';');
            bool shouldGroup = false;
            if (useCheckBox){
                if (cur[2] == QLatin1String("true")){
                    shouldGroup = true;
                }
            }
            AddItem(cur[0],cur[1],shouldGroup);
        }

    }
    catch(...){//added 5.15.2020

    }
}

void LogItemEditDialog::loadOtherItems(const QStringList &n)
{
    otherItems.clear();
    foreach(QString str, n) {
        otherItems << str;
    }
}

void LogItemEditDialog::CheckEdititing(){
    try {
        for (int i = 0; i < edits.length(); i++){
            if (edits.at(i)){
                if(edits.at(i)->hasFocus()){
                    edits.at(i)->setStyleSheet("background-color: yellow;");
                }else{
                    edits.at(i)->setStyleSheet("background-color: white;");
                }
            }
        }
    } catch (...) {

    }
}

void LogItemEditDialog::onSave()
{
    try {
        if (hasChangeMade()) {
            if (!openNew) {
                MessageDialog msgBox;
                msgBox.setText("Survey template file found in Project Directory\n"
                               "and will make changes on " + fields + ".\n\n "
                               "Do you want to apply these changes?");
                msgBox.setTwoButton("Yes", "No, do not apply");
                msgBox.resizeHeight((154 + 56) - 38);

                if (fields == QLatin1String("Specimen and Hotkeys"))
                    msgBox.resizeWidth(341);
                else
                    msgBox.resizeWidth(316);

                msgBox.exec();

                if (msgBox.isAccept()) {
                    validateItems();
                    /*for(int i = 0; i < logItems.length(); i++){
                        const QString &k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
                        if (k != nullptr){
                            QKeySequence s (k);
                            logItems[i]->shortcutKey = s;
                        }
                        if (useCheckBox){
                            const QString &c = ui->tableWidget->cellWidget(i,2)->property("checked").toString();
                            if (c == QLatin1String("true")){
                                logItems[i]->groupingEnabled = true;
                            }else{
                                logItems[i]->groupingEnabled = false;
                            }
                        }
                    }
                    emit Done();
                    this->hide();*/
                }
                else {
                    retainPreItems(preItems);
                    this->hide();
                }
            }
            else {
                validateItems();
                /*for(int i = 0; i < logItems.length(); i++){
                    const QString &k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
                    if (k != nullptr){
                        QKeySequence s (k);
                        logItems[i]->shortcutKey = s;
                    }
                    if (useCheckBox){
                        const QString &c = ui->tableWidget->cellWidget(i,2)->property("checked").toString();
                        if (c == QLatin1String("true")){
                            logItems[i]->groupingEnabled = true;
                        }else{
                            logItems[i]->groupingEnabled = false;
                        }
                    }
                }
                emit Done();
                this->hide();*/
            }
        }
        else {
            QMessageBox mb;
            mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
            mb.setWindowTitle("Message");
            mb.setText("Found no changes made, \nthe system will refresh the list.");
            mb.exec();

            RefreshItems(preItems);
            //this->hide();
        }
    } catch (...) {

    }
}


void LogItemEditDialog::AddItem(QString name, QString shortcut , bool useGrouping){
    const int &rowcount = ui->tableWidget->rowCount();

    ui->tableWidget->setRowCount(rowcount + 1);
    ui->tableWidget->scrollToTop();
    //ui->tableWidget->setColumnWidth(3,35);
    //QPushButton *p = new QPushButton("X", this);
    //ui->tableWidget->setCellWidget(rowcount,3,p);
    //connect(s,SIGNAL(keySequenceChanged(QKeySequence)),this,SLOT(StopRecording()));
    if (useCheckBox){
        QCheckBox *c = new QCheckBox(ui->tableWidget);
        //c->setText("Use Grouping");//remarked 5.1.2020 //checkbox label
        c->setStyleSheet("margin-left:10%; margin-right:10%; padding-left: 37px; background-color:transparent;");
        //if (useGrouping)
        c->setChecked(useGrouping);
        ui->tableWidget->setCellWidget(rowcount,2,c);
    }

    QString backcolor = "";
    if(rowcount % 2 == 0){
        backcolor = "#FFFFFF";
    }else{
        backcolor = "#F5F5F5";
    }
    //QString css = QString("border: none; border-width: thin; height: 22px; border-color: #C6C6C680; background-color: %1").arg(backcolor);
    const QString &css = QString("border: none; background-color: transparent");

    //require more research css not being applied
    QKeySequenceEdit *s =  new QKeySequenceEdit(ui->tableWidget);
    //for the line edit control embedded in qkeysequenceedit //added 7.25.2020
    QLineEdit *lineEdit = s->findChild<QLineEdit *>();
    if (lineEdit) {
        lineEdit->setStyleSheet( QString("QLineEdit{ border: none; background-color: %1;  font: 13px 'Segoe UI'; padding:0; margin:0; } QLineEdit:focus{ background-color: yellow }").arg(backcolor) );
        lineEdit->setToolTip("select the cell and press the delete key to remove the value.");
        lineEdit->setContextMenuPolicy(Qt::NoContextMenu);//CustomContextMenu
    }
    edits.append(s);

    connect(s, SIGNAL(keySequenceChanged(QKeySequence)), this, SLOT(StopRecording(QKeySequence)));
    connect(s, SIGNAL(editingFinished()), this, SLOT(OnEditingFinished()));//added 7.25.2020

    ui->tableWidget->setCellWidget(rowcount, 1, s);
    const QKeySequence &k = QKeySequence::fromString(shortcut);
    QFont font;
    font.setCapitalization(QFont::AllLowercase);
    s->setKeySequence(k);
    s->setFont(font);
    s->setStyleSheet(css);//added 5.25.2020

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
void LogItemEditDialog::RemoveRowItem(int row){
   // LogTemplateItem *logItem = logItems[row];

    ui->tableWidget->removeCellWidget(row, 1);
    if (useCheckBox)
        ui->tableWidget->removeCellWidget(row, 2);
    ui->tableWidget->removeRow(row);
    logItems.removeAt(row);
    edits.removeAt(row);
    //delete edits[row];
}
void LogItemEditDialog::on_pushButton_pressed()
{
    QString submitted = ui->lineEdit->text();
    if (submitted.isEmpty()){
        return;
    }
    if (!rx.exactMatch(submitted)){
        //QMessageBox mb;
        //mb.setText("Added item cannot contain a special character!");
        //mb.exec();
        MessageDialog mb;
        mb.setText("Added item cannot contain a special character!");
        mb.exec();
        return;
    }
    AddItem(submitted);
    ui->lineEdit->setText("");
}

/*void LogItemEditDialog::on_buttonBox_accepted()
{

}*/

void LogItemEditDialog::on_pushButton_2_pressed()
{
    if (!useCheckBox){
        if (ui->tableWidget->currentRow() == 0){
            //QMessageBox mb;
            //mb.setText("Cannot delete the 'Open' field!");
            //mb.exec();
            MessageDialog mb;
            mb.setText("Cannot delete the 'Open' field!");
            mb.exec();
            return;
        }
    }
    RemoveRowItem(ui->tableWidget->currentRow());
}

void LogItemEditDialog::StopRecording(QKeySequence key){
    //qDebug() << key.toString();
    QKeySequenceEdit *w = qobject_cast<QKeySequenceEdit *>(sender());
    int row = -1;

    if(w)
        row = ui->tableWidget->indexAt(w->pos()).row();

    //added to check delete key 8.24.2020
    if (w->keySequence().toString().toUpper() == "DEL"){
        w->clear();
        return;
        /*QString prevkey;
        for (int i = 0; i < edits.length(); i++){
            if (edits[i]->keySequence().toString() != "") {
                prevkey = edits[row]->keySequence().toString();
            }
        }
        qDebug() << "prevkey" << prevkey;*/
        /*MessageDialog mb;
        mb.setText("You are about to remove Key '" + prevkey + "',\ndo you want to continue?");
        mb.setTwoButton("Yes",60,"No",60);
        mb.exec();*/

        /*if (mb.isAccept()) {
            w->clear();
            return;
        }else{
            QFont font;
            font.setCapitalization(QFont::AllLowercase);
            edits[row]->setKeySequence(edits[row]->keySequence()[0]);
            //edits[i]->setKeySequence(edits[i]->keySequence()[0]);
            edits[row]->setStyleSheet("background-color: white;");
            edits[row]->setFont(font);
            edits[row]->clearFocus();
            return;
        }*/

    }

    //added to check for the default used hotkeys
    if(defaultUsedHotkeys.indexOf(key.toString(),0) != -1){
        for (int i = 0; i < edits.length(); i++){
            if (edits[i]->keySequence().toString() != "") {
                if (edits[i]->keySequence().toString() == key.toString()){
                    MessageDialog mb;
                    mb.setText("Key '" + edits[i]->keySequence().toString() + "' was already assigned \nin the system default keys");
                    mb.exec();

                    if(row > -1){
                        w->clear();
                        edits[i]->clear();
                        edits[i]->setKeySequence(QKeySequence());
                    }else{
                        edits[i]->clear();
                        edits[i]->setKeySequence(QKeySequence());
                    }
                    goto returnval;
                }
            }
        }
        returnval:{
            return;
        }
    }

    //added to check for the other used hotkeys
    QStringList othervals;
    for(int i = 0; i < otherItems.length(); i++){
        //qDebug() << otherItems[i];
        QStringList sval = otherItems[i].split(";");
        othervals.append(sval[1]);
    }
    if(othervals.indexOf(key.toString(),0) != -1){
        for (int i = 0; i < edits.length(); i++){
            if (edits[i]->keySequence().toString() != "") {
                QString sval = "species";
                if(this->windowTitle() == QLatin1String("Species/Items Edit"))
                    sval = "groupings";

                if (edits[i]->keySequence().toString() == key.toString()){
                    MessageDialog mb;
                    mb.setText("Key '" + edits[i]->keySequence().toString() + QString("' was already assigned \nin the %1 keys").arg(sval));
                    mb.exec();

                    if(row > -1){
                        w->clear();
                        edits[i]->clear();
                        edits[i]->setKeySequence(QKeySequence());
                    }else{
                        edits[i]->clear();
                        edits[i]->setKeySequence(QKeySequence());
                    }
                    goto returnotherval;
                }
            }
        }
        returnotherval:{
            return;
        }
    }

    Q_UNUSED(key)
    QStringList allKey;
    for (int i = 0; i < edits.length(); i++){
        QFont font;
        font.setCapitalization(QFont::AllLowercase);

        if (edits[i]->keySequence().toString() != "") {
            if (!allKey.contains(edits[i]->keySequence().toString(), Qt::CaseInsensitive))
                 allKey.append(edits[i]->keySequence().toString());
            else {
                MessageDialog mb;
                mb.setText("The '" + edits[i]->keySequence().toString() + "' key has already been assigned.");
                mb.exec();

                if(row > -1){
                    edits[row]->clear();//clear the key sequence //added 7.24.2020
                    w->clear();

                }else{
                    edits[i]->clear();
                }
                goto returneditval;
            }
        }

        edits[i]->setKeySequence(edits[i]->keySequence()[0]);
        edits[i]->setStyleSheet("background-color: white;");
        edits[i]->setFont(font);
        edits[i]->clearFocus();
    }

    returneditval:{
        return;
    }
}



void LogItemEditDialog::ReceiveItems(const QStringList &items){
    try{
        const int &lengthy = logItems.length();
        for (int i = 0; i < lengthy; i ++){
            logItems.removeAt(0);
        }
        edits.clear();
        ui->tableWidget->setRowCount(0);
        for(int s = 0; s < items.length(); s++){
            const QString &lineToTranslate = items.at(s);
            const QStringList &cur = lineToTranslate.split(';');
            bool shouldGroup = false;
            if (useCheckBox){
                if (cur[2] == QLatin1String("true")){
                    shouldGroup = true;
                }
            }
            AddItem(cur[0],cur[1],shouldGroup);
        }

        //add logic for loading stuff
        emit Loaded();
    }catch(...){//added 5.15.2020

    }
}
void LogItemEditDialog::reject(){
    //ffffff, whatever I guess warnings scare people
    /*for(int i = 0; i < logItems.length(); i++){
       QString k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
       if (k != nullptr){
           QKeySequence s (k);
           logItems[i]->shortcutKey = s;
           //qDebug()<< logItems[i]->shortcutKey.toString();

       }
       if (useCheckBox){
           QString c = ui->tableWidget->cellWidget(i,2)->property("checked").toString();
           //qDebug() << c;
           if (c == QLatin1String("true")){
               logItems[i]->groupingEnabled = true;
           }else{
               logItems[i]->groupingEnabled = false;
           }
           //qDebug()<< logItems[i]->groupingEnabled;
       }
   }
   emit Done();
   hide();*/
    //MessageDialog mb;
    //mb.setTitle("Warning");
    if (hasChangeMade()) {
        retainPreItems(preItems);
        //mb.setText("All Changes will not be saved!" );
        //mb.exec();
    }
    this->hide();
}

void LogItemEditDialog::OnEditingFinished()
{
    QKeySequenceEdit *w = qobject_cast<QKeySequenceEdit *>(sender());
    int row = -1;
    if(w){
        row = ui->tableWidget->indexAt(w->pos()).row();
        //qDebug() << "OnEditingFinished: " << w->keySequence().toString();
    }
}

bool LogItemEditDialog::hasChangeMade()
{
    duplicate = false;
    if (fields == QLatin1String("Specimen and Hotkeys")) {
        if (ui->tableWidget->rowCount() != preItems.count())
            return true;

        for(int i = 0; i < preItems.count(); i++){
            const QStringList &list = preItems[i].split(";");
            //const QString &itemName = list[0];
            const QString &itemShortCut = list[1];
            const QString &itemGrouping = list[2];

            const QString &key = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
            if(key != itemShortCut)
                return true;

            const QString &chk = ui->tableWidget->cellWidget(i,2)->property("checked").toString();
            if(chk != itemGrouping)
                return true;
        }

    }
    else {
        if (ui->tableWidget->rowCount() != preItems.count())
            return true;

         for(int i = 0; i < preItems.count(); i++){
            const QStringList &list = preItems[i].split(";");
            const QString &item = list[1];

            const QString &key = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
            if(key != item)
                return true;

         }
    }
    return false;
}

void LogItemEditDialog::validateItems()//added 7.23.2020
{
    QString errorMessage = "Error! The following shortcuts are already used, they won't be saved! \r\n";

    QStringList userDefinedKeys;
    bool conflictingKeys = false;
    QString conflictingKeyVal = "";

    QStringList usedHotkeys;
    usedHotkeys = defaultUsedHotkeys;

    for(int i = 0; i < logItems.length(); i++){
        //const QString &k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
        const QString &k = edits[i]->keySequence().toString();
        if (k != nullptr){
            QString keyVal = k;
            if(keyVal.length() > 0){
                if(usedHotkeys.indexOf(keyVal,0) != -1){
                    conflictingKeys = true;
                    errorMessage += "\r\n - '" + keyVal + "'";
                    conflictingKeyVal = keyVal;
                    goto testconflictkeys;
                }else{
                    usedHotkeys.append(keyVal);
                }
            }
        }
    }

    for(int i = 0; i < otherItems.length(); i++){
        const QString &lineToTranslate = otherItems.at(i);
        const QStringList &cur = lineToTranslate.split(';');
        QString keyVal = cur[1];
        if(keyVal.length() > 0){
            if(usedHotkeys.indexOf(keyVal,0) != -1){
                conflictingKeys = true;
                errorMessage += "\r\n - '" + keyVal + "'";
                conflictingKeyVal = keyVal;
                goto testconflictkeys;
            }else{
                usedHotkeys.append(keyVal);
            }
        }
    }

    testconflictkeys:
    if (conflictingKeys){
        QMessageBox mb;
        mb.setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
        mb.setWindowTitle("Error");
        mb.setText(errorMessage);
        mb.exec();

        for (int i = 0; i < edits.length(); i++){
            if (edits[i]->keySequence().toString() != "") {
                if (edits[i]->keySequence().toString() == conflictingKeyVal){
                    edits[i]->clear();
                    return;//added 7.23.2020
                }
            }
        }

        //qDebug() << "not saved";

        /*//refreah values
        for(int i = 0; i < logItems.length(); i++){
            const QString &k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
            if (k != nullptr){
                if(k == conflictingKeyVal){
                    RefreshItems(preItems);
                }
            }
        }*/

        return;
    }

    //qDebug() << "dosave";

    for(int i = 0; i < logItems.length(); i++){
        const QString &k = ui->tableWidget->cellWidget(i,1)->property("keySequence").toString();
        //qDebug() << i << "k: " << k;
        //if (k != nullptr){//remarked for the keys to be removed and saved to null
            QKeySequence s (k);
            logItems[i]->shortcutKey = s;
        //}
        if (useCheckBox){
            const QString &c = ui->tableWidget->cellWidget(i,2)->property("checked").toString();
            if (c == QLatin1String("true")){
                logItems[i]->groupingEnabled = true;
            }else{
                logItems[i]->groupingEnabled = false;
            }
        }
    }
    /*for(int i = 0; i < logItems.length(); i++){
        qDebug() << i << "logItems: " << logItems[i]->shortcutKey.toString();
    }*/
    emit Done();
    this->hide();
}

void LogItemEditDialog::on_tableWidget_viewportEntered()
{

}

void LogItemEditDialog::on_buttonBox_clicked()
{
    onSave();
}


void LogItemEditDialog::on_pushButton_clicked()
{

}

void LogItemEditDialog::on_pushButton_2_clicked()
{

}

void LogItemEditDialog::on_buttonBox_pressed()
{
    onSave();
}

void LogItemEditDialog::on_btnrefreshlist_pressed()
{
    RefreshItems(preItems);
}

void LogItemEditDialog::on_btndefaultlist_pressed()
{
    defaultKeysDlg = new DefaultHotKeys(this);
    defaultKeysDlg->defaultUsedHotkeys = defaultUsedHotkeys;
    defaultKeysDlg->initForm();
    defaultKeysDlg->setModal(true);
    defaultKeysDlg->show();
    defaultKeysDlg->setFocus();
}
