/**
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#pragma once

#include <app/AppBuildConfig.h>
#include <app/util/basic-types.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/CHIPTLV.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

#include "InvokeRequests.h"
#include "StructBuilder.h"
#include "StructParser.h"

namespace chip {
namespace app {
namespace InvokeRequestMessage {
enum class Tag : uint8_t
{
    kSuppressResponse = 0,
    kTimedRequest     = 1,
    kInvokeRequests   = 2,
};

class Parser : public StructParser
{
public:
    /**
     *  @brief Initialize the parser object with TLVReader
     *
     *  @param [in] aReader A pointer to a TLVReader, which should point to the beginning of this request
     *
     *  @return #CHIP_NO_ERROR on success
     */
    CHIP_ERROR Init(const TLV::TLVReader & aReader);

#if CHIP_CONFIG_IM_ENABLE_SCHEMA_CHECK
    /**
     *  @brief Roughly verify the message is correctly formed
     *   1) all mandatory tags are present
     *   2) all elements have expected data type
     *   3) any tag can only appear once
     *   4) At the top level of the structure, unknown tags are ignored for forward compatibility
     *  @note The main use of this function is to print out what we're
     *    receiving during protocol development and debugging.
     *    The encoding rule has changed in IM encoding spec so this
     *    check is only "roughly" conformant now.
     *
     *  @return #CHIP_NO_ERROR on success
     */
    CHIP_ERROR CheckSchemaValidity() const;
#endif

    /**
     *  @brief Get SuppressResponse. Next() must be called before accessing them.
     *
     *  @return #CHIP_NO_ERROR on success
     *          #CHIP_END_OF_TLV if there is no such element
     */
    CHIP_ERROR GetSuppressResponse(bool * const apSuppressResponse) const;

    /**
     *  @brief Get TimedRequest. Next() must be called before accessing them.
     *
     *  @return #CHIP_NO_ERROR on success
     *          #CHIP_END_OF_TLV if there is no such element
     */
    CHIP_ERROR GetTimedRequest(bool * const apTimedRequest) const;

    /**
     *  @brief Get a parser for an InvokeRequests.
     *
     *  @param [in] apInvokeRequests    A pointer to the invoke request list parser.
     *
     *  @return #CHIP_NO_ERROR on success
     *          #CHIP_END_OF_TLV if there is no such element
     */
    CHIP_ERROR GetInvokeRequests(InvokeRequests::Parser * const apInvokeRequests) const;
};

class Builder : public StructBuilder
{
public:
    /**
     *  @brief when sets to true, it means do not send a response to this action
     */
    InvokeRequestMessage::Builder & SuppressResponse(const bool aSuppressResponse);

    /**
     *  @brief This is flag to indication if ths action is part of a timed invoke transaction
     */
    InvokeRequestMessage::Builder & TimedRequest(const bool aTimedRequest);

    /**
     *  @brief Initialize a InvokeRequests::Builder for writing into the TLV stream
     *
     *  @return A reference to InvokeRequests::Builder
     */
    InvokeRequests::Builder & CreateInvokeRequests();

    /**
     *  @brief Get reference to InvokeRequests::Builder
     *
     *  @return A reference to InvokeRequests::Builder
     */
    InvokeRequests::Builder & GetInvokeRequests() { return mInvokeRequests; }

    /**
     *  @brief Mark the end of this InvokeRequestMessage
     *
     *  @return A reference to *this
     */
    InvokeRequestMessage::Builder & EndOfInvokeRequestMessage();

private:
    InvokeRequests::Builder mInvokeRequests;
};
}; // namespace InvokeRequestMessage
}; // namespace app
}; // namespace chip
