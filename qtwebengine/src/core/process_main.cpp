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

#include "process_main.h"

#include "content_main_delegate_qt.h"
#include "content/public/app/content_main.h"
#if defined(OS_WIN)
#include "sandbox/win/src/sandbox_types.h"
#include "content/public/app/sandbox_helper_win.h"
#elif defined(OS_MAC)
#include "base/logging.h"
#include "sandbox/mac/seatbelt_exec.h"
#endif

namespace QtWebEngineCore {

#if defined(OS_WIN)
extern sandbox::SandboxInterfaceInfo *staticSandboxInterfaceInfo(sandbox::SandboxInterfaceInfo *info = nullptr);
#endif

/*! \internal */
int processMain(int argc, const char **argv)
{
    ContentMainDelegateQt delegate;
    content::ContentMainParams params(&delegate);

#if defined(OS_WIN)
    HINSTANCE instance_handle = NULL;
    params.sandbox_info = staticSandboxInterfaceInfo();
    sandbox::SandboxInterfaceInfo sandbox_info = {0};
    if (!params.sandbox_info) {
        content::InitializeSandboxInfo(&sandbox_info);
        params.sandbox_info = &sandbox_info;
    }
    params.instance = instance_handle;
#else
    params.argc = argc;
    params.argv = argv;
#endif // OS_WIN
#if defined(OS_MAC)
  sandbox::SeatbeltExecServer::CreateFromArgumentsResult seatbelt =
          sandbox::SeatbeltExecServer::CreateFromArguments(argv[0], argc, const_cast<char**>(argv));
  if (seatbelt.sandbox_required) {
    CHECK(seatbelt.server->InitializeSandbox());
  }
#endif  // defined(OS_MAC)

    return content::ContentMain(params);
}

} // namespace QtWebEngineCore
