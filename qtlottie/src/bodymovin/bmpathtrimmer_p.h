// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef BMPATHTRIMMER_P_H
#define BMPATHTRIMMER_P_H

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

#include <QList>

#include <QtBodymovin/bmglobal.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;
class BMTrimPath;
class LottieRenderer;
class BMBase;
class BMShape;

class BODYMOVIN_EXPORT BMPathTrimmer
{
public:
    BMPathTrimmer(BMBase *root);

    void addTrim(BMTrimPath* trim);
    bool inUse() const;

    void applyTrim(BMShape *shape);

    void updateProperties(int frame);
    void render(LottieRenderer &renderer) const;

private:
    BMBase *m_root = nullptr;

    QList<BMTrimPath*> m_trimPaths;
    BMTrimPath *m_appliedTrim = nullptr;
};

QT_END_NAMESPACE

#endif // BMPATHTRIMMER_P_H

