#ifndef DEFAULTHOTKEYS_H
#define DEFAULTHOTKEYS_H

#include <QDialog>

namespace Ui {
class DefaultHotKeys;
}

class DefaultHotKeys : public QDialog
{
    Q_OBJECT

public:
    explicit DefaultHotKeys(QWidget *parent = nullptr);
    ~DefaultHotKeys();

    QStringList defaultUsedHotkeys;
    void initForm();

private:
    Ui::DefaultHotKeys *ui;
};

#endif // DEFAULTHOTKEYS_H
