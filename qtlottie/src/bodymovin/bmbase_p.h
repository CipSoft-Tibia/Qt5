// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMBASE_P_H
#define BMBASE_P_H

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

#include <QJsonObject>
#include <QList>
#include <QVersionNumber>

#include <QtBodymovin/bmglobal.h>
#include <QtBodymovin/private/bmconstants_p.h>

#include <QtBodymovin/private/lottierenderer_p.h>

QT_BEGIN_NAMESPACE
class BODYMOVIN_EXPORT BMBase
{
public:
    BMBase() = default;
    explicit BMBase(const BMBase &other);
    virtual ~BMBase();

    virtual BMBase *clone() const;

    virtual bool setProperty(BMLiteral::PropertyType propertyType, QVariant value);

    QString name() const;
    void setName(const QString &name);

    int type() const;
    void setType(int type);
    virtual void parse(const QJsonObject &definition);

    const QJsonObject& definition() const;

    virtual bool active(int frame) const;
    bool hidden() const;

    inline BMBase *parent() const { return m_parent; }
    void setParent(BMBase *parent);

    const QList<BMBase *> &children() const { return m_children; }
    void prependChild(BMBase *child);
    void appendChild(BMBase *child);

    virtual BMBase *findChild(const QString &childName);

    virtual void updateProperties(int frame);
    virtual void render(LottieRenderer &renderer) const;

protected:
    void resolveTopRoot();
    BMBase *topRoot() const;
    const QJsonObject resolveExpression(const QJsonObject& definition);

protected:
    QJsonObject m_definition;
    int m_type;
    bool m_hidden = false;
    QVersionNumber m_version;
    QString m_name;
    QString m_matchName;
    bool m_autoOrient = false;

    friend class BMRasterRenderer;
    friend class BMRenderer;

private:
    BMBase *m_parent = nullptr;
    QList<BMBase *> m_children;

    // Handle to the topmost element on which this element resides
    // Will be resolved when traversing effects
    BMBase *m_topRoot = nullptr;
};

QT_END_NAMESPACE

#endif // BMBASE_P_H
