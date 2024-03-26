// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbluetoothserviceinfo.h"
#include "qbluetoothserviceinfo_p.h"

#include <QUrl>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QBluetoothServiceInfo)
QT_IMPL_METATYPE_EXTERN_TAGGED(QBluetoothServiceInfo::Sequence, QBluetoothServiceInfo__Sequence)
QT_IMPL_METATYPE_EXTERN_TAGGED(QBluetoothServiceInfo::Alternative,
                               QBluetoothServiceInfo__Alternative)

/*!
    \class QBluetoothServiceInfo::Sequence
    \inmodule QtBluetooth
    \brief The Sequence class stores attributes of a Bluetooth Data Element
    Sequence.

    \since 5.2

*/

/*!
    \fn QBluetoothServiceInfo::Sequence::Sequence()

    Constructs a new empty sequence.
*/

/*!
    \fn QBluetoothServiceInfo::Sequence::Sequence(const QList<QVariant> &list)

    Constructs a new sequence that is a copy of \a list.
*/

/*!
    \class QBluetoothServiceInfo::Alternative
    \inmodule QtBluetooth
    \brief The Alternative class stores attributes of a Bluetooth Data Element
    Alternative.

    \since 5.2
*/

/*!
    \fn QBluetoothServiceInfo::Alternative::Alternative()

    Constructs a new empty alternative.
*/

/*!
    \fn QBluetoothServiceInfo::Alternative::Alternative(const QList<QVariant> &list)

    Constructs a new alternative that is a copy of \a list.
*/

/*!
    \class QBluetoothServiceInfo
    \inmodule QtBluetooth
    \brief The QBluetoothServiceInfo class enables access to the attributes of a
    Bluetooth service.

    \since 5.2

    QBluetoothServiceInfo provides information about a service offered by a Bluetooth device.
    In addition it can be used to register new services on the local device. Note that such
    a registration only affects the Bluetooth SDP entries. Any server listening
    for incoming connections (e.g an RFCOMM server) must be started before registerService()
    is called. Deregistration must happen in the reverse order.

    QBluetoothServiceInfo is not a value type in the traditional sense. All copies of the same
    service info object share the same data as they do not detach upon changing them. This
    ensures that two copies can (de)register the same Bluetooth service.

    On iOS, this class cannot be used because the platform does not expose
    an API which may permit access to QBluetoothServiceInfo related features.
*/

/*!
    \enum QBluetoothServiceInfo::AttributeId

    Bluetooth service attributes. Please check the Bluetooth Core Specification for a more detailed description of these attributes.

    \value ServiceRecordHandle      Specifies a service record from which attributes can be retrieved.
    \value ServiceClassIds          UUIDs of service classes that the service conforms to. The
                                    most common service classes are defined in (\l QBluetoothUuid::ServiceClassUuid)
    \value ServiceRecordState       Attibute changes when any other service attribute is added, deleted or modified.
    \value ServiceId                UUID that uniquely identifies the service.
    \value ProtocolDescriptorList   List of protocols used by the service. The most common protocol Uuids are defined
                                    in \l QBluetoothUuid::ProtocolUuid
    \value BrowseGroupList          List of browse groups the service is in.
    \value LanguageBaseAttributeIdList      List of language base attribute IDs to support human-readable attributes.
    \value ServiceInfoTimeToLive    Number of seconds for which the service record is expected to remain valid and unchanged.
    \value ServiceAvailability      Value indicating the availability of the service.
    \value BluetoothProfileDescriptorList   List of profiles to which the service conforms.
    \value DocumentationUrl         URL that points to the documentation on the service..
    \value ClientExecutableUrl      URL that refers to the location of an application that can be used to utilize the service.
    \value IconUrl                  URL to the location of the icon representing the service.
    \value AdditionalProtocolDescriptorList     Additional protocols used by the service. This attribute extends \c ProtocolDescriptorList.
    \value PrimaryLanguageBase      Base index for primary language text descriptors.
    \value ServiceName              Name of the Bluetooth service in the primary language.
    \value ServiceDescription       Description of the Bluetooth service in the primary language.
    \value ServiceProvider          Name of the company / entity that provides the Bluetooth service primary language.

    \note On Windows ServiceClassIds and ProtocolDescriptorList are automatically set to default
    values when a service is created. Manually setting values for these attributes will not work and
    might lead to unexpected results on this platform.
*/

