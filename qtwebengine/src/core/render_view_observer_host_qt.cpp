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

#include "render_view_observer_host_qt.h"

#include "common/qt_messages.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

#include "render_widget_host_view_qt.h"
#include "type_conversion.h"
#include "web_contents_adapter_client.h"

namespace QtWebEngineCore {

RenderViewObserverHostQt::RenderViewObserverHostQt(content::WebContents *webContents, WebContentsAdapterClient *adapterClient)
    : content::WebContentsObserver(webContents)
    , m_adapterClient(adapterClient)
{
}

void RenderViewObserverHostQt::fetchDocumentMarkup(quint64 requestId)
{
    web_contents()->GetRenderViewHost()->Send(
                new RenderViewObserverQt_FetchDocumentMarkup(
                        web_contents()->GetRenderViewHost()->GetRoutingID(), requestId));
}

void RenderViewObserverHostQt::fetchDocumentInnerText(quint64 requestId)
{
    web_contents()->GetRenderViewHost()->Send(
                new RenderViewObserverQt_FetchDocumentInnerText(
                        web_contents()->GetRenderViewHost()->GetRoutingID(), requestId));
}

bool RenderViewObserverHostQt::OnMessageReceived(const IPC::Message& message)
{
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(RenderViewObserverHostQt, message)
        IPC_MESSAGE_HANDLER(RenderViewObserverHostQt_DidFetchDocumentMarkup,
                            onDidFetchDocumentMarkup)
        IPC_MESSAGE_HANDLER(RenderViewObserverHostQt_DidFetchDocumentInnerText,
                            onDidFetchDocumentInnerText)
        IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
    return handled;

}

void RenderViewObserverHostQt::onDidFetchDocumentMarkup(quint64 requestId, const base::string16& markup)
{
    m_adapterClient->didFetchDocumentMarkup(requestId, toQt(markup));
}

void RenderViewObserverHostQt::onDidFetchDocumentInnerText(quint64 requestId, const base::string16& innerText)
{
    m_adapterClient->didFetchDocumentInnerText(requestId, toQt(innerText));
}

} // namespace QtWebEngineCore
