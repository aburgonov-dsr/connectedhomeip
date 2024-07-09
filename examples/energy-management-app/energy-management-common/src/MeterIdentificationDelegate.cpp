/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
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

#include <MeterIdentificationDelegate.h>
#include <app/reporting/reporting.h>

#include <app/clusters/meter-identification-server/meter-identification-server.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::MeterIdentification;
using namespace chip::app::Clusters::MeterIdentification::Attributes;

using chip::app::Clusters::MeterIdentification::MeterTypeEnum;
using chip::app::Clusters::MeterIdentification::PowerThresholdSourceEnum;

CHIP_ERROR MeterIdentificationInstance::Init()
{
    return Instance::Init();
}

void MeterIdentificationInstance::Shutdown()
{
    Instance::Shutdown();
}

// --------------- Internal Attribute Set APIs

chip::app::Clusters::MeterIdentification::MeterIdentificationDelegate::~MeterIdentificationDelegate()
{
    if (!mUtilityName.IsNull())
    {
        chip::Platform::MemoryFree((void *) mUtilityName.Value().data());
        mUtilityName.SetNull();
    }

    if (!mPointOfDelivery.IsNull())
    {
        chip::Platform::MemoryFree((void *) mPointOfDelivery.Value().data());
        mPointOfDelivery.SetNull();
    }
}

void MeterIdentificationDelegate::Init()
{
    SetMeterType(MeterTypeEnum::kPrivate);
    SetUtilityName(CharSpan::fromCharString("Test Utility Name"));
    SetPointOfDelivery(CharSpan::fromCharString("Test PointOfDelivery"));
    SetPowerThreshold(100);
    SetPowerThresholdSource(PowerThresholdSourceEnum::kEquipment);
}

CHIP_ERROR MeterIdentificationDelegate::LoadJson(Json::Value & root)
{
    Json::Value value;

    if (root.isMember("MeterType"))
    {
        value = root.get("MeterType", Json::Value());
        if(value.isUInt())
        {
            SetMeterType(static_cast<MeterTypeEnum>(value.asUInt()));
        }
        else
        {
            SetMeterType(std::nullopt);
        }
    }

    if (root.isMember("UtilityName"))
    {
        value = root.get("UtilityName", Json::Value());
        if(value.isString())
        {
            SetUtilityName(CharSpan::fromCharString(value.asCString()));
        }
        else
        {
            SetUtilityName(std::nullopt);
        }
    }

    if (root.isMember("PointOfDelivery"))
    {
        value = root.get("PointOfDelivery", Json::Value());
        if(value.isString())
        {
            SetPointOfDelivery(CharSpan::fromCharString(value.asCString()));
        }
        else
        {
            SetPointOfDelivery(std::nullopt);
        }
    }

    if (root.isMember("PowerThreshold"))
    {
        value = root.get("PowerThreshold", Json::Value());
        if(value.isUInt64())
        {
            SetPowerThreshold(value.asUInt64());
        }
        else
        {
            SetPowerThreshold(std::nullopt);
        }
    }

    if (root.isMember("PowerThresholdSource"))
    {
        value = root.get("PowerThresholdSource", Json::Value());
        if(value.isUInt())
        {
            SetPowerThresholdSource(static_cast<PowerThresholdSourceEnum>(value.asUInt()));
        }
        else
        {
            SetPowerThresholdSource(std::nullopt);
        }
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR MeterIdentificationDelegate::SetMeterType(DataModel::Nullable<MeterTypeEnum> newValue)
{
    // DataModel::Nullable<MeterTypeEnum> oldValue = mMeterType;

    mMeterType = newValue;
    // if (oldValue != newValue)
    //{
         MatterReportingAttributeChangeCallback(mEndpointId, MeterIdentification::Id, MeterType::Id);
    // }

    return CHIP_NO_ERROR;
}

CHIP_ERROR MeterIdentificationDelegate::SetUtilityName(DataModel::Nullable<CharSpan> newValue)
{
    // CharSpan oldValue = mUtilityName;

    if (!mUtilityName.IsNull())
    {
        chip::Platform::MemoryFree((void *) mUtilityName.Value().data());
        mUtilityName.SetNull();
    }

    if (!newValue.IsNull())
    {
        size_t len = newValue.Value().size();
        char * str = (char *) chip::Platform::MemoryAlloc(len);
        memcpy(str, newValue.Value().data(), len);
        CharSpan nameString(str, len);
        mUtilityName = MakeNullable(static_cast<CharSpan>(nameString));
    }

    // if (!oldValue.data_equal(newValue))
    //{
         MatterReportingAttributeChangeCallback(mEndpointId, MeterIdentification::Id, UtilityName::Id);
    // }

    return CHIP_NO_ERROR;
}

CHIP_ERROR MeterIdentificationDelegate::SetPointOfDelivery(DataModel::Nullable<CharSpan> newValue)
{
    // CharSpan oldValue = mPointOfDelivery;

    if (!mPointOfDelivery.IsNull())
    {
        chip::Platform::MemoryFree((void *) mPointOfDelivery.Value().data());
        mPointOfDelivery.SetNull();
    }

    if (!newValue.IsNull())
    {
        size_t len = newValue.Value().size();
        char * str = (char *) chip::Platform::MemoryAlloc(len);
        memcpy(str, newValue.Value().data(), len);
        CharSpan nameString(str, len);
        mPointOfDelivery = MakeNullable(static_cast<CharSpan>(nameString));
    }

    // if (!oldValue.data_equal(newValue))
    //{
         MatterReportingAttributeChangeCallback(mEndpointId, MeterIdentification::Id, PointOfDelivery::Id);
    // }

    return CHIP_NO_ERROR;
}

CHIP_ERROR MeterIdentificationDelegate::SetPowerThreshold(DataModel::Nullable<uint64_t> newValue)
{
    // DataModel::Nullable<uint64_t> oldValue = mPowerThreshold;

    mPowerThreshold = newValue;
    // if (oldValue != newValue)
    //{
         MatterReportingAttributeChangeCallback(mEndpointId, MeterIdentification::Id, PowerThreshold::Id);
    // }

    return CHIP_NO_ERROR;
}

CHIP_ERROR MeterIdentificationDelegate::SetPowerThresholdSource(DataModel::Nullable<PowerThresholdSourceEnum> newValue)
{
    // DataModel::Nullable<PowerThresholdSourceEnum> oldValue = mPowerThresholdSource;

    mPowerThresholdSource = newValue;
    // if (oldValue != newValue)
    //{
         MatterReportingAttributeChangeCallback(mEndpointId, MeterIdentification::Id, PowerThresholdSource::Id);
    // }

    return CHIP_NO_ERROR;
}