/*!
    \enum QBluetoothServiceInfo::Protocol

    This enum describes the socket protocol used by the service.

    \value UnknownProtocol  The service uses an unknown socket protocol.
    \value L2capProtocol    The service uses the L2CAP socket protocol. This protocol is not supported
                            for direct socket connections on Android.
    \value RfcommProtocol   The service uses the RFCOMM socket protocol.
*/

/*!
    \fn bool QBluetoothServiceInfo::isRegistered() const

    Returns true if the service information is registered with the platform's Service Discovery Protocol
    (SDP) implementation, otherwise returns false.
*/

bool QBluetoothServiceInfo::isRegistered() const
{
    return d_ptr->isRegistered();
}

/*!
    \fn bool QBluetoothServiceInfo::registerService(const QBluetoothAddress &localAdapter)

    Registers this service with the platform's Service Discovery Protocol (SDP) implementation,
    making it findable by other devices when they perform service discovery.  Returns true if the
    service is successfully registered, otherwise returns false.  Once registered changes to the record
    cannot be made. The service must be unregistered and registered again with the changes.

    The \a localAdapter parameter determines the local Bluetooth adapter under which
    the service should be registered. If \a localAdapter is \c null the default Bluetooth adapter
    will be used. If this service info object is already registered via a local adapter
    and this is function is called using a different local adapter, the previous registration
    is removed and the service reregistered using the new adapter.
*/

bool QBluetoothServiceInfo::registerService(const QBluetoothAddress &localAdapter)
{
#ifdef QT_OSX_BLUETOOTH
    Q_UNUSED(localAdapter);
    return d_ptr->registerService(*this);
#else
    return d_ptr->registerService(localAdapter);
#endif
}

/*!
    \fn bool QBluetoothServiceInfo::unregisterService()

    Unregisters this service with the platform's Service Discovery Protocol (SDP) implementation.
    After this, the service will no longer be findable by other devices through service discovery.

    Returns true if the service is successfully unregistered, otherwise returns false.
*/

bool QBluetoothServiceInfo::unregisterService()
{
    return d_ptr->unregisterService();
}


/*!
    \fn void QBluetoothServiceInfo::setAttribute(quint16 attributeId, const QBluetoothUuid &value)

    This is a convenience function.

    Sets the attribute identified by \a attributeId to \a value.

    If the service information is already registered with the platform's SDP database,
    the database entry will not be updated until \l registerService() was called again.
*/

/*!
    \fn void QBluetoothServiceInfo::setAttribute(quint16 attributeId, const QBluetoothServiceInfo::Sequence &value)

    This is a convenience function.

    Sets the attribute identified by \a attributeId to \a value.

    If the service information is already registered with the platform's SDP database,
    the database entry will not be updated until \l registerService() was called again.
*/

/*!
    \fn void QBluetoothServiceInfo::setAttribute(quint16 attributeId, const QBluetoothServiceInfo::Alternative &value)

    This is a convenience function.

    Sets the attribute identified by \a attributeId to \a value.

    If the service information is already registered with the platform's SDP database,
    the database entry will not be updated until \l registerService() was called again.
*/

/*!
    \fn void QBluetoothServiceInfo::setServiceName(const QString &name)

    This is a convenience function. It is equivalent to calling
    setAttribute(QBluetoothServiceInfo::ServiceName, name).

    Sets the service name in the primary language to \a name.

    \sa serviceName(), setAttribute()
*/

