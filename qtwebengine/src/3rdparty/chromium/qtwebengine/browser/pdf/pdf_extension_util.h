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

#ifndef QTWEBENGINE_BROWSER_PDF_PDF_EXTENSION_UTIL_H_
#define QTWEBENGINE_BROWSER_PDF_PDF_EXTENSION_UTIL_H_

#include <string>

#include "base/values.h"
#include "pdf/buildflags.h"

#if !BUILDFLAG(ENABLE_PDF)
#error "PDF must be enabled"
#endif

namespace pdf_extension_util {

// Represents the context within which the PDF Viewer runs.
enum class PdfViewerContext {
  kPdfViewer,
  kPrintPreview,
  kAll,
};

// Adds all strings used by the PDF Viewer depending on the provided |context|.
void AddStrings(PdfViewerContext context, base::Value::Dict* dict);

// Adds additional data used by the PDF Viewer UI in `dict`, for example
// whether certain features are enabled/disabled.
// `enable_annotations` only applies on platforms that supports annotations.
void AddAdditionalData(bool enable_annotations, base::Value::Dict* dict);

}  // namespace pdf_extension_util

#endif  // QTWEBENGINE_BROWSER_PDF_PDF_EXTENSION_UTIL_H_
