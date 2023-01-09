/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

// based on //chrome/browser/pdf/pdf_extension_util.h
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "qtwebengine/browser/pdf/pdf_extension_util.h"

#include "base/feature_list.h"
#include "base/values.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/zoom/page_zoom_constants.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "pdf/buildflags.h"
#include "pdf/pdf_features.h"
#include "qtwebengine/browser/pdf/pdf_extension_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

namespace pdf_extension_util {

namespace {

// Adds strings that are used both by the stand-alone PDF Viewer and the Print
// Preview PDF Viewer.
static void AddCommonStrings(base::Value* dict) {
  static constexpr webui::LocalizedString kPdfResources[] = {
      {"errorDialogTitle", IDS_PDF_ERROR_DIALOG_TITLE},
      {"pageLoadFailed", IDS_PDF_PAGE_LOAD_FAILED},
      {"pageLoading", IDS_PDF_PAGE_LOADING},
      {"pageReload", IDS_PDF_PAGE_RELOAD_BUTTON},
      {"tooltipFitToPage", IDS_PDF_TOOLTIP_FIT_PAGE},
      {"tooltipFitToWidth", IDS_PDF_TOOLTIP_FIT_WIDTH},
      {"tooltipZoomIn", IDS_PDF_TOOLTIP_ZOOM_IN},
      {"tooltipZoomOut", IDS_PDF_TOOLTIP_ZOOM_OUT},
      {"twoUpViewEnable", IDS_PDF_TWO_UP_VIEW_ENABLE},
  };
  for (const auto& resource : kPdfResources)
    dict->SetStringKey(resource.name, l10n_util::GetStringUTF16(resource.id));

  dict->SetStringKey("presetZoomFactors", zoom::GetPresetZoomFactorsAsJSON());
}

// Adds strings that are used only by the stand-alone PDF Viewer.
static void AddPdfViewerStrings(base::Value* dict) {
  static constexpr webui::LocalizedString kPdfResources[] = {
    {"annotationsShowToggle", IDS_PDF_ANNOTATIONS_SHOW_TOGGLE},
    {"bookmarks", IDS_PDF_BOOKMARKS},
    {"bookmarkExpandIconAriaLabel", IDS_PDF_BOOKMARK_EXPAND_ICON_ARIA_LABEL},
    {"downloadEdited", IDS_PDF_DOWNLOAD_EDITED},
    {"downloadOriginal", IDS_PDF_DOWNLOAD_ORIGINAL},
    {"labelPageNumber", IDS_PDF_LABEL_PAGE_NUMBER},
    {"menu", IDS_MENU},
    {"moreActions", IDS_DOWNLOAD_MORE_ACTIONS},
    {"passwordDialogTitle", IDS_PDF_PASSWORD_DIALOG_TITLE},
    {"passwordInvalid", IDS_PDF_PASSWORD_INVALID},
    {"passwordPrompt", IDS_PDF_NEED_PASSWORD},
    {"passwordSubmit", IDS_PDF_PASSWORD_SUBMIT},
    {"propertiesDialogClose", IDS_CLOSE},
    {"thumbnailPageAriaLabel", IDS_PDF_THUMBNAIL_PAGE_ARIA_LABEL},
    {"tooltipDocumentOutline", IDS_PDF_TOOLTIP_DOCUMENT_OUTLINE},
    {"tooltipDownload", IDS_PDF_TOOLTIP_DOWNLOAD},
    {"tooltipPrint", IDS_PDF_TOOLTIP_PRINT},
    {"tooltipRotateCCW", IDS_PDF_TOOLTIP_ROTATE_CCW},
    {"tooltipRotateCW", IDS_PDF_TOOLTIP_ROTATE_CW},
    {"tooltipThumbnails", IDS_PDF_TOOLTIP_THUMBNAILS},
    {"zoomTextInputAriaLabel", IDS_PDF_ZOOM_TEXT_INPUT_ARIA_LABEL},
  };
  for (const auto& resource : kPdfResources)
    dict->SetStringKey(resource.name, l10n_util::GetStringUTF16(resource.id));

  webui::SetLoadTimeDataDefaults(content::GetContentClient()->browser()->GetApplicationLocale(),
                                 static_cast<base::DictionaryValue*>(dict));

}

} // namespace

void AddStrings(PdfViewerContext context, base::Value* dict) {
  AddCommonStrings(dict);
  if (context == PdfViewerContext::kPdfViewer ||
      context == PdfViewerContext::kAll) {
    AddPdfViewerStrings(dict);
  }
  if (context == PdfViewerContext::kPrintPreview ||
      context == PdfViewerContext::kAll) {
    // Nothing to do yet, since there are no PrintPreview-only strings.
  }
}

void AddAdditionalData(base::Value* dict) {
  dict->SetKey("pdfFormSaveEnabled",
               base::Value(base::FeatureList::IsEnabled(
                   chrome_pdf::features::kSaveEditedPDFForm)));
  dict->SetStringKey(
      "pdfViewerUpdateEnabledAttribute",
      base::FeatureList::IsEnabled(chrome_pdf::features::kPDFViewerUpdate)
          ? "pdf-viewer-update-enabled"
          : "");

  dict->SetKey("printingEnabled", base::Value(false));
  dict->SetKey("pdfAnnotationsEnabled", base::Value(false));
}

} // namespace pdf_extension_util
