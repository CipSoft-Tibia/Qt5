// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_ANALYSIS_DEMO_HANDLER_H_
#define CONTENT_ANALYSIS_DEMO_HANDLER_H_

#include <time.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

#include "content_analysis/sdk/analysis_agent.h"
#include "demo/atomic_output.h"
#include "demo/request_queue.h"

// An AgentEventHandler that dumps requests information to stdout and blocks
// any requests that have the keyword "block" in their data
class Handler : public content_analysis::sdk::AgentEventHandler {
 public:
  using Event = content_analysis::sdk::ContentAnalysisEvent;

  Handler(unsigned long delay, const std::string& print_data_file_path) :
      delay_(delay), print_data_file_path_(print_data_file_path) {
  }

  unsigned long delay() { return delay_; }

 protected:
  // Analyzes one request from Google Chrome and responds back to the browser
  // with either an allow or block verdict.
  void AnalyzeContent(std::stringstream& stream, std::unique_ptr<Event> event) {
    // An event represents one content analysis request and response triggered
    // by a user action in Google Chrome.  The agent determines whether the
    // user is allowed to perform the action by examining event->GetRequest().
    // The verdict, which can be "allow" or "block" is written into
    // event->GetResponse().

    DumpEvent(stream, event.get());

    bool block = false;
    bool success = true;

    if (event->GetRequest().has_text_content()) {
      block = ShouldBlockRequest(
          event->GetRequest().text_content());
    } else if (event->GetRequest().has_file_path()) {
      std::string content;
      success =
          ReadContentFromFile(event->GetRequest().file_path(),
                              &content);
      if (success) {
        block = ShouldBlockRequest(content);
      }
    } else if (event->GetRequest().has_print_data()) {
      // In the case of print request, normally the PDF bytes would be parsed
      // for sensitive data violations. To keep this class simple, only the
      // URL is checked for the word "block".
      block = ShouldBlockRequest(event->GetRequest().request_data().url());
    }

    if (!success) {
      content_analysis::sdk::UpdateResponse(
          event->GetResponse(),
          std::string(),
          content_analysis::sdk::ContentAnalysisResponse::Result::FAILURE);
      stream << "  Verdict: failed to reach verdict: ";
      stream << event->DebugString() << std::endl;
    } else if (block) {
      auto rc = content_analysis::sdk::SetEventVerdictToBlock(event.get());
      stream << "  Verdict: block";
      if (rc != content_analysis::sdk::ResultCode::OK) {
        stream << " error: "
               << content_analysis::sdk::ResultCodeToString(rc) << std::endl;
        stream << "  " << event->DebugString() << std::endl;
      }
      stream << std::endl;
    } else {
      stream << "  Verdict: allow" << std::endl;
    }

    stream << std::endl;

    // If a delay is specified, wait that much.
    if (delay_ > 0) {
      std::this_thread::sleep_for(std::chrono::seconds(delay_));
    }

    // Send the response back to Google Chrome.
    auto rc = event->Send();
    if (rc != content_analysis::sdk::ResultCode::OK) {
      stream << "[Demo] Error sending response: "
             << content_analysis::sdk::ResultCodeToString(rc)
             << std::endl;
      stream << event->DebugString() << std::endl;
    }
  }

 private:
  void OnBrowserConnected(
      const content_analysis::sdk::BrowserInfo& info) override {
    AtomicCout aout;
    aout.stream() << std::endl << "==========" << std::endl;
    aout.stream() << "Browser connected pid=" << info.pid
                  << " path=" << info.binary_path << std::endl;
  }

  void OnBrowserDisconnected(
      const content_analysis::sdk::BrowserInfo& info) override {
    AtomicCout aout;
    aout.stream() << std::endl << "Browser disconnected pid=" << info.pid << std::endl;
    aout.stream() << "==========" << std::endl;
  }

  void OnAnalysisRequested(std::unique_ptr<Event> event) override {
    // If the agent is capable of analyzing content in the background, the
    // events may be handled in background threads.  Having said that, a
    // event should not be assumed to be thread safe, that is, it should not
    // be accessed by more than one thread concurrently.
    //
    // In this example code, the event is handled synchronously.
    AtomicCout aout;
    aout.stream() << std::endl << "----------" << std::endl << std::endl;
    AnalyzeContent(aout.stream(), std::move(event));
  }

