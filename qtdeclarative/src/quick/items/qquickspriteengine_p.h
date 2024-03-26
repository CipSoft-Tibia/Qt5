// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSPRITEENGINE_P_H
#define QQUICKSPRITEENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_sprite);

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QElapsedTimer>
#include <QList>
#include <QQmlListProperty>
#include <QImage>
#include <QPair>
#include <QRandomGenerator>
#include <private/qquickpixmapcache_p.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickSprite;
class Q_QUICK_PRIVATE_EXPORT QQuickStochasticState : public QObject //Currently for internal use only - Sprite and ParticleGroup
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged FINAL)
    Q_PROPERTY(int durationVariation READ durationVariation WRITE setDurationVariation NOTIFY durationVariationChanged FINAL)
    //Note that manually advanced sprites need to query this variable and implement own behaviour for it
    Q_PROPERTY(bool randomStart READ randomStart WRITE setRandomStart NOTIFY randomStartChanged FINAL)
    Q_PROPERTY(QVariantMap to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)

public:
    QQuickStochasticState(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    int duration() const
    {
        return m_duration;
    }

    QString name() const
    {
        return m_name;
    }

    QVariantMap to() const
    {
        return m_to;
    }

    int durationVariation() const
    {
        return m_durationVariation;
    }


    virtual int variedDuration() const
    {
        return qMax(0.0 , m_duration
                + (m_durationVariation * QRandomGenerator::global()->bounded(2.0))
                - m_durationVariation);
    }

    bool randomStart() const
    {
        return m_randomStart;
    }

Q_SIGNALS:
    void durationChanged(int arg);

    void nameChanged(const QString &arg);

    void toChanged(const QVariantMap &arg);

    void durationVariationChanged(int arg);

    void entered();//### Just playing around - don't expect full state API

    void randomStartChanged(bool arg);

public Q_SLOTS:
    void setDuration(int arg)
    {
        if (m_duration != arg) {
            m_duration = arg;
            Q_EMIT durationChanged(arg);
        }
    }

    void setName(const QString &arg)
    {
        if (m_name != arg) {
            m_name = arg;
            Q_EMIT nameChanged(arg);
        }
    }

    void setTo(const QVariantMap &arg)
    {
        if (m_to != arg) {
            m_to = arg;
            Q_EMIT toChanged(arg);
        }
    }

    void setDurationVariation(int arg)
    {
        if (m_durationVariation != arg) {
            m_durationVariation = arg;
            Q_EMIT durationVariationChanged(arg);
        }
    }

    void setRandomStart(bool arg)
    {
        if (m_randomStart != arg) {
            m_randomStart = arg;
            Q_EMIT randomStartChanged(arg);
        }
    }

private:
    QString m_name;
    QVariantMap m_to;
    int m_duration = -1;
    int m_durationVariation = 0;

    friend class QQuickStochasticEngine;
    bool m_randomStart = false;
};

class Q_QUICK_PRIVATE_EXPORT QQuickStochasticEngine : public QObject
{
    Q_OBJECT
    //TODO: Optimize single state case?
    Q_PROPERTY(QString globalGoal READ globalGoal WRITE setGlobalGoal NOTIFY globalGoalChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickStochasticState> states READ states FINAL)
public:
    explicit QQuickStochasticEngine(QObject *parent = nullptr);
    QQuickStochasticEngine(const QList<QQuickStochasticState*> &states, QObject *parent = nullptr);
    ~QQuickStochasticEngine() override;

    QQmlListProperty<QQuickStochasticState> states()
    {
        return QQmlListProperty<QQuickStochasticState>(this, &m_states);
    }

    QString globalGoal() const
    {
        return m_globalGoal;
    }

    int count() const {return m_things.size();}
    void setCount(int c);

    void setGoal(int state, int sprite=0, bool jump=false);
    void start(int index=0, int state=0);
    virtual void restart(int index=0);
    virtual void advance(int index=0);//Sends state to the next chosen state, unlike goal.
    void stop(int index=0);
    int curState(int index=0) const {return m_things[index];}

    QQuickStochasticState* state(int idx) const {return m_states[idx];}
    int stateIndex(QQuickStochasticState* s) const {return m_states.indexOf(s);}
    int stateIndex(const QString& s) const {
        for (int i=0; i<m_states.size(); i++)
            if (m_states[i]->name() == s)
                return i;
        return -1;
    }

