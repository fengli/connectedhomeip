/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
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

#include "AppTask.h"

#include <logging/log.h>

#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>

LOG_MODULE_REGISTER(app);

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::DeviceLayer;

int main()
{
    int ret        = 0;
    CHIP_ERROR err = CHIP_NO_ERROR;

    err = chip::Platform::MemoryInit();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("Platform::MemoryInit() failed");
        goto exit;
    }

    LOG_INF("Init CHIP stack");
    err = PlatformMgr().InitChipStack();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("PlatformMgr().InitChipStack() failed");
        goto exit;
    }

    LOG_INF("Starting CHIP task");
    err = PlatformMgr().StartEventLoopTask();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
        goto exit;
    }

    LOG_INF("Init Thread stack");
    err = ThreadStackMgr().InitThreadStack();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("ThreadStackMgr().InitThreadStack() failed");
        goto exit;
    }

#ifdef CONFIG_OPENTHREAD_MTD_SED
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
        goto exit;
    }

    ConnectivityManager::ThreadPollingConfig pollingConfig;
    pollingConfig.Clear();
    pollingConfig.ActivePollingIntervalMS   = CONFIG_OPENTHREAD_POLL_PERIOD;
    pollingConfig.InactivePollingIntervalMS = CONFIG_OPENTHREAD_POLL_PERIOD;

    err = ConnectivityMgr().SetThreadPollingConfig(pollingConfig);
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("ConnectivityMgr().SetThreadPollingConfig() failed");
        goto exit;
    }
#else
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
        goto exit;
    }
#endif

    ret = GetAppTask().StartApp();
    if (ret != 0)
    {
        err = chip::System::MapErrorZephyr(ret);
    }

exit:
    LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
    return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
