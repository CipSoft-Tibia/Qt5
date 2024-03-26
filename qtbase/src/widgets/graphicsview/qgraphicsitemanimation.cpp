// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*!
    \class QGraphicsItemAnimation
    \brief The QGraphicsItemAnimation class provides simple animation
    support for QGraphicsItem.
    \since 4.2
    \ingroup graphicsview-api
    \inmodule QtWidgets
    \deprecated

    The QGraphicsItemAnimation class animates a QGraphicsItem. You can
    schedule changes to the item's transformation matrix at
    specified steps. The QGraphicsItemAnimation class has a
    current step value. When this value changes the transformations
    scheduled at that step are performed. The current step of the
    animation is set with the \c setStep() function.

    QGraphicsItemAnimation will do a simple linear interpolation
    between the nearest adjacent scheduled changes to calculate the
    matrix. For instance, if you set the position of an item at values
    0.0 and 1.0, the animation will show the item moving in a straight
    line between these positions. The same is true for scaling and
    rotation.

    It is usual to use the class with a QTimeLine. The timeline's
    \l{QTimeLine::}{valueChanged()} signal is then connected to the
    \c setStep() slot. For example, you can set up an item for rotation
    by calling \c setRotationAt() for different step values.
    The animations timeline is set with the setTimeLine() function.

    An example animation with a timeline follows:

    \snippet timeline/main.cpp 0

    Note that steps lie between 0.0 and 1.0. It may be necessary to use
    \l{QTimeLine::}{setUpdateInterval()}. The default update interval
    is 40 ms. A scheduled transformation cannot be removed when set,
    so scheduling several transformations of the same kind (e.g.,
    rotations) at the same step is not recommended.

    \sa QTimeLine, {Graphics View Framework}
*/

#include "qgraphicsitemanimation.h"

#include "qgraphicsitem.h"

#include <QtCore/qtimeline.h>
#include <QtCore/qpoint.h>
#include <QtCore/qpointer.h>
#include <QtCore/qpair.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static inline bool check_step_valid(qreal step, const char *method)
{
    if (!(step >= 0 && step <= 1)) {
        qWarning("QGraphicsItemAnimation::%s: invalid step = %f", method, step);
        return false;
    }
    return true;
}

class QGraphicsItemAnimationPrivate
{
public:
    inline QGraphicsItemAnimationPrivate()
        : q(nullptr), timeLine(nullptr), item(nullptr), step(0)
    { }

    QGraphicsItemAnimation *q;

    QPointer<QTimeLine> timeLine;
    QGraphicsItem *item;

    QPointF startPos;
    QTransform startTransform;

    qreal step;

    struct Pair {
        bool operator <(const Pair &other) const
        { return step < other.step; }
        bool operator==(const Pair &other) const
        { return step == other.step; }
        qreal step;
        qreal value;
    };
    QList<Pair> xPosition;
    QList<Pair> yPosition;
    QList<Pair> rotation;
    QList<Pair> verticalScale;
    QList<Pair> horizontalScale;
    QList<Pair> verticalShear;
    QList<Pair> horizontalShear;
    QList<Pair> xTranslation;
    QList<Pair> yTranslation;

    qreal linearValueForStep(qreal step, const QList<Pair> &source, qreal defaultValue = 0);
    void insertUniquePair(qreal step, qreal value, QList<Pair> *binList, const char *method);
};
Q_DECLARE_TYPEINFO(QGraphicsItemAnimationPrivate::Pair, Q_PRIMITIVE_TYPE);

qreal QGraphicsItemAnimationPrivate::linearValueForStep(qreal step, const QList<Pair> &source,
                                                        qreal defaultValue)
{
    if (source.isEmpty())
        return defaultValue;
    step = qMin<qreal>(qMax<qreal>(step, 0), 1);

    if (step == 1)
        return source.back().value;

    qreal stepBefore = 0;
    qreal stepAfter = 1;
    qreal valueBefore = source.front().step == 0 ? source.front().value : defaultValue;
    qreal valueAfter = source.back().value;

    // Find the closest step and value before the given step.
    for (int i = 0; i < source.size() && step >= source[i].step; ++i) {
        stepBefore = source[i].step;
        valueBefore = source[i].value;
    }

    // Find the closest step and value after the given step.
    for (int i = source.size() - 1; i >= 0 && step < source[i].step; --i) {
        stepAfter = source[i].step;
        valueAfter = source[i].value;
    }

    // Do a simple linear interpolation.
    return valueBefore + (valueAfter - valueBefore) * ((step - stepBefore) / (stepAfter - stepBefore));
}

