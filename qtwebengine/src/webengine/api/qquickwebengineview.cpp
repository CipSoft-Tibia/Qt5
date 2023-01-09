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

#include "qquickwebengineview_p.h"
#include "qquickwebengineview_p_p.h"
#include "authentication_dialog_controller.h"
#include "profile_adapter.h"
#include "certificate_error_controller.h"
#include "file_picker_controller.h"
#include "find_text_helper.h"
#include "javascript_dialog_controller.h"
#include "touch_selection_menu_controller.h"

#include "qquickwebengineaction_p.h"
#include "qquickwebengineaction_p_p.h"
#include "qquickwebenginehistory_p.h"
#include "qquickwebenginecertificateerror_p.h"
#include "qquickwebengineclientcertificateselection_p.h"
#include "qquickwebenginecontextmenurequest_p.h"
#include "qquickwebenginedialogrequests_p.h"
#include "qquickwebenginefaviconprovider_p_p.h"
#include "qquickwebengineloadrequest_p.h"
#include "qquickwebenginenavigationrequest_p.h"
#include "qquickwebenginenewviewrequest_p.h"
#include "qquickwebengineprofile_p.h"
#include "qquickwebenginesettings_p.h"
#include "qquickwebenginescript_p.h"
#include "qquickwebenginetouchhandleprovider_p_p.h"
#include "qwebenginefindtextresult.h"
#include "qwebenginequotarequest.h"
#include "qwebengineregisterprotocolhandlerrequest.h"

#if QT_CONFIG(webengine_testsupport)
#include "qquickwebenginetestsupport_p.h"
#endif

#include "render_widget_host_view_qt_delegate_quick.h"
#include "render_widget_host_view_qt_delegate_quickwindow.h"
#include "renderer_host/user_resource_controller_host.h"
#include "ui_delegates_manager.h"
#include "web_contents_adapter.h"
#include "web_engine_error.h"
#include "web_engine_settings.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMarginsF>
#include <QMimeData>
#include <QPageLayout>
#include <QPageSize>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#if QT_CONFIG(webengine_webchannel)
#include <QQmlWebChannel>
#endif
#include <QQuickWebEngineProfile>
#include <QScreen>
#include <QUrl>
#include <QTimer>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE
using namespace QtWebEngineCore;

#ifndef QT_NO_ACCESSIBILITY
static QAccessibleInterface *webAccessibleFactory(const QString &, QObject *object)
{
    if (QQuickWebEngineView *v = qobject_cast<QQuickWebEngineView*>(object))
        return new QQuickWebEngineViewAccessible(v);
    return 0;
}
#endif // QT_NO_ACCESSIBILITY

static QLatin1String defaultMimeType("text/html;charset=UTF-8");

QQuickWebEngineViewPrivate::QQuickWebEngineViewPrivate()
    : m_profile(nullptr)
    , adapter(QSharedPointer<WebContentsAdapter>::create())
    , m_history(new QQuickWebEngineHistory(this))
#if QT_CONFIG(webengine_testsupport)
    , m_testSupport(0)
#endif
    , contextMenuExtraItems(0)
    , faviconProvider(0)
    , loadProgress(0)
    , m_fullscreenMode(false)
    , isLoading(false)
    , m_activeFocusOnPress(true)
    , devicePixelRatio(QGuiApplication::primaryScreen()->devicePixelRatio())
    , m_webChannel(0)
    , m_webChannelWorld(0)
    , m_defaultAudioMuted(false)
    , m_isBeingAdopted(false)
    , m_backgroundColor(Qt::white)
    , m_zoomFactor(1.0)
    , m_ui2Enabled(false)
    , m_profileInitialized(false)
{
    memset(actions, 0, sizeof(actions));

    QString platform = qApp->platformName().toLower();
    if (platform == QLatin1String("eglfs"))
        m_ui2Enabled = true;

    const QByteArray dialogSet = qgetenv("QTWEBENGINE_DIALOG_SET");

    if (!dialogSet.isEmpty()) {
        if (dialogSet == QByteArrayLiteral("QtQuickControls2")) {
            m_ui2Enabled = true;
        } else if (dialogSet == QByteArrayLiteral("QtQuickControls1")
                   && m_ui2Enabled) {
            m_ui2Enabled = false;
            qWarning("QTWEBENGINE_DIALOG_SET=QtQuickControls1 forces use of Qt Quick Controls 1 "
                     "on an eglfs backend. This can crash your application!");
        } else {
            qWarning("Ignoring QTWEBENGINE_DIALOG_SET environment variable set to %s. Accepted "
                     "values are \"QtQuickControls1\" and \"QtQuickControls2\"", dialogSet.data());
        }
    }

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(&webAccessibleFactory);
#endif // QT_NO_ACCESSIBILITY
}

QQuickWebEngineViewPrivate::~QQuickWebEngineViewPrivate()
{
    Q_ASSERT(m_profileInitialized);
    m_profile->d_ptr->removeWebContentsAdapterClient(this);
    if (faviconProvider)
        faviconProvider->detach(q_ptr);
    // q_ptr->d_ptr might be null due to destroy()
    if (q_ptr->d_ptr)
        bindViewAndWidget(q_ptr, nullptr);
}

void QQuickWebEngineViewPrivate::initializeProfile()
{
    if (!m_profileInitialized) {
        Q_ASSERT(!adapter->isInitialized());
        m_profileInitialized = true;
        if (!m_profile)
            m_profile = QQuickWebEngineProfile::defaultProfile();
        m_profile->d_ptr->addWebContentsAdapterClient(this);
        m_settings.reset(new QQuickWebEngineSettings(m_profile->settings()));
        adapter->setClient(this);
    }
}

bool QQuickWebEngineViewPrivate::profileInitialized() const
{
    return m_profileInitialized;
}

void QQuickWebEngineViewPrivate::releaseProfile()
{
    // The profile for this web contents is about to be
    // garbage collected, delete WebContents first and
    // let the QQuickWebEngineView be collected later by gc.
    bindViewAndWidget(q_ptr, nullptr);
    delete q_ptr->d_ptr.take();
}

UIDelegatesManager *QQuickWebEngineViewPrivate::ui()
{
    Q_Q(QQuickWebEngineView);
    if (m_uIDelegatesManager.isNull())
        m_uIDelegatesManager.reset(m_ui2Enabled ? new UI2DelegatesManager(q) : new UIDelegatesManager(q));
    return m_uIDelegatesManager.data();
}

RenderWidgetHostViewQtDelegate *QQuickWebEngineViewPrivate::CreateRenderWidgetHostViewQtDelegate(RenderWidgetHostViewQtDelegateClient *client)
{
    return new RenderWidgetHostViewQtDelegateQuick(client, /*isPopup = */ false);
}

RenderWidgetHostViewQtDelegate *QQuickWebEngineViewPrivate::CreateRenderWidgetHostViewQtDelegateForPopup(RenderWidgetHostViewQtDelegateClient *client)
{
    Q_Q(QQuickWebEngineView);
    const bool hasWindowCapability = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows);
    RenderWidgetHostViewQtDelegateQuick *quickDelegate = new RenderWidgetHostViewQtDelegateQuick(client, /*isPopup = */ true);
    if (hasWindowCapability) {
        RenderWidgetHostViewQtDelegateQuickWindow *wrapperWindow =
                new RenderWidgetHostViewQtDelegateQuickWindow(quickDelegate, q->window());
        wrapperWindow->setVirtualParent(q);
        quickDelegate->setParentItem(wrapperWindow->contentItem());
        return wrapperWindow;
    }
    quickDelegate->setParentItem(q);
    quickDelegate->show();
    return quickDelegate;
}

void QQuickWebEngineViewPrivate::contextMenuRequested(const WebEngineContextMenuData &data)
{
    Q_Q(QQuickWebEngineView);

    m_contextMenuData = data;

    QQmlEngine *engine = qmlEngine(q);

    // TODO: this is a workaround for QTBUG-65044
    if (!engine)
        return;

    // mark the object for gc by creating temporary jsvalue
    QQuickWebEngineContextMenuRequest *request = new QQuickWebEngineContextMenuRequest(data);
    engine->newQObject(request);
    Q_EMIT q->contextMenuRequested(request);

    if (request->isAccepted())
        return;

    // Assign the WebEngineView as the parent of the menu, so mouse events are properly propagated
    // on OSX.
    QObject *menu = ui()->addMenu(q, QString(), data.position());
    if (!menu)
        return;

    QQuickContextMenuBuilder contextMenuBuilder(data, q, menu);

    // Populate our menu
    contextMenuBuilder.initMenu();

    // FIXME: expose the context menu data as an attached property to make this more useful
    if (contextMenuExtraItems)
        contextMenuBuilder.appendExtraItems(engine);

    // Now fire the popup() method on the top level menu
    ui()->showMenu(menu);
}

void QQuickWebEngineViewPrivate::navigationRequested(int navigationType, const QUrl &url, int &navigationRequestAction, bool isMainFrame)
{
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineNavigationRequest navigationRequest(url, static_cast<QQuickWebEngineView::NavigationType>(navigationType), isMainFrame);
    Q_EMIT q->navigationRequested(&navigationRequest);

    navigationRequestAction = navigationRequest.action();
    if ((navigationRequestAction == WebContentsAdapterClient::AcceptRequest) && adapter->findTextHelper()->isFindTextInProgress())
        adapter->findTextHelper()->stopFinding();
}

void QQuickWebEngineViewPrivate::javascriptDialog(QSharedPointer<JavaScriptDialogController> dialog)
{
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineJavaScriptDialogRequest *request = new QQuickWebEngineJavaScriptDialogRequest(dialog);
    // mark the object for gc by creating temporary jsvalue
    qmlEngine(q)->newQObject(request);
    Q_EMIT q->javaScriptDialogRequested(request);
    if (!request->isAccepted())
        ui()->showDialog(dialog);
}

void QQuickWebEngineViewPrivate::allowCertificateError(const QSharedPointer<CertificateErrorController> &errorController)
{
    Q_Q(QQuickWebEngineView);

    QQuickWebEngineCertificateError *quickController = new QQuickWebEngineCertificateError(errorController);
    // mark the object for gc by creating temporary jsvalue
    qmlEngine(q)->newQObject(quickController);
    Q_EMIT q->certificateError(quickController);
    if (!quickController->overridable() || (!quickController->deferred() && !quickController->answered()))
        quickController->rejectCertificate();
    else
        m_certificateErrorControllers.append(errorController);
}

void QQuickWebEngineViewPrivate::selectClientCert(const QSharedPointer<ClientCertSelectController> &controller)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineClientCertificateSelection *certSelection = new QQuickWebEngineClientCertificateSelection(controller);
    // mark the object for gc by creating temporary jsvalue
    qmlEngine(q)->newQObject(certSelection);
    Q_EMIT q->selectClientCertificate(certSelection);
#else
    Q_UNUSED(controller);
#endif
}

