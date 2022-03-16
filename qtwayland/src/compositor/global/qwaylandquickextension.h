/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
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

#ifndef QWAYLANDQUICKEXTENSION_H
#define QWAYLANDQUICKEXTENSION_H

#include <QtWaylandCompositor/QWaylandCompositorExtension>
#include <QtQml/QQmlParserStatus>
#include <QtQml/QQmlListProperty>

QT_BEGIN_NAMESPACE

#define Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CLASS(className) \
    class Q_WAYLAND_COMPOSITOR_EXPORT className##QuickExtension : public className, public QQmlParserStatus \
    { \
/* qmake ignore Q_OBJECT */ \
        Q_OBJECT \
        Q_PROPERTY(QQmlListProperty<QObject> data READ data DESIGNABLE false) \
        Q_CLASSINFO("DefaultProperty", "data") \
        Q_INTERFACES(QQmlParserStatus) \
    public: \
        QQmlListProperty<QObject> data() \
        { \
            return QQmlListProperty<QObject>(this, m_objects); \
        } \
        void classBegin() override {} \
        void componentComplete() override { if (!isInitialized()) initialize(); } \
    private: \
        QList<QObject *> m_objects; \
    };

#define Q_COMPOSITOR_DECLARE_QUICK_EXTENSION_CONTAINER_CLASS(className) \
    class Q_WAYLAND_COMPOSITOR_EXPORT className##QuickExtensionContainer : public className \
    { \
/* qmake ignore Q_OBJECT */ \
        Q_OBJECT \
        Q_PROPERTY(QQmlListProperty<QWaylandCompositorExtension> extensions READ extensions) \
        Q_PROPERTY(QQmlListProperty<QObject> data READ data DESIGNABLE false) \
        Q_CLASSINFO("DefaultProperty", "data") \
    public: \
        QQmlListProperty<QObject> data() \
        { \
            return QQmlListProperty<QObject>(this, m_objects); \
        } \
        QQmlListProperty<QWaylandCompositorExtension> extensions() \
        { \
            return QQmlListProperty<QWaylandCompositorExtension>(this, this, \
                                                       &className##QuickExtensionContainer::append_extension, \
                                                       &className##QuickExtensionContainer::countFunction, \
                                                       &className##QuickExtensionContainer::atFunction, \
                                                       &className##QuickExtensionContainer::clearFunction); \
        } \
        static int countFunction(QQmlListProperty<QWaylandCompositorExtension> *list) \
        { \
            return static_cast<className##QuickExtensionContainer *>(list->data)->extension_vector.size(); \
        } \
        static QWaylandCompositorExtension *atFunction(QQmlListProperty<QWaylandCompositorExtension> *list, int index) \
        { \
            return static_cast<className##QuickExtensionContainer *>(list->data)->extension_vector.at(index); \
        } \
        static void append_extension(QQmlListProperty<QWaylandCompositorExtension> *list, QWaylandCompositorExtension *extension) \
        { \
            className##QuickExtensionContainer *quickExtObj = static_cast<className##QuickExtensionContainer *>(list->data); \
            extension->setExtensionContainer(quickExtObj); \
        } \
        static void clearFunction(QQmlListProperty<QWaylandCompositorExtension> *list) \
        { \
            static_cast<className##QuickExtensionContainer *>(list->data)->extension_vector.clear(); \
        } \
    private: \
        QList<QObject *> m_objects; \
    };

QT_END_NAMESPACE

#endif  /*QWAYLANDQUICKEXTENSION_H*/
