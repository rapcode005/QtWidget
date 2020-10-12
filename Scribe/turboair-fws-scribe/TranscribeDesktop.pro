#-------------------------------------------------
#
# Project created by QtCreator 2017-12-14T17:14:36
#
#-------------------------------------------------

CONFIG +=  static static-release

QT       += core gui multimedia qml quick quickwidgets network quick positioning location

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = scribe
TEMPLATE = app
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

QT += webenginewidgets

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    aboutdialog.cpp \
    aboutscribe.cpp \
    customJsonArray.cpp \
    customcombobox.cpp \
    customevent.cpp \
    customeventwidget.cpp \
    defaulthotkeys.cpp \
    distanceConfigObj.cpp \
    distanceconfig.cpp \
    docLayout.cpp \
    interpolategps.cpp \
    main.cpp \
    gpshandler.cpp \
    audioplayer.cpp \
    messagebox.cpp \
    newmainwindow.cpp \
    #projectsettingobj.cpp \
    progressmessage.cpp \
    projectinformation.cpp \
    projectsettingsdialog.cpp \
    projectfile.cpp \
    filestructureinfodialog.cpp \
    gpsrecording.cpp \
    exportdialog.cpp \
    logitemeditdialog.cpp \
    logtemplateitem.cpp \
    bookmark.cpp\
    additionalfieldsdialog.cpp \
    testmainwindow.cpp \
    transectinfo.cpp \
    openingscreen.cpp \
    #mainwindow2.cpp \
    globalsettings.cpp \
    userguide.cpp \
    wbps.cpp \
    wbpsObj.cpp \
    workerThread2nd.cpp \
    workerThreadImportFlight.cpp \
    workerThreadSurvey.cpp \
    workingThread.cpp

HEADERS += \
    aboutdialog.h \
    aboutscribe.h \
    customJsonArray.h \
    customcombobox.h \
    customevent.h \
    customeventwidget.h \
    defaulthotkeys.h \
    distanceConfigObj.h \
    distanceconfig.h \
    docLayout.h \
    gpshandler.h \
    audioplayer.h \
    interpolategps.h \
    messagebox.h \
    newmainwindow.h \
    #projectsettingobj.h \
    progressmessage.h \
    projectinformation.h \
    projectsettingsdialog.h \
    projectfile.h \
    filestructureinfodialog.h \
    gpsrecording.h \
    exportdialog.h \
    logitemeditdialog.h \
    logtemplateitem.h \
    bookmark.h \
    additionalfieldsdialog.h \
    testmainwindow.h \
    userguide.h \
    version.h \
    transectinfo.h \
    openingscreen.h \
    #mainwindow2.h \
    globalsettings.h \
    wbps.h \
    wbpsObj.h \
    workerThread2nd.h \
    workerThreadImportFlight.h \
    workerThreadSurvey.h \
    workingThread.h

FORMS += \
    aboutdialog.ui \
    aboutscribe.ui \
    defaulthotkeys.ui \
    distanceconfig.ui \
    interpolategps.ui \
    messagebox.ui \
    newmainwindow.ui \
    progressmessage.ui \
    projectinformation.ui \
    projectsettingsdialog.ui \
    filestructureinfodialog.ui \
    exportdialog.ui \
    logitemeditdialog.ui \
    additionalfieldsdialog.ui \
    openingscreen.ui \
    #mainwindow2.ui \
    testmainwindow.ui \
    userguide.ui

DISTFILES +=

RESOURCES += \
    qmlscripts.qrc

RC_FILE = resources.rc

