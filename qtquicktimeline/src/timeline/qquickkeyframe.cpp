// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquickkeyframe_p.h"

#include "qquicktimeline_p.h"
#include "qquickkeyframedatautils_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/QVariantAnimation>
#include <QtCore/qmath.h>
#include <QtGui/qpainter.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQml/QQmlProperty>
#include <QtQml/QQmlFile>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtCore/QFile>
#include <QtCore/QCborStreamReader>

#include <private/qvariantanimation_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

class QQuickKeyframeGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickKeyframeGroup)
public:
    QQuickKeyframeGroupPrivate() = default;

    QObject *target = nullptr;
    QString propertyName;
    QUrl keyframeSource;
    QByteArray keyframeData;
    bool componentComplete = false;
    int userType = -1;

protected:
    void setupKeyframes();
    void loadKeyframes(bool fromBinary = false);

    static void append_keyframe(QQmlListProperty<QQuickKeyframe> *list, QQuickKeyframe *a);
    static qsizetype keyframe_count(QQmlListProperty<QQuickKeyframe> *list);
    static QQuickKeyframe* keyframe_at(QQmlListProperty<QQuickKeyframe> *list, qsizetype pos);
    static void clear_keyframes(QQmlListProperty<QQuickKeyframe> *list);

    QList<QQuickKeyframe *> keyframes;
    QList<QQuickKeyframe *> sortedKeyframes;

    QVariant originalValue;
    QVariant lastValue;
};

void QQuickKeyframeGroupPrivate::setupKeyframes()
{
    sortedKeyframes = keyframes;
    std::sort(sortedKeyframes.begin(), sortedKeyframes.end(), [](const QQuickKeyframe *first, const QQuickKeyframe *second) {
        return first->frame() < second->frame();
    });
}

void QQuickKeyframeGroupPrivate::loadKeyframes(bool fromBinary)
{
    Q_Q(QQuickKeyframeGroup);

    QCborStreamReader reader;
    QFile dataFile;
    if (!fromBinary) {
        // Resolve URL similar to QQuickImage source
        QUrl loadUrl = keyframeSource;
        QQmlContext *context = qmlContext(q);
        if (context)
            loadUrl = context->resolvedUrl(keyframeSource);
        QString dataFilePath = QQmlFile::urlToLocalFileOrQrc(loadUrl);

        dataFile.setFileName(dataFilePath);
        if (!dataFile.open(QIODevice::ReadOnly)) {
            // Invalid file
            qWarning() << "Unable to open keyframeSource:" << dataFilePath;
            qDeleteAll(keyframes);
            keyframes.clear();
            return;
        }
        reader.setDevice(&dataFile);
    } else {
        reader.addData(keyframeData);
    }

    // Check that file is standard keyframes CBOR and get the version
    int version = readKeyframesHeader(reader);

    if (version == -1) {
        // Invalid file
        qWarning() << "Invalid keyframeSource version:" << version;
        return;
    }

    QMetaType::Type propertyType = QMetaType::UnknownType;
    if (reader.isInteger()) {
        propertyType = static_cast<QMetaType::Type>(reader.toInteger());
        reader.next();
    }

    // Start keyframes array
    reader.enterContainer();

    while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
        auto keyframe = new QQuickKeyframe(q);
        keyframe->setFrame(readReal(reader));
        keyframe->setEasing(QEasingCurve(static_cast<QEasingCurve::Type>(reader.toInteger())));
        reader.next();
        keyframe->setValue(readValue(reader, propertyType));
        keyframes.append(keyframe);
    }
    // Leave keyframes array
    reader.leaveContainer();

    // Leave root array
    reader.leaveContainer();

}

void QQuickKeyframeGroupPrivate::append_keyframe(QQmlListProperty<QQuickKeyframe> *list, QQuickKeyframe *a)
{
    auto q = static_cast<QQuickKeyframeGroup *>(list->object);
    q->d_func()->keyframes.append(a);
    q->d_func()->setupKeyframes();
    q->reset();
}

qsizetype QQuickKeyframeGroupPrivate::keyframe_count(QQmlListProperty<QQuickKeyframe> *list)
{
    auto q = static_cast<QQuickKeyframeGroup *>(list->object);
    return q->d_func()->keyframes.size();
}

QQuickKeyframe* QQuickKeyframeGroupPrivate::keyframe_at(QQmlListProperty<QQuickKeyframe> *list, qsizetype pos)
{
    auto q = static_cast<QQuickKeyframeGroup *>(list->object);
    return q->d_func()->keyframes.at(pos);
}

void QQuickKeyframeGroupPrivate::clear_keyframes(QQmlListProperty<QQuickKeyframe> *list)
{
    auto q = static_cast<QQuickKeyframeGroup *>(list->object);
    q->d_func()->keyframes.clear();
    q->d_func()->setupKeyframes();
}

class QQuickKeyframePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickKeyframe)
public:
    QQuickKeyframePrivate() = default;

    qreal frame = 0;
    QEasingCurve easingCurve;
    QVariant value;
};

/*!
    \qmltype Keyframe
    \inherits QtObject
    \instantiates QQuickKeyframe
    \inqmlmodule QtQuick.Timeline
    \ingroup qtqmltypes

    \brief A keyframe.

    The value of a keyframe on a timeline.

    An easing curve can be attached to the keyframe.
*/

/*!
    \qmlproperty double Keyframe::frame

    The position of the keyframe on the timeline.
*/

/*!
    \qmlproperty var Keyframe::easing

    The easing curve attached to the keyframe.
*/

/*!
    \qmlproperty var Keyframe::value

    The value of the keyframe.
*/

/*!
    \qmlsignal Keyframe::easingCurveChanged

    This signal is emitted when the easing curve attached to the keyframe
    changes.
*/

QQuickKeyframe::QQuickKeyframe(QObject *parent)
    : QObject(*(new QQuickKeyframePrivate), parent)
{
}

qreal QQuickKeyframe::frame() const
{
    Q_D(const QQuickKeyframe);
    return d->frame;
}

void QQuickKeyframe::setFrame(qreal f)
{
    Q_D(QQuickKeyframe);
    if (d->frame == f)
        return;
    d->frame = f;

    reset();

    emit frameChanged();
}

void QQuickKeyframe::reset()
{
    auto keyframes = qobject_cast<QQuickKeyframeGroup*>(parent());
    if (keyframes)
        keyframes->reset();
}

