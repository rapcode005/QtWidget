#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include "ui_aboutdialog.h"

class AboutDialog : public QDialog, public Ui::About {
    Q_OBJECT

public:
    AboutDialog(QWidget * parent = nullptr);
private slots:
    void on_buttonBox_3_clicked();
};

#endif // ABOUTDIALOG_H
