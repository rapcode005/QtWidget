#ifndef ADDITIONALFIELDSDIALOG_H
#define ADDITIONALFIELDSDIALOG_H

#include <QDialog>
#include <QList>
#include <QTableWidget>

namespace Ui {
class AdditionalFieldsDialog;
}

class AdditionalFieldsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdditionalFieldsDialog(QWidget *parent = nullptr);
    ~AdditionalFieldsDialog();
    QTableWidget *table;
private:
    Ui::AdditionalFieldsDialog *ui;
};

#endif // ADDITIONALFIELDSDIALOG_H
