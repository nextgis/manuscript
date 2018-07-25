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

#include "navigationwidget.h"

MSNavigationWidget::MSNavigationWidget(QWidget *parent) :
    NGNavigationWidget(parent),
    m_showButton(nullptr)
{

}

void MSNavigationWidget::setShowButton(QToolButton *button)
{
    m_showButton = button;
    if(isHidden()) {
        m_showButton->setChecked(false);
    }
    else {
        m_showButton->setChecked(true);
    }

    connect(m_showButton, &QAbstractButton::clicked, this,
                &MSNavigationWidget::onToggle);
}


void MSNavigationWidget::showEvent(QShowEvent *event)
{
    NGNavigationWidget::showEvent(event);
    if(m_showButton) {
        m_showButton->setChecked(true);
    }
}

void MSNavigationWidget::hideEvent(QHideEvent *event)
{
    NGNavigationWidget::hideEvent(event);
    if(m_showButton) {
        m_showButton->setChecked(false);
    }
}

void MSNavigationWidget::onToggle()
{
    if(isHidden()) {
        show();
    }
    else {
        hide();
    }
}
