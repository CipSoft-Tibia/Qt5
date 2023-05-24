// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TRIMPATH_P_H
#define TRIMPATH_P_H

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

#include <QMetaType>
#include <QPainterPath>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class TrimPath {
public:
    TrimPath() = default;
    TrimPath(const QPainterPath &path)
        : mPath(path) {}
    TrimPath(const TrimPath &other)
        : mPath(other.mPath), mLens(other.mLens) {}
    ~TrimPath() {}

    void setPath(const QPainterPath &path) {
        mPath = path;
        mLens.clear();
    }

    QPainterPath path() const {
        return mPath;
    }

    QPainterPath trimmed(qreal f1, qreal f2, qreal offset = 0.0) const;

private:
    bool lensIsDirty() const {
        return mLens.size() != mPath.elementCount();
    }
    void updateLens() const;
    int elementAtLength(qreal len) const;
    QPointF endPointOfElement(int elemIdx) const;
    void appendTrimmedElement(QPainterPath *to, int elemIdx, bool trimStart, qreal startLen, bool trimEnd, qreal endLen) const;
    void appendStartOfElement(QPainterPath *to, int elemIdx, qreal len) const {
        appendTrimmedElement(to, elemIdx, false, 0.0, true, len);
    }
    void appendEndOfElement(QPainterPath *to, int elemIdx, qreal len) const {
        appendTrimmedElement(to, elemIdx, true, len, false, 1.0);
    }

    void appendElementRange(QPainterPath *to, int first, int last) const;

    QPainterPath mPath;
    mutable QList<qreal> mLens;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(TrimPath)

#endif // TRIMPATH_P_H