static QQuickWebEngineView::Feature toFeature(QtWebEngineCore::ProfileAdapter::PermissionType type)
{
    switch (type) {
    case QtWebEngineCore::ProfileAdapter::NotificationPermission:
        return QQuickWebEngineView::Notifications;
    case QtWebEngineCore::ProfileAdapter::GeolocationPermission:
        return QQuickWebEngineView::Geolocation;
    default:
        break;
    }
    Q_UNREACHABLE();
    return QQuickWebEngineView::Feature(-1);
}


void QQuickWebEngineViewPrivate::runFeaturePermissionRequest(QtWebEngineCore::ProfileAdapter::PermissionType permission, const QUrl &url)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->featurePermissionRequested(url, toFeature(permission));
}

void QQuickWebEngineViewPrivate::showColorDialog(QSharedPointer<ColorChooserController> controller)
{
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineColorDialogRequest *request = new QQuickWebEngineColorDialogRequest(controller);
    // mark the object for gc by creating temporary jsvalue
    qmlEngine(q)->newQObject(request);
    Q_EMIT q->colorDialogRequested(request);
    if (!request->isAccepted())
        ui()->showColorDialog(controller);
}

void QQuickWebEngineViewPrivate::runFileChooser(QSharedPointer<FilePickerController> controller)
{
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineFileDialogRequest *request = new QQuickWebEngineFileDialogRequest(controller);
    // mark the object for gc by creating temporary jsvalue
    qmlEngine(q)->newQObject(request);
    Q_EMIT q->fileDialogRequested(request);
    if (!request->isAccepted())
        ui()->showFilePicker(controller);
}

bool QQuickWebEngineViewPrivate::passOnFocus(bool reverse)
{
    Q_Q(QQuickWebEngineView);
    // The child delegate currently has focus, find the next one from there and give it focus.
    QQuickItem *next = q->scopedFocusItem()->nextItemInFocusChain(!reverse);
    if (next) {
        next->forceActiveFocus(reverse ? Qt::BacktabFocusReason : Qt::TabFocusReason);
        return true;
    }
    return false;
}

void QQuickWebEngineViewPrivate::titleChanged(const QString &title)
{
    Q_Q(QQuickWebEngineView);
    Q_UNUSED(title);
    Q_EMIT q->titleChanged();
}

void QQuickWebEngineViewPrivate::urlChanged()
{
    Q_Q(QQuickWebEngineView);
    QUrl url = adapter->activeUrl();
    if (m_url != url) {
        m_url = url;
        Q_EMIT q->urlChanged();
    }
}

void QQuickWebEngineViewPrivate::iconChanged(const QUrl &url)
{
    Q_Q(QQuickWebEngineView);

    if (iconUrl == QQuickWebEngineFaviconProvider::faviconProviderUrl(url))
        return;

    if (!faviconProvider) {
        QQmlEngine *engine = qmlEngine(q);

        // TODO: this is a workaround for QTBUG-65044
        if (!engine)
            return;

        Q_ASSERT(engine);
        faviconProvider = static_cast<QQuickWebEngineFaviconProvider *>(
                    engine->imageProvider(QQuickWebEngineFaviconProvider::identifier()));
        Q_ASSERT(faviconProvider);
    }

    iconUrl = faviconProvider->attach(q, url);
    m_history->reset();
    QTimer::singleShot(0, q, &QQuickWebEngineView::iconChanged);
}

void QQuickWebEngineViewPrivate::loadProgressChanged(int progress)
{
    Q_Q(QQuickWebEngineView);
    loadProgress = progress;
    QTimer::singleShot(0, q, &QQuickWebEngineView::loadProgressChanged);
}

void QQuickWebEngineViewPrivate::didUpdateTargetURL(const QUrl &hoveredUrl)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->linkHovered(hoveredUrl);
}

void QQuickWebEngineViewPrivate::selectionChanged()
{
    updateEditActions();
}

void QQuickWebEngineViewPrivate::zoomUpdateIsNeeded()
{
    Q_Q(QQuickWebEngineView);
    q->setZoomFactor(m_zoomFactor);
}

void QQuickWebEngineViewPrivate::recentlyAudibleChanged(bool recentlyAudible)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->recentlyAudibleChanged(recentlyAudible);
}

void QQuickWebEngineViewPrivate::renderProcessPidChanged(qint64 pid)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->renderProcessPidChanged(pid);
}

QRectF QQuickWebEngineViewPrivate::viewportRect() const
{
    Q_Q(const QQuickWebEngineView);
    return QRectF(q->x(), q->y(), q->width(), q->height());
}

QColor QQuickWebEngineViewPrivate::backgroundColor() const
{
    return m_backgroundColor;
}

void QQuickWebEngineViewPrivate::loadStarted(const QUrl &provisionalUrl, bool isErrorPage)
{
    Q_Q(QQuickWebEngineView);
    if (isErrorPage) {
#if QT_CONFIG(webengine_testsupport)
        if (m_testSupport)
            m_testSupport->errorPage()->loadStarted(provisionalUrl);
#endif
        return;
    }

    isLoading = true;
    m_history->reset();
    m_certificateErrorControllers.clear();

    QTimer::singleShot(0, q, [q, provisionalUrl]() {
        QQuickWebEngineLoadRequest loadRequest(provisionalUrl, QQuickWebEngineView::LoadStartedStatus);

        emit q->loadingChanged(&loadRequest);
    });
}

void QQuickWebEngineViewPrivate::loadCommitted()
{
    m_history->reset();
}

void QQuickWebEngineViewPrivate::loadVisuallyCommitted()
{
#if QT_CONFIG(webengine_testsupport)
    if (m_testSupport)
        Q_EMIT m_testSupport->loadVisuallyCommitted();
#endif
}

Q_STATIC_ASSERT(static_cast<int>(WebEngineError::NoErrorDomain) == static_cast<int>(QQuickWebEngineView::NoErrorDomain));
Q_STATIC_ASSERT(static_cast<int>(WebEngineError::CertificateErrorDomain) == static_cast<int>(QQuickWebEngineView::CertificateErrorDomain));
Q_STATIC_ASSERT(static_cast<int>(WebEngineError::DnsErrorDomain) == static_cast<int>(QQuickWebEngineView::DnsErrorDomain));

void QQuickWebEngineViewPrivate::loadFinished(bool success, const QUrl &url, bool isErrorPage, int errorCode, const QString &errorDescription)
{
    Q_Q(QQuickWebEngineView);

    if (isErrorPage) {
#if QT_CONFIG(webengine_testsupport)
        if (m_testSupport)
            m_testSupport->errorPage()->loadFinished(success, url);
#endif
        return;
    }

    isLoading = false;
    m_history->reset();
    if (errorCode == WebEngineError::UserAbortedError) {
        QTimer::singleShot(0, q, [q, url]() {
            QQuickWebEngineLoadRequest loadRequest(url, QQuickWebEngineView::LoadStoppedStatus);
            emit q->loadingChanged(&loadRequest);
        });
        return;
    }
    if (success) {
        QTimer::singleShot(0, q, [q, url, errorDescription, errorCode]() {
            QQuickWebEngineLoadRequest loadRequest(url, QQuickWebEngineView::LoadSucceededStatus, errorDescription, errorCode);
            emit q->loadingChanged(&loadRequest);
        });
        return;
    }

    Q_ASSERT(errorCode);
    QQuickWebEngineView::ErrorDomain errorDomain = static_cast<QQuickWebEngineView::ErrorDomain>(WebEngineError::toQtErrorDomain(errorCode));
    QTimer::singleShot(0, q, [q, url, errorDescription, errorCode, errorDomain]() {
        QQuickWebEngineLoadRequest loadRequest(url, QQuickWebEngineView::LoadFailedStatus,errorDescription, errorCode, errorDomain);
        emit q->loadingChanged(&loadRequest);
    });
    return;
}

void QQuickWebEngineViewPrivate::focusContainer()
{
    Q_Q(QQuickWebEngineView);
    QQuickWindow *window = q->window();
    if (window)
        window->requestActivate();
    q->forceActiveFocus();
}

void QQuickWebEngineViewPrivate::unhandledKeyEvent(QKeyEvent *event)
{
    Q_Q(QQuickWebEngineView);
    if (q->parentItem())
        QCoreApplication::sendEvent(q->parentItem(), event);
}

QSharedPointer<WebContentsAdapter>
QQuickWebEngineViewPrivate::adoptNewWindow(QSharedPointer<WebContentsAdapter> newWebContents,
                                           WindowOpenDisposition disposition, bool userGesture,
                                           const QRect &, const QUrl &targetUrl)
{
    Q_Q(QQuickWebEngineView);
    Q_ASSERT(newWebContents);
    QQuickWebEngineNewViewRequest request;
    request.m_adapter = newWebContents;
    request.m_isUserInitiated = userGesture;
    request.m_requestedUrl = targetUrl;

    switch (disposition) {
    case WebContentsAdapterClient::NewForegroundTabDisposition:
        request.m_destination = QQuickWebEngineView::NewViewInTab;
        break;
    case WebContentsAdapterClient::NewBackgroundTabDisposition:
        request.m_destination = QQuickWebEngineView::NewViewInBackgroundTab;
        break;
    case WebContentsAdapterClient::NewPopupDisposition:
        request.m_destination = QQuickWebEngineView::NewViewInDialog;
        break;
    case WebContentsAdapterClient::NewWindowDisposition:
        request.m_destination = QQuickWebEngineView::NewViewInWindow;
        break;
    default:
        Q_UNREACHABLE();
    }

    Q_EMIT q->newViewRequested(&request);

    if (!request.m_isRequestHandled)
        return nullptr;

    return newWebContents;
}

bool QQuickWebEngineViewPrivate::isBeingAdopted()
{
    return false;
}

void QQuickWebEngineViewPrivate::close()
{
    Q_Q(QQuickWebEngineView);
    emit q->windowCloseRequested();
}

void QQuickWebEngineViewPrivate::windowCloseRejected()
{
#if QT_CONFIG(webengine_testsupport)
    if (m_testSupport)
        Q_EMIT m_testSupport->windowCloseRejected();
#endif
}

void QQuickWebEngineViewPrivate::requestFullScreenMode(const QUrl &origin, bool fullscreen)
{
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineFullScreenRequest request(this, origin, fullscreen);
    Q_EMIT q->fullScreenRequested(request);
}

bool QQuickWebEngineViewPrivate::isFullScreenMode() const
{
    return m_fullscreenMode;
}

void QQuickWebEngineViewPrivate::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID)
{
    Q_Q(QQuickWebEngineView);
    if (q->receivers(SIGNAL(javaScriptConsoleMessage(JavaScriptConsoleMessageLevel,QString,int,QString))) > 0) {
        Q_EMIT q->javaScriptConsoleMessage(static_cast<QQuickWebEngineView::JavaScriptConsoleMessageLevel>(level), message, lineNumber, sourceID);
        return;
    }

    static QLoggingCategory loggingCategory("js", QtWarningMsg);
    const QByteArray file = sourceID.toUtf8();
    QMessageLogger logger(file.constData(), lineNumber, nullptr, loggingCategory.categoryName());

    switch (level) {
    case JavaScriptConsoleMessageLevel::Info:
        if (loggingCategory.isInfoEnabled())
            logger.info().noquote() << message;
        break;
    case JavaScriptConsoleMessageLevel::Warning:
        if (loggingCategory.isWarningEnabled())
            logger.warning().noquote() << message;
        break;
    case JavaScriptConsoleMessageLevel::Error:
        if (loggingCategory.isCriticalEnabled())
            logger.critical().noquote() << message;
        break;
    }
}

