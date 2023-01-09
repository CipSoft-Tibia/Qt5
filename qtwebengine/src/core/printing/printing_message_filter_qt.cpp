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

// Based on chrome/browser/printing/printing_message_filter.cc:
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

#include "printing_message_filter_qt.h"

#include "web_engine_context.h"

#include "base/bind.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "components/printing/browser/print_manager_utils.h"
#include "components/printing/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/child_process_host.h"

namespace QtWebEngineCore {

PrintingMessageFilterQt::PrintingMessageFilterQt(int render_process_id)
    : BrowserMessageFilter(PrintMsgStart),
      render_process_id_(render_process_id),
      queue_(WebEngineContext::current()->getPrintJobManager()->queue()) {
  DCHECK(queue_.get());
}

PrintingMessageFilterQt::~PrintingMessageFilterQt() {
}

void PrintingMessageFilterQt::OnDestruct() const
{
    content::BrowserThread::DeleteOnUIThread::Destruct(this);
}

bool PrintingMessageFilterQt::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintingMessageFilterQt, message)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_ScriptedPrint, OnScriptedPrint)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_UpdatePrintSettings,
                                    OnUpdatePrintSettings)
    IPC_MESSAGE_HANDLER(PrintHostMsg_CheckForCancel, OnCheckForCancel)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PrintingMessageFilterQt::OnScriptedPrint(
    const printing::mojom::ScriptedPrintParams& params,
    IPC::Message* reply_msg) {
  std::unique_ptr<printing::PrinterQuery> printer_query =
      queue_->PopPrinterQuery(params.cookie);
  if (!printer_query.get()) {
    printer_query =
        queue_->CreatePrinterQuery(render_process_id_, reply_msg->routing_id());
  }
  printer_query->GetSettings(
      printing::PrinterQuery::GetSettingsAskParam::ASK_USER,
      params.expected_pages_count,
      params.has_selection,
      params.margin_type,
      params.is_scripted,
      params.is_modifiable,
      base::BindOnce(&PrintingMessageFilterQt::OnScriptedPrintReply,
                     this,
                     std::move(printer_query),
                     reply_msg));
}

void PrintingMessageFilterQt::OnScriptedPrintReply(
    std::unique_ptr<printing::PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  printing::mojom::PrintPagesParams params;
  params.params = printing::mojom::PrintParams::New();
  if (printer_query->last_status() != printing::PrintingContext::OK ||
      !printer_query->settings().dpi()) {
    params.params.reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), params.params.get());
    params.params->document_cookie = printer_query->cookie();
    params.pages = printing::PageRange::GetPages(printer_query->settings().ranges());
  }
  PrintHostMsg_ScriptedPrint::WriteReplyParams(reply_msg, params);
  Send(reply_msg);
  if (!params.params->dpi.IsEmpty() && params.params->document_cookie) {
    queue_->QueuePrinterQuery(std::move(printer_query));
  } else {
    printer_query->StopWorker();
  }
}

void PrintingMessageFilterQt::OnUpdatePrintSettings(int document_cookie,
                                                    base::Value job_settings,
                                                    IPC::Message* reply_msg) {
  if (!job_settings.is_dict() ||
      !job_settings.FindIntKey(printing::kSettingPrinterType)) {
    // Reply with null query.
    OnUpdatePrintSettingsReply(nullptr, reply_msg);
    return;
  }

  std::unique_ptr<printing::PrinterQuery> printer_query =
      queue_->PopPrinterQuery(document_cookie);
  if (!printer_query.get()) {
    printer_query = queue_->CreatePrinterQuery(
        content::ChildProcessHost::kInvalidUniqueID, MSG_ROUTING_NONE);
  }
  auto* printer_query_ptr = printer_query.get();
  printer_query_ptr->SetSettings(
      std::move(job_settings),
      base::BindOnce(&PrintingMessageFilterQt::OnUpdatePrintSettingsReply, this,
                     std::move(printer_query), reply_msg));
}

void PrintingMessageFilterQt::OnUpdatePrintSettingsReply(std::unique_ptr<printing::PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  printing::mojom::PrintPagesParams params;
  params.params = printing::mojom::PrintParams::New();
  if (!printer_query.get() ||
      printer_query->last_status() != printing::PrintingContext::OK) {
    params.params.reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), params.params.get());
    params.params->document_cookie = printer_query->cookie();
    params.pages = printing::PageRange::GetPages(printer_query->settings().ranges());
  }

  PrintHostMsg_UpdatePrintSettings::WriteReplyParams(
      reply_msg,
      params,
      printer_query.get() &&
          (printer_query->last_status() == printing::PrintingContext::CANCEL));
  Send(reply_msg);
  // If user hasn't cancelled.
  if (printer_query) {
    if (printer_query->cookie() && printer_query->settings().dpi()) {
      queue_->QueuePrinterQuery(std::move(printer_query));
    } else {
      printer_query->StopWorker();
    }
  }
}

void PrintingMessageFilterQt::OnCheckForCancel(const printing::mojom::PreviewIds& ids,
                                               bool* cancel) {
  *cancel = false;
}

}  // namespace printing
