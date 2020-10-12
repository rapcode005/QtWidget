#ifndef INTERPOLATEGPS_H
#define INTERPOLATEGPS_H

#include <QDialog>
#include "gpshandler.h"

namespace Ui {
class interpolategps;
}

class interpolategps : public QDialog
{
    Q_OBJECT

public:
    explicit interpolategps(QWidget *parent = nullptr);
    ~interpolategps();

    QStringList m_flightpath;
    QString m_flightFilePath;
    QStringList m_flightgpsdatetime;
    void initForm(GpsHandler *gps);
    int radius = 6371;
    double calculateBearing(double ax, double ay, double bx, double by);
    double calculateDistanceBetweenLocations(double ax, double ay, double bx, double by);
    QStringList calculateDestinationLocation(double x, double y, double bearing, double distance);
    GpsHandler *m_gps;
    QList<QStringList> missinglist;

    void calculateMissingSec();

private slots:
    void on_btnLoad_pressed();

    void on_btnTest_clicked();

    void on_btnInterpolate_pressed();

    void on_btnSave_pressed();

    void on_btnclear_pressed();

private:
    Ui::interpolategps *ui;
    void populateHeader();
    void populateFlightPath();

    void saveGps(QString filename);
    void clearForm();
};

#endif // INTERPOLATEGPS_H
