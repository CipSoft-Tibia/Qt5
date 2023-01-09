/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Virtual Keyboard module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtVirtualKeyboard/private/settings_p.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE
namespace QtVirtualKeyboard {

class SettingsPrivate : public QObjectPrivate
{
public:
    SettingsPrivate() :
        QObjectPrivate(),
        style(),
        styleName(),
        locale(),
        availableLocales(),
        activeLocales(),
        layoutPath(),
        wclAutoHideDelay(5000),
        wclAlwaysVisible(false),
        wclAutoCommitWord(false),
        fullScreenMode(false)
    {}

    QString style;
    QString styleName;
    QString locale;
    QStringList availableLocales;
    QStringList activeLocales;
    QUrl layoutPath;
    int wclAutoHideDelay;
    bool wclAlwaysVisible;
    bool wclAutoCommitWord;
    bool fullScreenMode;
};

static QScopedPointer<Settings> s_settingsInstance;

/*!
    \class QtVirtualKeyboard::Settings
    \internal
*/

Settings::Settings(QObject *parent) :
    QObject(*new SettingsPrivate(), parent)
{
}

Settings *Settings::instance()
{
    if (!s_settingsInstance)
        s_settingsInstance.reset(new Settings());
    return s_settingsInstance.data();
}

QString Settings::style() const
{
    Q_D(const Settings);
    return d->style;
}

void Settings::setStyle(const QString &style)
{
    Q_D(Settings);
    if (d->style != style) {
        d->style = style;
        emit styleChanged();
    }
}

QString Settings::styleName() const
{
    Q_D(const Settings);
    return d->styleName;
}

void Settings::setStyleName(const QString &styleName)
{
    Q_D(Settings);
    if (d->styleName != styleName) {
        d->styleName = styleName;
        emit styleNameChanged();
    }
}

QString Settings::locale() const
{
    Q_D(const Settings);
    return d->locale;
}

void Settings::setLocale(const QString &locale)
{
    Q_D(Settings);
    if (d->locale != locale) {
        d->locale = locale;
        emit localeChanged();
    }
}

QStringList Settings::availableLocales() const
{
    Q_D(const Settings);
    return d->availableLocales;
}

void Settings::setAvailableLocales(const QStringList &availableLocales)
{
    Q_D(Settings);
    if (d->availableLocales != availableLocales) {
        d->availableLocales = availableLocales;
        emit availableLocalesChanged();
    }
}

QStringList Settings::activeLocales() const
{
    Q_D(const Settings);
    return d->activeLocales;
}

void Settings::setActiveLocales(const QStringList &activeLocales)
{
    Q_D(Settings);
    if (d->activeLocales != activeLocales) {
        d->activeLocales = activeLocales;
        emit activeLocalesChanged();
    }
}

QUrl Settings::layoutPath() const
{
    Q_D(const Settings);
    return d->layoutPath;
}

void Settings::setLayoutPath(const QUrl &layoutPath)
{
    Q_D(Settings);
    if (d->layoutPath != layoutPath) {
        d->layoutPath = layoutPath;
        emit layoutPathChanged();
    }
}

int Settings::wclAutoHideDelay() const
{
    Q_D(const Settings);
    return d->wclAutoHideDelay;
}

void Settings::setWclAutoHideDelay(int wclAutoHideDelay)
{
    Q_D(Settings);
    if (d->wclAutoHideDelay != wclAutoHideDelay) {
        d->wclAutoHideDelay = wclAutoHideDelay;
        emit wclAutoHideDelayChanged();
    }
}

bool Settings::wclAlwaysVisible() const
{
    Q_D(const Settings);
    return d->wclAlwaysVisible;
}

void Settings::setWclAlwaysVisible(bool wclAlwaysVisible)
{
    Q_D(Settings);
    if (d->wclAlwaysVisible != wclAlwaysVisible) {
        d->wclAlwaysVisible = wclAlwaysVisible;
        emit wclAlwaysVisibleChanged();
    }
}

bool Settings::wclAutoCommitWord() const
{
    Q_D(const Settings);
    return d->wclAutoCommitWord;
}

void Settings::setWclAutoCommitWord(bool wclAutoCommitWord)
{
    Q_D(Settings);
    if (d->wclAutoCommitWord != wclAutoCommitWord) {
        d->wclAutoCommitWord = wclAutoCommitWord;
        emit wclAutoCommitWordChanged();
    }
}

bool Settings::fullScreenMode() const
{
    Q_D(const Settings);
    return d->fullScreenMode;
}

void Settings::setFullScreenMode(bool fullScreenMode)
{
    Q_D(Settings);
    if (d->fullScreenMode != fullScreenMode) {
        d->fullScreenMode = fullScreenMode;
        emit fullScreenModeChanged();
    }
}

} // namespace QtVirtualKeyboard
QT_END_NAMESPACE
