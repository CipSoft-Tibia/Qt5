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

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

#ifndef PRINT_VIEW_MANAGER_QT_H
#define PRINT_VIEW_MANAGER_QT_H

#include "print_view_manager_base_qt.h"

#include "qtwebenginecoreglobal_p.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "components/prefs/pref_member.h"
#include "components/printing/browser/print_manager.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_user_data.h"

struct PrintHostMsg_RequestPrintPreview_Params;
struct PrintHostMsg_DidPreviewDocument_Params;

namespace content {
class RenderViewHost;
}

namespace printing {
class JobEventDetails;
class MetafilePlayer;
class PrintJob;
class PrintJobWorkerOwner;
class PrintQueriesQueue;
}

QT_BEGIN_NAMESPACE
class QPageLayout;
class QString;
QT_END_NAMESPACE

namespace QtWebEngineCore {
class PrintViewManagerQt
        : public PrintViewManagerBaseQt
        , public content::WebContentsUserData<PrintViewManagerQt>
{
public:
    ~PrintViewManagerQt() override;
    typedef base::Callback<void(const std::vector<char> &result)> PrintToPDFCallback;
    typedef base::Callback<void(bool success)> PrintToPDFFileCallback;

    // Method to print a page to a Pdf document with page size \a pageSize in location \a filePath.
    void PrintToPDFFileWithCallback(const QPageLayout &pageLayout,
                                    bool printInColor,
                                    const QString &filePath,
                                    const PrintToPDFFileCallback& callback);
    void PrintToPDFWithCallback(const QPageLayout &pageLayout,
                                bool printInColor,
                                bool useCustomMargins,
                                const PrintToPDFCallback &callback);

    base::string16 RenderSourceName() override;

protected:
    explicit PrintViewManagerQt(content::WebContents*);

    // content::WebContentsObserver implementation.
    // Cancels the print job.
    void NavigationStopped() override;

    // Terminates or cancels the print job if one was pending.
    void RenderProcessGone(base::TerminationStatus status) override;

    // content::WebContentsObserver implementation.
    bool OnMessageReceived(const IPC::Message& message,
                           content::RenderFrameHost* render_frame_host) override;
    void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

    // IPC handlers
    void OnDidShowPrintDialog();
    void OnRequestPrintPreview(const PrintHostMsg_RequestPrintPreview_Params&);
    void OnMetafileReadyForPrinting(content::RenderFrameHost* rfh,
                                    const PrintHostMsg_DidPreviewDocument_Params& params,
                                    const PrintHostMsg_PreviewIds &ids);
    void OnSetupScriptedPrintPreview(content::RenderFrameHost* rfh,
                                      IPC::Message* reply_msg);
    void OnDidPreviewPage(content::RenderFrameHost* rfh,
                          const PrintHostMsg_DidPreviewPage_Params& params,
                          const PrintHostMsg_PreviewIds& ids);
    void OnShowScriptedPrintPreview(content::RenderFrameHost* rfh,
                                    bool source_is_modifiable);
    bool PrintToPDFInternal(const QPageLayout &, bool printInColor, bool useCustomMargins = true);

private:
    void resetPdfState();
    // content::WebContentsObserver implementation.
    void DidStartLoading() override;
    void PrintPreviewDone();

private:
    content::RenderFrameHost *m_printPreviewRfh;
    base::FilePath m_pdfOutputPath;
    PrintToPDFCallback m_pdfPrintCallback;
    PrintToPDFFileCallback m_pdfSaveCallback;
    std::unique_ptr<base::DictionaryValue> m_printSettings;
    friend class content::WebContentsUserData<PrintViewManagerQt>;
    DISALLOW_COPY_AND_ASSIGN(PrintViewManagerQt);
    struct FrameDispatchHelper;
};

} // namespace QtWebEngineCore
#endif // PRINT_VIEW_MANAGER_QT_H

