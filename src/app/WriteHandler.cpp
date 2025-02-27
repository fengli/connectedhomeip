/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include <app/AppBuildConfig.h>
#include <app/InteractionModelEngine.h>
#include <app/MessageDef/EventPathIB.h>
#include <app/WriteHandler.h>
#include <app/reporting/Engine.h>
#include <app/util/MatterCallbacks.h>
#include <lib/support/TypeTraits.h>

namespace chip {
namespace app {
CHIP_ERROR WriteHandler::Init(InteractionModelDelegate * apDelegate)
{
    IgnoreUnusedVariable(apDelegate);
    VerifyOrReturnLogError(mpExchangeCtx == nullptr, CHIP_ERROR_INCORRECT_STATE);

    System::PacketBufferHandle packet = System::PacketBufferHandle::New(chip::app::kMaxSecureSduLengthBytes);
    VerifyOrReturnLogError(!packet.IsNull(), CHIP_ERROR_NO_MEMORY);

    mMessageWriter.Init(std::move(packet));
    ReturnLogErrorOnFailure(mWriteResponseBuilder.Init(&mMessageWriter));

    AttributeStatusList::Builder attributeStatusListBuilder = mWriteResponseBuilder.CreateAttributeStatusListBuilder();
    ReturnLogErrorOnFailure(attributeStatusListBuilder.GetError());

    MoveToState(State::Initialized);

    return CHIP_NO_ERROR;
}

void WriteHandler::Shutdown()
{
    VerifyOrReturn(mState != State::Uninitialized);
    mMessageWriter.Reset();
    mpExchangeCtx = nullptr;
    ClearState();
}

CHIP_ERROR WriteHandler::OnWriteRequest(Messaging::ExchangeContext * apExchangeContext, System::PacketBufferHandle && aPayload)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    mpExchangeCtx  = apExchangeContext;

    err = ProcessWriteRequest(std::move(aPayload));
    SuccessOrExit(err);
    err = SendWriteResponse();

exit:
    Shutdown();
    return err;
}

CHIP_ERROR WriteHandler::FinalizeMessage(System::PacketBufferHandle & packet)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    AttributeStatusList::Builder attributeStatusList;
    VerifyOrExit(mState == State::AddStatus, err = CHIP_ERROR_INCORRECT_STATE);
    attributeStatusList = mWriteResponseBuilder.GetAttributeStatusListBuilder().EndOfAttributeStatusList();
    err                 = attributeStatusList.GetError();
    SuccessOrExit(err);

    mWriteResponseBuilder.EndOfWriteResponseMessage();
    err = mWriteResponseBuilder.GetError();
    SuccessOrExit(err);

    err = mMessageWriter.Finalize(&packet);
    SuccessOrExit(err);

exit:
    return err;
}

CHIP_ERROR WriteHandler::SendWriteResponse()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    System::PacketBufferHandle packet;

    VerifyOrExit(mState == State::AddStatus, err = CHIP_ERROR_INCORRECT_STATE);

    err = FinalizeMessage(packet);
    SuccessOrExit(err);

    VerifyOrExit(mpExchangeCtx != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    err = mpExchangeCtx->SendMessage(Protocols::InteractionModel::MsgType::WriteResponse, std::move(packet));
    SuccessOrExit(err);

    MoveToState(State::Sending);

exit:
    return err;
}

CHIP_ERROR WriteHandler::ProcessAttributeDataList(TLV::TLVReader & aAttributeDataListReader)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    while (CHIP_NO_ERROR == (err = aAttributeDataListReader.Next()))
    {
        chip::TLV::TLVReader dataReader;
        AttributeDataElement::Parser element;
        AttributePath::Parser attributePath;
        ClusterInfo clusterInfo;
        TLV::TLVReader reader = aAttributeDataListReader;
        err                   = element.Init(reader);
        SuccessOrExit(err);

        err = element.GetAttributePath(&attributePath);
        SuccessOrExit(err);

        err = attributePath.GetNodeId(&(clusterInfo.mNodeId));
        SuccessOrExit(err);

        err = attributePath.GetEndpointId(&(clusterInfo.mEndpointId));
        SuccessOrExit(err);

        err = attributePath.GetClusterId(&(clusterInfo.mClusterId));
        SuccessOrExit(err);

        err = attributePath.GetFieldId(&(clusterInfo.mFieldId));
        if (CHIP_NO_ERROR == err)
        {
            clusterInfo.mFlags.Set(ClusterInfo::Flags::kFieldIdValid);
        }
        else if (CHIP_END_OF_TLV == err)
        {
            err = CHIP_NO_ERROR;
        }
        SuccessOrExit(err);

        err = attributePath.GetListIndex(&(clusterInfo.mListIndex));
        if (CHIP_NO_ERROR == err)
        {
            VerifyOrExit(clusterInfo.mFlags.Has(ClusterInfo::Flags::kFieldIdValid), err = CHIP_ERROR_IM_MALFORMED_ATTRIBUTE_PATH);
            clusterInfo.mFlags.Set(ClusterInfo::Flags::kListIndexValid);
        }

        err = element.GetData(&dataReader);
        SuccessOrExit(err);

        {
            const ConcreteAttributePath concretePath =
                ConcreteAttributePath(clusterInfo.mEndpointId, clusterInfo.mClusterId, clusterInfo.mFieldId);
            MatterPreAttributeWriteCallback(concretePath);
            err = WriteSingleClusterData(clusterInfo, dataReader, this);
            MatterPostAttributeWriteCallback(concretePath);
        }
        SuccessOrExit(err);
    }

    if (CHIP_END_OF_TLV == err)
    {
        err = CHIP_NO_ERROR;
    }

