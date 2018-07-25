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

#include "finishwizardpage.h"
#include "importfromgitwizard.h"
#include "ui_finishwizardpage.h"

#include <QDir>
#include <QProcess>

MSFinishWizardPage::MSFinishWizardPage(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::MSFinishWizardPage)
{
    ui->setupUi(this);

    setTitle(tr("Importing finished successfully"));

    setFinalPage(true);
}

MSFinishWizardPage::~MSFinishWizardPage()
{
    delete ui;
}

bool MSFinishWizardPage::validatePage()
{
    QMap<QString, QVariant> currentBranchMap =
            wizard()->property("currentBranches").toMap();
    QString repoPath = field("clonePath").toString();
    for(const QString &repo : currentBranchMap.keys()) {
        QString workingDirectory;
        if(repo.compare(MAIN_REPO) == 0) {
            workingDirectory = repoPath;
        }
        else {
            workingDirectory = repoPath + QDir::separator() + repo;
        }

        QProcess cmd;
        cmd.setWorkingDirectory(workingDirectory);
        QStringList arguments;
        arguments << "checkout" << currentBranchMap[repo].toString();
        cmd.start("git", arguments, QIODevice::ReadOnly);
        cmd.waitForFinished();
    }
    return true;
}
