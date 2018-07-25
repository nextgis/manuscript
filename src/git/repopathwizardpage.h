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

#ifndef MANUSCRIPT_REPOPATHWIZARDPAGE_H
#define MANUSCRIPT_REPOPATHWIZARDPAGE_H

#include <QEventLoop>
#include <QProcess>
#include <QWizardPage>

namespace Ui {
class MSRepoPathWizardPage;
}

class MSRepoPathWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit MSRepoPathWizardPage(QWidget *parent = nullptr);
    ~MSRepoPathWizardPage() override;

private slots:
    void on_selectFolder_clicked();
    void onStdOutReady();
    void onStdErrReady();
    void finished(int exitCode, QProcess::ExitStatus e);
    void error(QProcess::ProcessError e);

private:
    Ui::MSRepoPathWizardPage *ui;
    QProcess m_process;
    QEventLoop m_eventLoop;
    bool m_execFailed;

    // QWizardPage interface
public:
    virtual bool validatePage() override;
    virtual bool isComplete() const override;
    virtual int nextId() const override;
};

#endif // MANUSCRIPT_REPOPATHWIZARDPAGE_H
