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

#ifndef USER_SCRIPT_DATA_MOJOM_TRAITS_H_
#define USER_SCRIPT_DATA_MOJOM_TRAITS_H_

#include <string>
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "qtwebengine/userscript/user_script_data.h"
#include "qtwebengine/userscript/userscript.mojom.h"
#include "url/gurl.h"

namespace mojo {

template <>
class StructTraits<qtwebengine::mojom::UserScriptDataDataView,
                   QtWebEngineCore::UserScriptData> {
 public:
  static std::string source(const QtWebEngineCore::UserScriptData& d) {
    return d.source;
  }
  static GURL url(const QtWebEngineCore::UserScriptData& d) { return d.url; }
  static uint8_t injectionPoint(const QtWebEngineCore::UserScriptData& d) {
    return d.injectionPoint;
  }
  static bool injectForSubframes(const QtWebEngineCore::UserScriptData& d) {
    return d.injectForSubframes;
  }
  static uint64_t worldId(const QtWebEngineCore::UserScriptData& d) {
    return d.worldId;
  }
  static uint8_t scriptId(const QtWebEngineCore::UserScriptData& d) {
    return d.scriptId;
  }
  static std::vector<std::string> globs(
      const QtWebEngineCore::UserScriptData& d) {
    return d.globs;
  }
  static std::vector<std::string> excludeGlobs(
      const QtWebEngineCore::UserScriptData& d) {
    return d.excludeGlobs;
  }
  static std::vector<std::string> urlPatterns(
      const QtWebEngineCore::UserScriptData& d) {
    return d.urlPatterns;
  }
  static bool Read(qtwebengine::mojom::UserScriptDataDataView data,
                   QtWebEngineCore::UserScriptData* out_data);
};

}  // namespace mojo
#endif
