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

#ifndef JAVASCRIPT_DIALOG_CONTROLLER_P_H
#define JAVASCRIPT_DIALOG_CONTROLLER_P_H

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

#include "base/callback.h"
#include "content/public/browser/javascript_dialog_manager.h"

#include "web_contents_adapter_client.h"

#include <QString>

namespace content {
class WebContents;
}

namespace QtWebEngineCore {

class JavaScriptDialogControllerPrivate {

public:
    void dialogFinished(bool accepted, const base::string16 &promptValue);
    JavaScriptDialogControllerPrivate(WebContentsAdapterClient::JavascriptDialogType, const QString &message, const QString &prompt
                                      , const QString& title, const QUrl &securityOrigin
                                      , content::JavaScriptDialogManager::DialogClosedCallback &&, content::WebContents *);

    WebContentsAdapterClient::JavascriptDialogType type;
    QString message;
    QString defaultPrompt;
    QUrl securityOrigin;
    QString userInput;
    QString title;
    content::JavaScriptDialogManager::DialogClosedCallback callback;
    content::WebContents *contents;
};

} // namespace QtWebEngineCore

#endif // JAVASCRIPT_DIALOG_CONTROLLER_P_H
