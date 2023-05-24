// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <functional>
#include <windows.system.h>
#include <windows.devices.geolocation.h>
#include <windows.foundation.h>
#include <wrl.h>

using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Devices::Geolocation;
using namespace ABI::Windows::Foundation;

typedef IAsyncOperationCompletedHandler<GeolocationAccessStatus> AccessHandler;

int main(int, char**)
{
    IGeolocator *locator;
    RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Devices_Geolocation_Geolocator).Get(),
                                    reinterpret_cast<IInspectable**>(&locator));
    return 0;
}
