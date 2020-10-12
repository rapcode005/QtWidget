#ifndef WORKERTHREADSURVEY_H
#define WORKERTHREADSURVEY_H

#include <QThread>
#include <QObject>
#include "projectfile.h"
#include "globalsettings.h"
#include "distanceconfig.h"

class WorkerThreadSurvey : public QThread
{
    Q_OBJECT
    void run() override;

public:
    ProjectFile *getProjectFile();
    void setProjectFile(ProjectFile *proj);

    int getAction() const;
    void setAction(const int &newAction);

    QString getSurveyFile() const;
    void setSurveyFile(const QString &survey);

    QString getDir() const;
    void setDir(const QString &dir);

    DistanceConfigObj *getDitanceConfig();
    void setDitanceConfig(DistanceConfigObj *dc);

    bool getIsSurveyWBPHS() const;
    void setIsSurveyWBPHS(const bool &newV);

    //Project Setting Save
    int getHeaderCount() const;
    void setHeaderCount(const int &newC);

    int getAddedCount() const;
    void setAddedCount(const int &newC);

signals:
    //Update Project Setting
    void saveHeaderNameLoop(const int &index);
    void saveHeaderNameDone();
    void saveAddedNameLoop(const int &index);
    void saveAddedNameDone();
    void saveHeaderValueChkLoop(const int &index);
    void saveHeaderValueChkDone();
    void saveAddedValueChkLoop(const int &index);
    void saveAddedValueChkDone();
    void saveSettingDone();

    void refreshDone();
    void createDone();
    void importDone();
    void saveDone();

    //Refresh Additional Fields
    void sendAdditionalFieldsOneVal(QTableWidgetItem *item, const int &index);
    void sendAdditionalFieldsValues(QTableWidgetItem *item, const int &index);
    void sendAdditionalFieldsNoVal(QTableWidgetItem *item, const int &index);
    //Refresh Header Fields
    void sendHeaderFieldsOneVal(QTableWidgetItem *item, const int &index);
    void sendHeaderFieldsOneValNoAddded(QTableWidgetItem *item, const int &index);
    void sendHeaderFieldsValues(QTableWidgetItem *item, const int &index);
    void sendHeaderFieldsNoVal(QTableWidgetItem *item, const int &index);
    //Distance Config
    void updateProjectFile(const double &newObsMaxD);
    void updateProjectFile(const double &newObsMaxD, const double &newAirMaxD);
    //Project Setting
    void preHeaderValue(const int &rowHeight,
                        const int &checkValue,
                        const int &currentRow,
                        QTableWidgetItem *item1,
                        QTableWidgetItem *item2);
    void preAddedValue(const int &rowHeight,
                        const int &checkValue,
                        const int &currentRow,
                        QTableWidgetItem *item1,
                        QTableWidgetItem *item2,
                       const int &index);

    void headerItemByIndex(const int &index);
    void addedItemByIndex(const int &index);

    //Progress Bar
    void load1();
    void load2();
    void load3();
    void unexpectedFinish();

private:
    ProjectFile *m_proj;
    int m_action, m_headerC, m_addedC;
    QString m_survey, m_dir;
    DistanceConfigObj *m_ditanceConfig;
    QTableWidget *m_table;
    bool m_isSurvey;
    QList<QStringList> headerValuesList, addedValuesList;

    void ImportSurvey();
    void CreateSurvey();

    void refreshHeaderFields();
    void refreshAdditionalFields();

    //Project Setting
    void saveHeaderNames();
    void saveAddedNames();
    void saveHeaderValueChk();
    void saveAddedValueChk();

    void preHeaderValues();
    void preAdditionalValues();

    //Auto Save
    void AutoSave();
};

#endif // WORKERTHREADSURVEY_H