    int stateCount() {return m_states.size();}
private:
Q_SIGNALS:

    void globalGoalChanged(const QString &arg);
    void stateChanged(int idx);

public Q_SLOTS:
    void setGlobalGoal(const QString &arg)
    {
        if (m_globalGoal != arg) {
            m_globalGoal = arg;
            Q_EMIT globalGoalChanged(arg);
        }
    }

    uint updateSprites(uint time);

protected:
    friend class QQuickParticleSystem;
    void addToUpdateList(uint t, int idx);
    int nextState(int curState, int idx=0);
    int goalSeek(int curState, int idx, int dist=-1);
    QList<QQuickStochasticState*> m_states;
    //### Consider struct or class for the four data variables?
    QVector<int> m_things;//int is the index in m_states of the current state
    QVector<int> m_goals;
    QVector<int> m_duration;
    QVector<int> m_startTimes;
    QVector<QPair<uint, QVector<int> > > m_stateUpdates;//### This could be done faster - priority queue?

    QElapsedTimer m_advanceTimer;
    uint m_timeOffset;
    QString m_globalGoal;
    int m_maxFrames;
    int m_imageStateCount;
    bool m_addAdvance;
};

class Q_QUICK_PRIVATE_EXPORT QQuickSpriteEngine : public QQuickStochasticEngine
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuickSprite> sprites READ sprites FINAL)
public:
    explicit QQuickSpriteEngine(QObject *parent = nullptr);
    QQuickSpriteEngine(const QList<QQuickSprite*> &sprites, QObject *parent = nullptr);
    ~QQuickSpriteEngine() override;
    QQmlListProperty<QQuickSprite> sprites()
    {
        return QQmlListProperty<QQuickSprite>(this, &m_sprites);
    }

    QQuickSprite* sprite(int sprite = 0) const;
    int spriteState(int sprite = 0) const;
    int spriteStart(int sprite = 0) const;
    int spriteFrames(int sprite = 0) const;
    int spriteDuration(int sprite = 0) const;
    int spriteX(int sprite = 0) const;
    int spriteY(int sprite = 0) const;
    int spriteWidth(int sprite = 0) const;
    int spriteHeight(int sprite = 0) const;
    int spriteCount() const;//Like state count
    int maxFrames() const;

    void restart(int index=0) override;
    void advance(int index=0) override;

    //Similar API to QQuickPixmap for async loading convenience
    bool isNull() const { return status() == QQuickPixmap::Null; }
    bool isReady() const { return status() == QQuickPixmap::Ready; }
    bool isLoading() const { return status() == QQuickPixmap::Loading; }
    bool isError() const { return status() == QQuickPixmap::Error; }
    QQuickPixmap::Status status() const; //Composed status of all Sprites
    void startAssemblingImage();
    QImage assembledImage(int maxSize = 2048);

private:
    int pseudospriteProgress(int, int, int *rd = nullptr) const;
    QList<QQuickSprite*> m_sprites;
    bool m_startedImageAssembly;
    bool m_loaded;
    bool m_errorsPrinted;
};

//Common use is to have your own list property which is transparently an engine
inline void spriteAppend(QQmlListProperty<QQuickSprite> *p, QQuickSprite* s)
{
    reinterpret_cast<QList<QQuickSprite *> *>(p->data)->append(s);
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

inline QQuickSprite* spriteAt(QQmlListProperty<QQuickSprite> *p, qsizetype idx)
{
    return reinterpret_cast<QList<QQuickSprite *> *>(p->data)->at(idx);
}

inline void spriteClear(QQmlListProperty<QQuickSprite> *p)
{
    reinterpret_cast<QList<QQuickSprite *> *>(p->data)->clear();
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

inline qsizetype spriteCount(QQmlListProperty<QQuickSprite> *p)
{
    return reinterpret_cast<QList<QQuickSprite *> *>(p->data)->size();
}

inline void spriteReplace(QQmlListProperty<QQuickSprite> *p, qsizetype idx, QQuickSprite *s)
{
    reinterpret_cast<QList<QQuickSprite *> *>(p->data)->replace(idx, s);
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

inline void spriteRemoveLast(QQmlListProperty<QQuickSprite> *p)
{
    reinterpret_cast<QList<QQuickSprite *> *>(p->data)->removeLast();
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

QT_END_NAMESPACE

#endif // QQUICKSPRITEENGINE_P_H