/*!
    \fn QString QBluetoothServiceInfo::serviceName() const

    This is a convenience function. It is equivalent to calling
    attribute(QBluetoothServiceInfo::ServiceName).toString().

    Returns the service name in the primary language.

    \sa setServiceName(), attribute()
*/

/*!
    \fn void QBluetoothServiceInfo::setServiceDescription(const QString &description)

    This is a convenience function. It is equivalent to calling
    setAttribute(QBluetoothServiceInfo::ServiceDescription, description).

    Sets the service description in the primary language to \a description.

    \sa serviceDescription(), setAttribute()
*/

/*!
    \fn QString QBluetoothServiceInfo::serviceDescription() const

    This is a convenience function. It is equivalent to calling
    attribute(QBluetoothServiceInfo::ServiceDescription).toString().

    Returns the service description in the primary language.

    \sa setServiceDescription(), attribute()
*/

/*!
    \fn void QBluetoothServiceInfo::setServiceProvider(const QString &provider)

    This is a convenience function. It is equivalent to calling
    setAttribute(QBluetoothServiceInfo::ServiceProvider, provider).

    Sets the service provider in the primary language to \a provider.

    \sa serviceProvider(), setAttribute()
*/

/*!
    \fn QString QBluetoothServiceInfo::serviceProvider() const

    This is a convenience function. It is equivalent to calling
    attribute(QBluetoothServiceInfo::ServiceProvider).toString().

    Returns the service provider in the primary language.

    \sa setServiceProvider(), attribute()
*/

/*!
    \fn void QBluetoothServiceInfo::setServiceAvailability(quint8 availability)

    This is a convenience function. It is equivalent to calling
    setAttribute(QBluetoothServiceInfo::ServiceAvailability, availability).

    Sets the availabiltiy of the service to \a availability.

    \sa serviceAvailability(), setAttribute()
*/

/*!
    \fn quint8 QBluetoothServiceInfo::serviceAvailability() const

    This is a convenience function. It is equivalent to calling
    attribute(QBluetoothServiceInfo::ServiceAvailability).toUInt().

    Returns the availability of the service.

    \sa setServiceAvailability(), attribute()
*/

/*!
    \fn void QBluetoothServiceInfo::setServiceUuid(const QBluetoothUuid &uuid)

    This is a convenience function. It is equivalent to calling
    setAttribute(QBluetoothServiceInfo::ServiceId, uuid).

    Sets the custom service UUID to \a uuid. This function should not be used
    to set a standardized service UUID.

    \sa serviceUuid(), setAttribute()
*/

/*!
    \fn QBluetoothUuid QBluetoothServiceInfo::serviceUuid() const

    This is a convenience function. It is equivalent to calling
    attribute(QBluetoothServiceInfo::ServiceId).value<QBluetoothUuid>().

    Returns the custom UUID of the service. This UUID may be null.
    UUIDs based on \l{https://bluetooth.org}{Bluetooth SIG standards}
    should be retrieved via \l serviceClassUuids().

    \sa setServiceUuid(), attribute()
*/

/*!
    Construct a new invalid QBluetoothServiceInfo;
*/
QBluetoothServiceInfo::QBluetoothServiceInfo()
    : d_ptr(QSharedPointer<QBluetoothServiceInfoPrivate>::create())
{
    qRegisterMetaType<QBluetoothServiceInfo>();
}

/*!
    Construct a new QBluetoothServiceInfo that is a copy of \a other.

    The two copies continue to share the same underlying data which does not detach
    upon write.
*/
QBluetoothServiceInfo::QBluetoothServiceInfo(const QBluetoothServiceInfo &other)
    : d_ptr(other.d_ptr)
{
}

/*!
    Destroys the QBluetoothServiceInfo object.
*/
QBluetoothServiceInfo::~QBluetoothServiceInfo()
{
}

