// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbluetoothserviceinfo.h"
#include "qbluetoothserviceinfo_p.h"
#include "qbluetoothserver_p.h"
#include "qbluetoothutils_winrt_p.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qfunctions_winrt_p.h>

#include <wrl.h>
#include <windows.devices.bluetooth.h>
#include <windows.devices.bluetooth.rfcomm.h>
#include <windows.foundation.h>
#include <windows.networking.sockets.h>
#include <windows.storage.streams.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Devices::Bluetooth;
using namespace ABI::Windows::Devices::Bluetooth::Rfcomm;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Networking::Sockets;
using namespace ABI::Windows::Storage::Streams;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_BT_WINDOWS)

#define TYPE_VOID              0
#define TYPE_UINT8             8
#define TYPE_UINT16            9
#define TYPE_UINT32           10
#define TYPE_UINT64           11
//#define TYPE_UINT128          12
#define TYPE_INT8             16
#define TYPE_INT16            17
#define TYPE_INT32            18
#define TYPE_INT64            19
//#define TYPE_INT128           20
#define TYPE_UUID16           25
#define TYPE_UUID32           26
#define TYPE_UUID128          28
#define TYPE_STRING_BASE      32
#define TYPE_BOOLEAN          40
#define TYPE_SEQUENCE_BASE    48
#define TYPE_ALTERNATIVE_BASE 56
#define TYPE_URL_BASE         64

extern QHash<QBluetoothServerPrivate *, int> __fakeServerPorts;

inline bool typeIsOfBase(unsigned char type, unsigned char baseType)
{
    return ((type & baseType) == baseType);
}

qint64 getLengthForBaseType(unsigned char type, ComPtr<IDataReader> &reader)
{
    const bool isOfBase = (typeIsOfBase(type, TYPE_STRING_BASE)
                           || typeIsOfBase(type, TYPE_SEQUENCE_BASE)
                           || typeIsOfBase(type, TYPE_ALTERNATIVE_BASE)
                           || typeIsOfBase(type, TYPE_URL_BASE));
    if (!isOfBase)
        return -1;

    HRESULT hr;
    // For these types, the first 5 bits are the base type followed by 3 bits
    // describing the size index. This index decides how many additional bits
    // have to be read to get the type's length.
    const unsigned char sizeIndex = (type & 0x7);
    switch (sizeIndex) {
    case 5: {
        quint8 length;
        hr = reader->ReadByte(&length);
        RETURN_IF_FAILED("Could not read length from buffer", return -1);
        return length;
    } case 6: {
        quint16 length;
        hr = reader->ReadUInt16(&length);
        RETURN_IF_FAILED("Could not read length from buffer", return -1);
        return length;
    } case 7: {
        quint32 length;
        hr = reader->ReadUInt32(&length);
        RETURN_IF_FAILED("Could not read length from buffer", return -1);
        return length;
    }
    }
    return -1;
}

bool writeStringHelper(const QString &string, ComPtr<IDataWriter> writer)
{
    HRESULT hr;
    const qsizetype stringLength = string.size();
    unsigned char type = TYPE_STRING_BASE;
    if (stringLength < 0) {
        qCWarning(QT_BT_WINDOWS) << "Can not write invalid string value to buffer";
        return false;
    } if (stringLength <= 0xff) {
        type += 5;
        hr = writer->WriteByte(type);
        RETURN_FALSE_IF_FAILED("Could not write string type data.");
        hr = writer->WriteByte(stringLength);
        RETURN_FALSE_IF_FAILED("Could not write string length.");
    } else if (stringLength <= 0xffff) {
        type += 6;
        hr = writer->WriteByte(type);
        RETURN_FALSE_IF_FAILED("Could not write string type data.");
        hr = writer->WriteUInt16(stringLength);
        RETURN_FALSE_IF_FAILED("Could not write string length.");
    } else {
        type += 7;
        hr = writer->WriteByte(type);
        RETURN_FALSE_IF_FAILED("Could not write string type data.");
        hr = writer->WriteUInt32(stringLength);
        RETURN_FALSE_IF_FAILED("Could not write string length.");
    }
    HStringReference stringRef(reinterpret_cast<LPCWSTR>(string.utf16()));
    quint32 bytesWritten;
    hr = writer->WriteString(stringRef.Get(), &bytesWritten);
    RETURN_FALSE_IF_FAILED("Could not write string to buffer.");
    if (bytesWritten != size_t(string.size())) {
        qCWarning(QT_BT_WINDOWS) << "Did not write full value to buffer";
        return false;
    }
    return true;
}

