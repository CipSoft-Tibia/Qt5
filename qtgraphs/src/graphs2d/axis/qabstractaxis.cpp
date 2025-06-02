// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGraphs/QAbstractAxis>
#include <private/qabstractaxis_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractAxis
    \inmodule QtGraphs
    \ingroup graphs_2D
    \brief The QAbstractAxis class is a base class used for specialized axis classes.

    Each series can be bound to only one horizontal and vertical axis.

    The properties and visibility of various axis elements, such as axis line, title, labels,
    and grid lines, can be individually controlled.
*/

/*!
    \qmltype AbstractAxis
    \nativetype QAbstractAxis
    \inqmlmodule QtGraphs
    \ingroup graphs_qml_2D

    \brief A base type used for specialized axis types.

    Each series can be bound to only one horizontal and vertical axis.

    The properties and visibility of various axis elements, such as axis line, title, labels,
    and grid lines, can be individually controlled.
*/

/*!
    \enum QAbstractAxis::AxisType

    This enum type specifies the type of the axis object.

    \value Value
    \value BarCategory
    \value DateTime
*/

/*!
    \fn void QAbstractAxis::type() const
    Returns the type of the axis.
*/

/*!
    \property QAbstractAxis::visible
    \brief The visibility of the axis.
    By default, the value is \c true.
*/
/*!
    \qmlproperty bool AbstractAxis::visible
    The visibility of the axis. By default, the value is \c true.
*/

/*!
    \property QAbstractAxis::lineVisible
    \brief The visibility of the axis line.
    By default, the value is \c true.
*/
/*!
    \qmlproperty bool AbstractAxis::lineVisible
    The visibility of the axis line. By default, the value is \c true.
*/

/*!
    \property QAbstractAxis::labelsVisible
    \brief Whether axis labels are visible.
    By default, the value is \c true.
*/
/*!
    \qmlproperty bool AbstractAxis::labelsVisible
    The visibility of axis labels. By default, the value is \c true.
*/

/*!
    \property QAbstractAxis::labelsAngle
    \brief The angle of the axis labels in degrees.
*/
/*!
    \qmlproperty real AbstractAxis::labelsAngle
    The angle of the axis labels in degrees.
*/

/*!
    \property QAbstractAxis::labelDelegate
    \brief A custom QML Component used as a label for the axis.
    This component should either be a \a Text component or contain "property string text",
    so that this property will be assigned the value of the label.
*/
/*!
    \qmlproperty Component AbstractAxis::labelDelegate
    A custom QML Component used as a label for the axis.
    This component should either be a \a Text component or contain "property string text",
    so that this property will be assigned the value of the label.
*/

/*!
    \property QAbstractAxis::gridVisible
    \brief The visibility of the grid lines.
    By default, the value is \c true.
*/
/*!
    \qmlproperty bool AbstractAxis::gridVisible
    The visibility of the grid lines. By default, the value is \c true.
*/

/*!
    \property QAbstractAxis::subGridVisible
    \brief The visibility of the subgrid lines.

    Applies only to axes that support subgrid lines.
    By default, the value is \c true.
*/
/*!
    \qmlproperty bool AbstractAxis::subGridVisible
    The visibility of the subgrid lines. Applies only to axes that support
    subgrid lines. By default, the value is \c true.
*/

/*!
    \property QAbstractAxis::titleText
    \brief The title of the axis.

    Empty by default. Axis titles support HTML formatting.
*/
/*!
    \qmlproperty string AbstractAxis::titleText
    The title of the axis. Empty by default. Axis titles support HTML formatting.
*/

/*!
    \property QAbstractAxis::titleColor
    \brief The color used to draw the title text.
*/
/*!
    \qmlproperty color AbstractAxis::titleColor
    The color used to draw the title text.
*/

/*!
    \property QAbstractAxis::titleVisible
    \brief The visibility of the axis title.

    By default, the value is \c true.
*/
/*!
    \qmlproperty bool AbstractAxis::titleVisible
    The visibility of the axis title. By default, the value is \c true.
*/

/*!
    \property QAbstractAxis::titleFont
    \brief The font of the title of the axis.
*/
/*!
    \qmlproperty font AbstractAxis::titleFont
    The font of the title of the axis.
*/

/*!
    \fn void QAbstractAxis::update()
    This signal is emitted when the axis needs to be updated.
*/