/*!
    Returns true if the QBluetoothServiceInfo object is valid, otherwise returns false.

    An invalid QBluetoothServiceInfo object will have no attributes.
*/
bool QBluetoothServiceInfo::isValid() const
{
    return !d_ptr->attributes.isEmpty();
}

/*!
    Returns true if the QBluetoothServiceInfo object is considered complete, otherwise returns false.

    A complete QBluetoothServiceInfo object contains a ProtocolDescriptorList attribute.
*/
bool QBluetoothServiceInfo::isComplete() const
{
    return d_ptr->attributes.contains(ProtocolDescriptorList);
}

/*!
    Returns the address of the Bluetooth device that provides this service.
*/
QBluetoothDeviceInfo QBluetoothServiceInfo::device() const
{
    return d_ptr->deviceInfo;
}

/*!
    Sets the Bluetooth device that provides this service to \a device.
*/
void QBluetoothServiceInfo::setDevice(const QBluetoothDeviceInfo &device)
{
    d_ptr->deviceInfo = device;
}

/*!
    Sets the attribute identified by \a attributeId to \a value.

    If the service information is already registered with the platform's SDP database,
    the database entry will not be updated until \l registerService() was called again.

    \note If an attribute expectes a byte-encoded value (e.g. Bluetooth HID services),
    it should be set as QByteArray.

    \sa isRegistered(), registerService()
*/
void QBluetoothServiceInfo::setAttribute(quint16 attributeId, const QVariant &value)
{
    d_ptr->attributes[attributeId] = value;
}

/*!
    Returns the value of the attribute \a attributeId.
*/
QVariant QBluetoothServiceInfo::attribute(quint16 attributeId) const
{
    return d_ptr->attributes.value(attributeId);
}

/*!
    Returns a list of all attribute ids that the QBluetoothServiceInfo object has.
*/
QList<quint16> QBluetoothServiceInfo::attributes() const
{
    return d_ptr->attributes.keys();
}

/*!
    Returns true if the QBluetoothServiceInfo object contains the attribute \a attributeId, otherwise returns
    false.
*/
bool QBluetoothServiceInfo::contains(quint16 attributeId) const
{
    return d_ptr->attributes.contains(attributeId);
}

/*!
    Removes the attribute \a attributeId from the QBluetoothServiceInfo object.

    If the service information is already registered with the platforms SDP database,
    the database entry will not be updated until \l registerService() was called again.
*/
void QBluetoothServiceInfo::removeAttribute(quint16 attributeId)
{
    d_ptr->attributes.remove(attributeId);
}

/*!
    Returns the protocol that the QBluetoothServiceInfo object uses.
*/
QBluetoothServiceInfo::Protocol QBluetoothServiceInfo::socketProtocol() const
{
    QBluetoothServiceInfo::Sequence parameters = protocolDescriptor(QBluetoothUuid::ProtocolUuid::Rfcomm);
    if (!parameters.isEmpty())
        return RfcommProtocol;

    parameters = protocolDescriptor(QBluetoothUuid::ProtocolUuid::L2cap);
    if (!parameters.isEmpty())
        return L2capProtocol;

    return UnknownProtocol;
}

/*!
    This is a convenience function. Returns the protocol/service multiplexer for services which
    support the L2CAP protocol, otherwise returns -1.

    This function is equivalent to extracting the information from
    QBluetoothServiceInfo::Sequence returned by
    QBluetoothServiceInfo::attribute(QBluetoothServiceInfo::ProtocolDescriptorList).
*/
int QBluetoothServiceInfo::protocolServiceMultiplexer() const
{
    QBluetoothServiceInfo::Sequence parameters = protocolDescriptor(QBluetoothUuid::ProtocolUuid::L2cap);

    if (parameters.isEmpty())
        return -1;
    else if (parameters.size() == 1)
        return 0;
    else
        return parameters.at(1).toUInt();
}

