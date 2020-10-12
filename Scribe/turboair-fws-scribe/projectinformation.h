#ifndef PROJECTINFORMATION_H
#define PROJECTINFORMATION_H

#include <QDialog>

namespace Ui {
class ProjectInformation;
}

class ProjectInformation : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectInformation(QWidget *parent = nullptr);
    ~ProjectInformation();


private slots:
    void on_buttonBox_clicked();

private:
    Ui::ProjectInformation *ui;
};

#endif // PROJECTINFORMATION_H
