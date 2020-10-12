#include "globalsettings.h"
#include "interpolategps.h"
#include "messagebox.h"
#include "ui_interpolategps.h"

#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QEvent>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QException>
#include <QMessageBox>
#include <QtMath>
#include "gpshandler.h"

bool lessThan( const QStringList& v1, const QStringList& v2)
{
    return v1.first() < v2.first();
}

interpolategps::interpolategps(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::interpolategps)
{
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->setupUi(this);

    ui->btnLoad->setFocus();
    ui->btnSave->setEnabled(false);
    ui->btnTest->setEnabled(false);
    ui->btnInterpolate->setEnabled(false);
    ui->txtFile->setReadOnly(true);
    ui->txtFile->setText("");
    ui->btnTest->setVisible(false);
}

interpolategps::~interpolategps()
{
    delete ui;
}

void interpolategps::initForm(GpsHandler *gps)
{
    m_gps = gps;
    ui->txtFile->setText("");
    QString flightFilePath;
    if(m_flightFilePath.trimmed().length() > 0){
        QDirIterator gpsFileIterator(m_flightFilePath, QStringList() << "*.GPS", QDir::Files, QDirIterator::Subdirectories);
        while(gpsFileIterator.hasNext()) {
            const QString &sourceFilePath = gpsFileIterator.next();
            flightFilePath = sourceFilePath;
        }

        QFile gpsFile(flightFilePath);
        if (gpsFile.exists()) {
            //qDebug() << "flightFilePath " << flightFilePath;
            ui->txtFile->setText(flightFilePath);
        }
    }

    ui->tblgps->setRowCount(0);
    ui->tblmissing->setRowCount(0);
    if(m_flightpath.length() > 0){
        populateHeader();
        populateFlightPath();
    }

}

double interpolategps::calculateBearing(double ax, double ay, double bx, double by)
{
    double lat1 = qDegreesToRadians(ax);
    double lat2 = qDegreesToRadians(bx);
    double deltaLon = qDegreesToRadians(by - ay);

    double y = qSin(deltaLon) * qCos(lat2);
    double x = qCos(lat1) * qSin(lat2) - qSin(lat1) * qCos(lat2) * qCos(deltaLon);
    double bearing = qAtan2(y, x);

    // since atan2 returns a value between -180 and +180,
    //we need to convert it to 0 - 360 degrees
    return (qRadiansToDegrees(bearing) + 360) / 360;
}

double interpolategps::calculateDistanceBetweenLocations(double ax, double ay, double bx, double by)
{
    double lat1 = qDegreesToRadians(ax);
    double lon1 = qDegreesToRadians(ay);

    double lat2 = qDegreesToRadians(bx);
    double lon2 = qDegreesToRadians(by);

    double deltaLat = lat2 - lat1;
    double deltaLon = lon2 - lon1;

    double a = qSin(deltaLat / 2) * qSin(deltaLat / 2) + qCos(lat1) * qCos(lat2) * qSin(deltaLon / 2) * qSin(deltaLon / 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));

    return (radius * c);
}

QStringList interpolategps::calculateDestinationLocation(double x, double y, double bearing, double distance)
{
    QStringList coor;
    distance = distance / radius; // convert to angular distance in radians
    bearing = qDegreesToRadians(bearing); // convert bearing in degrees to radians

    double lat1 = qDegreesToRadians(x);
    double lon1 = qDegreesToRadians(y);

    double lat2 = qAsin(qSin(lat1) * qCos(distance) +
                        qCos(lat1) * qSin(distance) * qCos(bearing));
    double lon2 = lon1 + qAtan2(qSin(bearing) * qSin(distance) * qCos(lat1),
                                qCos(distance) - qSin(lat1) * qSin(lat2));
    lon2 = (lon2 + 3 * M_PI) / ((2 * M_PI) - M_PI); // normalize to -180 - + 180 degrees

    coor << QString::number(qRadiansToDegrees(lat2),'f',8);
    coor << QString::number(qRadiansToDegrees(lon2),'f',8);
    return coor;
}