bool repairProfileDescriptorListIfNeeded(ComPtr<IBuffer> &buffer)
{
    ComPtr<IDataReaderStatics> dataReaderStatics;
    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_DataReader).Get(),
                                      &dataReaderStatics);
    Q_ASSERT_SUCCEEDED(hr);

    ComPtr<IDataReader> reader;
    hr = dataReaderStatics->FromBuffer(buffer.Get(), reader.GetAddressOf());
    Q_ASSERT_SUCCEEDED(hr);

    BYTE type;
    hr = reader->ReadByte(&type);
    Q_ASSERT_SUCCEEDED(hr);
    if (!typeIsOfBase(type, TYPE_SEQUENCE_BASE)) {
        qCWarning(QT_BT_WINDOWS) << Q_FUNC_INFO << "Malformed profile descriptor list read";
        return false;
    }

    qint64 length = getLengthForBaseType(type, reader);
    hr = reader->ReadByte(&type);
    Q_ASSERT_SUCCEEDED(hr);
    // We have to "repair" the structure if the outer sequence contains a uuid directly
    if (type == TYPE_UUID16 && length == 4) {
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Repairing profile descriptor list";
        quint16 uuid;
        hr = reader->ReadUInt16(&uuid);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<IDataWriter> writer;
        hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
            &writer);
        Q_ASSERT_SUCCEEDED(hr);

        hr = writer->WriteByte(TYPE_SEQUENCE_BASE + 5);
        Q_ASSERT_SUCCEEDED(hr);
        // 8 == length of nested sequence (outer sequence -> inner sequence -> uuid and version)
        hr = writer->WriteByte(8);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteByte(TYPE_SEQUENCE_BASE + 5);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteByte(7);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteByte(TYPE_UUID16);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteUInt16(uuid);
        Q_ASSERT_SUCCEEDED(hr);
        // Write default version to make WinRT happy
        hr = writer->WriteByte(TYPE_UINT16);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteUInt16(0x100);
        Q_ASSERT_SUCCEEDED(hr);

        hr = writer->DetachBuffer(&buffer);
        Q_ASSERT_SUCCEEDED(hr);
    }

    return true;
}

