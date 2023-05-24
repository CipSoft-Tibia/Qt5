/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef TOOLS_GN_RSP_TARGET_WRITER_H_
#define TOOLS_GN_RSP_TARGET_WRITER_H_

#include <iosfwd>
#include "gn/path_output.h"

class Target;
class NinjaCBinaryTargetWriter;

class RspTargetWriter {
 public:
  enum Type { NONE, OBJECTS, ARCHIVES, DEFINES, LFLAGS, LIBS, LDIR};
  RspTargetWriter(const NinjaCBinaryTargetWriter* writer,
                  const Target* target,
                  Type type,
                  std::ostream& out);
  ~RspTargetWriter();
  void Run();
  static Type strToType(const std::string& str);

 private:
  Type type_;
  const Target* target_;
  const NinjaCBinaryTargetWriter* nwriter_;
  std::ostream& out_;
};

#endif  // TOOLS_GN_RSP_TARGET_WRITER_H_
