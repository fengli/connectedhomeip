/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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
 *      This file defines chip vendor identifiers.
 *
 *      Vendor identifiers are assigned and administered by CHIP.
 *
 */

#pragma once

#include <cstdint>

namespace chip {

//
// CHIP Vendor Identifiers (16 bits max)
//

// As per specifications (section 2.5.2 Vendor Identifier)
enum VendorId : uint16_t
{
    Common       = 0x0000u,
    GoogleNest   = 0x235Au,
    TestVendor1  = 0xFFF1u,
    TestVendor2  = 0xFFF2u,
    TestVendor3  = 0xFFF3u,
    TestVendor4  = 0xFFF4u,
    NotSpecified = 0xFFFFu
};

} // namespace chip