/*!
    \qmlsignal AbstractAxis::visibleChanged(bool visible)
    This signal is emitted when the visibility of the axis changes to \a visible.
*/

/*!
    \qmlsignal AbstractAxis::lineVisibleChanged(bool visible)
    This signal is emitted when the visibility of the axis line changes to \a visible.
*/

/*!
    \qmlsignal AbstractAxis::labelsVisibleChanged(bool visible)
    This signal is emitted when the visibility of the labels of the axis changes to \a visible.
*/

/*!
    \qmlsignal AbstractAxis::labelsAngleChanged(string angle)
    This signal is emitted when the angle of the axis labels changes to \a angle.
*/

/*!
    \qmlsignal AbstractAxis::gridVisibleChanged(bool visible)
    This signal is emitted when the visibility of the grid lines of the axis changes to \a visible.
*/

/*!
    \qmlsignal AbstractAxis::minorGridVisibleChanged(bool visible)
    This signal is emitted when the visibility of the minor grid lines of the axis
    changes to \a visible.
*/

/*!
    \qmlsignal AbstractAxis::titleTextChanged(string text)
    This signal is emitted when the text of the axis title changes to \a text.
*/

/*!
    \qmlsignal AbstractAxis::titleColorChanged(color color)
    This signal is emitted when the color used to draw the axis title changes to \a color.
*/

/*!
    \qmlsignal AbstractAxis::titleVisibleChanged(bool visible)
    This signal is emitted when the visibility of the title text of the axis changes to \a visible.
*/

/*!
    \qmlsignal AbstractAxis::titleFontChanged(Font font)
    This signal is emitted when the font of the axis title changes to \a font.
*/

/*!
    \qmlsignal AbstractAxis::rangeChanged(string min, string max)
    This signal is emitted when the axis range changes. \a min and \a max are
    the min and max of the new range.
*/

/*!
    \internal
    Constructs a new axis object that is a child of \a parent. The ownership is taken by
    graph when the axis is added.
*/

