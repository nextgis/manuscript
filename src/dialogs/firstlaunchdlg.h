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

#ifndef MANUSCRIPT_FIRSTLAUNCHDLG_H
#define MANUSCRIPT_FIRSTLAUNCHDLG_H

#include <QDialog>

namespace Ui {
class MSFirstLaunchDlg;
}

class MSFirstLaunchDlg : public QDialog
{
    Q_OBJECT

public:
    explicit MSFirstLaunchDlg(QWidget *parent = nullptr);
    ~MSFirstLaunchDlg();
    QString email() const;
    QString name() const;

private:
    Ui::MSFirstLaunchDlg *ui;
};

#endif // MANUSCRIPT_FIRSTLAUNCHDLG_H
