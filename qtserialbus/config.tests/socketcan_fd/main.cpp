// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <linux/can.h>
#include <linux/can/raw.h>

int main()
{
    canfd_frame frame;
    int fd_payload = CANFD_MAX_DLEN;
    fd_payload = CAN_RAW_FD_FRAMES;
    fd_payload = CANFD_MTU;
    return 0;
}