QQuickKeyframe::QQuickKeyframe(QQuickKeyframePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    \qmltype KeyframeGroup
    \inherits QtObject
    \instantiates QQuickKeyframeGroup
    \inqmlmodule QtQuick.Timeline
    \ingroup qtqmltypes

    \brief A keyframe group.

    A keyframe group contains all keyframes for a specific property of an item
    and always belongs to a timeline.
*/

/*!
    \qmlproperty var KeyframeGroup::target

    The item that is targeted by the keyframe group.
*/

/*!
    \qmlproperty string KeyframeGroup::property

    The property that is targeted by the keyframe group.
*/

/*!
    \qmlproperty list KeyframeGroup::keyframes
    \readonly

    A list of keyframes that belong to the keyframe group.
*/

/*!
    \qmlproperty url KeyframeGroup::keyframeSource

    The URL to a file containing binary keyframe data.

    \note KeyframeGroup should either set this property or contain
    Keyframe child elements. Using both can lead to an undefined behavior.

    \since 1.1
*/

QQuickKeyframeGroup::QQuickKeyframeGroup(QObject *parent)
    : QObject(*(new QQuickKeyframeGroupPrivate), parent)
{

}

QQmlListProperty<QQuickKeyframe> QQuickKeyframeGroup::keyframes()
{
    Q_D(QQuickKeyframeGroup);

    return { this, &d->keyframes, QQuickKeyframeGroupPrivate::append_keyframe,
                QQuickKeyframeGroupPrivate::keyframe_count,
                QQuickKeyframeGroupPrivate::keyframe_at,
                QQuickKeyframeGroupPrivate::clear_keyframes };
}

QObject *QQuickKeyframeGroup::target() const
{
    Q_D(const QQuickKeyframeGroup);
    return d->target;
}

void QQuickKeyframeGroup::setTargetObject(QObject *o)
{
    Q_D(QQuickKeyframeGroup);
    if (d->target == o)
        return;
    d->target = o;

    if (!property().isEmpty())
        init();

    emit targetChanged();
}

QString QQuickKeyframeGroup::property() const
{
    Q_D(const QQuickKeyframeGroup);
    return d->propertyName;
}

void QQuickKeyframeGroup::setProperty(const QString &n)
{
    Q_D(QQuickKeyframeGroup);
    if (d->propertyName == n)
        return;
    d->propertyName = n;

    if (target())
        init();

    emit propertyChanged();
}

QUrl QQuickKeyframeGroup::keyframeSource() const
{
    Q_D(const QQuickKeyframeGroup);
    return d->keyframeSource;
}

void QQuickKeyframeGroup::setKeyframeSource(const QUrl &source)
{
    Q_D(QQuickKeyframeGroup);
    if (d->keyframeSource == source)
        return;

    if (d->keyframes.size() > 0) {
        // Remove possible previously loaded keyframes
        qDeleteAll(d->keyframes);
        d->keyframes.clear();
        d->keyframeData.clear();
    }

    d->keyframeSource = source;
    d->loadKeyframes();
    d->setupKeyframes();
    reset();

    emit keyframeSourceChanged();
}

const QByteArray QQuickKeyframeGroup::keyframeData() const
{
    Q_D(const QQuickKeyframeGroup);
    return d->keyframeData;
}

void QQuickKeyframeGroup::setKeyframeData(const QByteArray &data)
{
    Q_D(QQuickKeyframeGroup);
    if (d->keyframeData == data)
        return;

    if (d->keyframes.size() > 0) {
        // Remove possible previously loaded keyframes
        qDeleteAll(d->keyframes);
        d->keyframes.clear();
        d->keyframeSource.clear();
    }

    d->keyframeData = data;
    d->loadKeyframes(true);
    d->setupKeyframes();
    reset();

    emit keyframeSourceChanged();
}

QVariant QQuickKeyframeGroup::evaluate(qreal frame) const
{
    Q_D(const QQuickKeyframeGroup);

    if (d->sortedKeyframes.isEmpty())
        return QVariant();

    static QQuickKeyframe dummy;
    auto timeline = qobject_cast<QQuickTimeline*>(parent());
    if (timeline)
        dummy.setFrame(timeline->startFrame() - 0.0001);
    dummy.setValue(d->originalValue);

     QQuickKeyframe *lastFrame = &dummy;

    for (auto keyFrame :  std::as_const(d->sortedKeyframes)) {
        if (qFuzzyCompare(frame, keyFrame->frame()) || frame < keyFrame->frame())
            return keyFrame->evaluate(lastFrame, frame, d->userType);
        lastFrame = keyFrame;
    }

    return lastFrame->value();
}

void QQuickKeyframeGroup::setProperty(qreal frame)
{
    Q_D(QQuickKeyframeGroup);

    if (target()) {
        QQmlProperty qmlProperty(target(), property());

        d->lastValue = evaluate(frame);

        if (!qmlProperty.write(d->lastValue))
            qWarning() << "Cannot set property" << property();
    }
}

void QQuickKeyframeGroup::init()
{
    Q_D(QQuickKeyframeGroup);
    if (target()) {
        d->originalValue = QQmlProperty::read(target(), property());
        d->userType = QQmlProperty(target(), property()).property().userType();
        if (property().contains(QLatin1Char('.'))) {
            if (d->userType == QMetaType::QVector2D
                    || d->userType == QMetaType::QVector3D
                    || d->userType == QMetaType::QVector4D
                    || d->userType == QMetaType::QQuaternion)
                d->userType = QMetaType::Double;
        }
    }
}

void QQuickKeyframeGroup::resetDefaultValue()
{
    Q_D(QQuickKeyframeGroup);

    if (QQmlProperty::read(target(), property()) == d->lastValue)
        QQmlProperty::write(target(), property(), d->originalValue);
}

void QQuickKeyframeGroup::reset()
{
    Q_D(QQuickKeyframeGroup);
    if (!d->componentComplete)
        return;

    auto *timeline = qobject_cast<QQuickTimeline*>(parent());
    if (timeline)
        timeline->reevaluate();
}

void QQuickKeyframeGroup::setupKeyframes()
{
    Q_D(QQuickKeyframeGroup);

    if (d->componentComplete)
        d->setupKeyframes();
}

void QQuickKeyframeGroup::classBegin()
{
    Q_D(QQuickKeyframeGroup);
    d->componentComplete = false;
}

void QQuickKeyframeGroup::componentComplete()
{
    Q_D(QQuickKeyframeGroup);
    d->componentComplete = true;
    setupKeyframes();
}

QEasingCurve QQuickKeyframe::easing() const
{
    Q_D(const QQuickKeyframe);
    return d->easingCurve;
}

void QQuickKeyframe::setEasing(const QEasingCurve &e)
{
    Q_D(QQuickKeyframe);
    if (d->easingCurve == e)
        return;

    d->easingCurve = e;

    reset();

    emit easingCurveChanged();
}

QVariant QQuickKeyframe::value() const
{
    Q_D(const QQuickKeyframe);
    return d->value;
}

void QQuickKeyframe::setValue(const QVariant &v)
{
    Q_D(QQuickKeyframe);
    if (d->value == v)
        return;
    d->value = v;

    reset();

    emit valueChanged();
}

QVariant QQuickKeyframe::evaluate(QQuickKeyframe *pre, qreal frametime, int userType) const
{
    QVariantAnimation::Interpolator interpolator = QVariantAnimationPrivate::getInterpolator(userType);
    if (!pre)
        return value();

    QVariant preValue = pre->value();
    qreal preFrame = pre->frame();

    qreal duration = frame() - preFrame;
    qreal offset = frametime - preFrame;

    qreal progress = easing().valueForProgress(offset / duration);

    const QMetaType targetType(userType);
    preValue.convert(targetType);
    QVariant convertedValue = value();
    convertedValue.convert(targetType);

    if (!interpolator) {
        if (progress < 1.0)
            return preValue;

        return convertedValue;
    }

    if (preValue.isValid() && convertedValue.isValid())
        return interpolator(preValue.constData(), convertedValue.constData(), progress);

    qWarning() << "invalid keyframe target" << preValue << convertedValue << userType;

    return QVariant();
}

QT_END_NAMESPACE