void QQuickWebEngineViewPrivate::authenticationRequired(QSharedPointer<AuthenticationDialogController> controller)
{
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineAuthenticationDialogRequest *request = new QQuickWebEngineAuthenticationDialogRequest(controller);
    // mark the object for gc by creating temporary jsvalue
    qmlEngine(q)->newQObject(request);
    Q_EMIT q->authenticationDialogRequested(request);
    if (!request->isAccepted())
        ui()->showDialog(controller);
}

void QQuickWebEngineViewPrivate::runMediaAccessPermissionRequest(const QUrl &securityOrigin, WebContentsAdapterClient::MediaRequestFlags requestFlags)
{
    Q_Q(QQuickWebEngineView);
    if (!requestFlags)
        return;
    QQuickWebEngineView::Feature feature;
    if (requestFlags.testFlag(WebContentsAdapterClient::MediaAudioCapture) && requestFlags.testFlag(WebContentsAdapterClient::MediaVideoCapture))
        feature = QQuickWebEngineView::MediaAudioVideoCapture;
    else if (requestFlags.testFlag(WebContentsAdapterClient::MediaAudioCapture))
        feature = QQuickWebEngineView::MediaAudioCapture;
    else if (requestFlags.testFlag(WebContentsAdapterClient::MediaVideoCapture))
        feature = QQuickWebEngineView::MediaVideoCapture;
    else if (requestFlags.testFlag(WebContentsAdapterClient::MediaDesktopAudioCapture) &&
             requestFlags.testFlag(WebContentsAdapterClient::MediaDesktopVideoCapture))
        feature = QQuickWebEngineView::DesktopAudioVideoCapture;
    else // if (requestFlags.testFlag(WebContentsAdapterClient::MediaDesktopVideoCapture))
        feature = QQuickWebEngineView::DesktopVideoCapture;
    Q_EMIT q->featurePermissionRequested(securityOrigin, feature);
}

void QQuickWebEngineViewPrivate::runMouseLockPermissionRequest(const QUrl &securityOrigin)
{
    // TODO: Add mouse lock support
    adapter->grantMouseLockPermission(securityOrigin, false);
}

void QQuickWebEngineViewPrivate::runQuotaRequest(QWebEngineQuotaRequest request)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->quotaRequested(request);
}

void QQuickWebEngineViewPrivate::runRegisterProtocolHandlerRequest(QWebEngineRegisterProtocolHandlerRequest request)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->registerProtocolHandlerRequested(request);
}

QObject *QQuickWebEngineViewPrivate::accessibilityParentObject()
{
    Q_Q(QQuickWebEngineView);
    return q;
}

ProfileAdapter *QQuickWebEngineViewPrivate::profileAdapter()
{
    return m_profile->d_ptr->profileAdapter();
}

WebContentsAdapter *QQuickWebEngineViewPrivate::webContentsAdapter()
{
    return adapter.data();
}

void QQuickWebEngineViewPrivate::printRequested()
{
    Q_Q(QQuickWebEngineView);
    QTimer::singleShot(0, q, [q]() {
        Q_EMIT q->printRequested();
    });
}

void QQuickWebEngineViewPrivate::widgetChanged(RenderWidgetHostViewQtDelegate *newWidgetBase)
{
    Q_Q(QQuickWebEngineView);
    bindViewAndWidget(q, static_cast<RenderWidgetHostViewQtDelegateQuick *>(newWidgetBase));
}

void QQuickWebEngineViewPrivate::findTextFinished(const QWebEngineFindTextResult &result)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->findTextFinished(result);
}

WebEngineSettings *QQuickWebEngineViewPrivate::webEngineSettings() const
{
    return m_settings->d_ptr.data();
}

const QObject *QQuickWebEngineViewPrivate::holdingQObject() const
{
    Q_Q(const QQuickWebEngineView);
    return q;
}

void QQuickWebEngineViewPrivate::lifecycleStateChanged(LifecycleState state)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->lifecycleStateChanged(static_cast<QQuickWebEngineView::LifecycleState>(state));
}

void QQuickWebEngineViewPrivate::recommendedStateChanged(LifecycleState state)
{
    Q_Q(QQuickWebEngineView);
    QTimer::singleShot(0, q, [q, state]() {
        Q_EMIT q->recommendedStateChanged(static_cast<QQuickWebEngineView::LifecycleState>(state));
    });
}

void QQuickWebEngineViewPrivate::visibleChanged(bool visible)
{
    Q_UNUSED(visible);
}

#ifndef QT_NO_ACCESSIBILITY
QQuickWebEngineViewAccessible::QQuickWebEngineViewAccessible(QQuickWebEngineView *o)
    : QAccessibleObject(o)
{}

bool QQuickWebEngineViewAccessible::isValid() const
{
    if (!QAccessibleObject::isValid())
        return false;

    if (!engineView() || !engineView()->d_func())
        return false;

    return true;
}

QAccessibleInterface *QQuickWebEngineViewAccessible::parent() const
{
    QQuickItem *parent = engineView()->parentItem();
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(parent);
    if (!iface)
        return QAccessible::queryAccessibleInterface(engineView()->window());
    return iface;
}

QAccessibleInterface *QQuickWebEngineViewAccessible::focusChild() const
{
    if (child(0) && child(0)->focusChild())
        return child(0)->focusChild();
    return const_cast<QQuickWebEngineViewAccessible *>(this);
}

int QQuickWebEngineViewAccessible::childCount() const
{
    return child(0) ? 1 : 0;
}

QAccessibleInterface *QQuickWebEngineViewAccessible::child(int index) const
{
    if (index == 0 && isValid())
        return engineView()->d_func()->adapter->browserAccessible();
    return nullptr;
}

int QQuickWebEngineViewAccessible::indexOfChild(const QAccessibleInterface *c) const
{
    if (child(0) && c == child(0))
        return 0;
    return -1;
}

QString QQuickWebEngineViewAccessible::text(QAccessible::Text) const
{
    return QString();
}

QAccessible::Role QQuickWebEngineViewAccessible::role() const
{
    return QAccessible::Client;
}

QAccessible::State QQuickWebEngineViewAccessible::state() const
{
    QAccessible::State s;
    return s;
}
#endif // QT_NO_ACCESSIBILITY

class WebContentsAdapterOwner : public QObject
{
public:
    typedef QSharedPointer<QtWebEngineCore::WebContentsAdapter> AdapterPtr;
    WebContentsAdapterOwner(const AdapterPtr &ptr)
        : adapter(ptr)
    {}

private:
    AdapterPtr adapter;
};

void QQuickWebEngineViewPrivate::adoptWebContents(WebContentsAdapter *webContents)
{
    if (!webContents) {
        qWarning("Trying to open an empty request, it was either already used or was invalidated."
            "\nYou must complete the request synchronously within the newViewRequested signal handler."
            " If a view hasn't been adopted before returning, the request will be invalidated.");
        return;
    }

    if (webContents->profileAdapter() && profileAdapter() != webContents->profileAdapter()) {
        qWarning("Can not adopt content from a different WebEngineProfile.");
        return;
    }

    m_isBeingAdopted = true;

    // This throws away the WebContentsAdapter that has been used until now.
    // All its states, particularly the loading URL, are replaced by the adopted WebContentsAdapter.
    WebContentsAdapterOwner *adapterOwner = new WebContentsAdapterOwner(adapter->sharedFromThis());
    adapterOwner->deleteLater();

    adapter = webContents->sharedFromThis();
    adapter->setClient(this);
}

QQuickWebEngineView::QQuickWebEngineView(QQuickItem *parent)
    : QQuickItem(parent)
    , d_ptr(new QQuickWebEngineViewPrivate)
{
    Q_D(QQuickWebEngineView);
    d->q_ptr = this;
    this->setActiveFocusOnTab(true);
    this->setFlags(QQuickItem::ItemIsFocusScope | QQuickItem::ItemAcceptsDrops);

    connect(action(WebAction::Back), &QQuickWebEngineAction::enabledChanged,
            this, &QQuickWebEngineView::canGoBackChanged);
    connect(action(WebAction::Forward), &QQuickWebEngineAction::enabledChanged,
            this, &QQuickWebEngineView::canGoForwardChanged);
}

QQuickWebEngineView::~QQuickWebEngineView()
{
}

void QQuickWebEngineViewPrivate::ensureContentsAdapter()
{
    initializeProfile();
    if (!adapter->isInitialized()) {
        if (!m_html.isEmpty())
            adapter->setContent(m_html.toUtf8(), defaultMimeType, m_url);
        else if (m_url.isValid())
            adapter->load(m_url);
        else
            adapter->loadDefault();
    }
}

void QQuickWebEngineViewPrivate::initializationFinished()
{
    Q_Q(QQuickWebEngineView);

    Q_ASSERT(m_profileInitialized);
    if (m_backgroundColor != Qt::white) {
        adapter->setBackgroundColor(m_backgroundColor);
        emit q->backgroundColorChanged();
    }

    // apply if it was set before first ever navigation already
    q->setZoomFactor(m_zoomFactor);

#if QT_CONFIG(webengine_webchannel)
    if (m_webChannel)
        adapter->setWebChannel(m_webChannel, m_webChannelWorld);
#endif

    if (m_defaultAudioMuted != adapter->isAudioMuted())
        adapter->setAudioMuted(m_defaultAudioMuted);

    if (devToolsView && devToolsView->d_ptr->adapter)
        adapter->openDevToolsFrontend(devToolsView->d_ptr->adapter);

    for (QQuickWebEngineScript *script : qAsConst(m_userScripts))
        script->d_func()->bind(profileAdapter()->userResourceController(), adapter.data());

    if (q->window())
        adapter->setVisible(q->isVisible());

    if (!m_isBeingAdopted)
        return;

    // Ideally these would only be emitted if something actually changed.
    emit q->titleChanged();
    emit q->urlChanged();
    emit q->iconChanged();
    QQuickWebEngineLoadRequest loadRequest(m_url, QQuickWebEngineView::LoadSucceededStatus);
    emit q->loadingChanged(&loadRequest);
    emit q->loadProgressChanged();

    m_isBeingAdopted = false;
}

void QQuickWebEngineViewPrivate::setFullScreenMode(bool fullscreen)
{
    Q_Q(QQuickWebEngineView);
    if (m_fullscreenMode != fullscreen) {
        m_fullscreenMode = fullscreen;
        adapter->changedFullScreen();
        Q_EMIT q->isFullScreenChanged();
    }
}