void QGraphicsItemAnimationPrivate::insertUniquePair(qreal step, qreal value, QList<Pair> *binList,
                                                     const char *method)
{
    if (!check_step_valid(step, method))
        return;

    const Pair pair = { step, value };

    const QList<Pair>::iterator result = std::lower_bound(binList->begin(), binList->end(), pair);
    if (result == binList->end() || pair < *result)
        binList->insert(result, pair);
    else
        result->value = value;
}

/*!
  Constructs an animation object with the given \a parent.
*/
QGraphicsItemAnimation::QGraphicsItemAnimation(QObject *parent)
    : QObject(parent), d(new QGraphicsItemAnimationPrivate)
{
    d->q = this;
}

/*!
  Destroys the animation object.
*/
QGraphicsItemAnimation::~QGraphicsItemAnimation()
{
    delete d;
}

/*!
  Returns the item on which the animation object operates.

  \sa setItem()
*/
QGraphicsItem *QGraphicsItemAnimation::item() const
{
    return d->item;
}

/*!
  Sets the specified \a item to be used in the animation.

  \sa item()
*/
void QGraphicsItemAnimation::setItem(QGraphicsItem *item)
{
    d->item = item;
    d->startPos = d->item->pos();
}

/*!
  Returns the timeline object used to control the rate at which the animation
  occurs.

  \sa setTimeLine()
*/
QTimeLine *QGraphicsItemAnimation::timeLine() const
{
    return d->timeLine;
}

/*!
  Sets the timeline object used to control the rate of animation to the \a timeLine
  specified.

  \sa timeLine()
*/
void QGraphicsItemAnimation::setTimeLine(QTimeLine *timeLine)
{
    if (d->timeLine == timeLine)
        return;
    if (d->timeLine)
        delete d->timeLine;
    if (!timeLine)
        return;
    d->timeLine = timeLine;
    connect(timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(setStep(qreal)));
}

/*!
  Returns the position of the item at the given \a step value.

  \sa setPosAt()
*/
QPointF QGraphicsItemAnimation::posAt(qreal step) const
{
    check_step_valid(step, "posAt");
    return QPointF(d->linearValueForStep(step, d->xPosition, d->startPos.x()),
                   d->linearValueForStep(step, d->yPosition, d->startPos.y()));
}

/*!
  \fn void QGraphicsItemAnimation::setPosAt(qreal step, const QPointF &point)

  Sets the position of the item at the given \a step value to the \a point specified.

  \sa posAt()
*/
void QGraphicsItemAnimation::setPosAt(qreal step, const QPointF &pos)
{
    d->insertUniquePair(step, pos.x(), &d->xPosition, "setPosAt");
    d->insertUniquePair(step, pos.y(), &d->yPosition, "setPosAt");
}

/*!
  Returns all explicitly inserted positions.

  \sa posAt(), setPosAt()
*/
QList<QPair<qreal, QPointF> > QGraphicsItemAnimation::posList() const
{
    QList<QPair<qreal, QPointF> > list;
    const int xPosCount = d->xPosition.size();
    list.reserve(xPosCount);
    for (int i = 0; i < xPosCount; ++i)
        list << QPair<qreal, QPointF>(d->xPosition.at(i).step, QPointF(d->xPosition.at(i).value, d->yPosition.at(i).value));

    return list;
}

/*!
  Returns the transform used for the item at the specified \a step value.

  \since 5.14
*/
QTransform QGraphicsItemAnimation::transformAt(qreal step) const
{
    check_step_valid(step, "transformAt");

    QTransform transform;
    if (!d->rotation.isEmpty())
        transform.rotate(rotationAt(step));
    if (!d->verticalScale.isEmpty())
        transform.scale(horizontalScaleAt(step), verticalScaleAt(step));
    if (!d->verticalShear.isEmpty())
        transform.shear(horizontalShearAt(step), verticalShearAt(step));
    if (!d->xTranslation.isEmpty())
        transform.translate(xTranslationAt(step), yTranslationAt(step));
    return transform;
}

