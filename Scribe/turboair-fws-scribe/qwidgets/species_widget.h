#ifndef SPECIES_WIDGET_H
#define SPECIES_WIDGET_H

#include <QDialog>
#include <QList>
#include "logtemplateitem.h"
#include <QDebug>



namespace Ui {
class SpeciesWidget;
}



class SpeciesWidget : public QDialog
{
    Q_OBJECT

public:
    explicit SpeciesWidget(QWidget *parent = nullptr, QString windowTitle = "No Title", bool useCheck = false);
    ~SpeciesWidget();
    QList<LogTemplateItem*> logItems;

public slots:
    void ReceiveItems(QStringList items);
    void reject();

signals:
    void Done();
    void Loaded();

private:
    Ui::SpeciesWidget*                   ui;
    bool                        useCheckBox;
    QRegExp                              rx;
    QList<QKeySequenceEdit*>          edits;
    QTimer                     *hotKeyTimer;

private slots:
    void AddItem(QString name, QString shortcut = "", bool useGrouping = false);
    void RemoveRowItem(int row);
    void on_pushButton_pressed();
    void on_buttonBox_accepted();
    void on_pushButton_2_pressed();
    void StopRecording(QKeySequence key);
    void on_tableWidget_viewportEntered();
    void CheckEdititing();
};





#endif // SPECIES_WIDGET_H