void QQuickWebEngineViewPrivate::bindViewAndWidget(QQuickWebEngineView *view,
                                                   RenderWidgetHostViewQtDelegateQuick *widget)
{
    auto oldWidget = view ? view->d_func()->widget : nullptr;
    auto oldView = widget ? widget->m_view : nullptr;

    // Change pointers first.

    if (widget && oldView != view) {
        if (oldView)
            oldView->d_func()->widget = nullptr;
        widget->m_view = view;
    }

    if (view && oldWidget != widget) {
        if (oldWidget)
            oldWidget->m_view = nullptr;
        view->d_func()->widget = widget;
    }

    // Then notify.

    if (widget && oldView != view && oldView)
        oldView->d_func()->widgetChanged(widget, nullptr);

    if (view && oldWidget != widget)
        view->d_func()->widgetChanged(oldWidget, widget);
}

void QQuickWebEngineViewPrivate::widgetChanged(RenderWidgetHostViewQtDelegateQuick *oldWidget,
                                               RenderWidgetHostViewQtDelegateQuick *newWidget)
{
    Q_Q(QQuickWebEngineView);

    if (oldWidget) {
        oldWidget->setParentItem(nullptr);
#if QT_CONFIG(accessibility)
        if (!QtWebEngineCore::closingDown())
            QAccessible::deleteAccessibleInterface(
                    QAccessible::uniqueId(QAccessible::queryAccessibleInterface(oldWidget)));
#endif
    }

    if (newWidget) {
        Q_ASSERT(!QtWebEngineCore::closingDown());
#if QT_CONFIG(accessibility)
        QAccessible::registerAccessibleInterface(new QtWebEngineCore::RenderWidgetHostViewQtDelegateQuickAccessible(newWidget, q));
#endif
        newWidget->setParentItem(q);
        newWidget->setSize(q->boundingRect().size());
        // Focus on creation if the view accepts it
        if (q->activeFocusOnPress())
            newWidget->setFocus(true);
    }
}

void QQuickWebEngineViewPrivate::updateAction(QQuickWebEngineView::WebAction action) const
{
    QQuickWebEngineAction *a = actions[action];
    if (!a)
        return;

    bool enabled = true;

    switch (action) {
    case QQuickWebEngineView::Back:
        enabled = adapter->canGoBack();
        break;
    case QQuickWebEngineView::Forward:
        enabled = adapter->canGoForward();
        break;
    case QQuickWebEngineView::Stop:
        enabled = isLoading;
        break;
    case QQuickWebEngineView::Reload:
    case QQuickWebEngineView::ReloadAndBypassCache:
        enabled = !isLoading;
        break;
    case QQuickWebEngineView::ViewSource:
        enabled = adapter->canViewSource();
        break;
    case QQuickWebEngineView::Cut:
    case QQuickWebEngineView::Copy:
    case QQuickWebEngineView::Unselect:
        enabled = adapter->hasFocusedFrame() && !adapter->selectedText().isEmpty();
        break;
    case QQuickWebEngineView::Paste:
    case QQuickWebEngineView::Undo:
    case QQuickWebEngineView::Redo:
    case QQuickWebEngineView::SelectAll:
    case QQuickWebEngineView::PasteAndMatchStyle:
        enabled = adapter->hasFocusedFrame();
        break;
    default:
        break;
    }

    a->d_ptr->setEnabled(enabled);
}

void QQuickWebEngineViewPrivate::updateNavigationActions()
{
    updateAction(QQuickWebEngineView::Back);
    updateAction(QQuickWebEngineView::Forward);
    updateAction(QQuickWebEngineView::Stop);
    updateAction(QQuickWebEngineView::Reload);
    updateAction(QQuickWebEngineView::ReloadAndBypassCache);
    updateAction(QQuickWebEngineView::ViewSource);
}

void QQuickWebEngineViewPrivate::updateEditActions()
{
    updateAction(QQuickWebEngineView::Cut);
    updateAction(QQuickWebEngineView::Copy);
    updateAction(QQuickWebEngineView::Paste);
    updateAction(QQuickWebEngineView::Undo);
    updateAction(QQuickWebEngineView::Redo);
    updateAction(QQuickWebEngineView::SelectAll);
    updateAction(QQuickWebEngineView::PasteAndMatchStyle);
    updateAction(QQuickWebEngineView::Unselect);
}

QUrl QQuickWebEngineView::url() const
{
    Q_D(const QQuickWebEngineView);
        return d->m_url;
}

void QQuickWebEngineView::setUrl(const QUrl& url)
{
    Q_D(QQuickWebEngineView);
    if (url.isEmpty())
        return;

    if (d->m_url != url) {
        d->m_url = url;
        d->m_html.clear();
        emit urlChanged();
    }

    if (d->adapter->isInitialized()) {
        d->adapter->load(url);
    }
}

QUrl QQuickWebEngineView::icon() const
{
    Q_D(const QQuickWebEngineView);
    return d->iconUrl;
}

void QQuickWebEngineView::loadHtml(const QString &html, const QUrl &baseUrl)
{
    Q_D(QQuickWebEngineView);
    d->m_url = baseUrl;
    d->m_html = html;
    if (d->adapter->isInitialized()) {
        d->adapter->setContent(html.toUtf8(), defaultMimeType, baseUrl);
        return;
    }
}

void QQuickWebEngineView::goBack()
{
    Q_D(QQuickWebEngineView);
    d->adapter->navigateBack();
}

void QQuickWebEngineView::goForward()
{
    Q_D(QQuickWebEngineView);
    d->adapter->navigateForward();
}

void QQuickWebEngineView::reload()
{
    Q_D(QQuickWebEngineView);
    d->adapter->reload();
}

void QQuickWebEngineView::reloadAndBypassCache()
{
    Q_D(QQuickWebEngineView);
    d->adapter->reloadAndBypassCache();
}

void QQuickWebEngineView::stop()
{
    Q_D(QQuickWebEngineView);
    d->adapter->stop();
}

void QQuickWebEngineView::setZoomFactor(qreal arg)
{
    Q_D(QQuickWebEngineView);
    if (d->adapter->isInitialized() && !qFuzzyCompare(arg, zoomFactor())) {
        d->adapter->setZoomFactor(arg);
        // MEMO: should reset if factor was not applied due to being invalid
        d->m_zoomFactor = zoomFactor();
        emit zoomFactorChanged(d->m_zoomFactor);
    } else {
        d->m_zoomFactor = arg;
    }
}

QQuickWebEngineProfile *QQuickWebEngineView::profile()
{
    Q_D(QQuickWebEngineView);
    d->initializeProfile();
    return d->m_profile;
}

void QQuickWebEngineView::setProfile(QQuickWebEngineProfile *profile)
{
    Q_D(QQuickWebEngineView);

    if (d->m_profile == profile)
        return;

    if (!d->profileInitialized()) {
        d->m_profile = profile;
        return;
    }

    if (d->m_profile)
        d->m_profile->d_ptr->removeWebContentsAdapterClient(d);

    d->m_profile = profile;
    d->m_profile->d_ptr->addWebContentsAdapterClient(d);
    d->m_settings->setParentSettings(profile->settings());

    d->updateAdapter();
    Q_EMIT profileChanged();
}

QQuickWebEngineSettings *QQuickWebEngineView::settings()
{
    Q_D(QQuickWebEngineView);
    d->initializeProfile();
    return d->m_settings.data();
}

QQmlListProperty<QQuickWebEngineScript> QQuickWebEngineView::userScripts()
{
    Q_D(QQuickWebEngineView);
    return QQmlListProperty<QQuickWebEngineScript>(this, d,
                                                   d->userScripts_append,
                                                   d->userScripts_count,
                                                   d->userScripts_at,
                                                   d->userScripts_clear);
}

void QQuickWebEngineViewPrivate::updateAdapter()
{
    // When the profile changes we need to create a new WebContentAdapter and reload the active URL.
    bool wasInitialized = adapter->isInitialized();
    adapter = QSharedPointer<WebContentsAdapter>::create();
    adapter->setClient(this);
    if (wasInitialized) {
        if (!m_html.isEmpty())
            adapter->setContent(m_html.toUtf8(), defaultMimeType, m_url);
        else if (m_url.isValid())
            adapter->load(m_url);
        else
            adapter->loadDefault();
    }
}

#if QT_CONFIG(webengine_testsupport)
QQuickWebEngineTestSupport *QQuickWebEngineView::testSupport() const
{
    Q_D(const QQuickWebEngineView);
    return d->m_testSupport;
}

void QQuickWebEngineView::setTestSupport(QQuickWebEngineTestSupport *testSupport)
{
    Q_D(QQuickWebEngineView);
    d->m_testSupport = testSupport;
    Q_EMIT testSupportChanged();
}
#endif

bool QQuickWebEngineView::activeFocusOnPress() const
{
    Q_D(const QQuickWebEngineView);
    return d->m_activeFocusOnPress;
}

void QQuickWebEngineViewPrivate::didRunJavaScript(quint64 requestId, const QVariant &result)
{
    Q_Q(QQuickWebEngineView);
    QJSValue callback = m_callbacks.take(requestId);
    QJSValueList args;
    args.append(qmlEngine(q)->toScriptValue(result));
    callback.call(args);
}

void QQuickWebEngineViewPrivate::didPrintPage(quint64 requestId, QSharedPointer<QByteArray> result)
{
    Q_Q(QQuickWebEngineView);
    QJSValue callback = m_callbacks.take(requestId);
    QJSValueList args;
    args.append(qmlEngine(q)->toScriptValue(*(result.data())));
    callback.call(args);
}

void QQuickWebEngineViewPrivate::didPrintPageToPdf(const QString &filePath, bool success)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->pdfPrintingFinished(filePath, success);
}

void QQuickWebEngineViewPrivate::updateScrollPosition(const QPointF &position)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->scrollPositionChanged(position);
}

void QQuickWebEngineViewPrivate::updateContentsSize(const QSizeF &size)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->contentsSizeChanged(size);
}

void QQuickWebEngineViewPrivate::renderProcessTerminated(
        RenderProcessTerminationStatus terminationStatus, int exitCode)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->renderProcessTerminated(static_cast<QQuickWebEngineView::RenderProcessTerminationStatus>(
                                      renderProcessExitStatus(terminationStatus)), exitCode);
}

void QQuickWebEngineViewPrivate::requestGeometryChange(const QRect &geometry, const QRect &frameGeometry)
{
    Q_Q(QQuickWebEngineView);
    Q_EMIT q->geometryChangeRequested(geometry, frameGeometry);
}

void QQuickWebEngineViewPrivate::startDragging(const content::DropData &dropData,
                                               Qt::DropActions allowedActions,
                                               const QPixmap &pixmap, const QPoint &offset)
{
#if !QT_CONFIG(draganddrop)
    Q_UNUSED(dropData);
    Q_UNUSED(allowedActions);
    Q_UNUSED(pixmap);
    Q_UNUSED(offset);
#else
    adapter->startDragging(q_ptr->window(), dropData, allowedActions, pixmap, offset);
#endif // QT_CONFIG(draganddrop)
}

