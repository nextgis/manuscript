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

#include "bottomwidget.h"

MSBottomWidget::MSBottomWidget(QWidget *parent) : QTabWidget(parent),
    m_showButton(nullptr)
{
    m_outputLog = new MSOutputWidget;
    addTab(m_outputLog, tr("Output"));
}

void MSBottomWidget::setShowButton(QToolButton *button)
{
    m_showButton = button;
    if(isHidden()) {
        m_showButton->setChecked(false);
    }
    else {
        m_showButton->setChecked(true);
    }

    connect(m_showButton, &QAbstractButton::clicked, this,
            &MSBottomWidget::onToggle);
}

void MSBottomWidget::appendText(const QString &text,
                                MSOutputWidget::TextTypes format)
{
    m_outputLog->appendText(text, format);
}


void MSBottomWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if(m_showButton) {
        m_showButton->setChecked(true);
    }
}

void MSBottomWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if(m_showButton) {
        m_showButton->setChecked(false);
    }
}

void MSBottomWidget::onToggle()
{
    if(isHidden()) {
        show();
    }
    else {
        hide();
    }
}