void interpolategps::populateHeader()
{
    QStringList m_TableHeader;
    QStringList columncount;
    if(m_flightpath.length() > 1)
        columncount = m_flightpath[1].split(",");
    else if(m_flightpath.length() == 0){

    }
    ui->tblgps->setColumnCount(columncount.length());
    ui->tblgps->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tblgps->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->tblmissing->setColumnCount(columncount.length());
    ui->tblmissing->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tblmissing->setSelectionMode(QAbstractItemView::SingleSelection);

    QAbstractButton *button = ui->tblgps->findChild<QAbstractButton *>();
    if(button) {
        button->setText(tr("No."));
        button->setStyleSheet("font:13px 'Segoe UI'; padding-left: 5px; padding-bottom:5px;");
        button->setContentsMargins(0, 0, 0, 0);
        button->installEventFilter(this);

        QStyleOptionHeader *opt = new QStyleOptionHeader();
        opt->text = button->text();
        QSize *s = new QSize(button->style()->sizeFromContents(QStyle::CT_HeaderSection, opt,
                                                               QSize(), button).expandedTo(QApplication::globalStrut()));
        if (s->isValid()){
            ui->tblgps->verticalHeader()->setMinimumWidth(s->width());
            ui->tblmissing->verticalHeader()->setMinimumWidth(s->width());
        }
    }
//    qDebug() << "columncount: " << columncount;
//    qDebug() << "columncountlength: " << columncount.length();
    if(columncount.length() == 12)
        m_TableHeader << "Latitude" <<"Longitude" << "Year" << "Month" << "Day" << "Time" <<
                         "Altitude"  << "Speed" << "Dir" << "No Sat" << "Dilution" <<
                         "MilliSec";
    else if(columncount.length() == 13)
        m_TableHeader << "Latitude" <<"Longitude" << "Year" << "Month" << "Day" << "Time" <<
                         "Altitude"  << "Speed" << "Dir" << "No Sat" << "Dilution" <<
                         "MilliSec" << "column1";

    ui->tblgps->setHorizontalHeaderLabels(m_TableHeader);
    ui->tblmissing->setHorizontalHeaderLabels(m_TableHeader);

    ui->tblgps->setColumnWidth(0,100);//latitude
    ui->tblgps->setColumnWidth(1,100);//longitude
    ui->tblgps->setColumnWidth(2,25);//year
    ui->tblgps->setColumnWidth(3,50);//month
    ui->tblgps->setColumnWidth(4,25);//day
    ui->tblgps->setColumnWidth(5,75);//time
    ui->tblgps->setColumnWidth(6,50);//altitude
    ui->tblgps->setColumnWidth(7,50);//speed
    ui->tblgps->setColumnWidth(8,50);//direction
    ui->tblgps->setColumnWidth(9,50);//number of satellites
    ui->tblgps->setColumnWidth(10,50);//horizontal dilution of precision
    ui->tblgps->setColumnWidth(11,50);//milliseconds the hardware was running

    ui->tblmissing->setColumnWidth(0,100);//latitude
    ui->tblmissing->setColumnWidth(1,100);//longitude
    ui->tblmissing->setColumnWidth(2,25);//year
    ui->tblmissing->setColumnWidth(3,50);//month
    ui->tblmissing->setColumnWidth(4,25);//day
    ui->tblmissing->setColumnWidth(5,75);//time
    ui->tblmissing->setColumnWidth(6,50);//altitude
    ui->tblmissing->setColumnWidth(7,50);//speed
    ui->tblmissing->setColumnWidth(8,50);//direction
    ui->tblmissing->setColumnWidth(9,50);//number of satellites
    ui->tblmissing->setColumnWidth(10,50);//horizontal dilution of precision
    ui->tblmissing->setColumnWidth(11,50);//milliseconds the hardware was running

    if(columncount.length() == 13){
        ui->tblgps->setColumnWidth(12,50);//unknown
        ui->tblmissing->setColumnWidth(12,50);//unknown
    }

}

void interpolategps::populateFlightPath()
{
    for(int s = 0; s < m_flightpath.length(); s++){
        QStringList flightrow = m_flightpath[s].split(",");
        ui->tblgps->insertRow( s );
        bool ok;
        float lat = flightrow[0].trimmed().toFloat(&ok);
        if(ok){
            ok = false;
            float lon = flightrow[1].trimmed().toFloat(&ok);
            if(ok){
                if(QString::number(lat).length() > 0 && QString::number(lon).length() > 0){

                }
                for(int i = 0; i < flightrow.length(); i ++){
                    QTableWidgetItem *item = new QTableWidgetItem();
                    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                    item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                    item->setText(flightrow[i].trimmed());
                    /*ui->tblgps->setItem(s, i, item);
                    QFont cellFont;
                    cellFont.setFamily("Segoe UI");
                    cellFont.setPixelSize(13);
                    cellFont.setBold(true);
                    ui->tblgps->item(s,i)->setData(Qt::FontRole,QVariant(cellFont));*/
                    if(i < ui->tblgps->columnCount())
                        ui->tblgps->setItem( s , i , item);


                }
            }
        }
    }

    if(ui->tblgps->rowCount() > 2){
        ui->btnTest->setVisible(false);
        ui->btnInterpolate->setEnabled(true);
    }
}

