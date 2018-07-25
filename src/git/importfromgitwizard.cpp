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
#include "finishwizardpage.h"
#include "importfromgitwizard.h"
#include "repopathwizardpage.h"
#include "submoduleswizardpage.h"

#include <QDir>
#include <QIcon>
#include <QVariant>

MSImportFromGitWizard::MSImportFromGitWizard(QWidget *parent) : QWizard(parent)
{
    QPixmap pixmap = QIcon(":/icons/git-import.svg").pixmap(QSize(48, 48));
    setPixmap(QWizard::LogoPixmap, pixmap);
    setWindowTitle(tr("Project import wizard"));

    setPage(PAGE_INPUT_REPO, new MSRepoPathWizardPage);
    setPage(PAGE_FINISH, new MSFinishWizardPage);
    setPage(PAGE_SELECT_BRANCHES, new MSBranchesWizardPage);
    setPage(PAGE_INIT_SUBMODULES, new MSSubmodulesWizardPage);

    setStartId(PAGE_INPUT_REPO);
}

QString MSImportFromGitWizard::projectPath() const
{
    QString clonePath = field("clonePath").toString();
    clonePath += QDir::separator() + QLatin1String("source") + QDir::separator() +
            QLatin1String("conf.py");
    return clonePath;
}

QStringList MSImportFromGitWizard::toList(const QString &data)
{
    return data.split("\n", QString::SkipEmptyParts);
}