bool QQuickWebEngineViewPrivate::supportsDragging() const
{
    // QTBUG-57516
    // Fixme: This is just a band-aid workaround.
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows);
}

bool QQuickWebEngineViewPrivate::isEnabled() const
{
    const Q_Q(QQuickWebEngineView);
    return q->isEnabled();
}

void QQuickWebEngineViewPrivate::setToolTip(const QString &toolTipText)
{
    Q_Q(QQuickWebEngineView);
    QQuickWebEngineTooltipRequest *request = new QQuickWebEngineTooltipRequest(toolTipText, q);
    // mark the object for gc by creating temporary jsvalue
    qmlEngine(q)->newQObject(request);
    Q_EMIT q->tooltipRequested(request);
    if (!request->isAccepted())
        ui()->showToolTip(toolTipText);
}

QtWebEngineCore::TouchHandleDrawableClient *QQuickWebEngineViewPrivate::createTouchHandle(const QMap<int, QImage> &images)
{
    return new QQuickWebEngineTouchHandle(ui(), images);
}

void QQuickWebEngineViewPrivate::showTouchSelectionMenu(QtWebEngineCore::TouchSelectionMenuController *menuController, const QRect &selectionBounds, const QSize &handleSize)
{
    Q_UNUSED(handleSize);

    const int kSpacingBetweenButtons = 2;
    const int kMenuButtonMinWidth = 63;
    const int kMenuButtonMinHeight = 38;

    int buttonCount = menuController->buttonCount();
    if (buttonCount == 1) {
        menuController->runContextMenu();
        return;
    }

    int width = (kSpacingBetweenButtons * (buttonCount + 1)) + (kMenuButtonMinWidth * buttonCount);
    int height = kMenuButtonMinHeight + kSpacingBetweenButtons;
    int x = (selectionBounds.x() + selectionBounds.x() + selectionBounds.width() - width) / 2;
    int y = selectionBounds.y() - height - 2;

    QRect bounds(x, y, width, height);
    ui()->showTouchSelectionMenu(menuController, bounds, kSpacingBetweenButtons);
}

void QQuickWebEngineViewPrivate::hideTouchSelectionMenu()
{
    ui()->hideTouchSelectionMenu();
}

bool QQuickWebEngineView::isLoading() const
{
    Q_D(const QQuickWebEngineView);
    return d->isLoading;
}

int QQuickWebEngineView::loadProgress() const
{
    Q_D(const QQuickWebEngineView);
    return d->loadProgress;
}

QString QQuickWebEngineView::title() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->pageTitle();
}

bool QQuickWebEngineView::canGoBack() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->canGoBack();
}

bool QQuickWebEngineView::canGoForward() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->canGoForward();
}

void QQuickWebEngineView::runJavaScript(const QString &script, const QJSValue &callback)
{
    runJavaScript(script, QQuickWebEngineScript::MainWorld, callback);
}

void QQuickWebEngineView::runJavaScript(const QString &script, quint32 worldId, const QJSValue &callback)
{
    Q_D(QQuickWebEngineView);
    d->ensureContentsAdapter();
    if (!callback.isUndefined()) {
        quint64 requestId = d_ptr->adapter->runJavaScriptCallbackResult(script, worldId);
        d->m_callbacks.insert(requestId, callback);
    } else
        d->adapter->runJavaScript(script, worldId);
}

qreal QQuickWebEngineView::zoomFactor() const
{
    Q_D(const QQuickWebEngineView);
    if (!d->adapter->isInitialized())
        return d->m_zoomFactor;
    return d->adapter->currentZoomFactor();
}

QColor QQuickWebEngineView::backgroundColor() const
{
    Q_D(const QQuickWebEngineView);
    return d->m_backgroundColor;
}

void QQuickWebEngineView::setBackgroundColor(const QColor &color)
{
    Q_D(QQuickWebEngineView);
    if (color == d->m_backgroundColor)
        return;
    d->m_backgroundColor = color;
    if (d->adapter->isInitialized()) {
        d->adapter->setBackgroundColor(color);
        emit backgroundColorChanged();
    }
}

/*!
    \property QQuickWebEngineView::audioMuted
    \brief The state of whether the current page audio is muted.
    \since 5.7

    The default value is false.
*/
bool QQuickWebEngineView::isAudioMuted() const
{
    const Q_D(QQuickWebEngineView);
    if (d->adapter->isInitialized())
        return d->adapter->isAudioMuted();
    return d->m_defaultAudioMuted;
}

void QQuickWebEngineView::setAudioMuted(bool muted)
{
    Q_D(QQuickWebEngineView);
    bool wasAudioMuted = isAudioMuted();
    d->m_defaultAudioMuted = muted;
    d->adapter->setAudioMuted(muted);
    if (wasAudioMuted != isAudioMuted())
        Q_EMIT audioMutedChanged(muted);
}

bool QQuickWebEngineView::recentlyAudible() const
{
    const Q_D(QQuickWebEngineView);
    return d->adapter->recentlyAudible();
}

qint64 QQuickWebEngineView::renderProcessPid() const
{
    const Q_D(QQuickWebEngineView);
    return d->adapter->renderProcessPid();
}

void QQuickWebEngineView::printToPdf(const QString& filePath, PrintedPageSizeId pageSizeId, PrintedPageOrientation orientation)
{
#if QT_CONFIG(webengine_printing_and_pdf)
    Q_D(QQuickWebEngineView);
    QPageSize layoutSize(static_cast<QPageSize::PageSizeId>(pageSizeId));
    QPageLayout::Orientation layoutOrientation = static_cast<QPageLayout::Orientation>(orientation);
    QPageLayout pageLayout(layoutSize, layoutOrientation, QMarginsF(0.0, 0.0, 0.0, 0.0));
    d->ensureContentsAdapter();
    d->adapter->printToPDF(pageLayout, filePath);
#else
    Q_UNUSED(filePath);
    Q_UNUSED(pageSizeId);
    Q_UNUSED(orientation);
#endif
}

void QQuickWebEngineView::printToPdf(const QJSValue &callback, PrintedPageSizeId pageSizeId, PrintedPageOrientation orientation)
{
#if QT_CONFIG(webengine_printing_and_pdf)
    Q_D(QQuickWebEngineView);
    QPageSize layoutSize(static_cast<QPageSize::PageSizeId>(pageSizeId));
    QPageLayout::Orientation layoutOrientation = static_cast<QPageLayout::Orientation>(orientation);
    QPageLayout pageLayout(layoutSize, layoutOrientation, QMarginsF(0.0, 0.0, 0.0, 0.0));

    if (callback.isUndefined())
        return;

    d->ensureContentsAdapter();
    quint64 requestId = d->adapter->printToPDFCallbackResult(pageLayout);
    d->m_callbacks.insert(requestId, callback);
#else
    Q_UNUSED(pageSizeId);
    Q_UNUSED(orientation);

    // Call back with null result.
    QJSValueList args;
    args.append(QJSValue(QByteArray().data()));
    QJSValue callbackCopy = callback;
    callbackCopy.call(args);
#endif
}

void QQuickWebEngineView::replaceMisspelledWord(const QString &replacement)
{
    Q_D(QQuickWebEngineView);
    d->adapter->replaceMisspelling(replacement);
}

bool QQuickWebEngineView::isFullScreen() const
{
    Q_D(const QQuickWebEngineView);
    return d->m_fullscreenMode;
}

void QQuickWebEngineView::findText(const QString &subString, FindFlags options, const QJSValue &callback)
{
    Q_D(QQuickWebEngineView);
    if (!d->adapter->isInitialized())
        return;

    d->adapter->findTextHelper()->startFinding(subString, options & FindCaseSensitively, options & FindBackward, callback);
}

QQuickWebEngineHistory *QQuickWebEngineView::navigationHistory() const
{
    Q_D(const QQuickWebEngineView);
    return d->m_history.data();
}

QQmlWebChannel *QQuickWebEngineView::webChannel()
{
#if QT_CONFIG(webengine_webchannel)
    Q_D(QQuickWebEngineView);
    if (!d->m_webChannel) {
        d->m_webChannel = new QQmlWebChannel(this);
    }
    return d->m_webChannel;
#endif
    qWarning("WebEngine compiled without webchannel support");
    return nullptr;
}

void QQuickWebEngineView::setWebChannel(QQmlWebChannel *webChannel)
{
#if QT_CONFIG(webengine_webchannel)
    Q_D(QQuickWebEngineView);
    if (d->m_webChannel == webChannel)
        return;
    d->m_webChannel = webChannel;
    if (d->profileInitialized())
        d->adapter->setWebChannel(webChannel, d->m_webChannelWorld);
    Q_EMIT webChannelChanged();
#else
    Q_UNUSED(webChannel)
    qWarning("WebEngine compiled without webchannel support");
#endif
}

uint QQuickWebEngineView::webChannelWorld() const
{
    Q_D(const QQuickWebEngineView);
    return d->m_webChannelWorld;
}

void QQuickWebEngineView::setWebChannelWorld(uint webChannelWorld)
{
#if QT_CONFIG(webengine_webchannel)
    Q_D(QQuickWebEngineView);
    if (d->m_webChannelWorld == webChannelWorld)
        return;
    d->m_webChannelWorld = webChannelWorld;
    if (d->profileInitialized())
        d->adapter->setWebChannel(d->m_webChannel, d->m_webChannelWorld);
    Q_EMIT webChannelWorldChanged(webChannelWorld);
#else
    Q_UNUSED(webChannelWorld)
    qWarning("WebEngine compiled without webchannel support");
#endif
}

QQuickWebEngineView *QQuickWebEngineView::inspectedView() const
{
    Q_D(const QQuickWebEngineView);
    return d->inspectedView;
}

void QQuickWebEngineView::setInspectedView(QQuickWebEngineView *view)
{
    Q_D(QQuickWebEngineView);
    if (d->inspectedView == view)
        return;
    QQuickWebEngineView *oldView = d->inspectedView;
    d->inspectedView = nullptr;
    if (oldView)
        oldView->setDevToolsView(nullptr);
    d->inspectedView = view;
    if (view)
        view->setDevToolsView(this);
    Q_EMIT inspectedViewChanged();
}

QQuickWebEngineView *QQuickWebEngineView::devToolsView() const
{
    Q_D(const QQuickWebEngineView);
    return d->devToolsView;
}


void QQuickWebEngineView::setDevToolsView(QQuickWebEngineView *devToolsView)
{
    Q_D(QQuickWebEngineView);
    if (d->devToolsView == devToolsView)
        return;
    QQuickWebEngineView *oldView = d->devToolsView;
    d->devToolsView = nullptr;
    if (oldView)
        oldView->setInspectedView(nullptr);
    d->devToolsView = devToolsView;
    if (devToolsView)
        devToolsView->setInspectedView(this);
    if (d->profileInitialized() && d->adapter->isInitialized()) {
        if (devToolsView)
            d->adapter->openDevToolsFrontend(devToolsView->d_ptr->adapter);
        else
            d->adapter->closeDevToolsFrontend();
    }
    Q_EMIT devToolsViewChanged();
}