static ComPtr<IBuffer> bufferFromAttribute(const QVariant &attribute)
{
    ComPtr<IDataWriter> writer;
    HRESULT hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
                                  &writer);
    Q_ASSERT_SUCCEEDED(hr);

    switch (attribute.typeId()) {
    case QMetaType::Void:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::Void:";
        hr = writer->WriteByte(TYPE_VOID);
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::UChar:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::UChar:" << attribute.value<quint8>();
        hr = writer->WriteByte(TYPE_UINT8);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteByte(attribute.value<quint8>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::UShort:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::UShort:" << attribute.value<quint16>();
        hr = writer->WriteByte(TYPE_UINT16);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteUInt16(attribute.value<quint16>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::UInt:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::UInt:" << attribute.value<quint32>();
        hr = writer->WriteByte(TYPE_UINT32);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteUInt32(attribute.value<quint32>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::ULongLong:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::ULongLong:" << attribute.value<quint64>();
        hr = writer->WriteByte(TYPE_UINT64);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteUInt64(attribute.value<quint64>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::Char:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::Char:" << attribute.value<qint8>();
        hr = writer->WriteByte(TYPE_INT8);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteByte(attribute.value<qint8>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::Short:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::Short:" << attribute.value<qint16>();
        hr = writer->WriteByte(TYPE_INT16);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteInt16(attribute.value<qint16>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::Int:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::Int:" << attribute.value<qint32>();
        hr = writer->WriteByte(TYPE_INT32);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteInt32(attribute.value<qint32>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::LongLong:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::LongLong:" << attribute.value<qint64>();
        hr = writer->WriteByte(TYPE_INT64);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteInt64(attribute.value<qint64>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::QByteArray: {
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::QByteArray:" << attribute.value<QString>();
        const QString stringValue = QString::fromLatin1(attribute.value<QByteArray>().toHex());
        const bool writeSuccess = writeStringHelper(stringValue, writer);
        if (!writeSuccess)
            return nullptr;
        break;
    }
    case QMetaType::QString: {
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::QString:" << attribute.value<QString>();
        const QString stringValue = attribute.value<QString>();
        const bool writeSucces = writeStringHelper(stringValue, writer);
        if (!writeSucces)
            return nullptr;
        break;
    }
    case QMetaType::Bool:
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering attribute of type QMetaType::Bool:" << attribute.value<bool>();
        hr = writer->WriteByte(TYPE_BOOLEAN);
        Q_ASSERT_SUCCEEDED(hr);
        hr = writer->WriteByte(attribute.value<bool>());
        Q_ASSERT_SUCCEEDED(hr);
        break;
    case QMetaType::QUrl:
        qCWarning(QT_BT_WINDOWS) << "Don't know how to register QMetaType::QUrl";
        return nullptr;
        break;
    default:
        if (attribute.userType() == qMetaTypeId<QBluetoothUuid>()) {
            QBluetoothUuid uuid = attribute.value<QBluetoothUuid>();
            switch (uuid.minimumSize()) {
            case 0:
                qCWarning(QT_BT_WINDOWS) << "Don't know how to register Uuid of length 0";
                return nullptr;
            case 2:
                qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering Uuid attribute with length 2:" << uuid;
                hr = writer->WriteByte(TYPE_UUID16);
                Q_ASSERT_SUCCEEDED(hr);
                hr = writer->WriteUInt16(uuid.toUInt16());
                Q_ASSERT_SUCCEEDED(hr);
                break;
            case 4:
                qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering Uuid attribute with length 4:" << uuid;
                hr = writer->WriteByte(TYPE_UUID32);
                Q_ASSERT_SUCCEEDED(hr);
                hr = writer->WriteUInt32(uuid.toUInt32());
                Q_ASSERT_SUCCEEDED(hr);
                break;
            case 16:
            default:
                qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registering Uuid attribute:" << uuid;
                hr = writer->WriteByte(TYPE_UUID128);
                Q_ASSERT_SUCCEEDED(hr);
                hr = writer->WriteGuid(uuid);
                Q_ASSERT_SUCCEEDED(hr);
                break;
            }
        } else if (attribute.userType() == qMetaTypeId<QBluetoothServiceInfo::Sequence>()) {
            qCDebug(QT_BT_WINDOWS) << "Registering sequence attribute";
            const QBluetoothServiceInfo::Sequence *sequence =
                    static_cast<const QBluetoothServiceInfo::Sequence *>(attribute.data());
            ComPtr<IDataWriter> tmpWriter;
            HRESULT hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
                &tmpWriter);
            Q_ASSERT_SUCCEEDED(hr);
            for (const QVariant &v : *sequence) {
                ComPtr<IBuffer> tmpBuffer = bufferFromAttribute(v);
                if (!tmpBuffer) {
                    qCWarning(QT_BT_WINDOWS) << "Could not create buffer from attribute in sequence";
                    return nullptr;
                }
                quint32 l;
                hr = tmpBuffer->get_Length(&l);
                Q_ASSERT_SUCCEEDED(hr);
                hr = tmpWriter->WriteBuffer(tmpBuffer.Get());
                Q_ASSERT_SUCCEEDED(hr);
            }
            ComPtr<IBuffer> tmpBuffer;
            hr = tmpWriter->DetachBuffer(&tmpBuffer);
            Q_ASSERT_SUCCEEDED(hr);
            // write sequence length
            quint32 length;
            tmpBuffer->get_Length(&length);
            Q_ASSERT_SUCCEEDED(hr);
            unsigned char type = TYPE_SEQUENCE_BASE;
            length += 1;
            if (length <= 0xff) {
                type += 5;
                hr = writer->WriteByte(type);
                Q_ASSERT_SUCCEEDED(hr);
                hr = writer->WriteByte(length);
                Q_ASSERT_SUCCEEDED(hr);
            } else if (length <= 0xffff) {
                type += 6;
                hr = writer->WriteByte(type);
                Q_ASSERT_SUCCEEDED(hr);
                hr = writer->WriteUInt16(length);
                Q_ASSERT_SUCCEEDED(hr);
            } else {
                type += 7;
                hr = writer->WriteByte(type);
                Q_ASSERT_SUCCEEDED(hr);
                hr = writer->WriteUInt32(length);
                Q_ASSERT_SUCCEEDED(hr);
            }
            // write sequence data
            hr = writer->WriteBuffer(tmpBuffer.Get());
            Q_ASSERT_SUCCEEDED(hr);
            qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registered sequence attribute with length" << length;
        } else if (attribute.userType() == qMetaTypeId<QBluetoothServiceInfo::Alternative>()) {
            qCWarning(QT_BT_WINDOWS) << "Don't know how to register user type Alternative";
            return nullptr;
        } else {
            qCWarning(QT_BT_WINDOWS) << "Unknown variant type" << attribute.userType();
            return nullptr;
        }
    }
    ComPtr<IBuffer> buffer;
    hr = writer->DetachBuffer(&buffer);
    Q_ASSERT_SUCCEEDED(hr);
    return buffer;
}

QBluetoothServiceInfoPrivate::QBluetoothServiceInfoPrivate()
    : registered(false)
{
    mainThreadCoInit(this);
}

QBluetoothServiceInfoPrivate::~QBluetoothServiceInfoPrivate()
{
    mainThreadCoUninit(this);
}

bool QBluetoothServiceInfoPrivate::isRegistered() const
{
    return registered;
}

bool QBluetoothServiceInfoPrivate::registerService(const QBluetoothAddress &localAdapter)
{
    Q_UNUSED(localAdapter);
    if (registered)
        return false;

    if (protocolDescriptor(QBluetoothUuid::ProtocolUuid::Rfcomm).isEmpty()) {
        qCWarning(QT_BT_WINDOWS) << Q_FUNC_INFO << "Only RFCOMM services can be registered on WinRT";
        return false;
    }

    QBluetoothServerPrivate *sPriv = __fakeServerPorts.key(serverChannel());
    if (!sPriv)
        return false;

    HRESULT hr;
    QBluetoothUuid uuid = attributes.value(QBluetoothServiceInfo::ServiceId).value<QBluetoothUuid>();
    ComPtr<IRfcommServiceIdStatics> serviceIdStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Devices_Bluetooth_Rfcomm_RfcommServiceId).Get(),
                                IID_PPV_ARGS(&serviceIdStatics));
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IRfcommServiceId> serviceId;
    hr = serviceIdStatics->FromUuid(uuid, &serviceId);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IRfcommServiceProviderStatics> providerStatics;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Devices_Bluetooth_Rfcomm_RfcommServiceProvider).Get(),
                                IID_PPV_ARGS(&providerStatics));
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IAsyncOperation<RfcommServiceProvider *>> op;
    hr = providerStatics->CreateAsync(serviceId.Get(), &op);
    Q_ASSERT_SUCCEEDED(hr);
    hr = QWinRTFunctions::await(op, serviceProvider.GetAddressOf());
    if (hr == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE)) {
        qCWarning(QT_BT_WINDOWS) << Q_FUNC_INFO << "No bluetooth adapter available.";
        return false;
    } else {
        Q_ASSERT_SUCCEEDED(hr);
    }

    ComPtr<IStreamSocketListener> listener = sPriv->listener();
    if (!listener) {
        qCWarning(QT_BT_WINDOWS) << Q_FUNC_INFO << "Could not obtain listener from server.";
        return false;
    }


    HString serviceIdHString;
    serviceId->AsString(serviceIdHString.GetAddressOf());
    Q_ASSERT_SUCCEEDED(hr);
    const QString serviceIdString = QString::fromWCharArray(WindowsGetStringRawBuffer(serviceIdHString.Get(), nullptr));

    //tell the server what service name our listener should have
    //and start the real listener
    bool result = sPriv->initiateActiveListening(serviceIdString);
    if (!result) {
        return false;
    }

    result = writeSdpAttributes();
    if (!result) {
        qCWarning(QT_BT_WINDOWS) << "Could not write SDP attributes.";
        return false;
    }
    qCDebug(QT_BT_WINDOWS) << "SDP attributes written.";

    ComPtr<IRfcommServiceProvider2> serviceProvider2;
    hr = serviceProvider.As(&serviceProvider2);
    Q_ASSERT_SUCCEEDED(hr);
    hr = serviceProvider2->StartAdvertisingWithRadioDiscoverability(listener.Get(), true);
    if (FAILED(hr)) {
        qCWarning(QT_BT_WINDOWS) << Q_FUNC_INFO << "Could not start advertising. Check your SDP data.";
        return false;
    }

    registered = true;
    return true;
}

bool QBluetoothServiceInfoPrivate::unregisterService()
{
    if (!registered)
        return false;

    QBluetoothServerPrivate *sPriv = __fakeServerPorts.key(serverChannel());
    if (!sPriv) {
        //QBluetoothServer::close() was called without prior call to unregisterService().
        //Now it is unregistered anyway.
        registered = false;
        return true;
    }

    bool result = sPriv->deactivateActiveListening();
    if (!result)
        return false;

    HRESULT hr;
    hr = serviceProvider->StopAdvertising();
    Q_ASSERT_SUCCEEDED(hr);

    registered = false;
    return true;
}

bool QBluetoothServiceInfoPrivate::writeSdpAttributes()
{
    if (!serviceProvider)
        return false;

    ComPtr<IDataWriter> writer;
    HRESULT hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
                                  &writer);
    Q_ASSERT_SUCCEEDED(hr);
    ComPtr<IMap<UINT32, IBuffer *>> rawAttributes;
    hr = serviceProvider->get_SdpRawAttributes(&rawAttributes);
    Q_ASSERT_SUCCEEDED(hr);
    const QList<quint16> keys = attributes.keys();
    for (quint16 key : keys) {
        // The SDP Class Id List and RFCOMM and L2CAP protocol descriptors are automatically
        // generated by the RfcommServiceProvider. Do not specify it in the SDP raw attribute map.
        if (key == QBluetoothServiceInfo::ServiceClassIds
                || key == QBluetoothServiceInfo::ProtocolDescriptorList)
            continue;
        const QVariant attribute = attributes.value(key);
        HRESULT hr;
        ComPtr<IBuffer> buffer = bufferFromAttribute(attribute);
        if (!buffer) {
            qCWarning(QT_BT_WINDOWS) << "Could not create buffer from attribute with id:" << key;
            return false;
        }

        // Other backends support a wrong structure in profile descriptor list. In order to make
        // WinRT accept the list without breaking existing apps we have to repair this structure.
        if (key == QBluetoothServiceInfo::BluetoothProfileDescriptorList) {
            if (!repairProfileDescriptorListIfNeeded(buffer)) {
                qCWarning(QT_BT_WINDOWS) << Q_FUNC_INFO << "Error while checking/repairing structure of profile descriptor list";
                return false;
            }
        }

        hr = writer->WriteBuffer(buffer.Get());
        Q_ASSERT_SUCCEEDED(hr);

        hr = writer->DetachBuffer(&buffer);
        Q_ASSERT_SUCCEEDED(hr);

        boolean replaced;
        hr = rawAttributes->Insert(key, buffer.Get(), &replaced);
        Q_ASSERT_SUCCEEDED(hr);
        Q_ASSERT(!replaced);
        qCDebug(QT_BT_WINDOWS) << Q_FUNC_INFO << "Registered attribute" << QString::number(key, 16).rightJustified(4, QLatin1Char('0')) << "with value" << attribute;
    }
    return true;
}

QT_END_NAMESPACE
