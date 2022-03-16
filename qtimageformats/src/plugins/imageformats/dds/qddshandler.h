/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Ivan Komissarov.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the DDS plugin in the Qt ImageFormats module.
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

#ifndef QDDSHANDLER_H
#define QDDSHANDLER_H

#include <QtGui/qimageiohandler.h>
#include "ddsheader.h"

#ifndef QT_NO_DATASTREAM

QT_BEGIN_NAMESPACE

class QDDSHandler : public QImageIOHandler
{
public:
    QDDSHandler();

    QByteArray name() const override;

    bool canRead() const override;
    bool read(QImage *image) override;
    bool write(const QImage &image) override;

    QVariant option(QImageIOHandler::ImageOption option) const override;
    void setOption(ImageOption option, const QVariant &value) override;
    bool supportsOption(QImageIOHandler::ImageOption option) const override;

    int imageCount() const override;
    bool jumpToImage(int imageNumber) override;

    static bool canRead(QIODevice *device);

private:
    bool ensureScanned() const;
    bool verifyHeader(const DDSHeader &dds) const;

private:
    enum ScanState {
        ScanError = -1,
        ScanNotScanned = 0,
        ScanSuccess = 1,
    };

    DDSHeader m_header;
    int m_format;
    DDSHeaderDX10 m_header10;
    int m_currentImage;
    mutable ScanState m_scanState;
};

QT_END_NAMESPACE

#endif // QT_NO_DATASTREAM

#endif // QDDSHANDLER_H