void QQuickWebEngineView::grantFeaturePermission(const QUrl &securityOrigin, QQuickWebEngineView::Feature feature, bool granted)
{
    if (!granted && ((feature >= MediaAudioCapture && feature <= MediaAudioVideoCapture) ||
                     (feature >= DesktopVideoCapture && feature <= DesktopAudioVideoCapture))) {
         d_ptr->adapter->grantMediaAccessPermission(securityOrigin, WebContentsAdapterClient::MediaNone);
         return;
    }

    switch (feature) {
    case MediaAudioCapture:
        d_ptr->adapter->grantMediaAccessPermission(securityOrigin, WebContentsAdapterClient::MediaAudioCapture);
        break;
    case MediaVideoCapture:
        d_ptr->adapter->grantMediaAccessPermission(securityOrigin, WebContentsAdapterClient::MediaVideoCapture);
        break;
    case MediaAudioVideoCapture:
        d_ptr->adapter->grantMediaAccessPermission(securityOrigin, WebContentsAdapterClient::MediaRequestFlags(WebContentsAdapterClient::MediaAudioCapture                                                                                                               | WebContentsAdapterClient::MediaVideoCapture));
        break;
    case DesktopVideoCapture:
        d_ptr->adapter->grantMediaAccessPermission(securityOrigin, WebContentsAdapterClient::MediaDesktopVideoCapture);
        break;
    case DesktopAudioVideoCapture:
        d_ptr->adapter->grantMediaAccessPermission(
            securityOrigin,
            WebContentsAdapterClient::MediaRequestFlags(
                WebContentsAdapterClient::MediaDesktopAudioCapture |
                WebContentsAdapterClient::MediaDesktopVideoCapture));
        break;
    case Geolocation:
        d_ptr->adapter->grantFeaturePermission(securityOrigin, ProfileAdapter::GeolocationPermission,
                                               granted ? ProfileAdapter::AllowedPermission : ProfileAdapter::DeniedPermission);
        break;
    case Notifications:
        d_ptr->adapter->grantFeaturePermission(securityOrigin, ProfileAdapter::NotificationPermission,
                                               granted ? ProfileAdapter::AllowedPermission : ProfileAdapter::DeniedPermission);
        break;
    default:
        Q_UNREACHABLE();
    }
}

void QQuickWebEngineView::setActiveFocusOnPress(bool arg)
{
    Q_D(QQuickWebEngineView);
    if (d->m_activeFocusOnPress == arg)
        return;

    d->m_activeFocusOnPress = arg;
    emit activeFocusOnPressChanged(arg);
}

void QQuickWebEngineView::goBackOrForward(int offset)
{
    Q_D(QQuickWebEngineView);
    const int current = d->adapter->currentNavigationEntryIndex();
    const int count = d->adapter->navigationEntryCount();
    const int index = current + offset;

    if (index < 0 || index >= count)
        return;

    d->adapter->navigateToIndex(index);
}

void QQuickWebEngineView::fullScreenCancelled()
{
    Q_D(QQuickWebEngineView);
    d->adapter->exitFullScreen();
}

void QQuickWebEngineView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    Q_D(QQuickWebEngineView);
    if (d->widget)
        d->widget->setSize(newGeometry.size());
}

void QQuickWebEngineView::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickWebEngineView);
    if (d && d->profileInitialized() && d->adapter->isInitialized()
            && (change == ItemSceneChange || change == ItemVisibleHasChanged)) {
        if (window())
            d->adapter->setVisible(isVisible());
    }
    QQuickItem::itemChange(change, value);
}

#if QT_CONFIG(draganddrop)
static QPoint mapToScreen(const QQuickItem *item, const QPoint &clientPos)
{
    return item->window()->position() + item->mapToScene(clientPos).toPoint();
}

void QQuickWebEngineView::dragEnterEvent(QDragEnterEvent *e)
{
    Q_D(QQuickWebEngineView);
    e->accept();
    d->adapter->enterDrag(e, mapToScreen(this, e->pos()));
}

void QQuickWebEngineView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_D(QQuickWebEngineView);
    e->accept();
    d->adapter->leaveDrag();
}

void QQuickWebEngineView::dragMoveEvent(QDragMoveEvent *e)
{
    Q_D(QQuickWebEngineView);
    Qt::DropAction dropAction = d->adapter->updateDragPosition(e, mapToScreen(this, e->pos()));
    if (Qt::IgnoreAction == dropAction) {
        e->ignore();
    } else {
        e->setDropAction(dropAction);
        e->accept();
    }
}

void QQuickWebEngineView::dropEvent(QDropEvent *e)
{
    Q_D(QQuickWebEngineView);
    e->accept();
    d->adapter->endDragging(e, mapToScreen(this, e->pos()));
}
#endif // QT_CONFIG(draganddrop)

