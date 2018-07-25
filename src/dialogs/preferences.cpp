#include "preferences.h"
#include "ui_preferences.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSettings>

MSPreferences::MSPreferences(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MSPreferences)
{
    ui->setupUi(this);

    // Init control values
    initMainTab();
    initGitTab();
}

MSPreferences::~MSPreferences()
{
    delete ui;
}

void MSPreferences::initMainTab()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("General"));
    ui->nameEdit->setText(settings.value("user").toString());
    ui->emailEdit->setText(settings.value("email").toString());
    settings.endGroup();

    settings.beginGroup(QLatin1String("Common"));
    QString themeName = settings.value("theme", QLatin1String("light")).toString();
    m_themeName = tr("Dark");
    if(themeName.compare("light") == 0)
        m_themeName = tr("Light");
    ui->theme->setCurrentText(m_themeName);
    settings.endGroup();
}

void MSPreferences::initGitTab()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("Git"));
    ui->gitName->setText(settings.value("user", ui->nameEdit->text()).toString());
    ui->gitEmail->setText(settings.value("email", ui->emailEdit->text()).toString());
    ui->gitLogin->setText(settings.value("login").toString());
    ui->gitPassword->setText(settings.value("password").toString());
    settings.endGroup();
}

void MSPreferences::applyMainTab()
{
    if(m_themeName.compare(ui->theme->currentText()) != 0) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("You mast restart application to apply new theme"));
    }

    QSettings settings;
    settings.beginGroup(QLatin1String("General"));
    settings.setValue("user", ui->nameEdit->text());
    settings.setValue("email", ui->emailEdit->text());

    settings.endGroup();

    settings.beginGroup(QLatin1String("Common"));
    QString themeName("dark");
    if(ui->theme->currentText().compare(tr("Light")) == 0)
        themeName = QLatin1String("light");
    settings.setValue("theme", themeName);
    settings.endGroup();
}

void MSPreferences::applyGitTab()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("Git"));
    settings.setValue("user", ui->gitName->text());
    settings.setValue("email", ui->gitEmail->text());
    settings.setValue("login", ui->gitLogin->text());
    settings.setValue("password", ui->gitPassword->text());
    settings.endGroup();
}


void MSPreferences::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole) {
        applyMainTab();
        applyGitTab();

        accept();
    }
}
