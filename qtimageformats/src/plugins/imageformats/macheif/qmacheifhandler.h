// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACHEIFHANDLER_H
#define QMACHEIFHANDLER_H

#include <QScopedPointer>
#include <QImageIOHandler>

QT_BEGIN_NAMESPACE

class QImage;
class QByteArray;
class QIODevice;
class QVariant;
class QIIOFHelper;

class QMacHeifHandler : public QImageIOHandler
{
public:
    QMacHeifHandler();
    ~QMacHeifHandler() override;

    bool canRead() const override;
    bool read(QImage *image) override;
    bool write(const QImage &image) override;
    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant &value) override;
    bool supportsOption(ImageOption option) const override;

    static bool canRead(QIODevice *iod);

private:
    QScopedPointer<QIIOFHelper> d;
};

QT_END_NAMESPACE

#endif // QMACHEIFHANDLER_P_H
