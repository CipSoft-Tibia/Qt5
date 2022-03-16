/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the ActiveQt framework of the Qt Toolkit.
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

#include "qaxobject.h"

#include <quuid.h>
#include <qmetaobject.h>
#include <qstringlist.h>

#include <windows.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAxObject
    \brief The QAxObject class provides a QObject that wraps a COM object.

    \inmodule QAxContainer

    A QAxObject can be instantiated as an empty object, with the name
    of the COM object it should wrap, or with a pointer to the
    IUnknown that represents an existing COM object. If the COM object
    implements the \c IDispatch interface, the properties, methods and
    events of that object become available as Qt properties, slots and
    signals. The base class, QAxBase, provides an API to access the
    COM object directly through the IUnknown pointer.

    QAxObject is a QObject and can be used as such, e.g. it can be
    organized in an object hierarchy, receive events and connect to
    signals and slots.

    QAxObject also inherits most of its ActiveX-related functionality
    from QAxBase, notably dynamicCall() and querySubObject().

    \warning
    You can subclass QAxObject, but you cannot use the Q_OBJECT macro
    in the subclass (the generated moc-file will not compile), so you
    cannot add further signals, slots or properties. This limitation is
    due to the metaobject information generated in runtime.
    To work around this problem, aggregate the QAxObject as a member of
    the QObject subclass.

    \sa QAxBase, QAxWidget, QAxScript, {ActiveQt Framework}
*/

const QMetaObject QAxObject::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QAxBase.data,
      qt_meta_data_QAxBase, qt_static_metacall, 0, 0 }
};

/*!
    Creates an empty COM object and propagates \a parent to the
    QObject constructor. To initialize the object, call \link
    QAxBase::setControl() setControl \endlink.
*/
QAxObject::QAxObject(QObject *parent)
: QObject(parent)
{
}

/*!
    Creates a QAxObject that wraps the COM object \a c. \a parent is
    propagated to the QObject constructor.

    \sa setControl()
*/
QAxObject::QAxObject(const QString &c, QObject *parent)
: QObject(parent)
{
    setControl(c);
}

/*!
    Creates a QAxObject that wraps the COM object referenced by \a
    iface. \a parent is propagated to the QObject constructor.
*/
QAxObject::QAxObject(IUnknown *iface, QObject *parent)
: QObject(parent), QAxBase(iface)
{
}

/*!
    Releases the COM object and destroys the QAxObject,
    cleaning up all allocated resources.
*/
QAxObject::~QAxObject()
{
    clear();
}

/*!
    \internal
*/
void QAxObject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    QAxBase::qt_static_metacall(qobject_cast<QAxObject*>(_o), _c, _id, _a);
}

/*!
    \internal
*/
const QMetaObject *QAxObject::fallbackMetaObject() const
{
    return &staticMetaObject;
}

/*!
    \internal
*/
const QMetaObject *QAxObject::metaObject() const
{
    return QAxBase::metaObject();
}

/*!
    \internal
*/
const QMetaObject *QAxObject::parentMetaObject() const
{
    return &QObject::staticMetaObject;
}

/*!
    \internal
*/
void *QAxObject::qt_metacast(const char *cname)
{
    if (!qstrcmp(cname, "QAxObject")) return static_cast<void *>(this);
    if (!qstrcmp(cname, "QAxBase")) return static_cast<QAxBase *>(this);
    return QObject::qt_metacast(cname);
}

/*!
    \internal
*/
const char *QAxObject::className() const
{
    return "QAxObject";
}

/*!
    \internal
*/
int QAxObject::qt_metacall(QMetaObject::Call call, int id, void **v)
{
    id = QObject::qt_metacall(call, id, v);
    if (id < 0)
        return id;
    return QAxBase::qt_metacall(call, id, v);
}

/*!
    \fn QObject *QAxObject::qObject() const
    \internal
*/

/*!
    \reimp
*/
void QAxObject::connectNotify(const QMetaMethod &)
{
    QAxBase::connectNotify();
}

/*!
    \since 4.1

    Requests the COM object to perform the action \a verb. The
    possible verbs are returned by verbs().

    The function returns true if the object could perform the action, otherwise returns false.
*/
bool QAxObject::doVerb(const QString &verb)
{
    if (!verbs().contains(verb))
        return false;
    IOleObject *ole = 0;
    queryInterface(IID_IOleObject, reinterpret_cast<void **>(&ole));
    if (!ole)
        return false;

    LONG index = indexOfVerb(verb);

    HRESULT hres = ole->DoVerb(index, 0, 0, 0, 0, 0);

    ole->Release();

    return hres == S_OK;
}

QT_END_NAMESPACE
