// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusinterface.h"
#include "qdbusinterface_p.h"

#include "qdbus_symbols_p.h"
#include <QtCore/qpointer.h>
#include <QtCore/qstringlist.h>

#include "qdbusmetatype_p.h"
#include "qdbusconnection_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static void copyArgument(void *to, int id, const QVariant &arg)
{
    if (id == arg.metaType().id()) {
        switch (id) {
        case QMetaType::Bool:
            *reinterpret_cast<bool *>(to) = arg.toBool();
            return;

        case QMetaType::UChar:
            *reinterpret_cast<uchar *>(to) = qvariant_cast<uchar>(arg);
            return;

        case QMetaType::Short:
            *reinterpret_cast<short *>(to) = qvariant_cast<short>(arg);
            return;

        case QMetaType::UShort:
            *reinterpret_cast<ushort *>(to) = qvariant_cast<ushort>(arg);
            return;

        case QMetaType::Int:
            *reinterpret_cast<int *>(to) = arg.toInt();
            return;

        case QMetaType::UInt:
            *reinterpret_cast<uint *>(to) = arg.toUInt();
            return;

        case QMetaType::LongLong:
            *reinterpret_cast<qlonglong *>(to) = arg.toLongLong();
            return;

        case QMetaType::ULongLong:
            *reinterpret_cast<qulonglong *>(to) = arg.toULongLong();
            return;

        case QMetaType::Double:
            *reinterpret_cast<double *>(to) = arg.toDouble();
            return;

        case QMetaType::QString:
            *reinterpret_cast<QString *>(to) = arg.toString();
            return;

        case QMetaType::QByteArray:
            *reinterpret_cast<QByteArray *>(to) = arg.toByteArray();
            return;

        case QMetaType::QStringList:
            *reinterpret_cast<QStringList *>(to) = arg.toStringList();
            return;
        }

        if (id == QDBusMetaTypeId::variant().id()) {
            *reinterpret_cast<QDBusVariant *>(to) = qvariant_cast<QDBusVariant>(arg);
            return;
        } else if (id == QDBusMetaTypeId::objectpath().id()) {
            *reinterpret_cast<QDBusObjectPath *>(to) = qvariant_cast<QDBusObjectPath>(arg);
            return;
        } else if (id == QDBusMetaTypeId::signature().id()) {
            *reinterpret_cast<QDBusSignature *>(to) = qvariant_cast<QDBusSignature>(arg);
            return;
        }

        // those above are the only types possible
        // the demarshaller code doesn't demarshall anything else
        qFatal("Found a decoded basic type in a D-Bus reply that shouldn't be there");
    }

    // if we got here, it's either an un-dermarshalled type or a mismatch
    if (arg.metaType() != QDBusMetaTypeId::argument()) {
        // it's a mismatch
        //qWarning?
        return;
    }

    // is this type registered?
    const char *userSignature = QDBusMetaType::typeToSignature(QMetaType(id));
    if (!userSignature || !*userSignature) {
        // type not registered
        //qWarning?
        return;
    }

    // is it the same signature?
    QDBusArgument dbarg = qvariant_cast<QDBusArgument>(arg);
    if (dbarg.currentSignature() != QLatin1StringView(userSignature)) {
        // not the same signature, another mismatch
        //qWarning?
        return;
    }

    // we can demarshall
    QDBusMetaType::demarshall(dbarg, QMetaType(id), to);
}

QDBusInterfacePrivate::QDBusInterfacePrivate(const QString &serv, const QString &p,
                                             const QString &iface, const QDBusConnection &con)
    : QDBusAbstractInterfacePrivate(serv, p, iface, con, true), metaObject(nullptr)
{
    // QDBusAbstractInterfacePrivate's constructor checked the parameters for us
    if (connection.isConnected()) {
        metaObject = connectionPrivate()->findMetaObject(service, path, interface, lastError);

        if (!metaObject) {
            // creation failed, somehow
            // most common causes are that the service doesn't exist or doesn't support introspection
            // those are not fatal errors, so we continue working

            if (!lastError.isValid())
                lastError = QDBusError(QDBusError::InternalError, "Unknown error"_L1);
        }
    }
}

QDBusInterfacePrivate::~QDBusInterfacePrivate()
{
    if (metaObject && !metaObject->cached)
        delete metaObject;
}


