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

#ifndef MANUSCRIPT_TOOLTIP_H
#define MANUSCRIPT_TOOLTIP_H

#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QTimer>
#include <QWidget>

/**
 * @brief The QTipLabel class
 */
class QTipLabel : public QLabel
{
    Q_OBJECT
public:
    QTipLabel(QWidget *parent);

    virtual void setContent(const QVariant &content) = 0;
    virtual bool isInteractive() const { return false; }
    virtual int showTime() const = 0;
    virtual void configure(const QPoint &pos, QWidget *w) = 0;
    virtual bool canHandleContentReplacement(int typeId) const = 0;
    virtual bool equals(int typeId, const QVariant &other, const QString &helpId) const = 0;
    virtual void setHelpId(const QString &id);
    virtual QString helpId() const;

private:
    QString m_helpId;
};

/**
 * @brief The TextTip class
 */
class TextTip : public QTipLabel
{
public:
    TextTip(QWidget *parent);

    virtual void setContent(const QVariant &content);
    virtual bool isInteractive() const;
    virtual void configure(const QPoint &pos, QWidget *w);
    virtual bool canHandleContentReplacement(int typeId) const;
    virtual int showTime() const;
    virtual bool equals(int typeId, const QVariant &other, const QString &otherHelpId) const;
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

private:
    QString m_text;
};

/**
 * @brief The ColorTip class
 */
class ColorTip : public QTipLabel
{
public:
    ColorTip(QWidget *parent);

    virtual void setContent(const QVariant &content);
    virtual void configure(const QPoint &pos, QWidget *w);
    virtual bool canHandleContentReplacement(int typeId) const;
    virtual int showTime() const { return 4000; }
    virtual bool equals(int typeId, const QVariant &other, const QString &otherHelpId) const;
    virtual void paintEvent(QPaintEvent *event);

private:
    QColor m_color;
    QPixmap m_tilePixmap;
};

/**
 * @brief The WidgetTip class
 */
class WidgetTip : public QTipLabel
{
    Q_OBJECT

public:
    explicit WidgetTip(QWidget *parent = nullptr);
    void pinToolTipWidget(QWidget *parent);

    virtual void setContent(const QVariant &content);
    virtual void configure(const QPoint &pos, QWidget *w);
    virtual bool canHandleContentReplacement(int typeId) const;
    virtual int showTime() const { return 30000; }
    virtual bool equals(int typeId, const QVariant &other, const QString &otherHelpId) const;
    virtual bool isInteractive() const { return true; }

private:
    QWidget *m_widget;
    QVBoxLayout *m_layout;
};

/**
 * @brief The FakeToolTip class
 */
class FakeToolTip : public QWidget
{
    Q_OBJECT

public:
    explicit FakeToolTip(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);
};

/**
 * @brief The ToolTip class
 */
class ToolTip : public QObject
{
    Q_OBJECT
protected:
    ToolTip();

public:
    ~ToolTip();

    enum {
        ColorContent = 0,
        TextContent = 1,
        WidgetContent = 42
    };

    bool eventFilter(QObject *o, QEvent *event);

    static ToolTip *instance();

    static void show(const QPoint &pos, const QString &content, QWidget *w = nullptr,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void show(const QPoint &pos, const QColor &color, QWidget *w = nullptr,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void show(const QPoint &pos, QWidget *content, QWidget *w = nullptr,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void show(const QPoint &pos, QLayout *content, QWidget *w = nullptr,
                     const QString &helpId = QString(), const QRect &rect = QRect());
    static void move(const QPoint &pos, QWidget *w);
    static void hide();
    static void hideImmediately();
    static bool isVisible();

    static QPoint offsetFromPosition();

    // Helper to 'pin' (show as real window) a tooltip shown
    // using WidgetContent
    static bool pinToolTip(QWidget *w, QWidget *parent);

    static QString contextHelpId();

signals:
    void shown();
    void hidden();

private:
    void showInternal(const QPoint &pos, const QVariant &content, int typeId, QWidget *w,
                      const QString &helpId, const QRect &rect);
    void hideTipImmediately();
    bool acceptShow(const QVariant &content, int typeId, const QPoint &pos, QWidget *w,
                    const QString &helpId, const QRect &rect);
    void setUp(const QPoint &pos, QWidget *w, const QRect &rect);
    bool tipChanged(const QPoint &pos, const QVariant &content, int typeId, QWidget *w,
                    const QString &helpId) const;
    void setTipRect(QWidget *w, const QRect &rect);
    void placeTip(const QPoint &pos, QWidget *w);
    void showTip();
    void hideTipWithDelay();

    QPointer<QTipLabel> m_tip;
    QWidget *m_widget;
    QRect m_rect;
    QTimer m_showTimer;
    QTimer m_hideDelayTimer;
    QString m_helpId;
};

#endif // MANUSCRIPT_TOOLTIP_H
