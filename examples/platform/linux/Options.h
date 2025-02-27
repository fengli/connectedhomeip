/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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
 *      Support functions for parsing command-line arguments.
 *
 */

#pragma once

#include <cstdint>

#include <lib/core/CHIPError.h>
#include <setup_payload/SetupPayload.h>

struct LinuxDeviceOptions
{
    chip::SetupPayload payload;
    uint32_t mBleDevice                = 0;
    bool mWiFi                         = false;
    bool mThread                       = false;
    uint32_t securedDevicePort         = CHIP_PORT;
    uint32_t securedCommissionerPort   = CHIP_PORT + 2;
    uint32_t unsecuredCommissionerPort = CHIP_UDC_PORT;
    const char * command               = nullptr;

    static LinuxDeviceOptions & GetInstance();
};

CHIP_ERROR ParseArguments(int argc, char * argv[]);