void interpolategps::calculateMissingSec()
{
    try{
        int cntr1 = 0;
        for(int s = 0; s < (m_flightpath.length() -1); s++){
            QStringList missing;
            QStringList flightrow = m_flightpath[s].split(",");
            if(flightrow.length() >= 12){
                QString strtime = flightrow[5].trimmed();
                QStringList spltime = strtime.split(":");
                QDateTime t1(
                        QDate(flightrow[2].trimmed().toInt(),
                            flightrow[3].trimmed().toInt(),
                            flightrow[4].trimmed().toInt()),
                        QTime(spltime[0].trimmed().toInt(),
                            spltime[1].trimmed().toInt(),
                            spltime[2].trimmed().toInt())
                        );


                QStringList flightrow1 = m_flightpath[s+1].split(",");
                QString strtime1 = flightrow1[5].trimmed();
                QStringList spltime1 = strtime1.split(":");
                QDateTime t2(
                        QDate(flightrow1[2].trimmed().toInt(),
                            flightrow1[3].trimmed().toInt(),
                            flightrow1[4].trimmed().toInt()),
                        QTime(spltime1[0].trimmed().toInt(),
                            spltime1[1].trimmed().toInt(),
                            spltime1[2].trimmed().toInt())
                        );       

                if(t1.secsTo(t2) >= 2){
                    if(t1.secsTo(t2) == 2){
                        missing.clear();
                        //qDebug() << "sec: " << cntr1 << t1.addSecs(1);
                        ui->tblmissing->insertRow(cntr1);
                        QStringList sval;
                        sval << QString::number(flightrow[0].toDouble(),'f',8)
                                << QString::number(flightrow[1].toDouble(),'f',8)
                                << QString::number(flightrow1[0].toDouble(),'f',8)
                                << QString::number(flightrow1[1].toDouble(),'f',8)
                                << QString::number(2)
                                << QString::number(1);

                        QVariant returnedval;
                        QMetaObject::invokeMethod(m_gps->wayHandler, "findCoor",
                            Q_RETURN_ARG(QVariant,returnedval),
                            Q_ARG(QVariant, sval));
                        QStringList splt = returnedval.toString().split(",");
                        QString lat = splt[0];
                        QString lon = splt[1];


                        QTableWidgetItem *itemlat = new QTableWidgetItem();
                        itemlat->setFlags(itemlat->flags() ^ Qt::ItemIsEditable);
                        itemlat->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                        itemlat->setText(QString::number(lat.toDouble(),'f',8));
                        ui->tblmissing->setItem( cntr1 , 0 , itemlat);//latitude
                        missing << QString::number(lat.toDouble(),'f',8);

                        QTableWidgetItem *itemlon = new QTableWidgetItem();
                        itemlon->setFlags(itemlon->flags() ^ Qt::ItemIsEditable);
                        itemlon->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                        itemlon->setText(QString::number(lon.toDouble(),'f',8));
                        ui->tblmissing->setItem( cntr1 , 1 , itemlon );//longitude
                        missing << QString::number(lon.toDouble(),'f',8);

                        QDateTime dt = t1.addSecs(1);
                        QString sdate = dt.toString("yyyy.MM.dd.hh:mm:ss");
                        QStringList lstsdate = sdate.split(".");

                        for(int i=0; i<lstsdate.length();i++ ){
                            QTableWidgetItem *item = new QTableWidgetItem();
                            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                            item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                            item->setText(lstsdate[i].trimmed());
                            ui->tblmissing->setItem( cntr1 , i+2 , item );
                            missing << lstsdate[i].trimmed();
                        }

                        int j=6;
                        do{
                            QTableWidgetItem *item = new QTableWidgetItem();
                            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                            item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                            item->setText("*");
                            ui->tblmissing->setItem( cntr1 , j , item );
                            missing << "*";
                            j+=1;
                        }while(j < ui->tblmissing->columnCount());//flightrow.length());

                        missinglist.append(missing);
                        cntr1 += 1;

                    }else if(t1.secsTo(t2) > 2){
                        //qDebug() << "secto: " << t1.secsTo(t2);
                        int cntr = 1;
                        do{
                            missing.clear();
                            //qDebug() << "secto: " << cntr1 << t1.addSecs(cntr);
                            ui->tblmissing->insertRow(cntr1);
                            QStringList sval;
                            sval << QString::number(flightrow[0].toDouble(),'f',8)
                                    << QString::number(flightrow[1].toDouble(),'f',8)
                                    << QString::number(flightrow1[0].toDouble(),'f',8)
                                    << QString::number(flightrow1[1].toDouble(),'f',8)
                                    << QString::number(t1.secsTo(t2))
                                    << QString::number(cntr);

                            QVariant returnedval;
                            QMetaObject::invokeMethod(m_gps->wayHandler, "findCoor",
                                Q_RETURN_ARG(QVariant,returnedval),
                                Q_ARG(QVariant, sval));
                            QStringList splt = returnedval.toString().split(",");
                            QString lat = splt[0];
                            QString lon = splt[1];

                            QTableWidgetItem *itemlat = new QTableWidgetItem();
                            itemlat->setFlags(itemlat->flags() ^ Qt::ItemIsEditable);
                            itemlat->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                            itemlat->setText(QString::number(lat.toDouble(),'f',8));
                            ui->tblmissing->setItem( cntr1 , 0 , itemlat);//latitude
                            missing << QString::number(lat.toDouble(),'f',8);

                            QTableWidgetItem *itemlon = new QTableWidgetItem();
                            itemlon->setFlags(itemlon->flags() ^ Qt::ItemIsEditable);
                            itemlon->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                            itemlon->setText(QString::number(lon.toDouble(),'f',8));
                            ui->tblmissing->setItem( cntr1 , 1 , itemlon );//longitude
                            missing << QString::number(lon.toDouble(),'f',8);

                            QDateTime dt = t1.addSecs(cntr);
                            QString sdate = dt.toString("yyyy.MM.dd.hh:mm:ss");
                            QStringList lstsdate = sdate.split(".");

                            for(int i=0; i<lstsdate.length();i++ ){
                                QTableWidgetItem *item = new QTableWidgetItem();
                                item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                                item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                                item->setText(lstsdate[i].trimmed());
                                ui->tblmissing->setItem( cntr1 , i+2 , item );
                                missing << lstsdate[i].trimmed();
                            }

                            int j=6;
                            do{
                                QTableWidgetItem *item = new QTableWidgetItem();
                                item->setFlags(item->flags() ^ Qt::ItemIsEditable);
                                item->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                                item->setText("*");
                                ui->tblmissing->setItem( cntr1 , j , item);
                                missing << "*";
                                j+=1;
                            }while(j < ui->tblmissing->columnCount());//flightrow.length());

                            missinglist.append(missing);

                            cntr1 += 1;
                            cntr += 1;
                        }while(cntr < t1.secsTo(t2));
                    }
                }
            }
        }
        if(ui->tblmissing->rowCount() > 0)
            ui->btnSave->setEnabled(true);
        else
            ui->btnSave->setEnabled(false);
    }catch(QException &ex){
        //QMessageBox msg
        qDebug() << ex.what();
    }catch (...) {

    }
}

