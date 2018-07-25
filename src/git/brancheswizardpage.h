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

#ifndef MANUSCRIPT_BRANCHESWIZARDPAGE_H
#define MANUSCRIPT_BRANCHESWIZARDPAGE_H

#include <QEventLoop>
#include <QProcess>
#include <QWizardPage>

namespace Ui {
class MSBranchesWizardPage;
}

class MSBranchesWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit MSBranchesWizardPage(QWidget *parent = nullptr);
    ~MSBranchesWizardPage() override;

private:
    Ui::MSBranchesWizardPage *ui;

    // QWizardPage interface
public:
    virtual bool validatePage() override;
    virtual int nextId() const override;
    virtual void initializePage() override;

private slots:
    void onStdOutReady();
    void onStdErrReady();
    void finished(int exitCode, QProcess::ExitStatus e);
    void error(QProcess::ProcessError e);

private:
    QProcess m_process;
    QEventLoop m_eventLoop;
    bool m_execFailed;
    short m_currentSubmodule;
};

#endif // MANUSCRIPT_BRANCHESWIZARDPAGE_H
