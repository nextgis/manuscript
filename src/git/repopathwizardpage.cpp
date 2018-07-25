/******************************************************************************
*  Project: Manuscript
*  Purpose: GUI interface to prepare Sphinx documentation.
*  Author:  Dmitry Baryshnikov, dmitry.baryshnikov@nextgis.com
*******************************************************************************
*  Copyright (C) 2018 NextGIS, <info@nextgis.com>
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "importfromgitwizard.h"
#include "repopathwizardpage.h"
#include "ui_repopathwizardpage.h"

#include <QFileDialog>
#include <QStandardPaths>

#include <QDebug>

MSRepoPathWizardPage::MSRepoPathWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::MSRepoPathWizardPage)
{
    ui->setupUi(this);

    setTitle(tr("Input repository to clone"));
    setSubTitle(tr("Input repository URL to clone and path in your target system."));

    ui->progressBar->hide();
    ui->progressEdit->hide();

    registerField("clonePath*", ui->pathEdit);
    registerField("cloneUrl*", ui->repoURLEdit);

    connect(&m_process, &QProcess::readyReadStandardOutput,
                this, &MSRepoPathWizardPage::onStdOutReady);
    connect(&m_process, &QProcess::readyReadStandardError,
                this, &MSRepoPathWizardPage::onStdErrReady);
    connect(&m_process,
                static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                this, &MSRepoPathWizardPage::finished);
    connect(&m_process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &MSRepoPathWizardPage::error);

    setCommitPage(true);
}

MSRepoPathWizardPage::~MSRepoPathWizardPage()
{
    delete ui;
}

void MSRepoPathWizardPage::on_selectFolder_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select folder to clone"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QFileDialog::ShowDirsOnly |
        QFileDialog::DontResolveSymlinks );
    ui->pathEdit->setText(dir);
}

void MSRepoPathWizardPage::onStdOutReady()
{
    const QByteArray newStdOut = m_process.readAllStandardOutput();
    if(!newStdOut.isEmpty()) {
        ui->progressEdit->appendPlainText(newStdOut);
    }
}

void MSRepoPathWizardPage::onStdErrReady()
{
    const QByteArray newStdErr = m_process.readAllStandardError();
    if (!newStdErr.isEmpty()) {
        QString str(newStdErr);
        if(str.startsWith("remote: Compressing objects:")) {
            QString percent = str.mid(29, 3).trimmed();
            int value = percent.toInt();
            ui->progressBar->setValue( value );
            if(value == 100) {
                ui->progressEdit->appendPlainText(str);
            }
        }
        else if(str.startsWith("Receiving objects:")) {
            QString percent = str.mid(19, 3).trimmed();
            int value = percent.toInt();
            ui->progressBar->setValue( value );
            if(value == 100) {
                ui->progressEdit->appendPlainText(str);
            }
        }
        else if(str.startsWith("Resolving deltas:")) {
            QString percent = str.mid(18, 3).trimmed();
            int value = percent.toInt();
            ui->progressBar->setValue( value );
            if(value == 100) {
                ui->progressEdit->appendPlainText(str);
            }
        }
        else if(str.startsWith("Checking out files:")) {
            QString percent = str.mid(20, 3).trimmed();
            int value = percent.toInt();
            ui->progressBar->setValue( value );
            if(value == 100) {
                ui->progressEdit->appendPlainText(str);
            }
        }
        else {
            ui->progressEdit->appendPlainText(str);
            qDebug() << str;
        }
    }
}

void MSRepoPathWizardPage::finished(int exitCode, QProcess::ExitStatus e)
{
    Q_UNUSED(exitCode)
    m_execFailed = e != QProcess::NormalExit;
    m_eventLoop.quit();
}

void MSRepoPathWizardPage::error(QProcess::ProcessError e)
{
    Q_UNUSED(e)
    m_execFailed = true;
    m_eventLoop.quit();
}

bool MSRepoPathWizardPage::validatePage()
{
    m_execFailed = false;
    ui->progressBar->setValue(0);
    ui->progressBar->show();
    ui->progressEdit->show();

    QStringList arguments;
    arguments << "clone" << "--progress" <<
                 ui->repoURLEdit->text() << ui->pathEdit->text();

    QString cmdStr("qit " + arguments.join(" ") + "\n");
    ui->progressEdit->appendPlainText(tr("Started") + ": " + cmdStr);


    m_process.start("git", arguments, QIODevice::ReadOnly);
    m_process.closeWriteChannel();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    ui->progressEdit->appendPlainText(tr("Finished") + ": " + cmdStr);
    QApplication::restoreOverrideCursor();

//    Check submodules
    ui->progressEdit->appendPlainText(tr("Check submodules"));
//    git submodule--helper list
    QProcess cmd;
    cmd.setWorkingDirectory(ui->pathEdit->text());
    cmd.start("git", QStringList() << "submodule--helper" << "list",
              QIODevice::ReadOnly);
    cmd.closeWriteChannel();
    cmd.waitForFinished();
    QStringList submodules = MSImportFromGitWizard::toList(cmd.readAll());
    // Remove everything before \t
    for (int i = 0; i < submodules.size(); ++i) {
        int index = submodules[i].indexOf('\t');
        if(index > -1) {
            submodules[i] = submodules[i].mid(index + 1);
        }
    }

    if(!submodules.isEmpty()) {
        wizard()->setProperty("submodules", submodules);
    }

//    If no submodules, check branches
    ui->progressEdit->appendPlainText(tr("Check branches"));
//    git branch -r
    cmd.start("git", QStringList() << "branch" << "-r",
              QIODevice::ReadOnly);
    cmd.closeWriteChannel();
    cmd.waitForFinished();
    QStringList branches = MSImportFromGitWizard::toList(cmd.readAll());

    branches.removeAt(0);
    // Trim whitespaces
    for (int i = 0; i < branches.size(); ++i) {
        branches[i] = branches[i].trimmed();
    }

    QMap<QString, QVariant> branchMap;
    branchMap[MAIN_REPO] = branches;
    wizard()->setProperty("branches", branchMap);

    return true;
}

bool MSRepoPathWizardPage::isComplete() const
{
    if(ui->repoURLEdit->text().isEmpty()) {
        return false;
    }

    QDir clonePath(ui->pathEdit->text());
    if(clonePath.entryInfoList(QDir::NoDotAndDotDot |
                               QDir::AllEntries).count() != 0) {
        return false;
    }

    return true;
}

int MSRepoPathWizardPage::nextId() const
{
//    Check submodules
    QStringList submodules = wizard()->property("submodules").toStringList();
    if(!submodules.isEmpty()) {
        return MSImportFromGitWizard::PAGE_INIT_SUBMODULES;
    }

//    If no submodules, check branches
    QMap<QString, QVariant> branchMap = wizard()->property("branches").toMap();
    QStringList branches = branchMap[MAIN_REPO].toStringList();
    if(branches.count() > 1) {
        return MSImportFromGitWizard::PAGE_SELECT_BRANCHES;
    }

    if(!branches.isEmpty()) {
        QMap<QString, QVariant> currentBranchMap;
        currentBranchMap[MAIN_REPO] = branches[0].mid(7);
        wizard()->setProperty("currentBranches", currentBranchMap);
    }

//    If no branches - goto finish.
    return MSImportFromGitWizard::PAGE_FINISH;
}
