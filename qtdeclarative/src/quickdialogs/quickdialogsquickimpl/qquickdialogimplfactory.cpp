// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdialogimplfactory_p.h"

#include <QtCore/qloggingcategory.h>

#include <QtQuick/private/qtquickglobal_p.h>
#if QT_CONFIG(quick_listview)
#include "qquickplatformfiledialog_p.h"
#include "qquickplatformfolderdialog_p.h"
#include "qquickplatformfontdialog_p.h"
#endif
#include "qquickplatformcolordialog_p.h"
#include "qquickplatformmessagedialog_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    Creates concrete QML-based dialogs.
*/

Q_LOGGING_CATEGORY(lcQuickDialogImplFactory, "qt.quick.dialogs.quickdialogimplfactory")

std::unique_ptr<QPlatformDialogHelper> QQuickDialogImplFactory::createPlatformDialogHelper(QQuickDialogType type, QObject *parent)
{
    std::unique_ptr<QPlatformDialogHelper> dialogHelper;
    switch (type) {
    case QQuickDialogType::ColorDialog: {
        dialogHelper.reset(new QQuickPlatformColorDialog(parent));
        break;
    }
#if QT_CONFIG(quick_listview)
    case QQuickDialogType::FileDialog: {
        dialogHelper.reset(new QQuickPlatformFileDialog(parent));
        break;
    }
    case QQuickDialogType::FolderDialog: {
        dialogHelper.reset(new QQuickPlatformFolderDialog(parent));
        break;
    }
    case QQuickDialogType::FontDialog: {
        dialogHelper.reset(new QQuickPlatformFontDialog(parent));
        break;
    }
#endif
    case QQuickDialogType::MessageDialog: {
        dialogHelper.reset(new QQuickPlatformMessageDialog(parent));
        break;
    }
    default:
        break;
    }

    return dialogHelper;
}

QT_END_NAMESPACE
