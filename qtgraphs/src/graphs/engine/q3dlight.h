// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef Q3DLIGHT_H
#define Q3DLIGHT_H

#include <QtGraphs/q3dobject.h>

QT_BEGIN_NAMESPACE

class Q3DLightPrivate;

class Q_GRAPHS_EXPORT Q3DLight : public Q3DObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DLight)
    Q_PROPERTY(bool autoPosition READ isAutoPosition WRITE setAutoPosition NOTIFY autoPositionChanged)

public:
    explicit Q3DLight(QObject *parent = nullptr);
    virtual ~Q3DLight();

    void setAutoPosition(bool enabled);
    bool isAutoPosition();

Q_SIGNALS:
    void autoPositionChanged(bool autoPosition);

private:
    Q_DISABLE_COPY(Q3DLight)

    friend class Q3DScenePrivate;
};

QT_END_NAMESPACE

#endif