/*!
    This is a convenience function. Returns the server channel for services which support the
    RFCOMM protocol, otherwise returns -1.

    This function is equivalent to extracting the information from
    QBluetoothServiceInfo::Sequence returned by
    QBluetoothServiceInfo::attribute(QBluetootherServiceInfo::ProtocolDescriptorList).
*/
int QBluetoothServiceInfo::serverChannel() const
{
    return d_ptr->serverChannel();
}

/*!
    Returns the protocol parameters as a QBluetoothServiceInfo::Sequence for protocol \a protocol.

    An empty QBluetoothServiceInfo::Sequence is returned if \a protocol is not supported.
*/
QBluetoothServiceInfo::Sequence QBluetoothServiceInfo::protocolDescriptor(QBluetoothUuid::ProtocolUuid protocol) const
{
    return d_ptr->protocolDescriptor(protocol);
}

/*!
    Returns a list of UUIDs describing the service classes that this service conforms to.

    This is a convenience function. It is equivalent to calling
    attribute(QBluetoothServiceInfo::ServiceClassIds).value<QBluetoothServiceInfo::Sequence>()
    and subsequently iterating over its QBluetoothUuid entries.

    \sa attribute()
*/
QList<QBluetoothUuid> QBluetoothServiceInfo::serviceClassUuids() const
{
    QList<QBluetoothUuid> results;

    const QVariant var = attribute(QBluetoothServiceInfo::ServiceClassIds);
    if (!var.isValid())
        return results;

    const QBluetoothServiceInfo::Sequence seq = var.value<QBluetoothServiceInfo::Sequence>();
    for (qsizetype i = 0; i < seq.size(); ++i)
        results.append(seq.at(i).value<QBluetoothUuid>());

    return results;
}

/*!
    Makes a copy of the \a other and assigns it to this QBluetoothServiceInfo object.
    The two copies continue to share the same service and registration details.
*/
QBluetoothServiceInfo &QBluetoothServiceInfo::operator=(const QBluetoothServiceInfo &other)
{
    d_ptr = other.d_ptr;

    return *this;
}