void QQuickWebEngineView::triggerWebAction(WebAction action)
{
    Q_D(QQuickWebEngineView);
    switch (action) {
    case Back:
        d->adapter->navigateBack();
        break;
    case Forward:
        d->adapter->navigateForward();
        break;
    case Stop:
        d->adapter->stop();
        break;
    case Reload:
        d->adapter->reload();
        break;
    case ReloadAndBypassCache:
        d->adapter->reloadAndBypassCache();
        break;
    case Cut:
        d->adapter->cut();
        break;
    case Copy:
        d->adapter->copy();
        break;
    case Paste:
        d->adapter->paste();
        break;
    case Undo:
        d->adapter->undo();
        break;
    case Redo:
        d->adapter->redo();
        break;
    case SelectAll:
        d->adapter->selectAll();
        break;
    case PasteAndMatchStyle:
        d->adapter->pasteAndMatchStyle();
        break;
    case Unselect:
        d->adapter->unselect();
        break;
    case OpenLinkInThisWindow:
        if (d->m_contextMenuData.linkUrl().isValid())
            setUrl(d->m_contextMenuData.linkUrl());
        break;
    case OpenLinkInNewWindow:
        if (d->m_contextMenuData.linkUrl().isValid()) {
            QQuickWebEngineNewViewRequest request;
            request.m_requestedUrl = d->m_contextMenuData.linkUrl();
            request.m_isUserInitiated = true;
            request.m_destination = NewViewInWindow;
            Q_EMIT newViewRequested(&request);
        }
        break;
    case OpenLinkInNewTab:
        if (d->m_contextMenuData.linkUrl().isValid()) {
            QQuickWebEngineNewViewRequest request;
            request.m_requestedUrl = d->m_contextMenuData.linkUrl();
            request.m_isUserInitiated = true;
            request.m_destination = NewViewInBackgroundTab;
            Q_EMIT newViewRequested(&request);
        }
        break;
    case CopyLinkToClipboard:
        if (!d->m_contextMenuData.unfilteredLinkUrl().isEmpty()) {
            QString urlString = d->m_contextMenuData.unfilteredLinkUrl().toString(QUrl::FullyEncoded);
            QString linkText = d->m_contextMenuData.linkText().toHtmlEscaped();
            QString title = d->m_contextMenuData.titleText();
            if (!title.isEmpty())
                title = QStringLiteral(" title=\"%1\"").arg(title.toHtmlEscaped());
            QMimeData *data = new QMimeData();
            data->setText(urlString);
            QString html = QStringLiteral("<a href=\"") + urlString + QStringLiteral("\"") + title + QStringLiteral(">")
                         + linkText + QStringLiteral("</a>");
            data->setHtml(html);
            data->setUrls(QList<QUrl>() << d->m_contextMenuData.unfilteredLinkUrl());
            qApp->clipboard()->setMimeData(data);
        }
        break;
    case DownloadLinkToDisk:
        if (d->m_contextMenuData.linkUrl().isValid())
            d->adapter->download(d->m_contextMenuData.linkUrl(), d->m_contextMenuData.suggestedFileName(),
                                 d->m_contextMenuData.referrerUrl(), d->m_contextMenuData.referrerPolicy());
        break;
    case CopyImageToClipboard:
        if (d->m_contextMenuData.hasImageContent() &&
                (d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeImage ||
                 d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeCanvas))
        {
            d->adapter->copyImageAt(d->m_contextMenuData.position());
        }
        break;
    case CopyImageUrlToClipboard:
        if (d->m_contextMenuData.mediaUrl().isValid() && d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeImage) {
            QString urlString = d->m_contextMenuData.mediaUrl().toString(QUrl::FullyEncoded);
            QString alt = d->m_contextMenuData.altText();
            if (!alt.isEmpty())
                alt = QStringLiteral(" alt=\"%1\"").arg(alt.toHtmlEscaped());
            QString title = d->m_contextMenuData.titleText();
            if (!title.isEmpty())
                title = QStringLiteral(" title=\"%1\"").arg(title.toHtmlEscaped());
            QMimeData *data = new QMimeData();
            data->setText(urlString);
            QString html = QStringLiteral("<img src=\"") + urlString + QStringLiteral("\"") + title + alt + QStringLiteral("></img>");
            data->setHtml(html);
            data->setUrls(QList<QUrl>() << d->m_contextMenuData.mediaUrl());
            qApp->clipboard()->setMimeData(data);
        }
        break;
    case DownloadImageToDisk:
    case DownloadMediaToDisk:
        if (d->m_contextMenuData.mediaUrl().isValid())
            d->adapter->download(d->m_contextMenuData.mediaUrl(), d->m_contextMenuData.suggestedFileName(),
                                 d->m_contextMenuData.referrerUrl(), d->m_contextMenuData.referrerPolicy());
        break;
    case CopyMediaUrlToClipboard:
        if (d->m_contextMenuData.mediaUrl().isValid() &&
                (d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeAudio ||
                 d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeVideo))
        {
            QString urlString = d->m_contextMenuData.mediaUrl().toString(QUrl::FullyEncoded);
            QString title = d->m_contextMenuData.titleText();
            if (!title.isEmpty())
                title = QStringLiteral(" title=\"%1\"").arg(title.toHtmlEscaped());
            QMimeData *data = new QMimeData();
            data->setText(urlString);
            if (d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeAudio)
                data->setHtml(QStringLiteral("<audio src=\"") + urlString + QStringLiteral("\"") + title +
                              QStringLiteral("></audio>"));
            else
                data->setHtml(QStringLiteral("<video src=\"") + urlString + QStringLiteral("\"") + title +
                              QStringLiteral("></video>"));
            data->setUrls(QList<QUrl>() << d->m_contextMenuData.mediaUrl());
            qApp->clipboard()->setMimeData(data);
        }
        break;
    case ToggleMediaControls:
        if (d->m_contextMenuData.mediaUrl().isValid() && d->m_contextMenuData.mediaFlags() & WebEngineContextMenuData::MediaCanToggleControls) {
            bool enable = !(d->m_contextMenuData.mediaFlags() & WebEngineContextMenuData::MediaControls);
            d->adapter->executeMediaPlayerActionAt(d->m_contextMenuData.position(), WebContentsAdapter::MediaPlayerControls, enable);
        }
        break;
    case ToggleMediaLoop:
        if (d->m_contextMenuData.mediaUrl().isValid() &&
                (d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeAudio ||
                 d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeVideo))
        {
            bool enable = !(d->m_contextMenuData.mediaFlags() & WebEngineContextMenuData::MediaLoop);
            d->adapter->executeMediaPlayerActionAt(d->m_contextMenuData.position(), WebContentsAdapter::MediaPlayerLoop, enable);
        }
        break;
    case ToggleMediaPlayPause:
        if (d->m_contextMenuData.mediaUrl().isValid() &&
                (d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeAudio ||
                 d->m_contextMenuData.mediaType() == WebEngineContextMenuData::MediaTypeVideo))
        {
            bool enable = (d->m_contextMenuData.mediaFlags() & WebEngineContextMenuData::MediaPaused);
            d->adapter->executeMediaPlayerActionAt(d->m_contextMenuData.position(), WebContentsAdapter::MediaPlayerPlay, enable);
        }
        break;
    case ToggleMediaMute:
        if (d->m_contextMenuData.mediaUrl().isValid() && d->m_contextMenuData.mediaFlags() & WebEngineContextMenuData::MediaHasAudio) {
            bool enable = !(d->m_contextMenuData.mediaFlags() & WebEngineContextMenuData::MediaMuted);
            d->adapter->executeMediaPlayerActionAt(d->m_contextMenuData.position(), WebContentsAdapter::MediaPlayerMute, enable);
        }
        break;
    case InspectElement:
        d->adapter->inspectElementAt(d->m_contextMenuData.position());
        break;
    case ExitFullScreen:
        d->adapter->exitFullScreen();
        break;
    case RequestClose:
        d->adapter->requestClose();
        break;
    case SavePage:
        d->adapter->save();
        break;
    case ViewSource:
        d->adapter->viewSource();
        break;
    case ToggleBold:
        runJavaScript(QStringLiteral("document.execCommand('bold');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case ToggleItalic:
        runJavaScript(QStringLiteral("document.execCommand('italic');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case ToggleUnderline:
        runJavaScript(QStringLiteral("document.execCommand('underline');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case ToggleStrikethrough:
        runJavaScript(QStringLiteral("document.execCommand('strikethrough');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case AlignLeft:
        runJavaScript(QStringLiteral("document.execCommand('justifyLeft');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case AlignCenter:
        runJavaScript(QStringLiteral("document.execCommand('justifyCenter');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case AlignRight:
        runJavaScript(QStringLiteral("document.execCommand('justifyRight');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case AlignJustified:
        runJavaScript(QStringLiteral("document.execCommand('justifyFull');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case Indent:
        runJavaScript(QStringLiteral("document.execCommand('indent');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case Outdent:
        runJavaScript(QStringLiteral("document.execCommand('outdent');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case InsertOrderedList:
        runJavaScript(QStringLiteral("document.execCommand('insertOrderedList');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    case InsertUnorderedList:
        runJavaScript(QStringLiteral("document.execCommand('insertUnorderedList');"), QQuickWebEngineScript::ApplicationWorld);
        break;
    default:
        Q_UNREACHABLE();
    }
}

QQuickWebEngineAction *QQuickWebEngineView::action(WebAction action)
{
    Q_D(QQuickWebEngineView);
    if (action == QQuickWebEngineView::NoWebAction)
        return nullptr;
    if (d->actions[action]) {
        d->updateAction(action);
        return d->actions[action];
    }

    QString text;
    QString iconName;

    switch (action) {
    case Back:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Back);
        iconName = QStringLiteral("go-previous");
        break;
    case Forward:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Forward);
        iconName = QStringLiteral("go-next");
        break;
    case Stop:
        text = tr("Stop");
        iconName = QStringLiteral("process-stop");
        break;
    case Reload:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Reload);
        iconName = QStringLiteral("view-refresh");
        break;
    case ReloadAndBypassCache:
        text = tr("Reload and Bypass Cache");
        iconName = QStringLiteral("view-refresh");
        break;
    case Cut:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Cut);
        iconName = QStringLiteral("edit-cut");
        break;
    case Copy:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Copy);
        iconName = QStringLiteral("edit-copy");
        break;
    case Paste:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Paste);
        iconName = QStringLiteral("edit-paste");
        break;
    case Undo:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Undo);
        iconName = QStringLiteral("edit-undo");
        break;
    case Redo:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::Redo);
        iconName = QStringLiteral("edit-redo");
        break;
    case SelectAll:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::SelectAll);
        iconName = QStringLiteral("edit-select-all");
        break;
    case PasteAndMatchStyle:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::PasteAndMatchStyle);
        iconName = QStringLiteral("edit-paste");
        break;
    case OpenLinkInThisWindow:
        text = tr("Open link in this window");
        break;
    case OpenLinkInNewWindow:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::OpenLinkInNewWindow);
        break;
    case OpenLinkInNewTab:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::OpenLinkInNewTab);
        break;
    case CopyLinkToClipboard:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::CopyLinkToClipboard);
        break;
    case DownloadLinkToDisk:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::DownloadLinkToDisk);
        break;
    case CopyImageToClipboard:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::CopyImageToClipboard);
        break;
    case CopyImageUrlToClipboard:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::CopyImageUrlToClipboard);
        break;
    case DownloadImageToDisk:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::DownloadImageToDisk);
        break;
    case CopyMediaUrlToClipboard:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::CopyMediaUrlToClipboard);
        break;
    case ToggleMediaControls:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::ToggleMediaControls);
        break;
    case ToggleMediaLoop:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::ToggleMediaLoop);
        break;
    case ToggleMediaPlayPause:
        text = tr("Toggle Play/Pause");
        iconName = QStringLiteral("media-playback-start");
        break;
    case ToggleMediaMute:
        text = tr("Toggle Mute");
        iconName = QStringLiteral("audio-volume-muted");
        break;
    case DownloadMediaToDisk:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::DownloadMediaToDisk);
        break;
    case InspectElement:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::InspectElement);
        break;
    case ExitFullScreen:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::ExitFullScreen);
        iconName = QStringLiteral("view-fullscreen");
        break;
    case RequestClose:
        text = tr("Close Page");
        iconName = QStringLiteral("window-close");
        break;
    case Unselect:
        text = tr("Unselect");
        iconName = QStringLiteral("edit-select-none");
        break;
    case SavePage:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::SavePage);
        iconName = QStringLiteral("document-save");
        break;
    case ViewSource:
        text = RenderViewContextMenuQt::getMenuItemName(RenderViewContextMenuQt::ContextMenuItem::ViewSource);
        break;
    case ToggleBold:
        text = tr("&Bold");
        iconName = QStringLiteral("format-text-bold");
        break;
    case ToggleItalic:
        text = tr("&Italic");
        iconName = QStringLiteral("format-text-italic");
        break;
    case ToggleUnderline:
        text = tr("&Underline");
        iconName = QStringLiteral("format-text-underline");
        break;
    case ToggleStrikethrough:
        text = tr("&Strikethrough");
        iconName = QStringLiteral("format-text-strikethrough");
        break;
    case AlignLeft:
        text = tr("Align &Left");
        break;
    case AlignCenter:
        text = tr("Align &Center");
        break;
    case AlignRight:
        text = tr("Align &Right");
        break;
    case AlignJustified:
        text = tr("Align &Justified");
        break;
    case Indent:
        text = tr("&Indent");
        iconName = QStringLiteral("format-indent-more");
        break;
    case Outdent:
        text = tr("&Outdent");
        iconName = QStringLiteral("format-indent-less");
        break;
    case InsertOrderedList:
        text = tr("Insert &Ordered List");
        break;
    case InsertUnorderedList:
        text = tr("Insert &Unordered List");
        break;
    case NoWebAction:
    case WebActionCount:
        Q_UNREACHABLE();
        break;
    }

    QQuickWebEngineAction *retVal = new QQuickWebEngineAction(action, text, iconName, false, this);

    d->actions[action] = retVal;
    d->updateAction(action);
    return retVal;
}

QSizeF QQuickWebEngineView::contentsSize() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->lastContentsSize();
}

QPointF QQuickWebEngineView::scrollPosition() const
{
    Q_D(const QQuickWebEngineView);
    return d->adapter->lastScrollOffset();
}

void QQuickWebEngineViewPrivate::userScripts_append(QQmlListProperty<QQuickWebEngineScript> *p, QQuickWebEngineScript *script)
{
    Q_ASSERT(p && p->data);
    QQuickWebEngineViewPrivate *d = static_cast<QQuickWebEngineViewPrivate*>(p->data);
    d->m_userScripts.append(script);
    // If the adapter hasn't been initialized, we'll bind the scripts in initializationFinished()
    if (!d->adapter->isInitialized())
        return;
    UserResourceControllerHost *resourceController = d->profileAdapter()->userResourceController();
    script->d_func()->bind(resourceController, d->adapter.data());
}

int QQuickWebEngineViewPrivate::userScripts_count(QQmlListProperty<QQuickWebEngineScript> *p)
{
    Q_ASSERT(p && p->data);
    QQuickWebEngineViewPrivate *d = static_cast<QQuickWebEngineViewPrivate*>(p->data);
    return d->m_userScripts.count();
}

QQuickWebEngineScript *QQuickWebEngineViewPrivate::userScripts_at(QQmlListProperty<QQuickWebEngineScript> *p, int idx)
{
    Q_ASSERT(p && p->data);
    QQuickWebEngineViewPrivate *d = static_cast<QQuickWebEngineViewPrivate*>(p->data);
    return d->m_userScripts.at(idx);
}

void QQuickWebEngineViewPrivate::userScripts_clear(QQmlListProperty<QQuickWebEngineScript> *p)
{
    Q_ASSERT(p && p->data);
    QQuickWebEngineViewPrivate *d = static_cast<QQuickWebEngineViewPrivate*>(p->data);
    d->m_userScripts.clear();
    if (!d->adapter->isInitialized())
        return;
    UserResourceControllerHost *resourceController = d->profileAdapter()->userResourceController();
    resourceController->clearAllScripts(d->adapter.data());
}

void QQuickWebEngineView::componentComplete()
{
    QQuickItem::componentComplete();
    Q_D(QQuickWebEngineView);
    d->initializeProfile();
#ifndef QT_NO_ACCESSIBILITY
    // Enable accessibility via a dynamic QQmlProperty, instead of using private API call
    // QQuickAccessibleAttached::qmlAttachedProperties(this). The qmlContext is required, otherwise
    // it is not possible to reference attached properties.
    QQmlContext *qmlContext = QQmlEngine::contextForObject(this);
    QQmlProperty role(this, QStringLiteral("Accessible.role"), qmlContext);
    role.write(QAccessible::Grouping);
#endif // QT_NO_ACCESSIBILITY

    QTimer::singleShot(0, this, &QQuickWebEngineView::lazyInitialize);
}