  void OnResponseAcknowledged(
      const content_analysis::sdk::ContentAnalysisAcknowledgement&
          ack) override {
    const char* final_action = "<Unknown>";
    if (ack.has_final_action()) {
      switch (ack.final_action()) {
      case content_analysis::sdk::ContentAnalysisAcknowledgement::ACTION_UNSPECIFIED:
        final_action = "<Unspecified>";
        break;
      case content_analysis::sdk::ContentAnalysisAcknowledgement::ALLOW:
        final_action = "Allow";
        break;
      case content_analysis::sdk::ContentAnalysisAcknowledgement::REPORT_ONLY:
        final_action = "Report only";
        break;
      case content_analysis::sdk::ContentAnalysisAcknowledgement::WARN:
        final_action = "Warn";
        break;
      case content_analysis::sdk::ContentAnalysisAcknowledgement::BLOCK:
        final_action = "Block";
        break;
      }
    }

    AtomicCout aout;
    aout.stream() << "  Ack: " << ack.request_token() << std::endl;
    aout.stream() << "  Final action: " << final_action << std::endl;
  }
  void OnCancelRequests(
      const content_analysis::sdk::ContentAnalysisCancelRequests& cancel)
      override {
    AtomicCout aout;
    aout.stream() << "Cancel: " << std::endl;
    aout.stream() << "  User action ID: " << cancel.user_action_id() << std::endl;
  }

  void OnInternalError(
      const char* context,
      content_analysis::sdk::ResultCode error) override {
    AtomicCout aout;
    aout.stream() << std::endl
                  << "*ERROR*: context=\"" << context << "\" "
                  << content_analysis::sdk::ResultCodeToString(error)
                  << std::endl;
  }

  void DumpEvent(std::stringstream& stream, Event* event) {
    time_t now = time(nullptr);
    stream << "Received at: " << ctime(&now);  // Returned string includes \n.

    const content_analysis::sdk::ContentAnalysisRequest& request =
        event->GetRequest();
    std::string connector = "<Unknown>";
    if (request.has_analysis_connector()) {
      switch (request.analysis_connector()) {
        case content_analysis::sdk::FILE_DOWNLOADED:
          connector = "download";
          break;
        case content_analysis::sdk::FILE_ATTACHED:
          connector = "attach";
          break;
        case content_analysis::sdk::BULK_DATA_ENTRY:
          connector = "bulk-data-entry";
          break;
        case content_analysis::sdk::PRINT:
          connector = "print";
          break;
        case content_analysis::sdk::FILE_TRANSFER:
          connector = "file-transfer";
          break;
        default:
          break;
      }
    }
    std::string reason;
    if (request.has_reason()) {
      using content_analysis::sdk::ContentAnalysisRequest;
      switch (request.reason()) {
        case content_analysis::sdk::ContentAnalysisRequest::UNKNOWN:
          reason = "<Unknown>";
          break;
        case content_analysis::sdk::ContentAnalysisRequest::CLIPBOARD_PASTE:
          reason = "CLIPBOARD_PASTE";
          break;
        case content_analysis::sdk::ContentAnalysisRequest::DRAG_AND_DROP:
          reason = "DRAG_AND_DROP";
          break;
        case content_analysis::sdk::ContentAnalysisRequest::FILE_PICKER_DIALOG:
          reason = "FILE_PICKER_DIALOG";
          break;
        case content_analysis::sdk::ContentAnalysisRequest::PRINT_PREVIEW_PRINT:
          reason = "PRINT_PREVIEW_PRINT";
          break;
        case content_analysis::sdk::ContentAnalysisRequest::SYSTEM_DIALOG_PRINT:
          reason = "SYSTEM_DIALOG_PRINT";
          break;
        case content_analysis::sdk::ContentAnalysisRequest::NORMAL_DOWNLOAD:
          reason = "NORMAL_DOWNLOAD";
          break;
        case content_analysis::sdk::ContentAnalysisRequest::SAVE_AS_DOWNLOAD:
          reason = "SAVE_AS_DOWNLOAD";
          break;
      }
    }

    std::string url =
        request.has_request_data() && request.request_data().has_url()
        ? request.request_data().url() : "<No URL>";

    std::string tab_title =
        request.has_request_data() && request.request_data().has_tab_title()
        ? request.request_data().tab_title() : "<No tab title>";

    std::string filename =
        request.has_request_data() && request.request_data().has_filename()
        ? request.request_data().filename() : "<No filename>";

    std::string digest =
        request.has_request_data() && request.request_data().has_digest()
        ? request.request_data().digest() : "<No digest>";

    std::string file_path =
        request.has_file_path()
        ? request.file_path() : "None, bulk text entry or print";

    std::string machine_user =
        request.has_client_metadata() &&
        request.client_metadata().has_browser() &&
        request.client_metadata().browser().has_machine_user()
      ? request.client_metadata().browser().machine_user() : "<No machine user>";

    std::string email =
        request.has_request_data() && request.request_data().has_email()
      ? request.request_data().email() : "<No email>";

    time_t t = request.expires_at();
    std::string expires_at_str = ctime(&t);
    // Returned string includes trailing \n, overwrite with null.
    expires_at_str[expires_at_str.size() - 1] = 0;
    time_t secs_remaining = t - now;

    std::string user_action_id = request.has_user_action_id()
        ? request.user_action_id() : "<No user action id>";

    stream << "Request: " << request.request_token() << std::endl;
    stream << "  User action ID: " << user_action_id << std::endl;
    stream << "  Expires at: " << expires_at_str << " ("
           << secs_remaining << " seconds from now)" << std::endl;
    stream << "  Connector: " << connector << std::endl;
    if (!reason.empty()) {
      stream << "  Reason: " << reason << std::endl;
    }
    stream << "  URL: " << url << std::endl;
    stream << "  Tab title: " << tab_title << std::endl;
    stream << "  Filename: " << filename << std::endl;
    stream << "  Digest: " << digest << std::endl;
    stream << "  Filepath: " << file_path << std::endl;
    stream << "  Machine user: " << machine_user << std::endl;
    stream << "  Email: " << email << std::endl;

    if (request.has_text_content() && !request.text_content().empty()) {
      std::string prefix = "  Pasted data: ";
      std::string text_content = request.text_content();

      // Truncate the text past 50 bytes to keep it to a reasonable length in
      // the terminal window.
      if (text_content.size() > 50) {
        prefix = "  Pasted data (truncated): ";
        text_content = text_content.substr(0, 50) + "...";
      }
      stream << prefix
             << text_content
             << std::endl;
      stream << "  Pasted data size (bytes): "
             << request.text_content().size()
             << std::endl;
    }

    if (request.has_print_data() && !print_data_file_path_.empty()) {
      if (request.request_data().has_print_metadata() &&
          request.request_data().print_metadata().has_printer_name()) {
        stream << "  Printer name: "
               << request.request_data().print_metadata().printer_name()
               << std::endl;
      } else {
        stream << "  No printer name in request" << std::endl;
      }

      stream << "  Print data saved to: " << print_data_file_path_
                << std::endl;
      using content_analysis::sdk::ContentAnalysisEvent;
      auto print_data =
          content_analysis::sdk::CreateScopedPrintHandle(event->GetRequest(),
                   event->GetBrowserInfo().pid);
      std::ofstream file(print_data_file_path_,
                         std::ios::out | std::ios::trunc | std::ios::binary);
      file.write(print_data->data(), print_data->size());
      file.flush();
      file.close();
    }
  }