/*!
  Returns the angle at which the item is rotated at the specified \a step value.

  \sa setRotationAt()
*/
qreal QGraphicsItemAnimation::rotationAt(qreal step) const
{
    check_step_valid(step, "rotationAt");
    return d->linearValueForStep(step, d->rotation);
}

/*!
  Sets the rotation of the item at the given \a step value to the \a angle specified.

  \sa rotationAt()
*/
void QGraphicsItemAnimation::setRotationAt(qreal step, qreal angle)
{
    d->insertUniquePair(step, angle, &d->rotation, "setRotationAt");
}

/*!
  Returns all explicitly inserted rotations.

  \sa rotationAt(), setRotationAt()
*/
QList<QPair<qreal, qreal> > QGraphicsItemAnimation::rotationList() const
{
    QList<QPair<qreal, qreal> > list;
    const int numRotations = d->rotation.size();
    list.reserve(numRotations);
    for (int i = 0; i < numRotations; ++i)
        list << QPair<qreal, qreal>(d->rotation.at(i).step, d->rotation.at(i).value);

    return list;
}

/*!
  Returns the horizontal translation of the item at the specified \a step value.

  \sa setTranslationAt()
*/
qreal QGraphicsItemAnimation::xTranslationAt(qreal step) const
{
    check_step_valid(step, "xTranslationAt");
    return d->linearValueForStep(step, d->xTranslation);
}

/*!
  Returns the vertical translation of the item at the specified \a step value.

  \sa setTranslationAt()
*/
qreal QGraphicsItemAnimation::yTranslationAt(qreal step) const
{
    check_step_valid(step, "yTranslationAt");
    return d->linearValueForStep(step, d->yTranslation);
}

/*!
  Sets the translation of the item at the given \a step value using the horizontal
  and vertical coordinates specified by \a dx and \a dy.

  \sa xTranslationAt(), yTranslationAt()
*/
void QGraphicsItemAnimation::setTranslationAt(qreal step, qreal dx, qreal dy)
{
    d->insertUniquePair(step, dx, &d->xTranslation, "setTranslationAt");
    d->insertUniquePair(step, dy, &d->yTranslation, "setTranslationAt");
}

/*!
  Returns all explicitly inserted translations.

  \sa xTranslationAt(), yTranslationAt(), setTranslationAt()
*/
QList<QPair<qreal, QPointF> > QGraphicsItemAnimation::translationList() const
{
    QList<QPair<qreal, QPointF> > list;
    const int numTranslations = d->xTranslation.size();
    list.reserve(numTranslations);
    for (int i = 0; i < numTranslations; ++i)
        list << QPair<qreal, QPointF>(d->xTranslation.at(i).step, QPointF(d->xTranslation.at(i).value, d->yTranslation.at(i).value));

    return list;
}

/*!
  Returns the vertical scale for the item at the specified \a step value.

  \sa setScaleAt()
*/
qreal QGraphicsItemAnimation::verticalScaleAt(qreal step) const
{
    check_step_valid(step, "verticalScaleAt");

    return d->linearValueForStep(step, d->verticalScale, 1);
}

/*!
  Returns the horizontal scale for the item at the specified \a step value.

  \sa setScaleAt()
*/
qreal QGraphicsItemAnimation::horizontalScaleAt(qreal step) const
{
    check_step_valid(step, "horizontalScaleAt");
    return d->linearValueForStep(step, d->horizontalScale, 1);
}

/*!
  Sets the scale of the item at the given \a step value using the horizontal and
  vertical scale factors specified by \a sx and \a sy.

  \sa verticalScaleAt(), horizontalScaleAt()
*/
void QGraphicsItemAnimation::setScaleAt(qreal step, qreal sx, qreal sy)
{
    d->insertUniquePair(step, sx, &d->horizontalScale, "setScaleAt");
    d->insertUniquePair(step, sy, &d->verticalScale, "setScaleAt");
}

