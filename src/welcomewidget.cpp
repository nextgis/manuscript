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

#include "mainwindow.h"
#include "welcomewidget.h"

#include <QHBoxLayout>

MSWelcomeWidget::MSWelcomeWidget(QWidget *parent) :
    QWidget(parent)
{
    QAction *openCommand = MSMainWindow::instance()->commandByKey("file.open");
    QAction *importCommand = MSMainWindow::instance()->commandByKey("file.import");

   const QString placeholderText = tr("<html><body style=\"color:#909090; font-size:14px\">"
          "<div align='center'>"
          "<div style=\"font-size:20px\">Open a project</div>"
          "<table><tr><td>"
          "<hr/>"
          "<div style=\"margin-top: 5px\">&bull; File > Open Project (%1)</div>"
          "<div style=\"margin-top: 5px\">&bull; File > Import Project (%2)</div>"
          "<div style=\"margin-top: 5px\">&bull; File > Recent Files</div>"
          "</td></tr></table>"
          "</div>"
          "</body></html>")
         .arg(openCommand->shortcut().toString(QKeySequence::NativeText))
         .arg(importCommand->shortcut().toString(QKeySequence::NativeText));


   QLabel *label = new QLabel(this);
   QHBoxLayout *layout = new QHBoxLayout();
   label->setText(placeholderText);
   layout->addWidget(label);
   setLayout(layout);
}
