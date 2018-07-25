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

#include "brancheswizardpage.h"
#include "importfromgitwizard.h"
#include "ui_brancheswizardpage.h"

#include <QComboBox>
#include <QDir>
#include <QProcess>
#include <QTreeWidgetItem>

MSBranchesWizardPage::MSBranchesWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::MSBranchesWizardPage)
{
    ui->setupUi(this);

    ui->progressBar->hide();

    connect(&m_process, &QProcess::readyReadStandardOutput,
                this, &MSBranchesWizardPage::onStdOutReady);
    connect(&m_process, &QProcess::readyReadStandardError,
                this, &MSBranchesWizardPage::onStdErrReady);
    connect(&m_process,
                static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                this, &MSBranchesWizardPage::finished);
    connect(&m_process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &MSBranchesWizardPage::error);
}

MSBranchesWizardPage::~MSBranchesWizardPage()
{
    delete ui;
}

bool MSBranchesWizardPage::validatePage()
{
    m_currentSubmodule = 0;
    int count = wizard()->property("branches").toMap().count();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(count * 300);
    ui->progressBar->show();
    int counter = 0;

    QMap<QString, QVariant> currentBranchMap =
            wizard()->property("currentBranches").toMap();

    QString repoPath = field("clonePath").toString();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QTreeWidgetItemIterator it(ui->branchesTree);
    while (*it) {
        QString repo = (*it)->text(0);
        QString workingDirectory;
        if(repo.compare(MAIN_REPO) == 0) {
            workingDirectory = repoPath;
        }
        else {
            workingDirectory = repoPath + QDir::separator() + repo;
        }

        QComboBox *comboBox = static_cast<QComboBox *>(ui->branchesTree->itemWidget(*it, 1));
        QString branch = comboBox->currentText();
        QString localBranch = branch.mid(7);
        currentBranchMap[repo] = localBranch;

//        QProcess cmd;
//        cmd.setWorkingDirectory(workingDirectory);
//        cmd.start("git", QStringList() << "branch", QIODevice::ReadOnly);
//        cmd.waitForFinished();
//        QString currentBanch = cmd.readAll().replace("* ", "");

//        if(currentBanch.compare(localBranch) != 0) {

            // git checkout -b experimental origin/experimental
            QStringList arguments;
            arguments << "checkout" << "--progress" << "-b" << localBranch << branch;
            m_process.setWorkingDirectory(workingDirectory);

            m_process.start("git", arguments, QIODevice::ReadOnly);
            m_process.closeWriteChannel();

            m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
//        }

        ui->progressBar->setValue(counter++);
        m_currentSubmodule++;
        ++it;
    }

    QApplication::restoreOverrideCursor();
    wizard()->setProperty("currentBranches", currentBranchMap);

    return true;
}

int MSBranchesWizardPage::nextId() const
{
    return MSImportFromGitWizard::PAGE_FINISH;
}

void MSBranchesWizardPage::initializePage()
{
    ui->branchesTree->clear();
    QMap<QString, QVariant> branchMap = wizard()->property("branches").toMap();
    for(const QString &repo : branchMap.keys()) {
        QStringList branches = branchMap[repo].toStringList();
        if(branches.count() > 1) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, repo);
            ui->branchesTree->addTopLevelItem(item);
            QComboBox *comboBox = new QComboBox(this);
            comboBox->addItems(branches);
            ui->branchesTree->setItemWidget(item, 1, comboBox);
        }
    }
}

void MSBranchesWizardPage::onStdOutReady()
{
}

void MSBranchesWizardPage::onStdErrReady()
{
    char currentStep = -1;
    const QByteArray newStdErr = m_process.readAllStandardError();
    if (!newStdErr.isEmpty()) {
        QString str(newStdErr);
        QString percent;
        if(str.startsWith("remote: Compressing objects:")) {
            percent = str.mid(29, 3).trimmed();
            currentStep = 0;
        }
        else if(str.startsWith("Receiving objects:")) {
            percent = str.mid(19, 3).trimmed();
            currentStep = 1;
        }
        else if(str.startsWith("Resolving deltas:")) {
            percent = str.mid(18, 3).trimmed();
            currentStep = 2;
        }
//        else if(str.startsWith("Checking out files:")) {
//            QString percent = str.mid(20, 3).trimmed();
//            int value = percent.toInt();
//            ui->progressBar->setValue( value );
//        }

        if(currentStep > -1) {
            int percentNum = percent.toInt();
            int percentTotal = m_currentSubmodule * 300 +
                    currentStep * 100 + percentNum;
            ui->progressBar->setValue( percentTotal );
        }
    }
}

void MSBranchesWizardPage::finished(int exitCode, QProcess::ExitStatus e)
{
    Q_UNUSED(exitCode)
    m_execFailed = e != QProcess::NormalExit;
    m_eventLoop.quit();
}

void MSBranchesWizardPage::error(QProcess::ProcessError e)
{
    Q_UNUSED(e)
    m_execFailed = true;
    m_eventLoop.quit();
}
