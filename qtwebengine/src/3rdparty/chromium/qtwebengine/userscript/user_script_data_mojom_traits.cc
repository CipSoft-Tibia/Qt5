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

#include "qtwebengine/userscript/user_script_data_mojom_traits.h"

namespace mojo {

bool StructTraits<qtwebengine::mojom::UserScriptDataDataView,
                  QtWebEngineCore::UserScriptData>::
    Read(qtwebengine::mojom::UserScriptDataDataView data,
         QtWebEngineCore::UserScriptData* out_data) {
  if (!data.ReadSource(&out_data->source))
    return false;
  if (!data.ReadUrl(&out_data->url))
    return false;
  out_data->injectionPoint = data.injectionPoint();
  out_data->injectForSubframes = data.injectForSubframes();
  out_data->worldId = data.worldId();
  out_data->scriptId = data.scriptId();
  if (!data.ReadGlobs(&out_data->globs))
    return false;
  if (!data.ReadExcludeGlobs(&out_data->excludeGlobs))
    return false;
  if (!data.ReadUrlPatterns(&out_data->urlPatterns))
    return false;
  return true;
}

}  // namespace mojo
