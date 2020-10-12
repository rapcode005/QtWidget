#include "filehandler.h"
#include <QFileDialog>
#include <QDialog>
#include <QList>
#include<QWidget>

FileHandler::FileHandler()
{
    //setting the application path for gps files
    localDirectory = QCoreApplication::applicationDirPath() + "/UserData/GPSFiles/";
    qDebug() << localDirectory << " This should be your local directory";
    qDebug() << "File Handling Module Initialized";
}

void FileHandler::RefreshDirectory(QString path, QListWidget *list, QString fileType, bool checkMarks){
    list->clear();
    //making sure our directory path exists
    if (!path.isEmpty()){
        //get a LIST of FILES ending in .txt in the given path
        QDirIterator it(path, QStringList() << "*" + fileType, QDir::Files, QDirIterator::Subdirectories);
        //loop through the files in the list
        int index = 0;
        while (it.hasNext()){
            qDebug() << it.next();
            //Removing the directory prefix from the file, for aesthetic reasons
            QString newName = it.filePath().remove(0,path.length());
            //Adding an item with the newly created name
            list->addItem(newName);
            if (checkMarks) list->item(index)->setCheckState(Qt::CheckState::Checked); //if we pass checkmarks with true, it'll add checkboxs to the item
            index++;
        }
    }
}
void FileHandler::TransferFiles(QString path, QListWidget *inlist, QListWidget *outlist, QCheckBox *box){
    outlist->clear(); //this clears the list we move the files to
    if (localDirectory.isEmpty()){
        return;
    }
    QDir dir(localDirectory);  //defining that as a directory so we can check if it exists, if it doesn't
    if (!dir.exists()) {
        dir.mkpath(".");  //then we make one
    }
    QList<QString> newItems;
    QDir idir;  //this defines a directory that we use in our loop
    for (int i = 0; i < inlist->count(); i++){  //this loop takes all the items in the list we move files from and iterates through it
        if (inlist->item(i)->checkState() == Qt::Checked){
            QFile file(path + inlist->item(i)->text()); //reference to a file with a name
            idir = localDirectory + inlist->item(i)->text();  //using that directory we defined
            if(!file.setPermissions( QFile::WriteOwner | QFile::ReadOwner)){ qDebug() << "Couldn't set permissions";} //setting permissions for a later step
            if (!file.exists()){  //making sure our file exists before we mess with it
                return;
            }else{
                if(!file.open(QIODevice::ReadWrite)){ //making sure we can actually do stuff with this file
                    return;
                }else{
                    if (!idir.exists()) {
                        idir.mkpath(".."); //the dots in the make path tell us to make folders, instead of folders named after our file
                    }
                    qDebug() << idir.path();
                    //removing the file in our local directory if it already exists
                    QFile f (idir.path());
                    if (f.open(QIODevice::ReadWrite)){
                        if (!f.remove())
                            qDebug()<< idir.path() << " could not be overwritten.";
                    }
                if (file.copy(idir.path())){ //attempting to copy the file to our new directory, based on the file's name
                    QFile newFile(idir.path());
                    if (newFile.size() == file.size()){ //making sure our new file's size is equal to the one on the sd card
                        qDebug()<< "Success!";
                        inlist->item(i)->setBackgroundColor(QColor(Qt::GlobalColor::green));
                        if (box->isChecked()){ //if we have our checkbox checked, then remove the file from the device
                            if (!file.remove()){
                                qDebug()<< file.fileName() << " could not be removed from the device.";
                            }
                        }
                    QString name = inlist->item(i)->text();
                    newItems << name; //add our successfuly copied file to our outlist
                    }else{ //if our file isn't the right size when it's copied over, then report this error
                        qDebug()<< "Copied an empty file.";
                        inlist->item(i)->setBackgroundColor(QColor(Qt::GlobalColor::yellow));
                    }
                }else{
                        //if the transfer fails, tell us if there was ever any hope
                        qDebug()<< "Transfer failed - " << "Read : " << file.isReadable() << " Write :  "<< file.isWritable();
                        inlist->item(i)->setBackgroundColor(QColor(Qt::GlobalColor::red));
                    }
                file.close();
                }
            }
        }
    }
    //finally, getting all the .txt files in our new directories and showing them in the list widget
    RefreshDirectory(localDirectory,outlist,".gps", false);
    RefreshDirectory(fsearchDirectory, inlist,".gps", true);
    //Then color coating the new items in the outlist as green
    for (int i = 0; i < outlist->count(); i++){//for every list item
        for (int n = 0; n < newItems.length(); n++) {//compare to every successfuly transferred item
            if (newItems[n].startsWith('/')){
            newItems[n].remove(0,1);
            }
            if (outlist->item(i)->text() == newItems[n]){//remove the first slash in new items and compare it to an item in the outlist
                outlist->item(i)->setBackgroundColor(QColor(Qt::GlobalColor::green));//when it's successful, color coat it green
            }
        }
    }
}

