/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SOCKETCANBACKEND_H
#define SOCKETCANBACKEND_H

#include <QtSerialBus/qcanbusframe.h>
#include <QtSerialBus/qcanbusdevice.h>
#include <QtSerialBus/qcanbusdeviceinfo.h>

#include <QtCore/qsocketnotifier.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

// The order of the following includes is mandatory, because some
// distributions use sa_family_t in can.h without including socket.h
#include <sys/socket.h>
#include <sys/uio.h>
#include <linux/can.h>
#include <sys/time.h>

#include <memory>

#ifndef CANFD_MTU
// CAN FD support was added by Linux kernel 3.6
// For prior kernels we redefine the missing defines here
// they are taken from linux/can/raw.h & linux/can.h

enum {
    CAN_RAW_FD_FRAMES = 5
};

#define CAN_MAX_DLEN 8
#define CANFD_MAX_DLEN 64
struct canfd_frame {
    canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
    __u8    len;     /* frame payload length in byte */
    __u8    flags;   /* additional flags for CAN FD */
    __u8    __res0;  /* reserved / padding */
    __u8    __res1;  /* reserved / padding */
    __u8    data[CANFD_MAX_DLEN] __attribute__((aligned(8)));
};
#define CAN_MTU     (sizeof(struct can_frame))
#define CANFD_MTU   (sizeof(struct canfd_frame))

#endif

QT_BEGIN_NAMESPACE

class LibSocketCan;

class SocketCanBackend : public QCanBusDevice
{
    Q_OBJECT
public:
    explicit SocketCanBackend(const QString &name);
    ~SocketCanBackend();

    bool open() override;
    void close() override;

    void setConfigurationParameter(int key, const QVariant &value) override;

    bool writeFrame(const QCanBusFrame &newData) override;

    QString interpretErrorFrame(const QCanBusFrame &errorFrame) override;

    static QList<QCanBusDeviceInfo> interfaces();

private Q_SLOTS:
    void readSocket();

private:
    void resetConfigurations();
    bool connectSocket();
    bool applyConfigurationParameter(int key, const QVariant &value);
    void resetController();
    bool hasBusStatus() const;
    QCanBusDevice::CanBusStatus busStatus() const;

    int protocol = CAN_RAW;
    canfd_frame m_frame;
    sockaddr_can m_address;
    msghdr m_msg;
    iovec m_iov;
    sockaddr_can m_addr;
    char m_ctrlmsg[CMSG_SPACE(sizeof(timeval)) + CMSG_SPACE(sizeof(__u32))];

    qint64 canSocket = -1;
    QSocketNotifier *notifier = nullptr;
    std::unique_ptr<LibSocketCan> libSocketCan;
    QString canSocketName;
    bool canFdOptionEnabled = false;
};

QT_END_NAMESPACE

#endif // SOCKETCANBACKEND_H