exit:
    return err;
}

CHIP_ERROR WriteHandler::ProcessWriteRequest(System::PacketBufferHandle && aPayload)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    System::PacketBufferTLVReader reader;

    WriteRequestMessage::Parser writeRequestParser;
    AttributeDataList::Parser attributeDataListParser;
    TLV::TLVReader attributeDataListReader;
    bool needSuppressResponse = false;

    reader.Init(std::move(aPayload));

    err = reader.Next();
    SuccessOrExit(err);

    err = writeRequestParser.Init(reader);
    SuccessOrExit(err);

#if CHIP_CONFIG_IM_ENABLE_SCHEMA_CHECK
    err = writeRequestParser.CheckSchemaValidity();
    SuccessOrExit(err);
#endif
    err = writeRequestParser.GetSuppressResponse(&needSuppressResponse);
    if (err == CHIP_END_OF_TLV)
    {
        err = CHIP_NO_ERROR;
    }
    SuccessOrExit(err);

    err = writeRequestParser.GetAttributeDataList(&attributeDataListParser);
    SuccessOrExit(err);
    attributeDataListParser.GetReader(&attributeDataListReader);
    err = ProcessAttributeDataList(attributeDataListReader);

exit:
    return err;
}

CHIP_ERROR WriteHandler::ConstructAttributePath(const AttributePathParams & aAttributePathParams,
                                                AttributeStatusIB::Builder aAttributeStatusIB)
{
    AttributePath::Builder attributePath = aAttributeStatusIB.CreateAttributePathBuilder();
    if (aAttributePathParams.mFlags.Has(AttributePathParams::Flags::kFieldIdValid))
    {
        attributePath.FieldId(aAttributePathParams.mFieldId);
    }

    if (aAttributePathParams.mFlags.Has(AttributePathParams::Flags::kListIndexValid))
    {
        attributePath.ListIndex(aAttributePathParams.mListIndex);
    }

    attributePath.NodeId(aAttributePathParams.mNodeId)
        .ClusterId(aAttributePathParams.mClusterId)
        .EndpointId(aAttributePathParams.mEndpointId)
        .EndOfAttributePath();

    return attributePath.GetError();
}

CHIP_ERROR WriteHandler::AddStatus(const AttributePathParams & aAttributePathParams,
                                   const Protocols::InteractionModel::Status aStatus)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    StatusIB::Builder statusIBBuilder;
    StatusIB statusIB;
    AttributeStatusIB::Builder attributeStatusIB =
        mWriteResponseBuilder.GetAttributeStatusListBuilder().CreateAttributeStatusBuilder();
    err = attributeStatusIB.GetError();
    SuccessOrExit(err);

    err = ConstructAttributePath(aAttributePathParams, attributeStatusIB);
    SuccessOrExit(err);

    statusIB.mStatus = aStatus;
    statusIBBuilder  = attributeStatusIB.CreateStatusIBBuilder();
    statusIBBuilder.EncodeStatusIB(statusIB);
    err = statusIBBuilder.GetError();
    SuccessOrExit(err);

    attributeStatusIB.EndOfAttributeStatusIB();
    err = attributeStatusIB.GetError();
    SuccessOrExit(err);
    MoveToState(State::AddStatus);

exit:
    return err;
}

const char * WriteHandler::GetStateStr() const
{
#if CHIP_DETAIL_LOGGING
    switch (mState)
    {
    case State::Uninitialized:
        return "Uninitialized";

    case State::Initialized:
        return "Initialized";

    case State::AddStatus:
        return "AddStatus";
    case State::Sending:
        return "Sending";
    }
#endif // CHIP_DETAIL_LOGGING
    return "N/A";
}

void WriteHandler::MoveToState(const State aTargetState)
{
    mState = aTargetState;
    ChipLogDetail(DataManagement, "IM RH moving to [%s]", GetStateStr());
}

void WriteHandler::ClearState()
{
    MoveToState(State::Uninitialized);
}

} // namespace app
} // namespace chip

void __attribute__((weak)) MatterPreAttributeWriteCallback(const chip::app::ConcreteAttributePath & attributePath) {}
void __attribute__((weak)) MatterPostAttributeWriteCallback(const chip::app::ConcreteAttributePath & attributePath) {}