QAbstractAxis::QAbstractAxis(QAbstractAxisPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{}

/*!
  Destructs the axis object. When the axis is added to a graph, the graph object takes ownership.
*/

QAbstractAxis::~QAbstractAxis()
{
}

/*!
  Determines whether the axis line and tick marks are \a visible.
 */
void QAbstractAxis::setLineVisible(bool visible)
{
    Q_D(QAbstractAxis);
    if (d->m_lineVisible != visible) {
        d->m_lineVisible = visible;
        emit update();
        emit lineVisibleChanged(visible);
    }
}

bool QAbstractAxis::isLineVisible() const
{
    Q_D(const QAbstractAxis);
    return d->m_lineVisible;
}

void QAbstractAxis::setGridVisible(bool visible)
{
    Q_D(QAbstractAxis);
    if (d->m_gridVisible != visible) {
        d->m_gridVisible = visible;
        emit update();
        emit gridVisibleChanged(visible);
    }
}

bool QAbstractAxis::isGridVisible() const
{
    Q_D(const QAbstractAxis);
    return d->m_gridVisible;
}

void QAbstractAxis::setSubGridVisible(bool visible)
{
    Q_D(QAbstractAxis);
    if (d->m_subGridVisible != visible) {
        d->m_subGridVisible = visible;
        emit update();
        emit subGridVisibleChanged(visible);
    }
}

bool QAbstractAxis::isSubGridVisible() const
{
    Q_D(const QAbstractAxis);
    return d->m_subGridVisible;
}

void QAbstractAxis::setLabelsVisible(bool visible)
{
    Q_D(QAbstractAxis);
    if (d->m_labelsVisible != visible) {
        d->m_labelsVisible = visible;
        emit update();
        emit labelsVisibleChanged(visible);
    }
}

bool QAbstractAxis::labelsVisible() const
{
    Q_D(const QAbstractAxis);
    return d->m_labelsVisible;
}

void QAbstractAxis::setLabelsAngle(qreal angle)
{
    Q_D(QAbstractAxis);
    if (d->m_labelsAngle != angle) {
        d->m_labelsAngle = angle;
        emit update();
        emit labelsAngleChanged(angle);
    }
}

qreal QAbstractAxis::labelsAngle() const
{
    Q_D(const QAbstractAxis);
    return d->m_labelsAngle;
}

QQmlComponent *QAbstractAxis::labelDelegate() const
{
    Q_D(const QAbstractAxis);
    return d->m_labelDelegate;
}

void QAbstractAxis::setLabelDelegate(QQmlComponent *newLabelDelegate)
{
    Q_D(QAbstractAxis);
    if (d->m_labelDelegate == newLabelDelegate)
        return;
    d->m_labelDelegate = newLabelDelegate;
    emit labelDelegateChanged();
    emit update();
}

void QAbstractAxis::setTitleVisible(bool visible)
{
    Q_D(QAbstractAxis);
    if (d->m_titleVisible != visible) {
        d->m_titleVisible = visible;
        emit update();
        emit titleVisibleChanged(visible);
    }
}

bool QAbstractAxis::isTitleVisible() const
{
    Q_D(const QAbstractAxis);
    return d->m_titleVisible;
}

/*!
  Sets the color used to draw titles to \a color.
 */
void QAbstractAxis::setTitleColor(QColor color)
{
    Q_D(QAbstractAxis);
    if (d->m_titleColor != color) {
        d->m_titleColor = color;
        emit update();
        emit titleColorChanged(color);
    }
}

/*!
  Returns the color used to draw titles.
*/
QColor QAbstractAxis::titleColor() const
{
    Q_D(const QAbstractAxis);
    return d->m_titleColor;
}

/*!
  Sets the font used to draw titles to \a font.
*/
void QAbstractAxis::setTitleFont(const QFont &font)
{
    Q_D(QAbstractAxis);
    if (d->m_titleFont != font) {
        d->m_titleFont = font;
        emit update();
        emit titleFontChanged(font);
    }
}

/*!
  Returns the font used to draw titles.
*/
QFont QAbstractAxis::titleFont() const
{
    Q_D(const QAbstractAxis);
    return d->m_titleFont;
}

void QAbstractAxis::setTitleText(const QString &title)
{
    Q_D(QAbstractAxis);
    if (d->m_title != title) {
        d->m_title = title;
        emit update();
        emit titleTextChanged(title);
    }
}

QString QAbstractAxis::titleText() const
{
    Q_D(const QAbstractAxis);
    return d->m_title;
}

bool QAbstractAxis::isVisible() const
{
    Q_D(const QAbstractAxis);
    return d->m_visible;
}

/*!
  Sets the visibility of the axis, labels, and grid lines to \a visible.
*/
void QAbstractAxis::setVisible(bool visible)
{
    Q_D(QAbstractAxis);
    if (d->m_visible != visible) {
        d->m_visible = visible;
        emit update();
        emit visibleChanged(visible);
    }
}


/*!
  Makes the axis, labels, and grid lines visible.
*/
void QAbstractAxis::show()
{
    setVisible(true);
}

/*!
  Makes the axis, labels, and grid lines invisible.
*/
void QAbstractAxis::hide()
{
    setVisible(false);
}

/*!
  Sets the minimum value shown on the axis.
  Depending on the actual axis type, the \a min parameter is converted to the appropriate type
  of value. If the conversion is impossible, the function call does nothing.
*/
void QAbstractAxis::setMin(const QVariant &min)
{
    Q_D(QAbstractAxis);
    d->setMin(min);
}

/*!
  Sets the maximum value shown on the axis.
  Depending on the actual axis type, the \a max parameter is converted to the appropriate type
  of value. If the conversion is impossible, the function call does nothing.
*/
void QAbstractAxis::setMax(const QVariant &max)
{
    Q_D(QAbstractAxis);
    d->setMax(max);
}

/*!
  Sets the range shown on the axis.
  Depending on the actual axis type, the \a min and \a max parameters are converted to
  appropriate types of values. If the conversion is impossible, the function call does nothing.
*/
void QAbstractAxis::setRange(const QVariant &min, const QVariant &max)
{
    Q_D(QAbstractAxis);
    d->setRange(min, max);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QAbstractAxisPrivate::QAbstractAxisPrivate() {}

QAbstractAxisPrivate::~QAbstractAxisPrivate()
{
    Q_Q(QAbstractAxis);
    if (m_graph)
        m_graph->removeAxis(q);
}

void QAbstractAxisPrivate::handleRangeChanged(qreal min, qreal max)
{
    setRange(min,max);
}

QT_END_NAMESPACE