/*!
  Returns all explicitly inserted scales.

  \sa verticalScaleAt(), horizontalScaleAt(), setScaleAt()
*/
QList<QPair<qreal, QPointF> > QGraphicsItemAnimation::scaleList() const
{
    QList<QPair<qreal, QPointF> > list;
    const int numScales = d->horizontalScale.size();
    list.reserve(numScales);
    for (int i = 0; i < numScales; ++i)
        list << QPair<qreal, QPointF>(d->horizontalScale.at(i).step, QPointF(d->horizontalScale.at(i).value, d->verticalScale.at(i).value));

    return list;
}

/*!
  Returns the vertical shear for the item at the specified \a step value.

  \sa setShearAt()
*/
qreal QGraphicsItemAnimation::verticalShearAt(qreal step) const
{
    check_step_valid(step, "verticalShearAt");
    return d->linearValueForStep(step, d->verticalShear, 0);
}

/*!
  Returns the horizontal shear for the item at the specified \a step value.

  \sa setShearAt()
*/
qreal QGraphicsItemAnimation::horizontalShearAt(qreal step) const
{
    check_step_valid(step, "horizontalShearAt");
    return d->linearValueForStep(step, d->horizontalShear, 0);
}

/*!
  Sets the shear of the item at the given \a step value using the horizontal and
  vertical shear factors specified by \a sh and \a sv.

  \sa verticalShearAt(), horizontalShearAt()
*/
void QGraphicsItemAnimation::setShearAt(qreal step, qreal sh, qreal sv)
{
    d->insertUniquePair(step, sh, &d->horizontalShear, "setShearAt");
    d->insertUniquePair(step, sv, &d->verticalShear, "setShearAt");
}

/*!
  Returns all explicitly inserted shears.

  \sa verticalShearAt(), horizontalShearAt(), setShearAt()
*/
QList<QPair<qreal, QPointF> > QGraphicsItemAnimation::shearList() const
{
    QList<QPair<qreal, QPointF> > list;
    const int numShears = d->horizontalShear.size();
    list.reserve(numShears);
    for (int i = 0; i < numShears; ++i)
        list << QPair<qreal, QPointF>(d->horizontalShear.at(i).step, QPointF(d->horizontalShear.at(i).value, d->verticalShear.at(i).value));

    return list;
}

/*!
  Clears the scheduled transformations used for the animation, but
  retains the item and timeline.
*/
void QGraphicsItemAnimation::clear()
{
    d->xPosition.clear();
    d->yPosition.clear();
    d->rotation.clear();
    d->verticalScale.clear();
    d->horizontalScale.clear();
    d->verticalShear.clear();
    d->horizontalShear.clear();
    d->xTranslation.clear();
    d->yTranslation.clear();
}

/*!
  \fn void QGraphicsItemAnimation::setStep(qreal step)

  Sets the current \a step value for the animation, causing the
  transformations scheduled at this step to be performed.
*/
void QGraphicsItemAnimation::setStep(qreal step)
{
    if (!check_step_valid(step, "setStep"))
        return;

    beforeAnimationStep(step);

    d->step = step;
    if (d->item) {
        if (!d->xPosition.isEmpty() || !d->yPosition.isEmpty())
            d->item->setPos(posAt(step));
        if (!d->rotation.isEmpty()
            || !d->verticalScale.isEmpty()
            || !d->horizontalScale.isEmpty()
            || !d->verticalShear.isEmpty()
            || !d->horizontalShear.isEmpty()
            || !d->xTranslation.isEmpty()
            || !d->yTranslation.isEmpty()) {
            d->item->setTransform(d->startTransform * transformAt(step));
        }
    }

    afterAnimationStep(step);
}

/*!
  \fn void QGraphicsItemAnimation::beforeAnimationStep(qreal step)

  This method is meant to be overridden by subclassed that needs to
  execute additional code before a new step takes place. The
  animation \a step is provided for use in cases where the action
  depends on its value.
*/
void QGraphicsItemAnimation::beforeAnimationStep(qreal step)
{
    Q_UNUSED(step);
}

/*!
  \fn void QGraphicsItemAnimation::afterAnimationStep(qreal step)

  This method is meant to be overridden in subclasses that need to
  execute additional code after a new step has taken place. The
  animation \a step is provided for use in cases where the action
  depends on its value.
*/
void QGraphicsItemAnimation::afterAnimationStep(qreal step)
{
    Q_UNUSED(step);
}

QT_END_NAMESPACE

#include "moc_qgraphicsitemanimation.cpp"
