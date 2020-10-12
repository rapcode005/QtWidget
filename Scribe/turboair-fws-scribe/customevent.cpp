#include "customevent.h"

CustomEvent::CustomEvent(QObject *parent, int newMethod)
{
    //Set What kind of prompt
    method = newMethod;
    addTo(parent);
}

void CustomEvent::addTo(QObject *obj)
{
    if (obj) {
        obj->installEventFilter(this);
        objectEvent = obj;
    }
}

void CustomEvent::removeFilter()
{
    try {

        if (objectEvent)
            objectEvent->removeEventFilter(this);

    } catch (...) {
    }
}

bool CustomEvent::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        switch(method) {
            case 0: {

                QString curDir = QDir::homePath();
                const QString &dirPath = "C:/Users/Public/Downloads";
                //Get current directory path
                QFile file(dirPath + "/projectDir");
                if (!file.open(QFile::ReadOnly)){

                }
                else {
                    //qDebug() << file.readAll();
                    curDir = file.readAll();
                }

                //qDebug() << curDir;

                const QString &dir = QFileDialog::getExistingDirectory(this,
                                                                tr("Choose Project Directory"),
                                                                curDir,
                                                                QFileDialog::ShowDirsOnly
                                                                | QFileDialog::DontResolveSymlinks);

                if(dir.length() > 0){
                    QGenericArgument genericArg =  Q_ARG(QString, dir);
                    bool invd = QMetaObject::invokeMethod(watched, "setText", Qt::QueuedConnection, genericArg);
                    if (invd) {
                        //Save new directory path
                        QFile savedFile(dirPath + "/projectDir");
                        //savedFile.setDirectWriteFallback(true);
                        if(!savedFile.open(QFile::WriteOnly)) {
                            qDebug() << "Unable to read";
                        }
                        else {
                            qDebug() << savedFile.write(dir.toUtf8());
                            savedFile.close();
                        }
                    }
                    else
                        qDebug() << "Error";
                }
                break;

            }

            case 1: {
                openFileDialog("Select Survey JSON", "Survey JSON (*.geojson)", "survey", watched);
                break;
            }

            default: {
                openFileDialog("Select Air Ground JSON", "Air Ground JSON (*.geojson)", "air", watched);
                break;
            }

        }

    }
    return QObject::eventFilter(watched, event);
}

void CustomEvent::openFileDialog(const QString &title, const QString &filter, const QString &fileTitle, QObject *watched)
{
    QString dir = QDir::homePath();
    const QString &dirPath = "C:/Users/Public/Downloads";
    //Get current directory path
    QFile file(dirPath + "/" + fileTitle);
    //qDebug() << dirPath + "/" + fileTitle;
    if (!file.open(QFile::ReadOnly)){

    }
    else {
        dir = file.readAll();
    }

    QString surveyFile = QFileDialog::getOpenFileName(this,
                                                      title,
                                                      dir,
                                                      filter);
    if (surveyFile.isEmpty()){
        return;
    }

    //qDebug() << surveyFile;

    QDir directory(surveyFile);

    bool inv = QMetaObject::invokeMethod(watched, "setText", Qt::QueuedConnection, Q_ARG(QString, directory.path()));
    if (inv == true) {
        //Save new directory path
        QFile savedFile(dirPath + "/" + fileTitle);
        if(!savedFile.open(QFile::WriteOnly)) {
            qDebug() << "Unable to read";
        }
        else {
            savedFile.write(directory.path().toUtf8());
            savedFile.close();
        }
    }
    else
        qDebug() << "Error";

}
