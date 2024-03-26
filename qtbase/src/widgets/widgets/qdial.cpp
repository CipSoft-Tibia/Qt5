// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdial.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qcolor.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpolygon.h>
#include <qregion.h>
#include <qstyle.h>
#include <qstylepainter.h>
#include <qstyleoption.h>
#include <qslider.h>
#include <private/qabstractslider_p.h>
#include <private/qmath_p.h>
#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif
#include <qmath.h>

QT_BEGIN_NAMESPACE

class QDialPrivate : public QAbstractSliderPrivate
{
    Q_DECLARE_PUBLIC(QDial)
public:
    QDialPrivate()
    {
        wrapping = false;
        tracking = true;
        doNotEmit = false;
        target = qreal(3.7);
    }

    qreal target;
    uint showNotches : 1;
    uint wrapping : 1;
    uint doNotEmit : 1;

    int valueFromPoint(const QPoint &) const;
    double angle(const QPoint &, const QPoint &) const;
    void init();
    virtual int bound(int val) const override;
};

void QDialPrivate::init()
{
    Q_Q(QDial);
    showNotches = false;
    q->setFocusPolicy(Qt::WheelFocus);
}

int QDialPrivate::bound(int val) const
{
    if (wrapping) {
        if ((val >= minimum) && (val <= maximum))
            return val;
        if (minimum == maximum)
            return minimum;
        val = minimum + ((val - minimum) % (maximum - minimum));
        if (val < minimum)
            val += maximum - minimum;
        return val;
    } else {
        return QAbstractSliderPrivate::bound(val);
    }
}

/*!
    Initialize \a option with the values from this QDial. This method
    is useful for subclasses when they need a QStyleOptionSlider, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QDial::initStyleOption(QStyleOptionSlider *option) const
{
    if (!option)
        return;

    Q_D(const QDial);
    option->initFrom(this);
    option->minimum = d->minimum;
    option->maximum = d->maximum;
    option->sliderPosition = d->position;
    option->sliderValue = d->value;
    option->singleStep = d->singleStep;
    option->pageStep = d->pageStep;
    option->upsideDown = !d->invertedAppearance;
    option->notchTarget = d->target;
    option->dialWrapping = d->wrapping;
    option->subControls = QStyle::SC_All;
    option->activeSubControls = QStyle::SC_None;
    if (!d->showNotches) {
        option->subControls &= ~QStyle::SC_DialTickmarks;
        option->tickPosition = QSlider::TicksAbove;
    } else {
        option->tickPosition = QSlider::NoTicks;
    }
    option->tickInterval = notchSize();
}

int QDialPrivate::valueFromPoint(const QPoint &p) const
{
    Q_Q(const QDial);
    double yy = q->height()/2.0 - p.y();
    double xx = p.x() - q->width()/2.0;
    double a = (xx || yy) ? std::atan2(yy, xx) : 0;

    if (a < Q_PI / -2)
        a = a + Q_PI * 2;

    int dist = 0;
    int minv = minimum, maxv = maximum;

    if (minimum < 0) {
        dist = -minimum;
        minv = 0;
        maxv = maximum + dist;
    }

    int r = maxv - minv;
    int v;
    if (wrapping)
        v =  (int)(0.5 + minv + r * (Q_PI * 3 / 2 - a) / (2 * Q_PI));
    else
        v =  (int)(0.5 + minv + r* (Q_PI * 4 / 3 - a) / (Q_PI * 10 / 6));

    if (dist > 0)
        v -= dist;

    return !invertedAppearance ? bound(v) : maximum - bound(v);
}

/*!
    \class QDial

    \brief The QDial class provides a rounded range control (like a speedometer or potentiometer).

    \ingroup basicwidgets
    \inmodule QtWidgets

    \image windows-dial.png

    QDial is used when the user needs to control a value within a
    program-definable range, and the range either wraps around
    (for example, with angles measured from 0 to 359 degrees) or the
    dialog layout needs a square widget.

    Since QDial inherits from QAbstractSlider, the dial behaves in
    a similar way to a \l{QSlider}{slider}. When wrapping() is false
    (the default setting) there is no real difference between a slider
    and a dial. They both share the same signals, slots and member
    functions. Which one you use depends on the expectations of
    your users and on the type of application.

    The dial initially emits valueChanged() signals continuously while
    the slider is being moved; you can make it emit the signal less
    often by disabling the \l{QAbstractSlider::tracking} {tracking}
    property. The sliderMoved() signal is emitted continuously even
    when tracking is disabled.

    The dial also emits sliderPressed() and sliderReleased() signals
    when the mouse button is pressed and released. Note that the
    dial's value can change without these signals being emitted since
    the keyboard and wheel can also be used to change the value.

    Unlike the slider, QDial attempts to draw a "nice" number of
    notches rather than one per line step. If possible, the number of
    notches drawn is one per line step, but if there aren't enough pixels
    to draw every one, QDial will skip notches to try and draw a uniform
    set (e.g. by drawing every second or third notch).

    Like the slider, the dial makes the QAbstractSlider function setValue()
    available as a slot.

    The dial's keyboard interface is fairly simple: The
    \uicontrol{left}/\uicontrol{up} and \uicontrol{right}/\uicontrol{down} arrow keys adjust
    the dial's \l {QAbstractSlider::value} {value} by the defined
    \l {QAbstractSlider::singleStep} {singleStep}, \uicontrol{Page Up} and
    \uicontrol{Page Down} by the defined \l {QAbstractSlider::pageStep}
    {pageStep}, and the \uicontrol Home and \uicontrol End keys set the value to
    the defined \l {QAbstractSlider::minimum} {minimum} and
    \l {QAbstractSlider::maximum} {maximum} values.

    If you are using the mouse wheel to adjust the dial, the increment
    value is determined by the lesser value of
    \l{QApplication::wheelScrollLines()} {wheelScrollLines} multiplied
    by \l {QAbstractSlider::singleStep} {singleStep}, and
    \l {QAbstractSlider::pageStep} {pageStep}.

    \sa QScrollBar, QSpinBox, QSlider, {Sliders Example}
*/

