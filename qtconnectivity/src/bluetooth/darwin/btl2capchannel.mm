// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbluetoothaddress.h"
#include "btl2capchannel_p.h"
#include "btdelegates_p.h"
#include "btutility_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

@implementation DarwinBTL2CAPChannel
{
    QT_PREPEND_NAMESPACE(DarwinBluetooth)::ChannelDelegate *delegate;
    IOBluetoothDevice *device;
    IOBluetoothL2CAPChannel *channel;
    bool connected;
}

- (id)initWithDelegate:(DarwinBluetooth::ChannelDelegate *)aDelegate
{
    Q_ASSERT_X(aDelegate, Q_FUNC_INFO, "invalid delegate (null)");

    if (self = [super init]) {
        delegate = aDelegate;
        device = nil;
        channel = nil;
        connected = false;
    }

    return self;
}

- (id)initWithDelegate:(QT_PREPEND_NAMESPACE(DarwinBluetooth::ChannelDelegate) *)aDelegate
      channel:(IOBluetoothL2CAPChannel *)aChannel
{
    // This type of channel does not require connect, it's created with
    // already open channel.
    Q_ASSERT_X(aDelegate, Q_FUNC_INFO, "invalid delegate (null)");
    Q_ASSERT_X(channel, Q_FUNC_INFO, "invalid channel (nil)");

    if (self = [super init]) {
        delegate = aDelegate;
        channel = [aChannel retain];
        [channel setDelegate:self];
        device = [channel.device retain];
        connected = true;
    }

    return self;
}

- (void)dealloc
{
    // TODO: test if this implementation works at all!
    if (channel) {
        [channel setDelegate:nil];
        // From Apple's docs:
        // "This method may only be called by the client that opened the channel
        // in the first place. In the future asynchronous and synchronous versions
        // will be provided that let the client know when the close process has been finished."
        [channel closeChannel];
        [channel release];
    }

    [device release];

    [super dealloc];
}

- (IOReturn)connectAsyncToDevice:(const QBluetoothAddress &)address
            withPSM:(BluetoothL2CAPChannelID)psm
{
    if (address.isNull()) {
        qCCritical(QT_BT_DARWIN) << "invalid peer address";
        return kIOReturnNoDevice;
    }

    // Can never be called twice.
    if (connected || device || channel) {
        qCCritical(QT_BT_DARWIN) << "connection is already active";
        return kIOReturnStillOpen;
    }

    QT_BT_MAC_AUTORELEASEPOOL;

    const BluetoothDeviceAddress iobtAddress = DarwinBluetooth::iobluetooth_address(address);
    device = [IOBluetoothDevice deviceWithAddress:&iobtAddress];
    if (!device) {
        qCCritical(QT_BT_DARWIN) << "failed to create a device";
        return kIOReturnNoDevice;
    }

    const IOReturn status = [device openL2CAPChannelAsync:&channel withPSM:psm delegate:self];
    if (status != kIOReturnSuccess) {
        qCCritical(QT_BT_DARWIN) << "failed to open L2CAP channel";
        // device is still autoreleased.
        device = nil;
        return status;
    }

    [channel retain];// What if we're closed already?
    [device retain];

    return kIOReturnSuccess;
}

// IOBluetoothL2CAPChannelDelegate:

- (void)l2capChannelData:(IOBluetoothL2CAPChannel*)l2capChannel
        data:(void *)dataPointer length:(size_t)dataLength
{
    Q_UNUSED(l2capChannel);

    Q_ASSERT_X(delegate, Q_FUNC_INFO, "invalid delegate (null)");

    if (dataPointer && dataLength)
        delegate->readChannelData(dataPointer, dataLength);
}

- (void)l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*)
        l2capChannel status:(IOReturn)error
{
    Q_UNUSED(l2capChannel);

    Q_ASSERT_X(delegate, Q_FUNC_INFO, "invalid delegate (null)");

    if (error != kIOReturnSuccess) {
        delegate->setChannelError(error);
    } else {
        connected = true;
        delegate->channelOpenComplete();
    }
}

- (void)l2capChannelClosed:(IOBluetoothL2CAPChannel*)l2capChannel
{
    Q_UNUSED(l2capChannel);

    Q_ASSERT_X(delegate, Q_FUNC_INFO, "invalid delegate (null)");
    delegate->channelClosed();
    connected = false;
}

- (void)l2capChannelReconfigured:(IOBluetoothL2CAPChannel*)l2capChannel
{
    Q_UNUSED(l2capChannel);
}

- (void)l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*)l2capChannel
        refcon:(void*)refcon status:(IOReturn)error
{
    Q_UNUSED(l2capChannel);
    Q_UNUSED(refcon);

    Q_ASSERT_X(delegate, Q_FUNC_INFO, "invalid delegate (null)");

    if (error != kIOReturnSuccess)
        delegate->setChannelError(error);
    else
        delegate->writeComplete();
}

- (void)l2capChannelQueueSpaceAvailable:(IOBluetoothL2CAPChannel*)l2capChannel
{
    Q_UNUSED(l2capChannel);
}

// Aux. methods.
- (BluetoothL2CAPPSM)getPSM
{
    if (channel)
        return channel.PSM;

    return 0;
}

- (BluetoothDeviceAddress)peerAddress
{
    const BluetoothDeviceAddress *const addr = device ? [device getAddress]
                                                      : nullptr;
    if (addr)
        return *addr;

    return BluetoothDeviceAddress();
}

- (NSString *)peerName
{
    if (device)
        return device.name;

    return nil;
}

- (bool)isConnected
{
    return connected;
}

- (IOReturn) writeSync:(void*)data length:(UInt16)length
{
    Q_ASSERT_X(data, Q_FUNC_INFO, "invalid data (null)");
    Q_ASSERT_X(length, Q_FUNC_INFO, "invalid data size");
    Q_ASSERT_X(connected && channel, Q_FUNC_INFO, "invalid L2CAP channel");

    return [channel writeSync:data length:length];
}

- (IOReturn) writeAsync:(void*)data length:(UInt16)length
{
    Q_ASSERT_X(data, Q_FUNC_INFO, "invalid data (null)");
    Q_ASSERT_X(length, Q_FUNC_INFO, "invalid data size");
    Q_ASSERT_X(connected && channel, Q_FUNC_INFO, "invalid L2CAP channel");

    return [channel writeAsync:data length:length refcon:nullptr];
}


@end
