// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSCXMLGLOBALS_P_H
#define QSCXMLGLOBALS_P_H

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

#include "qscxmlglobals.h"

#if defined(BUILD_QSCXMLC)
#  define Q_SCXML_PRIVATE_EXPORT
#else
#  include <QtScxml/private/qtscxmlexports_p.h>
#endif

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qscxmlLog)
Q_DECLARE_LOGGING_CATEGORY(scxmlLog)

QT_END_NAMESPACE

#endif // QSCXMLGLOBALS_P_H
