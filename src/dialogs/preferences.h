#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QAbstractButton>
#include <QDialog>

namespace Ui {
class MSPreferences;
}

class MSPreferences : public QDialog
{
    Q_OBJECT

public:
    explicit MSPreferences(QWidget *parent = nullptr);
    ~MSPreferences() override;

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::MSPreferences *ui;
    QString m_themeName;

private:
    void initMainTab();
    void initGitTab();
    void applyMainTab();
    void applyGitTab();
};

#endif // PREFERENCES_H
