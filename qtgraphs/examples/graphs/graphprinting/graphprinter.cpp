// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "graphprinter.h"
#include <QtGui/qtguiglobal.h>
#include <QtGui/qtransform.h>
#include <QtPrintSupport/QtPrintSupport>
#if QT_CONFIG(opengl)
#include <QtGui/qoffscreensurface.h>
#endif
#include <rhi/qrhi.h>

GraphPrinter::GraphPrinter(QObject *parent)
    : QObject(parent)
{}

GraphPrinter::~GraphPrinter() {}

static qreal s_maxTextureSize = 0.;

qreal GraphPrinter::maxTextureSize()
{
    // Query maximum texture size only once
    if (!s_maxTextureSize) {
        std::unique_ptr<QRhi> rhi;
#if defined(Q_OS_WIN)
        QRhiD3D12InitParams params;
        rhi.reset(QRhi::create(QRhi::D3D12, &params));
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        QRhiMetalInitParams params;
        rhi.reset(QRhi::create(QRhi::Metal, &params));
#elif QT_CONFIG(opengl)
        QRhiGles2InitParams params;
        params.fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
        rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
#elif QT_CONFIG(vulkan)
        if (!qEnvironmentVariable("QSG_RHI_BACKEND").compare("vulkan")) {
            QVulkanInstance inst;
            inst.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
            if (inst.create()) {
                QRhiVulkanInitParams params;
                params.inst = &inst;
                rhi.reset(QRhi::create(QRhi::Vulkan, &params));
            } else {
                qWarning("Failed to create Vulkan instance");
            }
        }
#endif
        if (rhi)
            s_maxTextureSize = qreal(rhi->resourceLimit(QRhi::TextureSizeMax));
        else
            s_maxTextureSize = 4096.; // Use 4096 as the minimum
    }

    return s_maxTextureSize;
}

QString GraphPrinter::generatePDF(const QUrl &path, const QImage &image)
{
    //! [0]
    const QFile file = QFile(path.toLocalFile() + QStringLiteral("/graph.pdf"));

    QPdfWriter writer(file.fileName());
    writer.setResolution(90);
    writer.setTitle("Graph");
    writer.setPageSize(QPageSize(image.size()));
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    writer.newPage();
    //! [0]

    //! [1]
    QPainter painter(&writer);
    const QImage finalImage = image.scaled(painter.viewport().size(), Qt::KeepAspectRatio);
    painter.setRenderHint(QPainter::LosslessImageRendering);
    painter.drawImage(finalImage.rect(), finalImage);
    //! [1]

    return file.fileName();
}

//! [2]
QString GraphPrinter::print(const QImage &image, const QString printerName)
{
    QPrinterInfo printInfo = QPrinterInfo::printerInfo(printerName);
    if (printInfo.isNull())
        return QLatin1String("%1 is not a valid printer").arg(printerName);

    QPrinter printer(printInfo, QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::NativeFormat);

    QPainter painter(&printer);
    const QImage finalImage = image.scaled(painter.viewport().size(), Qt::KeepAspectRatio);
    painter.setRenderHint(QPainter::LosslessImageRendering);
    painter.drawImage(finalImage.rect(), finalImage);

    return QLatin1String("Printed to %1").arg(printerName);
}
//! [2]

//! [3]
QStringList GraphPrinter::getPrinters()
{
    return QPrinterInfo::availablePrinterNames();
}
//! [3]
