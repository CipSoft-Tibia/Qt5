/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include "domimage.h"

#include <QVariant>

#include <qscriptcontext.h>

QScriptValue DomImage::s_self;

DomImage::DomImage()
{
}


int DomImage::width() const
{
    return m_image.width();
}


int DomImage::height() const
{
    return m_image.height();
}


QString DomImage::src() const
{
    return m_src;
}

void DomImage::setSrc(const QString &src)
{
    m_src = src;
    m_image = QImage(m_src);
}


QString DomImage::name() const
{
    return m_src;
}

static QScriptValue Image(QScriptContext *context, QScriptEngine *env)
{
    QScriptValue val = context->thisObject();
    DomImage *image = new DomImage();
    QScriptValue klass = env->newVariant(QVariant::fromValue(image));
    klass.setPrototype(DomImage::s_self);
    return klass;
}


static QScriptValue width(QScriptContext *context, QScriptEngine *)
{
    QScriptValue val = context->thisObject();

    DomImage *image = qvariant_cast<DomImage*> (val.toVariant());
    if (image)
        return image->width();

    return 0;
}


static QScriptValue height(QScriptContext *context, QScriptEngine *)
{
    QScriptValue val = context->thisObject();

    DomImage *image = qvariant_cast<DomImage*> (val.toVariant());
    if (image)
        return image->height();

    return 0;
}


static QScriptValue setSrc(QScriptContext *context, QScriptEngine *env)
{
    QScriptValue val = context->thisObject();
    QString src  = context->argument(0).toString();

    DomImage *image = qvariant_cast<DomImage*> (val.toVariant());
    if (image)
        image->setSrc(src);

    return env->undefinedValue();
}


static QScriptValue name(QScriptContext *context, QScriptEngine *)
{
    QScriptValue val = context->thisObject();

    DomImage *image = qvariant_cast<DomImage*> (val.toVariant());
    if (image)
        return image->name();

    return QString();
}


void DomImage::setup(QScriptEngine *e)
{
    qRegisterMetaType<DomImage>();

    e->globalObject().setProperty("Image",
                                  e->newFunction(::Image, 0));

    s_self = e->newObject();
    s_self.setProperty("setSrc", e->newFunction(&::setSrc, 1));
    s_self.setProperty("width", e->newFunction(&::width));
    s_self.setProperty("height", e->newFunction(&::height));
    s_self.setProperty("name", e->newFunction(&::name));

    e->setDefaultPrototype(qMetaTypeId<DomImage>(), s_self);
}
