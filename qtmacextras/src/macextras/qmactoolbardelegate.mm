/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtMacExtras module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmactoolbardelegate_p.h"

#include <QtGui/QImage>
#include <QtGui/QPixmap>

#include <QtCore/QString>

#include "qmacfunctions.h"
#include "qmactoolbar.h"

QT_USE_NAMESPACE

NSArray *toNSArray(const QList<QString> &stringList)
{
    NSMutableArray *array = [[NSMutableArray alloc] init];
    for (const QString &string : stringList) {
        [array addObject:string.toNSString()];
    }
    return array;
}

// from QMacToolButton.cpp
QString qt_strippedText(QString s)
{
    s.remove( QString::fromLatin1("...") );
    int i = 0;
    while (i < s.size()) {
        ++i;
        if (s.at(i-1) != QLatin1Char('&'))
            continue;
        if (i < s.size() && s.at(i) == QLatin1Char('&'))
            ++i;
        s.remove(i-1,1);
    }
    return s.trimmed();
}

@implementation QMacToolbarDelegate

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar*)toolbar
{
    Q_UNUSED(toolbar);
    return toolbarPrivate->getItemIdentifiers(toolbarPrivate->items, false);
}

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar*)toolbar
{
    Q_UNUSED(toolbar);
    return toolbarPrivate->getItemIdentifiers(toolbarPrivate->allowedItems, false);
}

- (NSArray *)toolbarSelectableItemIdentifiers: (NSToolbar *)toolbar
{
    Q_UNUSED(toolbar);
    NSMutableArray *array = toolbarPrivate->getItemIdentifiers(toolbarPrivate->items, true);
    [array addObjectsFromArray:toolbarPrivate->getItemIdentifiers(toolbarPrivate->allowedItems, true)];
    return array;
}

- (IBAction)itemClicked:(id)sender
{
    NSToolbarItem *item = reinterpret_cast<NSToolbarItem *>(sender);
    toolbarPrivate->itemClicked(item);
}

- (NSToolbarItem *) toolbar: (NSToolbar *)toolbar itemForItemIdentifier: (NSString *) itemIdentifier willBeInsertedIntoToolbar:(BOOL) willBeInserted
{
    Q_UNUSED(toolbar);
    Q_UNUSED(willBeInserted);
    const QString identifier = QString::fromNSString(itemIdentifier);
    QMacToolBarItem *toolButton = reinterpret_cast<QMacToolBarItem *>(identifier.toULongLong()); // string -> unisgned long long -> pointer
    NSToolbarItem *toolbarItem = toolButton->nativeToolBarItem();

    [toolbarItem setTarget:self];
    [toolbarItem setAction:@selector(itemClicked:)];

    return toolbarItem;
}

@end
