/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Provides the implementation of the Device Layer ConfigurationManager object
 *          for Linux platforms.
 */

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <app-common/zap-generated/enums.h>
#include <ifaddrs.h>
#include <lib/core/CHIPVendorIdentifiers.hpp>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <netpacket/packet.h>
#include <platform/ConfigurationManager.h>
#include <platform/Linux/PosixConfig.h>
#include <platform/internal/GenericConfigurationManagerImpl.cpp>

namespace chip {
namespace DeviceLayer {

using namespace ::chip::DeviceLayer::Internal;

/** Singleton instance of the ConfigurationManager implementation object.
 */
ConfigurationManagerImpl ConfigurationManagerImpl::sInstance;

CHIP_ERROR ConfigurationManagerImpl::Init()
{
    CHIP_ERROR err;
    uint32_t rebootCount;
    bool failSafeArmed;

    // Force initialization of NVS namespaces if they doesn't already exist.
    err = EnsureNamespace(kConfigNamespace_ChipFactory);
    SuccessOrExit(err);
    err = EnsureNamespace(kConfigNamespace_ChipConfig);
    SuccessOrExit(err);
    err = EnsureNamespace(kConfigNamespace_ChipCounters);
    SuccessOrExit(err);

    // Initialize the generic implementation base class.
    err = Internal::GenericConfigurationManagerImpl<ConfigurationManagerImpl>::Init();
    SuccessOrExit(err);

    if (ConfigValueExists(kCounterKey_RebootCount))
    {
        err = GetRebootCount(rebootCount);
        SuccessOrExit(err);

        err = StoreRebootCount(rebootCount + 1);
        SuccessOrExit(err);
    }
    else
    {
        // The first boot after factory reset of the Node.
        err = StoreRebootCount(1);
        SuccessOrExit(err);
    }

    if (!ConfigValueExists(kCounterKey_TotalOperationalHours))
    {
        err = StoreTotalOperationalHours(0);
        SuccessOrExit(err);
    }

    if (!ConfigValueExists(kCounterKey_BootReason))
    {
        err = StoreBootReasons(EMBER_ZCL_BOOT_REASON_TYPE_UNSPECIFIED);
        SuccessOrExit(err);
    }

    // If the fail-safe was armed when the device last shutdown, initiate a factory reset.
    if (GetFailSafeArmed(failSafeArmed) == CHIP_NO_ERROR && failSafeArmed)
    {
        ChipLogProgress(DeviceLayer, "Detected fail-safe armed on reboot; initiating factory reset");
        InitiateFactoryReset();
    }

    err = CHIP_NO_ERROR;

exit:
    return err;
}

CHIP_ERROR ConfigurationManagerImpl::GetPrimaryWiFiMACAddress(uint8_t * buf)
{
    struct ifaddrs * addresses = NULL;
    CHIP_ERROR error           = CHIP_NO_ERROR;
    bool found                 = false;

    VerifyOrExit(getifaddrs(&addresses) == 0, error = CHIP_ERROR_INTERNAL);
    for (auto addr = addresses; addr != NULL; addr = addr->ifa_next)
    {
        if ((addr->ifa_addr) && (addr->ifa_addr->sa_family == AF_PACKET) && strncmp(addr->ifa_name, "lo", IFNAMSIZ) != 0)
        {
            struct sockaddr_ll * mac = (struct sockaddr_ll *) addr->ifa_addr;
            memcpy(buf, mac->sll_addr, mac->sll_halen);
            found = true;
            break;
        }
    }
    freeifaddrs(addresses);
    if (!found)
    {
        error = CHIP_ERROR_NO_ENDPOINT;
    }

exit:
    return error;
}

bool ConfigurationManagerImpl::CanFactoryReset()
{
    // TODO(#742): query the application to determine if factory reset is allowed.
    return true;
}

void ConfigurationManagerImpl::InitiateFactoryReset()
{
    PlatformMgr().ScheduleWork(DoFactoryReset);
}

CHIP_ERROR ConfigurationManagerImpl::ReadPersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t & value)
{
    PosixConfig::Key configKey{ kConfigNamespace_ChipCounters, key };

    CHIP_ERROR err = ReadConfigValue(configKey, value);
    if (err == CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
    }
    return err;
}

CHIP_ERROR ConfigurationManagerImpl::WritePersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t value)
{
    PosixConfig::Key configKey{ kConfigNamespace_ChipCounters, key };
    return WriteConfigValue(configKey, value);
}

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION
CHIP_ERROR ConfigurationManagerImpl::GetWiFiStationSecurityType(WiFiAuthSecurityType & secType)
{
    CHIP_ERROR err;
    uint32_t secTypeInt;

    err = ReadConfigValue(kConfigKey_WiFiStationSecType, secTypeInt);
    if (err == CHIP_NO_ERROR)
    {
        secType = static_cast<WiFiAuthSecurityType>(secTypeInt);
    }
    return err;
}

