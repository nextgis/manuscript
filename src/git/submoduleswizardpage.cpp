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
#include "submoduleswizardpage.h"
#include "ui_submoduleswizardpage.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

MSSubmodulesWizardPage::MSSubmodulesWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::MSSubmodulesWizardPage),
    m_currentSubmodule(0)
{
    ui->setupUi(this);

    setTitle(tr("Submodules to clone"));
    setSubTitle(tr("Select repository submodules to clone."));

    ui->progressBar->hide();

    connect(&m_process, &QProcess::readyReadStandardOutput,
                this, &MSSubmodulesWizardPage::onStdOutReady);
    connect(&m_process, &QProcess::readyReadStandardError,
                this, &MSSubmodulesWizardPage::onStdErrReady);
    connect(&m_process,
                static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                this, &MSSubmodulesWizardPage::finished);
    connect(&m_process, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &MSSubmodulesWizardPage::error);
    setCommitPage(true);
}

MSSubmodulesWizardPage::~MSSubmodulesWizardPage()
{
    delete ui;
}


void MSSubmodulesWizardPage::onStdOutReady()
{
}

void MSSubmodulesWizardPage::onStdErrReady()
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
        else if(str.startsWith("Получение объектов:")) {
            percent = str.mid(20, 3).trimmed();
            currentStep = 1;
        }
        else if(str.startsWith("Определение изменений:")) {
            percent = str.mid(23, 3).trimmed();
            currentStep = 2;
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

void MSSubmodulesWizardPage::finished(int exitCode, QProcess::ExitStatus e)
{
    Q_UNUSED(exitCode)
    m_execFailed = e != QProcess::NormalExit;
    m_eventLoop.quit();
}

void MSSubmodulesWizardPage::error(QProcess::ProcessError e)
{
    Q_UNUSED(e)
    m_execFailed = true;
    m_eventLoop.quit();
}

bool MSSubmodulesWizardPage::validatePage()
{
    m_execFailed = false;
    m_currentSubmodule = 0;


    QStringList submodules;
    for(int i = 0; i < ui->submodulesList->count(); ++i) {
        QListWidgetItem *item = ui->submodulesList->item(i);
        if(item->checkState()) {
            submodules << item->text();
        }
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(submodules.count() * 300);
    ui->progressBar->show();

    QMap<QString, QVariant> branchMap = wizard()->property("branches").toMap();

    QString repoPath = field("clonePath").toString();

    // In case the submodule has prefix 'git@github.com:' switch to https://github.com/
    // Replace : to / and git@ to https://

    struct RepositoryInfo {
        QString path;
        QString url;
    };

    QMap<QString, struct RepositoryInfo> repositoriesInfo;

    QFile file(repoPath + QDir::separator() + ".gitmodules");
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString submoduleName, submodulePath, submoduleUrl;
    int pos = -1;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if(line.startsWith("[submodule \"")) {
            submoduleName = line.mid(12);
            submoduleName = submoduleName.replace("\"]", "").trimmed();
        }
        else if((pos = line.indexOf("url = ")) != -1) {
            submoduleUrl = line.mid(pos + 6).trimmed();
            submoduleUrl.replace(':', '/');
            submoduleUrl.replace("git@", "https://");
        }
        else if((pos = line.indexOf("path = ")) != -1) {
            submodulePath = line.mid(pos + 7).trimmed();
        }

        if(!submoduleName.isEmpty() && !submodulePath.isEmpty() && !submoduleUrl.isEmpty()) {
            repositoriesInfo[submoduleName] = {submodulePath, submoduleUrl};
            submoduleName.clear();
            submodulePath.clear();
            submoduleUrl.clear();
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    for(const QString &submodule : submodules) {
        QStringList arguments;

//#ifdef Q_OS_WIN
        struct RepositoryInfo repoInfo = repositoriesInfo[submodule];

        arguments << "clone" << "--recurse-submodules" << "--progress" << repoInfo.url << QDir::toNativeSeparators(repoPath + QDir::separator() + repoInfo.path);

//#else
//        arguments << "submodule" << "update" << "--init" << "--recursive" << "--progress" << submodule;
//        m_process.setWorkingDirectory(repoPath);
//#endif
        m_process.start("git", arguments, QIODevice::ReadOnly);
        m_process.closeWriteChannel();

        m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);


        //    Check branches
        QProcess cmd;
        cmd.setWorkingDirectory(repoPath + QDir::separator() + submodule);

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

        if(branches.count() > 1) {
            branchMap[submodule] = branches;
        }
        else {
            QMap<QString, QVariant> currentBranchMap =
                    wizard()->property("currentBranches").toMap();
            // Expected have at least one branch
            currentBranchMap[submodule] = branches[0].mid(7);
            wizard()->setProperty("currentBranches", currentBranchMap);
        }

        m_currentSubmodule++;
        ui->progressBar->setValue( m_currentSubmodule * 300);
    }

    QApplication::restoreOverrideCursor();
    wizard()->setProperty("branches", branchMap);

    return true;
}


int MSSubmodulesWizardPage::nextId() const
{

//    Check branches
    QMap<QString, QVariant> branchMap = wizard()->property("branches").toMap();
    bool hasBranches = false;
    for(const QVariant &branchList : branchMap) {
        QStringList branches = branchList.toStringList();
        if(!branches.isEmpty()) {
            hasBranches = true;
            break;
        }
    }

    if(hasBranches) {
        return MSImportFromGitWizard::PAGE_SELECT_BRANCHES;
    }

//    If no branches - goto finish.
    return MSImportFromGitWizard::PAGE_FINISH;
}

void MSSubmodulesWizardPage::initializePage()
{
    ui->submodulesList->clear();
    QStringList submodules = wizard()->property("submodules").toStringList();
    for(const QString &submodule : submodules) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(submodule);
        item->setCheckState(Qt::Unchecked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        ui->submodulesList->addItem(item);
    }
}
