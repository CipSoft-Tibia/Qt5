/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtXmlPatterns>

class Schema
{
    public:
        void loadFromUrl() const;
        void loadFromFile() const;
        void loadFromData() const;
};

void Schema::loadFromUrl() const
{
//! [0]
    QUrl url("http://www.schema-example.org/myschema.xsd");

    QXmlSchema schema;
    if (schema.load(url) == true)
        qDebug() << "schema is valid";
    else
        qDebug() << "schema is invalid";
//! [0]
}

void Schema::loadFromFile() const
{
//! [1]
    QFile file("myschema.xsd");
    file.open(QIODevice::ReadOnly);

    QXmlSchema schema;
    schema.load(&file, QUrl::fromLocalFile(file.fileName()));

    if (schema.isValid())
        qDebug() << "schema is valid";
    else
        qDebug() << "schema is invalid";
//! [1]
}

void Schema::loadFromData() const
{
//! [2]
    QByteArray data( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                     "<xsd:schema"
                     "        xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
                     "        xmlns=\"http://www.qt-project.org/xmlschematest\""
                     "        targetNamespace=\"http://www.qt-project.org/xmlschematest\""
                     "        version=\"1.0\""
                     "        elementFormDefault=\"qualified\">"
                     "</xsd:schema>" );

    QXmlSchema schema;
    schema.load(data);

    if (schema.isValid())
        qDebug() << "schema is valid";
    else
        qDebug() << "schema is invalid";
//! [2]
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Schema schema;

    schema.loadFromUrl();
    schema.loadFromFile();
    schema.loadFromData();

    return 0;
}