CHIP_ERROR ConfigurationManagerImpl::UpdateWiFiStationSecurityType(WiFiAuthSecurityType secType)
{
    CHIP_ERROR err;
    WiFiAuthSecurityType curSecType;

    if (secType != kWiFiSecurityType_NotSpecified)
    {
        err = GetWiFiStationSecurityType(curSecType);
        if (err == CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND || (err == CHIP_NO_ERROR && secType != curSecType))
        {
            uint32_t secTypeInt = static_cast<uint32_t>(secType);
            err                 = WriteConfigValue(kConfigKey_WiFiStationSecType, secTypeInt);
        }
        SuccessOrExit(err);
    }
    else
    {
        err = ClearConfigValue(kConfigKey_WiFiStationSecType);
        SuccessOrExit(err);
    }

exit:
    return err;
}
#endif // CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION

void ConfigurationManagerImpl::DoFactoryReset(intptr_t arg)
{
    CHIP_ERROR err;

    ChipLogProgress(DeviceLayer, "Performing factory reset");

    err = FactoryResetConfig();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Failed to factory reset configurations: %s", ErrorStr(err));
    }

    err = FactoryResetCounters();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Failed to factory reset counters: %s", ErrorStr(err));
    }

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD

    ChipLogProgress(DeviceLayer, "Clearing Thread provision");
    ThreadStackMgr().ErasePersistentInfo();

#endif // CHIP_DEVICE_CONFIG_ENABLE_THREAD

    // Restart the system.
    ChipLogProgress(DeviceLayer, "System restarting (not implemented)");
    // TODO(#742): restart CHIP exe
}

CHIP_ERROR ConfigurationManagerImpl::GetRebootCount(uint32_t & rebootCount)
{
    return ReadConfigValue(kCounterKey_RebootCount, rebootCount);
}

CHIP_ERROR ConfigurationManagerImpl::StoreRebootCount(uint32_t rebootCount)
{
    return WriteConfigValue(kCounterKey_RebootCount, rebootCount);
}

CHIP_ERROR ConfigurationManagerImpl::GetTotalOperationalHours(uint32_t & totalOperationalHours)
{
    return ReadConfigValue(kCounterKey_TotalOperationalHours, totalOperationalHours);
}

CHIP_ERROR ConfigurationManagerImpl::StoreTotalOperationalHours(uint32_t totalOperationalHours)
{
    return WriteConfigValue(kCounterKey_TotalOperationalHours, totalOperationalHours);
}

CHIP_ERROR ConfigurationManagerImpl::GetBootReasons(uint32_t & bootReasons)
{
    return ReadConfigValue(kCounterKey_BootReason, bootReasons);
}

CHIP_ERROR ConfigurationManagerImpl::StoreBootReasons(uint32_t bootReasons)
{
    return WriteConfigValue(kCounterKey_BootReason, bootReasons);
}

} // namespace DeviceLayer
} // namespace chip
