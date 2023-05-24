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

// based on //chrome/browser/extensions/api/resources_private/resources_private_api.cc
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "qtwebengine/browser/extensions/api/resources_private/resources_private_api.h"

#include <memory>
#include <string>
#include <utility>

#include "pdf/buildflags.h"
#if BUILDFLAG(ENABLE_PDF)
#include "qtwebengine/browser/pdf/pdf_extension_util.h"
#endif
#include "qtwebengine/common/extensions/api/resources_private.h"

namespace extensions {

namespace get_strings = api::resources_private::GetStrings;

ResourcesPrivateGetStringsFunction::ResourcesPrivateGetStringsFunction() {
}

ResourcesPrivateGetStringsFunction::~ResourcesPrivateGetStringsFunction() {}

ExtensionFunction::ResponseAction ResourcesPrivateGetStringsFunction::Run() {
  std::unique_ptr<get_strings::Params> params(
      get_strings::Params::Create(args()));
  base::Value::Dict dict;

  api::resources_private::Component component = params->component;

  switch (component) {
    case api::resources_private::COMPONENT_IDENTITY:
      break;
    case api::resources_private::COMPONENT_PDF:
#if BUILDFLAG(ENABLE_PDF)
      pdf_extension_util::AddStrings(pdf_extension_util::PdfViewerContext::kAll, &dict);
      pdf_extension_util::AddAdditionalData(true, &dict);
#else
      NOTREACHED();
#endif  // BUILDFLAG(ENABLE_PDF)
      break;
    case api::resources_private::COMPONENT_NONE:
      NOTREACHED();
  }

  return RespondNow(WithArguments(std::move(dict)));
}

}  // namespace extensions
