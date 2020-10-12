#ifndef PROJECTFILE_H
#define PROJECTFILE_H

#include <QObject>
#include "projectsettings.h"
#include <QDir>
#include <QJsonArray>
#include <QTableWidget>
#include <QJsonObject>
#include <QTextEdit>
#include <QComboBox>
#include "customeventwidget.h"
#include <QHash>
#include "projectsettingobj.h"
#include "customcombobox.h"
#include "wbps.h"
//#include "customJsonArray.h"

class ProjectFile : public QObject
{
    Q_OBJECT
public:
    ProjectFile();

    bool changesMade;

    QJsonArray addionalFieldsNamesArray;
    QJsonArray addionalFieldsValuesArray;
    QJsonArray addionalFieldsValuesChkArray; //added 4.22.2020 used in save settings
    QJsonArray headerNamesArray;
    QJsonArray headerValuesArray;
    QJsonArray headerValuesChkArray; //added 4.22.2020 used in save settings

    QDir dirPath;
    QString fileName;
    QString surveyType;

    QJsonObject geoJSON;
    QJsonObject airGround;

    QStringList addedGroupings;
    QStringList addedSpecies;

    //Get the Index of the max length
    int maxSpecies = -1;

    QStringList usedAudio;
    QStringList usedGps;

    bool Save(QDir dir, bool checkChanges = false);
    void buildStandardSave(QJsonObject &obj);

    bool Load(QDir dir);
    bool HasAutoSave();
    QString getMostRecentAutoSaveFile();

    int getHeaderCount();
    QString getHeaderName(int idx);
    void buildHeaderCell(QTableWidget *tbl, int idx);
    QString getInstanceHeaderDataValue(QString headerName);
    int getInstanceHeaderPosition(QString headerName);
    bool isHeaderSingleValue(int idx);
    QStringList getHeaderNamesList();
    QString getHeaderValue(QTableWidget *tbl, int idx);
    QString getHeaderValue(QTableWidget *tbl, int idx, QString title);
    QStringList getHeaderValueList(int idx);
    int getHeaderIndex(QString headerName);
    void setHeaderNames(QJsonArray h);
    void setHeaderValues(QJsonArray h);
    void setHeaderValuesChk(QJsonArray h);//added 4.22.2020 used in save settings
    QString getHeaderValueChk(int idx);//added 4.22.2020 used in save settings
    QStringList getHeaderValueChk2(int idx);//added 4.26.2020 used in save settings

    QStringList getAdditionalFieldsNamesList();
    void buildAdditionalfieldsTable(QTableWidget *tbl);
    void buildAdditionalFieldCell(QTableWidget *tbl, int idx);
    QString getAdditionalFieldName(int idx);
    int getAdditionalFieldsCount();
    bool isAdditionalFieldSingleValue(int idx);
    QString getAdditionalFieldValue(QTableWidget *tbl, int idx);
    QString getAdditionalFieldValue(QTableWidget *tbl, int idx, QString title);
    QStringList getAdditionalFieldValueList(int idx);
    void setAdditionalFieldNames(QJsonArray h);
    void setAdditionalFieldValues(QJsonArray h);
    void setAdditionalFieldValuesChk(QJsonArray h);//added 4.22.2020 used in save settings
    int getAdditionalFieldIndex(QString additionalFieldName);
    QString getAdditionalFieldChk(int idx);//added 4.23.2020 used in save settings
    QStringList getAdditionalFieldChk2(int idx);//added 4.26.2020 used in save settings

    QString getSurveyType();

    //QTextEdit *commentTxt;
    //QTextEdit *commentTxtH;

    void setCurMainWindows(QObject *newObj);

    //Observation Max D
    void setObsMaxD(const double &newVal);
    double getObsMaxD() const;

    //Air Ground Max D
    void setAirGroundMaxD(const double &newVal);
    double getAirGroundMaxD() const;

    //Conditional values
    void setConditionalValueFields(QJsonArray &newArray);
    QJsonArray getConditionalValueFieldsArray();

    //TextEdit List
    //QList<QTextEdit*> additionalFields;
    QList<CustomEventWidget*> additionalFields;
    QList<CustomEventWidget*> headerFields;
    QStringList allHeaderV;
    int countA = 0;
    int countH = 0;
    QList<int> additionalIndex, headerIndex;
    //ComboBox List
    QList<CustomComboBox*> headerFieldsC;
    QList<QComboBox*> additionalFieldsC;
    int countHC = 0;
    int countAC = 0;
    QList<int> additionalIndexC, headerIndexC;

    int stratumIndex;
    int transectIndex;
    int segmentIndex;
    int plotnameIndex; //added 5.18.2020
    int aGNameIndex;
    int bcrIndex;
    int trnIndex;
    int distanceIndex; //added 6.19.2020
    int commentIndex; //added 8.11.2020

    //Storage of list of unchecked fields
    QStringList headerFieldsChk, additionalFieldsChk;
    QStringList hAFieldsCb, aFieldsCb;

    //bool eventFilter(QObject *watched, QEvent *event);
    //GOEA
    void loadGOEAProperties();
    QStringList getTRN() const;
    QStringList getBCR() const;
    QStringList getBCRTRN() const;

    //WBPS
    void loadWBPSProperties();
    WBPS *getWBPS();

    //Air Ground
    void loadAirGroundProperties();
    QStringList getAllAGNameList() const;

    //BAEA
    void loadPlotNameList();
    QStringList getAllPlotNameList() const;

    //Previous Value for GOEA
    QString getTRNPre() const;
    QString getBCRPre() const;

    bool closeItems;

    QFont getFontItem(bool bold = true);

    //Changes
    void loadChanges();
    bool getChanges() const;
    void setChanges(const bool &newC);

    bool newOpen;

    QString cssReadOnly() const;

signals:
    void Loaded();
    //Use for distance configuration
    void LoadedDistanceConfig();

private:
    QList<QString> instancehList;
    QObject *m_curMainWindow;
    double m_obsMaxD, m_airGroundMaxD;
    QStringList m_trn, m_bcr, m_bcrTrn, m_agName, m_plotNameList;
    WBPS *m_wbps;
    QFont m_font;
    bool m_changes;
    QJsonArray m_conditionalvaluefieldsarray;
};

#endif // PROJECTFILE_H
