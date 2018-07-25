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

#ifndef MANUSCRIPT_BOTTOMWIDGET_H
#define MANUSCRIPT_BOTTOMWIDGET_H

#include <QTabWidget>
#include <QToolButton>

#include <logs/outputwidget.h>

class MSBottomWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit MSBottomWidget(QWidget *parent = nullptr);
    void setShowButton(QToolButton *button);
    void appendText(const QString &text, enum MSOutputWidget::TextTypes format);

signals:

public slots:

    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event) override;
    virtual void hideEvent(QHideEvent *event) override;

private:
    void onToggle();

private:
    QToolButton *m_showButton;
    MSOutputWidget *m_outputLog;
};

#endif // MANUSCRIPT_BOTTOMWIDGET_H
