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

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

#ifndef PRINTING_PRINTING_MESSAGE_FILTER_QT_H_
#define PRINTING_PRINTING_MESSAGE_FILTER_QT_H_

#include <string>

#include "content/public/browser/browser_message_filter.h"

namespace printing {
namespace mojom {
class ScriptedPrintParams;
class PreviewIds;
}
class PrintQueriesQueue;
class PrinterQuery;
}

namespace QtWebEngineCore {
// This class filters out incoming printing related IPC messages for the
// renderer process on the IPC thread.
class PrintingMessageFilterQt : public content::BrowserMessageFilter {
 public:
  PrintingMessageFilterQt(int render_process_id);

  // content::BrowserMessageFilter methods.
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  friend class base::DeleteHelper<PrintingMessageFilterQt>;
  friend class content::BrowserThread;

  ~PrintingMessageFilterQt() override;

  void OnDestruct() const override;

  // The renderer host have to show to the user the print dialog and returns
  // the selected print settings. The task is handled by the print worker
  // thread and the UI thread. The reply occurs on the IO thread.
  void OnScriptedPrint(const printing::mojom::ScriptedPrintParams& params,
                       IPC::Message* reply_msg);
  void OnScriptedPrintReply(std::unique_ptr<printing::PrinterQuery> printer_query,
                            IPC::Message* reply_msg);

  // Modify the current print settings based on |job_settings|. The task is
  // handled by the print worker thread and the UI thread. The reply occurs on
  // the IO thread.
  void OnUpdatePrintSettings(int document_cookie,
                             base::Value job_settings,
                             IPC::Message* reply_msg);
  void OnUpdatePrintSettingsReply(std::unique_ptr<printing::PrinterQuery> printer_query,
                                  IPC::Message* reply_msg);

  // Check to see if print preview has been cancelled.
  void OnCheckForCancel(const printing::mojom::PreviewIds& ids, bool* cancel);

  const int render_process_id_;

  scoped_refptr<printing::PrintQueriesQueue> queue_;

  DISALLOW_COPY_AND_ASSIGN(PrintingMessageFilterQt);
};

}  // namespace printing

#endif  // PRINTING_PRINTING_MESSAGE_FILTER_QT_H_
