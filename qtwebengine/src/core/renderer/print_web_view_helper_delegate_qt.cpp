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

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/common/webui_url_constants.h"
#include "extensions/common/constants.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#include "print_web_view_helper_delegate_qt.h"
#include "web_engine_library_info.h"

namespace QtWebEngineCore {
PrintWebViewHelperDelegateQt::~PrintWebViewHelperDelegateQt() {}

blink::WebElement PrintWebViewHelperDelegateQt::GetPdfElement(blink::WebLocalFrame *frame)
{
#if BUILDFLAG(ENABLE_EXTENSIONS)
    GURL url = frame->GetDocument().Url();
    bool inside_print_preview = url.GetOrigin() == chrome::kChromeUIPrintURL;
    bool inside_pdf_extension = url.SchemeIs(extensions::kExtensionScheme) &&
                                url.host_piece() == extension_misc::kPdfExtensionId;
    if (inside_print_preview || inside_pdf_extension) {
        // <object> with id="plugin" is created in
        // chrome/browser/resources/pdf/pdf_viewer_base.js.
        auto viewer_element = frame->GetDocument().GetElementById("viewer");
        if (!viewer_element.IsNull() && !viewer_element.ShadowRoot().IsNull()) {
            auto plugin_element = viewer_element.ShadowRoot().QuerySelector("#plugin");
            if (!plugin_element.IsNull())
                return plugin_element;
        }
        NOTREACHED();
    }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
    return blink::WebElement();
}

bool PrintWebViewHelperDelegateQt::IsPrintPreviewEnabled()
{
    return true;
}

bool PrintWebViewHelperDelegateQt::OverridePrint(blink::WebLocalFrame *frame)
{
    return false;
}

}

namespace printing {
// std::string PrintingContextDelegate::GetAppLocale()
std::string getApplicationLocale()
{
    return WebEngineLibraryInfo::getApplicationLocale();
}
}
