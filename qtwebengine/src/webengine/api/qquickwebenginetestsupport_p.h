/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
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

#ifndef QQUICKWEBENGINETESTSUPPORT_P_H
#define QQUICKWEBENGINETESTSUPPORT_P_H

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

#include <QtGui/private/qinputmethod_p.h>
#include <QtWebEngine/private/qtwebengineglobal_p.h>

#include <QEvent>
#include <QObject>
#include <QUrl>

QT_BEGIN_NAMESPACE

class QQuickWebEngineLoadRequest;
class QWindow;

class Q_WEBENGINE_PRIVATE_EXPORT QQuickWebEngineErrorPage : public QObject {
    Q_OBJECT

public:
    QQuickWebEngineErrorPage();

    void loadFinished(bool success, const QUrl &url);
    void loadStarted(const QUrl &provisionalUrl);

Q_SIGNALS:
    void loadingChanged(QQuickWebEngineLoadRequest *loadRequest);
};

class Q_WEBENGINE_PRIVATE_EXPORT QQuickWebEngineTestInputContext : public QPlatformInputContext {
    Q_OBJECT

public:
    QQuickWebEngineTestInputContext();
    ~QQuickWebEngineTestInputContext();

    Q_INVOKABLE void create();
    Q_INVOKABLE void release();

    void showInputPanel() override;
    void hideInputPanel() override;
    bool isInputPanelVisible() const override;

private:
    bool m_visible;
};

class Q_WEBENGINE_PRIVATE_EXPORT QQuickWebEngineTestEvent : public QObject {
    Q_OBJECT

public:
    QQuickWebEngineTestEvent();

public Q_SLOTS:
    bool mouseMultiClick(QObject *item, qreal x, qreal y, int clickCount);

private:
    QWindow *eventWindow(QObject *item = 0);
    void mouseEvent(QEvent::Type type, QWindow *window, QObject *item, const QPointF &_pos);
};

class Q_WEBENGINE_PRIVATE_EXPORT QQuickWebEngineTestSupport : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQuickWebEngineErrorPage *errorPage READ errorPage CONSTANT FINAL)
    Q_PROPERTY(QQuickWebEngineTestInputContext *testInputContext READ testInputContext CONSTANT FINAL)
    Q_PROPERTY(QQuickWebEngineTestEvent *testEvent READ testEvent CONSTANT FINAL)

public:
    QQuickWebEngineTestSupport();
    QQuickWebEngineErrorPage *errorPage() const;
    QQuickWebEngineTestInputContext *testInputContext() const;
    QQuickWebEngineTestEvent *testEvent() const;

Q_SIGNALS:
    void windowCloseRejected();
    void loadVisuallyCommitted();

private:
    QScopedPointer<QQuickWebEngineErrorPage> m_errorPage;
    QScopedPointer<QQuickWebEngineTestInputContext> m_testInputContext;
    QScopedPointer<QQuickWebEngineTestEvent> m_testEvent;
};

QT_END_NAMESPACE

#endif // QQUICKWEBENGINETESTSUPPORT_P_H
