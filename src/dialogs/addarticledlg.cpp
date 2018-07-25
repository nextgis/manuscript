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

#include "addarticledlg.h"
#include "ui_addarticledlg.h"

#include <QMessageBox>

#include "utils.h"

MSAddArticleDlg::MSAddArticleDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MSAddArticleDlg)
{
    ui->setupUi(this);
}

MSAddArticleDlg::~MSAddArticleDlg()
{
    delete ui;
}

QString MSAddArticleDlg::name() const
{
    return ui->articleName->text();
}

QString MSAddArticleDlg::fileName() const
{
    return ui->articleFileName->text();
}

QString MSAddArticleDlg::bookmark() const
{
    return ui->articleBookmark->text();
}

void MSAddArticleDlg::accept()
{
    if(name().isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("Article name is mandatory!"));
        return;
    }

    QString fileNameStr = fileName();
    if(fileNameStr.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("File name is mandatory!"));
        return;
    }

    if(!isAscii(fileNameStr)) {
        QMessageBox::critical(this, tr("Error"), tr("Only ASCII characters available in file name!"));
        return;
    }

    QDialog::accept();
}