void interpolategps::saveGps(QString filename)
{
    try {
        QList<QStringList> newlist;
        if(ui->tblgps->rowCount() > 1 && ui->tblmissing->rowCount() > 1){
            for(int i=0;i< m_flightpath.length();i++){
                QStringList strlst;
                QStringList flightrow = m_flightpath[i].split(",");
                if(flightrow.length() >= 12){
                    QString strtime = flightrow[5].trimmed();
                    QStringList spltime = strtime.split(":");
                    QDateTime t1(
                            QDate(flightrow[2].trimmed().toInt(),
                                flightrow[3].trimmed().toInt(),
                                flightrow[4].trimmed().toInt()),
                            QTime(spltime[0].trimmed().toInt(),
                                spltime[1].trimmed().toInt(),
                                spltime[2].trimmed().toInt())
                            );

                    strlst << t1.toString();
                    for(int ii = 0; ii < flightrow.length(); ii++){
                        if(ii < ui->tblgps->columnCount())
                            strlst << flightrow[ii].trimmed();
                    }

                    newlist << strlst;
                }
            }

            for(int i=0;i<missinglist.length();i++){
                //qDebug() << "missinglist: " << missinglist[i];
                QStringList row = missinglist[i];
                QStringList strlst;
                for(int ii=0;ii<row.length();ii++){
                    strlst.clear();
                    QString strtime = row[5];
                    QStringList spltime = strtime.split(":");
                    QDateTime t1(
                            QDate(row[2].toInt(),
                                row[3].toInt(),
                                row[4].toInt()),
                            QTime(spltime[0].trimmed().toInt(),
                                spltime[1].trimmed().toInt(),
                                spltime[2].trimmed().toInt())
                            );
                    strlst << t1.toString();
                    for(int iii = 0; iii < row.length(); iii++){
                        if(iii < ui->tblgps->columnCount())
                            strlst << row[iii].trimmed();
                    }
                }
                newlist << strlst;
            }

            /*for(int s=0;s < newlist.length();s++){
                qDebug() << newlist[s];
            }*/

        }else{
            MessageDialog msgBox;
            msgBox.setText("Something went wrong \n\rwhile saving the GPS file.");
            msgBox.setFontSize(13);
            msgBox.exec();
            return;
        }

        //qDebug() << filename;
        QFile data(filename);
        if (data.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&data);

            if(newlist.length() > 0){
                std::sort( newlist.begin(), newlist.end(),  lessThan );
            }

            // Print out the list data so we can see that it got sorted ok
            for( int i = 0; i < newlist.size(); i++ )
            {
                QStringList newrow = newlist[i];
                newrow.removeFirst();
                QString data = newrow.join( "," );
                //qDebug() << QString( "Item %1: %2" ).arg( i + 1 ).arg( data );

                out << data << "\n";
            }
        }
        MessageDialog msgBox;
        msgBox.setText("You have successfully saved the file.");
        msgBox.setFontSize(13);
        QFont font("Segoe UI");
        msgBox.setFont(font);
        msgBox.exec();
    } catch(...) {
        MessageDialog msgBox;
        msgBox.setText("Something went wrong \n\rwhile saving the GPS file.");
        msgBox.setFontSize(13);
        QFont font("Segoe UI");
        msgBox.setFont(font);
        msgBox.exec();
    }
}

