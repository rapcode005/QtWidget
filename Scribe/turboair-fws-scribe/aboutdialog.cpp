#include "aboutdialog.h"

AboutDialog::AboutDialog( QWidget * parent) : QDialog(parent) {
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setWindowIcon(QIcon(":/img/misc/scribeIconFlipped.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setupUi(this);

    // perform additional setup here ...
}

void AboutDialog::on_buttonBox_3_clicked()
{
    this->close();
}
