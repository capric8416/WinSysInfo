// self
#include "gpu.h"

// windows
#include <Windows.h>


QString GpuInfo::GetBrandName()
{
    QString deviceNames;

    for (int i = 0; ; i++)
    {
        DISPLAY_DEVICE displayDevice = { sizeof(displayDevice), 0 };
        if (!EnumDisplayDevices(NULL, i, &displayDevice, EDD_GET_DEVICE_INTERFACE_NAME)) {
            break;
        }

        QString deviceName = QString::fromWCharArray(displayDevice.DeviceString);
        if (deviceNames.indexOf(deviceName) != -1) {
            continue;
        }

        if (!deviceNames.isEmpty()) {
            deviceNames += "; ";
        }

        deviceNames += deviceName;
    }

    return deviceNames;
}