/*!
    Constructs a dial.

    The \a parent argument is sent to the QAbstractSlider constructor.
*/
QDial::QDial(QWidget *parent)
    : QAbstractSlider(*new QDialPrivate, parent)
{
    Q_D(QDial);
    d->init();
}

/*!
    Destroys the dial.
*/
QDial::~QDial()
{
}

/*! \reimp */
void QDial::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
}

/*!
  \reimp
*/

void QDial::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionSlider option;
    initStyleOption(&option);
    p.drawComplexControl(QStyle::CC_Dial, option);
}

/*!
  \reimp
*/

void QDial::mousePressEvent(QMouseEvent *e)
{
    Q_D(QDial);
    if (d->maximum == d->minimum ||
        (e->button() != Qt::LeftButton)  ||
        (e->buttons() ^ e->button())) {
        e->ignore();
        return;
    }
    e->accept();
    setSliderPosition(d->valueFromPoint(e->position().toPoint()));
    // ### This isn't quite right,
    // we should be doing a hit test and only setting this if it's
    // the actual dial thingie (similar to what QSlider does), but we have no
    // subControls for QDial.
    setSliderDown(true);
}


/*!
  \reimp
*/

void QDial::mouseReleaseEvent(QMouseEvent * e)
{
    Q_D(QDial);
    if (e->buttons() & (~e->button()) ||
       (e->button() != Qt::LeftButton)) {
        e->ignore();
        return;
    }
    e->accept();
    setValue(d->valueFromPoint(e->position().toPoint()));
    setSliderDown(false);
}


/*!
  \reimp
*/

void QDial::mouseMoveEvent(QMouseEvent * e)
{
    Q_D(QDial);
    if (!(e->buttons() & Qt::LeftButton)) {
        e->ignore();
        return;
    }
    e->accept();
    d->doNotEmit = true;
    setSliderPosition(d->valueFromPoint(e->position().toPoint()));
    d->doNotEmit = false;
}


/*!
    \reimp
*/

void QDial::sliderChange(SliderChange change)
{
    QAbstractSlider::sliderChange(change);
}

void QDial::setWrapping(bool enable)
{
    Q_D(QDial);
    if (d->wrapping == enable)
        return;
    d->wrapping = enable;
    update();
}


/*!
    \property QDial::wrapping
    \brief whether wrapping is enabled

    If true, wrapping is enabled; otherwise some space is inserted at the bottom
    of the dial to separate the ends of the range of valid values.

    If enabled, the arrow can be oriented at any angle on the dial. If disabled,
    the arrow will be restricted to the upper part of the dial; if it is rotated
    into the space at the bottom of the dial, it will be clamped to the closest
    end of the valid range of values.

    By default this property is \c false.
*/

bool QDial::wrapping() const
{
    Q_D(const QDial);
    return d->wrapping;
}


/*!
    \property QDial::notchSize
    \brief the current notch size

    The notch size is in range control units, not pixels, and is
    calculated to be a multiple of singleStep() that results in an
    on-screen notch size near notchTarget().

    \sa notchTarget(), singleStep()
*/

int QDial::notchSize() const
{
    Q_D(const QDial);
    // radius of the arc
    qreal r = qMin(width(), height())/2.0;
    // length of the whole arc
    int l = qRound(r * (d->wrapping ? 6.0 : 5.0) * Q_PI / 6.0);
    // length of the arc from minValue() to minValue()+pageStep()
    if (d->maximum > d->minimum + d->pageStep)
        l = qRound(l * d->pageStep / double(d->maximum - d->minimum));
    // length of a singleStep arc
    l = qMax(l * d->singleStep / (d->pageStep ? d->pageStep : 1), 1);
    // how many times singleStep can be draw in d->target pixels
    l = qMax(qRound(d->target / l), 1);
    // we want notchSize() to be a non-zero multiple of singleStep()
    return d->singleStep * l;
}

void QDial::setNotchTarget(double target)
{
    Q_D(QDial);
    d->target = target;
    update();
}

/*!
    \property QDial::notchTarget
    \brief the target number of pixels between notches

    The notch target is the number of pixels QDial attempts to put
    between each notch.

    The actual size may differ from the target size.

    The default notch target is 3.7 pixels.
*/
qreal QDial::notchTarget() const
{
    Q_D(const QDial);
    return d->target;
}


void QDial::setNotchesVisible(bool visible)
{
    Q_D(QDial);
    d->showNotches = visible;
    update();
}

/*!
    \property QDial::notchesVisible
    \brief whether the notches are shown

    If the property is \c true, a series of notches are drawn around the dial
    to indicate the range of values available; otherwise no notches are
    shown.

    By default, this property is disabled.
*/
bool QDial::notchesVisible() const
{
    Q_D(const QDial);
    return d->showNotches;
}

/*!
  \reimp
*/

QSize QDial::minimumSizeHint() const
{
    return QSize(50, 50);
}

/*!
  \reimp
*/

QSize QDial::sizeHint() const
{
    return QSize(100, 100);
}

/*!
  \reimp
*/
bool QDial::event(QEvent *e)
{
    return QAbstractSlider::event(e);
}

QT_END_NAMESPACE

#include "moc_qdial.cpp"