void QQuickWebEngineView::lazyInitialize()
{
    Q_D(QQuickWebEngineView);
    d->ensureContentsAdapter();
}

QQuickWebEngineView::LifecycleState QQuickWebEngineView::lifecycleState() const
{
    Q_D(const QQuickWebEngineView);
    return static_cast<LifecycleState>(d->adapter->lifecycleState());
}

void QQuickWebEngineView::setLifecycleState(LifecycleState state)
{
    Q_D(QQuickWebEngineView);
    d->adapter->setLifecycleState(static_cast<WebContentsAdapterClient::LifecycleState>(state));
}

QQuickWebEngineView::LifecycleState QQuickWebEngineView::recommendedState() const
{
    Q_D(const QQuickWebEngineView);
    return static_cast<LifecycleState>(d->adapter->recommendedState());
}

QQuickWebEngineFullScreenRequest::QQuickWebEngineFullScreenRequest()
    : m_viewPrivate(0)
    , m_toggleOn(false)
{
}

QQuickWebEngineFullScreenRequest::QQuickWebEngineFullScreenRequest(QQuickWebEngineViewPrivate *viewPrivate, const QUrl &origin, bool toggleOn)
    : m_viewPrivate(viewPrivate)
    , m_origin(origin)
    , m_toggleOn(toggleOn)
{
}

void QQuickWebEngineFullScreenRequest::accept()
{
    if (m_viewPrivate)
        m_viewPrivate->setFullScreenMode(m_toggleOn);
}

void QQuickWebEngineFullScreenRequest::reject()
{
    if (m_viewPrivate)
        m_viewPrivate->setFullScreenMode(!m_toggleOn);
}

QQuickContextMenuBuilder::QQuickContextMenuBuilder(const QtWebEngineCore::WebEngineContextMenuData &data,
                                                   QQuickWebEngineView *view,
                                                   QObject *menu)
    : QtWebEngineCore::RenderViewContextMenuQt(data)
    , m_view(view)
    , m_menu(menu)
{
}

void QQuickContextMenuBuilder::appendExtraItems(QQmlEngine *engine)
{
    m_view->d_ptr->ui()->addMenuSeparator(m_menu);
    if (QObject *menuExtras = m_view->d_ptr->contextMenuExtraItems->create(qmlContext(m_view))) {
        menuExtras->setParent(m_menu);
        QQmlListReference entries(m_menu, defaultPropertyName(m_menu), engine);
        if (entries.isValid())
            entries.append(menuExtras);
    }
}

bool QQuickContextMenuBuilder::hasInspector()
{
    return m_view->d_ptr->adapter->hasInspector();
}

bool QQuickContextMenuBuilder::isFullScreenMode()
{
    return m_view->d_ptr->isFullScreenMode();
}

void QQuickContextMenuBuilder::addMenuItem(ContextMenuItem menuItem)
{
    QQuickWebEngineAction *action = nullptr;

    switch (menuItem) {
    case ContextMenuItem::Back:
        action = m_view->action(QQuickWebEngineView::Back);
        break;
    case ContextMenuItem::Forward:
        action = m_view->action(QQuickWebEngineView::Forward);
        break;
    case ContextMenuItem::Reload:
        action = m_view->action(QQuickWebEngineView::Reload);
        break;
    case ContextMenuItem::Cut:
        action = m_view->action(QQuickWebEngineView::Cut);
        break;
    case ContextMenuItem::Copy:
        action = m_view->action(QQuickWebEngineView::Copy);
        break;
    case ContextMenuItem::Paste:
        action = m_view->action(QQuickWebEngineView::Paste);
        break;
    case ContextMenuItem::Undo:
        action = m_view->action(QQuickWebEngineView::Undo);
        break;
    case ContextMenuItem::Redo:
        action = m_view->action(QQuickWebEngineView::Redo);
        break;
    case ContextMenuItem::SelectAll:
        action = m_view->action(QQuickWebEngineView::SelectAll);
        break;
    case ContextMenuItem::PasteAndMatchStyle:
        action = m_view->action(QQuickWebEngineView::PasteAndMatchStyle);
        break;
    case ContextMenuItem::OpenLinkInNewWindow:
        action = m_view->action(QQuickWebEngineView::OpenLinkInNewWindow);
        break;
    case ContextMenuItem::OpenLinkInNewTab:
        action = m_view->action(QQuickWebEngineView::OpenLinkInNewTab);
        break;
    case ContextMenuItem::CopyLinkToClipboard:
        action = m_view->action(QQuickWebEngineView::CopyLinkToClipboard);
        break;
    case ContextMenuItem::DownloadLinkToDisk:
        action = m_view->action(QQuickWebEngineView::DownloadLinkToDisk);
        break;
    case ContextMenuItem::CopyImageToClipboard:
        action = m_view->action(QQuickWebEngineView::CopyImageToClipboard);
        break;
    case ContextMenuItem::CopyImageUrlToClipboard:
        action = m_view->action(QQuickWebEngineView::CopyImageUrlToClipboard);
        break;
    case ContextMenuItem::DownloadImageToDisk:
        action = m_view->action(QQuickWebEngineView::DownloadImageToDisk);
        break;
    case ContextMenuItem::CopyMediaUrlToClipboard:
        action = m_view->action(QQuickWebEngineView::CopyMediaUrlToClipboard);
        break;
    case ContextMenuItem::ToggleMediaControls:
        action = m_view->action(QQuickWebEngineView::ToggleMediaControls);
        break;
    case ContextMenuItem::ToggleMediaLoop:
        action = m_view->action(QQuickWebEngineView::ToggleMediaLoop);
        break;
    case ContextMenuItem::DownloadMediaToDisk:
        action = m_view->action(QQuickWebEngineView::DownloadMediaToDisk);
        break;
    case ContextMenuItem::InspectElement:
        action = m_view->action(QQuickWebEngineView::InspectElement);
        break;
    case ContextMenuItem::ExitFullScreen:
        action = m_view->action(QQuickWebEngineView::ExitFullScreen);
        break;
    case ContextMenuItem::SavePage:
        action = m_view->action(QQuickWebEngineView::SavePage);
        break;
    case ContextMenuItem::ViewSource:
        action = m_view->action(QQuickWebEngineView::ViewSource);
        break;
    case ContextMenuItem::SpellingSuggestions:
    {
        QPointer<QQuickWebEngineView> thisRef(m_view);
        for (int i=0; i < m_contextData.spellCheckerSuggestions().count() && i < 4; i++) {
            action = new QQuickWebEngineAction(m_menu);
            QString replacement = m_contextData.spellCheckerSuggestions().at(i);
            QObject::connect(action, &QQuickWebEngineAction::triggered, [thisRef, replacement] { thisRef->replaceMisspelledWord(replacement); });
            m_view->d_ptr->ui()->addMenuItem(action, m_menu);
        }
        return;
    }
    case ContextMenuItem::Separator:
        m_view->d_ptr->ui()->addMenuSeparator(m_menu);
        return;
    }
    // Set enabled property directly with avoiding binding loops caused by its notifier signal.
    action->d_ptr->m_enabled = isMenuItemEnabled(menuItem);
    m_view->d_ptr->ui()->addMenuItem(action, m_menu);
}

bool QQuickContextMenuBuilder::isMenuItemEnabled(ContextMenuItem menuItem)
{
    switch (menuItem) {
    case ContextMenuItem::Back:
        return m_view->canGoBack();
    case ContextMenuItem::Forward:
        return m_view->canGoForward();
    case ContextMenuItem::Reload:
        return true;
    case ContextMenuItem::Cut:
        return m_contextData.editFlags() & QtWebEngineCore::WebEngineContextMenuData::CanCut;
    case ContextMenuItem::Copy:
        return m_contextData.editFlags() & QtWebEngineCore::WebEngineContextMenuData::CanCopy;
    case ContextMenuItem::Paste:
        return m_contextData.editFlags() & QtWebEngineCore::WebEngineContextMenuData::CanPaste;
    case ContextMenuItem::Undo:
        return m_contextData.editFlags() & QtWebEngineCore::WebEngineContextMenuData::CanUndo;
    case ContextMenuItem::Redo:
        return m_contextData.editFlags() & QtWebEngineCore::WebEngineContextMenuData::CanRedo;
    case ContextMenuItem::SelectAll:
        return m_contextData.editFlags() & QtWebEngineCore::WebEngineContextMenuData::CanSelectAll;
    case ContextMenuItem::PasteAndMatchStyle:
        return m_contextData.editFlags() & QtWebEngineCore::WebEngineContextMenuData::CanPaste;
    case ContextMenuItem::OpenLinkInNewWindow:
    case ContextMenuItem::OpenLinkInNewTab:
    case ContextMenuItem::CopyLinkToClipboard:
    case ContextMenuItem::DownloadLinkToDisk:
    case ContextMenuItem::CopyImageToClipboard:
    case ContextMenuItem::CopyImageUrlToClipboard:
    case ContextMenuItem::DownloadImageToDisk:
    case ContextMenuItem::CopyMediaUrlToClipboard:
    case ContextMenuItem::ToggleMediaControls:
    case ContextMenuItem::ToggleMediaLoop:
    case ContextMenuItem::DownloadMediaToDisk:
    case ContextMenuItem::InspectElement:
    case ContextMenuItem::ExitFullScreen:
    case ContextMenuItem::SavePage:
        return true;
    case ContextMenuItem::ViewSource:
        return m_view->d_ptr->adapter->canViewSource();
    case ContextMenuItem::SpellingSuggestions:
    case ContextMenuItem::Separator:
        return true;
    }
    Q_UNREACHABLE();
}


QQuickWebEngineTouchHandle::QQuickWebEngineTouchHandle(QtWebEngineCore::UIDelegatesManager *ui, const QMap<int, QImage> &images)
{
    Q_ASSERT(ui);
    m_item.reset(ui->createTouchHandle());

    QQmlEngine *engine = qmlEngine(m_item.data());
    Q_ASSERT(engine);
    QQuickWebEngineTouchHandleProvider *touchHandleProvider =
            static_cast<QQuickWebEngineTouchHandleProvider *>(engine->imageProvider(QQuickWebEngineTouchHandleProvider::identifier()));
    Q_ASSERT(touchHandleProvider);
    touchHandleProvider->init(images);
}

void QQuickWebEngineTouchHandle::setImage(int orientation)
{
    QUrl url = QQuickWebEngineTouchHandleProvider::url(orientation);
    m_item->setProperty("source", url);
}

void QQuickWebEngineTouchHandle::setBounds(const QRect &bounds)
{
    m_item->setProperty("x", bounds.x());
    m_item->setProperty("y", bounds.y());
    m_item->setProperty("width", bounds.width());
    m_item->setProperty("height", bounds.height());
}

void QQuickWebEngineTouchHandle::setVisible(bool visible)
{
    m_item->setProperty("visible", visible);
}

void QQuickWebEngineTouchHandle::setOpacity(float opacity)
{
    m_item->setProperty("opacity", opacity);
}

QT_END_NAMESPACE

