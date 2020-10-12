#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QtCore>
#include <QFile>
#include <QDir>
#include <QMediaPlayer>
#include <QtWidgets>
#include "filehandler.h"
#include "gpshandler.h"
#include "audioplayer.h"
#include "projectsettingsdialog.h"
#include "filestructureinfodialog.h"
#include "bookmark.h"
#include "gpsrecording.h"
#include "exportdialog.h"
#include "openingscreen.h"
#include "distanceconfig.h"
#include "docLayout.h"
#include "wbps.h"
#include "wbpsObj.h"
#include "customeventwidget.h"
#include "customcombobox.h"
#include <QFocusEvent>
#include "aboutscribe.h"
#include "userguide.h"
#include "projectinformation.h"
#include "aboutdialog.h"
#include "messagebox.h"
#include "workerthread2nd.h"
#include "workerThreadImportFlight.h"
#include "progressmessage.h"
#include "workerThreadSurvey.h"
#include "interpolategps.h"

namespace Ui {
class TestMainWindow;
}

class TestMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TestMainWindow(QWidget *parent = nullptr);
    ~TestMainWindow();

private:
    Ui::TestMainWindow *ui;
};

#endif // TESTMAINWINDOW_H
