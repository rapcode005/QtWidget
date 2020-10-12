#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <QFile>
#include <QtWidgets>

class FileHandler
{
public:
    FileHandler();

    QString                   fsearchDirectory;
    QString                   localDirectory;
    QString                   startdir;
    QString                   curFileDirectory;
    QString                   curProjFile;
    QStringList            assocRawFiles;

    void RefreshDirectory(QString path, QListWidget *list, QString fileType, bool checkMarks);
    void TransferFiles(QString path, QListWidget *inlist, QListWidget *outlist, QCheckBox *box);
};

#endif // FILEHANDLER_H