static void dumpAttributeVariant(QDebug dbg, const QVariant &var, const QString& indent)
{
    switch (var.typeId()) {
    case QMetaType::Void:
        dbg << QString::asprintf("%sEmpty\n", indent.toUtf8().constData());
        break;
    case QMetaType::UChar:
        dbg << QString::asprintf("%suchar %u\n", indent.toUtf8().constData(), var.toUInt());
        break;
    case QMetaType::UShort:
        dbg << QString::asprintf("%sushort %u\n", indent.toUtf8().constData(), var.toUInt());
        break;
    case QMetaType::UInt:
        dbg << QString::asprintf("%suint %u\n", indent.toUtf8().constData(), var.toUInt());
        break;
    case QMetaType::Char:
        dbg << QString::asprintf("%schar %d\n", indent.toUtf8().constData(), var.toInt());
        break;
    case QMetaType::Short:
        dbg << QString::asprintf("%sshort %d\n", indent.toUtf8().constData(), var.toInt());
        break;
    case QMetaType::Int:
        dbg << QString::asprintf("%sint %d\n", indent.toUtf8().constData(), var.toInt());
        break;
    case QMetaType::QString:
        dbg << QString::asprintf("%sstring %s\n", indent.toUtf8().constData(),
                                 var.toString().toUtf8().constData());
        break;
    case QMetaType::QByteArray:
        dbg << QString::asprintf("%sbytearray %s\n", indent.toUtf8().constData(),
                                 var.toByteArray().toHex().constData());
        break;
    case QMetaType::Bool:
        dbg << QString::asprintf("%sbool %d\n", indent.toUtf8().constData(), var.toBool());
        break;
    case QMetaType::QUrl:
        dbg << QString::asprintf("%surl %s\n", indent.toUtf8().constData(),
                                 var.toUrl().toString().toUtf8().constData());
        break;
    default:
        if (var.typeId() == qMetaTypeId<QBluetoothUuid>()) {
            QBluetoothUuid uuid = var.value<QBluetoothUuid>();
            switch (uuid.minimumSize()) {
            case 0:
                dbg << QString::asprintf("%suuid NULL\n", indent.toUtf8().constData());
                break;
            case 2:
                dbg << QString::asprintf("%suuid2 %04x\n", indent.toUtf8().constData(),
                                         uuid.toUInt16());
                break;
            case 4:
                dbg << QString::asprintf("%suuid %08x\n", indent.toUtf8().constData(),
                                         uuid.toUInt32());
                break;
            case 16:
                dbg << QString::asprintf("%suuid %s\n",
                            indent.toUtf8().constData(),
                            uuid.toByteArray(QUuid::Id128).constData());
                break;
            default:
                dbg << QString::asprintf("%suuid ???\n", indent.toUtf8().constData());
            }
        } else if (var.typeId() == qMetaTypeId<QBluetoothServiceInfo::Sequence>()) {
            dbg << QString::asprintf("%sSequence\n", indent.toUtf8().constData());
            const QBluetoothServiceInfo::Sequence *sequence = static_cast<const QBluetoothServiceInfo::Sequence *>(var.data());
            for (const QVariant &v : *sequence)
                dumpAttributeVariant(dbg, v, indent + QLatin1Char('\t'));
        } else if (var.typeId() == qMetaTypeId<QBluetoothServiceInfo::Alternative>()) {
            dbg << QString::asprintf("%sAlternative\n", indent.toUtf8().constData());
            const QBluetoothServiceInfo::Alternative *alternative = static_cast<const QBluetoothServiceInfo::Alternative *>(var.data());
            for (const QVariant &v : *alternative)
                dumpAttributeVariant(dbg, v, indent + QLatin1Char('\t'));
        } else {
            dbg << QString::asprintf("%sunknown variant type %d\n", indent.toUtf8().constData(), var.typeId());
        }
    }
}

#ifndef QT_NO_DEBUG_STREAM
QDebug QBluetoothServiceInfo::streamingOperator(QDebug dbg, const QBluetoothServiceInfo &info)
{
    QDebugStateSaver saver(dbg);
    dbg.noquote() << "\n";
    const QList<quint16> attributes = info.attributes();
    for (quint16 id : attributes) {
        dumpAttributeVariant(dbg, info.attribute(id), QStringLiteral("(%1)\t").arg(id));
    }
    return dbg;
}
#endif

QBluetoothServiceInfo::Sequence QBluetoothServiceInfoPrivate::protocolDescriptor(QBluetoothUuid::ProtocolUuid protocol) const
{
    if (!attributes.contains(QBluetoothServiceInfo::ProtocolDescriptorList))
        return QBluetoothServiceInfo::Sequence();

    const QBluetoothServiceInfo::Sequence sequence
            = attributes.value(QBluetoothServiceInfo::ProtocolDescriptorList).value<QBluetoothServiceInfo::Sequence>();
    for (const QVariant &v : sequence) {
        QBluetoothServiceInfo::Sequence parameters = v.value<QBluetoothServiceInfo::Sequence>();
        if (parameters.empty())
            continue;
        if (parameters.at(0).userType() == qMetaTypeId<QBluetoothUuid>()) {
            if (parameters.at(0).value<QBluetoothUuid>() == protocol)
                return parameters;
        }
    }

    return QBluetoothServiceInfo::Sequence();
}

int QBluetoothServiceInfoPrivate::serverChannel() const
{
    QBluetoothServiceInfo::Sequence parameters = protocolDescriptor(QBluetoothUuid::ProtocolUuid::Rfcomm);

    if (parameters.isEmpty())
        return -1;
    else if (parameters.size() == 1)
        return 0;
    else
        return parameters.at(1).toUInt();
}

QT_END_NAMESPACE
