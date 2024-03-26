// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef XMLCONVERTER_H
#define XMLCONVERTER_H

#include "converter.h"

class XmlConverter : public Converter
{
    // Converter interface
public:
    QString name() const override;
    Directions directions() const override;
    Options outputOptions() const override;
    const char *optionsHelp() const override;
    bool probeFile(QIODevice *f) const override;
    QVariant loadFile(QIODevice *f, const Converter *&outputConverter) const override;
    void saveFile(QIODevice *f, const QVariant &contents,
                  const QStringList &options) const override;
};

#endif // XMLCONVERTER_H