/*!
    \class QDBusInterface
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusInterface class is a proxy for interfaces on remote objects.

    QDBusInterface is a generic accessor class that is used to place calls to remote objects,
    connect to signals exported by remote objects and get/set the value of remote properties. This
    class is useful for dynamic access to remote objects: that is, when you do not have a generated
    code that represents the remote interface.

    Calls are usually placed by using the call() function, which constructs the message, sends it
    over the bus, waits for the reply and decodes the reply. Signals are connected to by using the
    normal QObject::connect() function. Finally, properties are accessed using the
    QObject::property() and QObject::setProperty() functions.

    The following code snippet demonstrates how to perform a
    mathematical operation of \tt{"2 + 2"} in a remote application
    called \c com.example.Calculator, accessed via the session bus.

    \snippet code/src_qdbus_qdbusinterface.cpp 0

    \sa {Qt D-Bus XML compiler (qdbusxml2cpp)}
*/

/*!
    Creates a dynamic QDBusInterface object associated with the
    interface \a interface on object at path \a path on service \a
    service, using the given \a connection. If \a interface is an
    empty string, the object created will refer to the merging of all
    interfaces found by introspecting that object. Otherwise if
    \a interface is not empty, the  QDBusInterface object will be cached
    to speedup further creations of the same interface.

    \a parent is passed to the base class constructor.

    If the remote service \a service is not present or if an error
    occurs trying to obtain the description of the remote interface
    \a interface, the object created will not be valid (see
    isValid()).
*/
QDBusInterface::QDBusInterface(const QString &service, const QString &path, const QString &interface,
                               const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(*new QDBusInterfacePrivate(service, path, interface, connection),
                             parent)
{
}

/*!
    Destroy the object interface and frees up any resource used.
*/
QDBusInterface::~QDBusInterface()
{
    // resources are freed in QDBusInterfacePrivate::~QDBusInterfacePrivate()
}

/*!
    \internal
    Overrides QObject::metaObject to return our own copy.
*/
const QMetaObject *QDBusInterface::metaObject() const
{
    return d_func()->metaObject ? d_func()->metaObject : &QDBusAbstractInterface::staticMetaObject;
}

/*!
    \internal
    Override QObject::qt_metacast to catch the interface name too.
*/
void *QDBusInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, "QDBusInterface"))
        return static_cast<void*>(const_cast<QDBusInterface*>(this));
    if (d_func()->interface.toLatin1() == _clname)
        return static_cast<void*>(const_cast<QDBusInterface*>(this));
    return QDBusAbstractInterface::qt_metacast(_clname);
}

/*!
    \internal
    Dispatch the call through the private.
*/
int QDBusInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractInterface::qt_metacall(_c, _id, _a);
    if (_id < 0 || !d_func()->isValid || !d_func()->metaObject)
        return _id;
    return d_func()->metacall(_c, _id, _a);
}

int QDBusInterfacePrivate::metacall(QMetaObject::Call c, int id, void **argv)
{
    Q_Q(QDBusInterface);

    if (c == QMetaObject::InvokeMetaMethod) {
        int offset = metaObject->methodOffset();
        QMetaMethod mm = metaObject->method(id + offset);

        if (mm.methodType() == QMetaMethod::Signal) {
            // signal relay from D-Bus world to Qt world
            QMetaObject::activate(q, metaObject, id, argv);

        } else if (mm.methodType() == QMetaMethod::Slot || mm.methodType() == QMetaMethod::Method) {
            // method call relay from Qt world to D-Bus world
            // get D-Bus equivalent signature
            QString methodName = QString::fromLatin1(mm.name());
            const int *inputTypes = metaObject->inputTypesForMethod(id);
            int inputTypesCount = *inputTypes;

            // we will assume that the input arguments were passed correctly
            QVariantList args;
            args.reserve(inputTypesCount);
            int i = 1;
            for ( ; i <= inputTypesCount; ++i)
                args << QVariant(QMetaType(inputTypes[i]), argv[i]);

            // make the call
            QDBusMessage reply = q->callWithArgumentList(QDBus::Block, methodName, args);

            if (reply.type() == QDBusMessage::ReplyMessage) {
                // attempt to demarshall the return values
                args = reply.arguments();
                QVariantList::ConstIterator it = args.constBegin();
                const int *outputTypes = metaObject->outputTypesForMethod(id);
                int outputTypesCount = *outputTypes++;

                if (mm.returnType() != QMetaType::UnknownType && mm.returnType() != QMetaType::Void) {
                    // this method has a return type
                    if (argv[0] && it != args.constEnd())
                        copyArgument(argv[0], *outputTypes++, *it);

                    // skip this argument even if we didn't copy it
                    --outputTypesCount;
                    ++it;
                }

                for (int j = 0; j < outputTypesCount && it != args.constEnd(); ++i, ++j, ++it) {
                    copyArgument(argv[i], outputTypes[j], *it);
                }
            }

            // done
            lastError = QDBusError(reply);
            return -1;
        }
    }
    return id;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
