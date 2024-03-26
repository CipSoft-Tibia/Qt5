// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <bluetooth/bluetooth.h>

int main()
{
    bdaddr_t anyTmp = {{0, 0, 0, 0, 0, 0}};
    bdaddr_t localTmp = {{0, 0, 0, 0xff, 0xff, 0xff}};

    bacmp(&anyTmp, &localTmp);

    uint32_t field0 = 1;
    uint16_t field1 = 1;

    field0 = htonl(field0);
    field1 = htons(field1);
    return 0;
}