  bool ReadContentFromFile(const std::string& file_path,
                          std::string* content) {
    std::ifstream file(file_path,
                      std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
      return false;

    // Get file size.  This example does not handle files larger than 1MB.
    // Make sure content string can hold the contents of the file.
    int size = file.tellg();
    if (size > 1024 * 1024)
      return false;

    content->resize(size + 1);

    // Read file into string.
    file.seekg(0, std::ios::beg);
    file.read(&(*content)[0], size);
    content->at(size) = 0;
    return true;
  }

  bool ShouldBlockRequest(const std::string& content) {
    // Determines if the request should be blocked.  For this simple example
    // the content is blocked if the string "block" is found.  Otherwise the
    // content is allowed.
    return content.find("block") != std::string::npos;
  }

  unsigned long delay_;
  std::string print_data_file_path_;
};

// An AgentEventHandler that dumps requests information to stdout and blocks
// any requests that have the keyword "block" in their data
class QueuingHandler : public Handler {
 public:
  QueuingHandler(unsigned long threads, unsigned long delay, const std::string& print_data_file_path)
      : Handler(delay, print_data_file_path)  {
    StartBackgroundThreads(threads);
  }

  ~QueuingHandler() override {
    // Abort background process and wait for it to finish.
    request_queue_.abort();
    WaitForBackgroundThread();
  }

 private:
  void OnAnalysisRequested(std::unique_ptr<Event> event) override {
    {
      time_t now = time(nullptr);
      const content_analysis::sdk::ContentAnalysisRequest& request =
        event->GetRequest();
      AtomicCout aout;
      aout.stream() << std::endl << "Queuing request: " << request.request_token()
                    << " at " << ctime(&now) << std::endl;
    }

    request_queue_.push(std::move(event));
  }

  static void* ProcessRequests(void* qh) {
    QueuingHandler* handler = reinterpret_cast<QueuingHandler*>(qh);

    while (true) {
      auto event = handler->request_queue_.pop();
      if (!event)
        break;

      AtomicCout aout;
      aout.stream()  << std::endl << "----------" << std::endl;
      aout.stream() << "Thread: " << std::this_thread::get_id() << std::endl;
      aout.stream() << "Delaying request processing for "
                    << handler->delay() << "s" << std::endl << std::endl;
      aout.flush();

      handler->AnalyzeContent(aout.stream(), std::move(event));
    }

    return 0;
  }

  // A list of outstanding content analysis requests.
  RequestQueue request_queue_;

  void StartBackgroundThreads(unsigned long threads) {
    threads_.reserve(threads);
    for (unsigned long i = 0; i < threads; ++i) {
      threads_.emplace_back(std::make_unique<std::thread>(ProcessRequests, this));
    }
  }

  void WaitForBackgroundThread() {
    for (auto& thread : threads_) {
      thread->join();
    }
  }

  // Thread id of backgrond thread.
  std::vector<std::unique_ptr<std::thread>> threads_;
};

#endif  // CONTENT_ANALYSIS_DEMO_HANDLER_H_