void interpolategps::clearForm()
{
    ui->txtFile->setText("");
    ui->btnTest->setVisible(false);
    ui->btnInterpolate->setEnabled(false);
    ui->btnSave->setEnabled(false);
    ui->tblgps->setRowCount(0);
    ui->tblmissing->setRowCount(0);
    ui->btnLoad->setEnabled(true);
    ui->btnLoad->setFocus();

    m_flightpath.clear();
    m_flightFilePath.clear();
}

void interpolategps::on_btnLoad_pressed()
{
    QString defaultDir = QDir::homePath();

    GlobalSettings &globalSettings = GlobalSettings::getInstance();
    defaultDir = globalSettings.get("projectDirectory");
    QString sfile = QFileDialog::getOpenFileName(this, "Select GPS File",defaultDir,"GPS files (*.gps)");

    QFile openFile(sfile);
    if (!openFile.exists()) {
        qDebug() << "No FLIGHT.GPS file. Exiting. Import failed.";
        return;
    }
    if (openFile.open(QIODevice::ReadOnly)){
        clearForm();
        QTextStream stream(&openFile);

        while(!stream.atEnd()){
            QString currentFlightLine = stream.readLine();
            //QStringList column = currentFlightLine.split(",");
            m_flightpath << currentFlightLine;
            m_flightFilePath = sfile;
        }
        openFile.close();

        if(m_flightFilePath.length() > 0){
            ui->txtFile->setText(m_flightFilePath);
        }
        if(m_flightpath.length() > 0)
            populateFlightPath();

    }



    //qDebug() << sfile;
}

void interpolategps::on_btnTest_clicked()
{

}

void interpolategps::on_btnInterpolate_pressed()
{
    calculateMissingSec();
}

void interpolategps::on_btnSave_pressed()
{
    QString filename = QFileDialog::getSaveFileName(
            this,
            tr("Save Document"),
            QDir::currentPath(),
            tr("GPS (*.GPS)") );
    if( !filename.isNull() )
        saveGps(filename);

}

void interpolategps::on_btnclear_pressed()
{
    clearForm();
}
