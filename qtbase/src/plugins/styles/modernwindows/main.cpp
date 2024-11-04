// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtWidgets/qstyleplugin.h>
#include <QtCore/qoperatingsystemversion.h>
#include "qwindowsvistastyle_p.h"
#include "qwindows11style_p.h"

QT_BEGIN_NAMESPACE

class QModernWindowsStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "modernwindowsstyles.json")
public:
    QStyle *create(const QString &key) override;
};

QStyle *QModernWindowsStylePlugin::create(const QString &key)
{
    bool isWin11OrAbove = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11;
    if (isWin11OrAbove && key.compare(QLatin1String("windows11"), Qt::CaseInsensitive) == 0) {
        return new QWindows11Style();
    } else if (!isWin11OrAbove && key.compare(QLatin1String("windows11"), Qt::CaseInsensitive) == 0) {
        qWarning("QWindows11Style: Style is only supported on Windows11 and above");
        return new QWindowsVistaStyle();
    } else if (key.compare(QLatin1String("windowsvista"), Qt::CaseInsensitive) == 0) {
        return new QWindowsVistaStyle();
    }
    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
