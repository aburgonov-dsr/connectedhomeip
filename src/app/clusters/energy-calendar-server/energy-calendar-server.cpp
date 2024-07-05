/*
 *    Copyright (c) 2024 Project CHIP Authors
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

#include "energy-calendar-server.h"

#include <app/AttributeAccessInterface.h>
#include <app/AttributeAccessInterfaceRegistry.h>
#include <app/ConcreteAttributePath.h>
#include <app/InteractionModelEngine.h>
#include <app/util/attribute-storage.h>
#include <platform/ThreadStackManager.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyCalendar;
using namespace chip::app::Clusters::EnergyCalendar::Attributes;

namespace chip {
namespace app {
namespace Clusters {
namespace EnergyCalendar {

constexpr uint32_t kSecInOneDay = 60 * 60 * 24;

uint32_t EnergyCalendarServer::mCurrentDate = 0;

#define VerifyOrReturnLogSend(expr, ep, ...)                                                                                       \
    do                                                                                                                             \
    {                                                                                                                              \
        if (!(expr))                                                                                                               \
        {                                                                                                                          \
            ErrorMessage(ep, __VA_ARGS__);                                                                                         \
            return false;                                                                                                          \
        }                                                                                                                          \
    } while (false)


#if 0
static TransitionDayOfWeekBitmap GetWeekDate(uint32_t date)
{
    tm calendarTime{};
    time_t tm = date;
    localtime_r(&tm, &calendarTime);
    return (TransitionDayOfWeekBitmap)(calendarTime.tm_wday);
}
#endif

static uint32_t GetCurrentTime(void)
{
    System::Clock::Microseconds64 utcTimeUnix;
    uint64_t chipEpochTime;
    System::SystemClock().GetClock_RealTime(utcTimeUnix);
    UnixEpochToChipEpochMicros(utcTimeUnix.count(), chipEpochTime);
    
    chipEpochTime = (chipEpochTime / chip::kMicrosecondsPerSecond);
    return static_cast<uint32_t>(chipEpochTime % kSecInOneDay);
}

static uint32_t GetCurrentDay(void)
{
    System::Clock::Microseconds64 utcTimeUnix;
    uint64_t chipEpochTime;
    System::SystemClock().GetClock_RealTime(utcTimeUnix);
    UnixEpochToChipEpochMicros(utcTimeUnix.count(), chipEpochTime);
    
    chipEpochTime = (chipEpochTime / chip::kMicrosecondsPerSecond);
    return static_cast<uint32_t>(chipEpochTime - (chipEpochTime % kSecInOneDay));
}

void LockThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().LockThreadStack();
}

void UnlockThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().UnlockThreadStack();
}

CHIP_ERROR CalendarProvider::SetCalendarID(DataModel::Nullable<uint32_t> calendarID)
{
    if (_calendarID.Update(calendarID))
    {
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, CalendarID::Id);
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR CalendarProvider::SetName(DataModel::Nullable<CharSpan> name)
{
    bool change = (name.IsNull() != _name.IsNull()) ||
                  (!name.IsNull() &&  name.Value().data_equal(_name.Value()));

    if (change)
    {
        if (name.IsNull())
        {
            _name.SetNull();
        }
        else
        {
            _name.SetNonNull(*name);
        }

        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, Name::Id);
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR CalendarProvider::SetProviderID(DataModel::Nullable<uint32_t> providerID)
{
    if (_providerID.Update(providerID))
    {
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, ProviderID::Id);
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR CalendarProvider::SetEventID(DataModel::Nullable<uint32_t> eventID)
{
    if (_eventID.Update(eventID))
    {
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, EventID::Id);
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR CalendarProvider::SetCalendarPeriod(DataModel::Nullable<uint32_t> startDate,
                                               DataModel::List<Structs::CalendarPeriodStruct::Type> calendarPeriods)
{
    bool check = true;
    bool change;
    if ((startDate.IsNull() && !calendarPeriods.empty()) ||
        (!startDate.IsNull() && calendarPeriods.empty()))
    {
        ErrorMessage(_endpoint, "StartDate and CalendarPeriods must together either have values ​​or be empty");
        check = false;

    }
    else if (!calendarPeriods.empty())
    {
        check = CheckPeriods(calendarPeriods);
    }

    LockThreadTask();

    if (check)
    {
        change = _startDate.Update(startDate);
        _calendarPeriods = calendarPeriods;
    }
    else
    {
        change = _startDate.Update(DataModel::Nullable<uint32_t>());
        _calendarPeriods = DataModel::List<Structs::CalendarPeriodStruct::Type>();
    }

    if (change)
    {
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, StartDate::Id);
    }
    MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, CalendarPeriods::Id);

    UnlockThreadTask();

    EnergyCalendarServer::mCurrentDate = 0;
        
    return check ? CHIP_NO_ERROR : CHIP_ERROR_INCORRECT_STATE;
}

CHIP_ERROR CalendarProvider::SetSpecialDays(DataModel::List<Structs::DayStruct::Type> specialDays)
{
    bool check = CheckSpecialDays(specialDays);

    LockThreadTask();

    if (check)
    {
        _specialDays = specialDays;
    }
    else
    {
        _specialDays = DataModel::List<Structs::DayStruct::Type>();
    }

    MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, SpecialDays::Id);

    UnlockThreadTask();

    EnergyCalendarServer::mCurrentDate = 0;

    return check ? CHIP_NO_ERROR : CHIP_ERROR_INCORRECT_STATE;
}

CHIP_ERROR CalendarProvider::SetCurrentAndNextDays(DataModel::Nullable<Structs::DayStruct::Type> CurrentDay,
                                                   DataModel::Nullable<Structs::DayStruct::Type> NextDay)
{
    bool change;

    LockThreadTask();

    change = true; // *CurrentDay != *_currentDay;
    if (change)
    {
        _currentDay.SetNonNull(*CurrentDay);
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, CurrentDay::Id);
    }

    change = true; // *NextDay != *_nextDay;
    if (change)
    {
        _nextDay.SetNonNull(*NextDay);
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, NextDay::Id);
    }

    UnlockThreadTask();
    return CHIP_NO_ERROR;
}

CHIP_ERROR CalendarProvider::SetPeakPeriods(DataModel::Nullable<Structs::PeakPeriodStruct::Type> CurrentPeakPeriod,
                                            DataModel::Nullable<Structs::PeakPeriodStruct::Type> NextPeakPeriod)
{
    bool change;
    LockThreadTask();

    change = true; // CurrentPeakPeriod != _currentPeakPeriod;
    if (change)
    {
        _currentPeakPeriod.SetNonNull(*CurrentPeakPeriod);
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, CurrentPeakPeriod::Id);
    }

    change = true; // NextPeakPeriod != _nextPeakPeriod;
    if (change)
    {
        _nextPeakPeriod.SetNonNull(*NextPeakPeriod);
        MatterReportingAttributeChangeCallback(_endpoint, EnergyCalendar::Id, NextPeakPeriod::Id);
    }

    UnlockThreadTask();
    return CHIP_NO_ERROR;
}

bool CalendarProvider::CheckPeriods(DataModel::List<Structs::CalendarPeriodStruct::Type> &periods)
{
    uint32_t date = 0;
    VerifyOrReturnLogSend((periods.size() > 0 || periods.size() <= 4), _endpoint, "Periods list size must be from 1 to 4");
    for (auto & period : periods)
    {
        if (!period.startDate.IsNull())
        {
            if (period.startDate.Value() < date)
            {
                return false;
            }
            date = period.startDate.Value();
        }

        for (auto & day : period.days)
        {
            if (!CheckDay(day))
            {
                return false;
            }
        }
    }

    return true;
}

bool CalendarProvider::CheckSpecialDays(DataModel::List<Structs::DayStruct::Type> & specialDays)
{
    uint32_t date = 0;
    VerifyOrReturnLogSend(specialDays.size() <= 50, _endpoint, "Special Days list size must be no more 50");
    for (auto & day : specialDays)
    {
        VerifyOrReturnLogSend(!day.daysOfWeek.HasValue(), _endpoint, "Day in Special Days cannot have DayOfWeek value");
        VerifyOrReturnLogSend(!day.calendarID.HasValue(), _endpoint, "Day in Special Days cannot have CalendarID value");
        VerifyOrReturnLogSend(day.date.HasValue(), _endpoint, "Day in Special Days must have Date value");
        VerifyOrReturnLogSend(day.date.Value() > date, _endpoint, "Days in Special Days must be order");
        if(!CheckDay(day))
        {
            return false;
        }
        date = day.date.Value();
    }

    return true;
}

bool CalendarProvider::CheckDay(const Structs::DayStruct::Type & day)
{
    VerifyOrReturnLogSend(day.daysOfWeek.HasValue() && day.date.HasValue(), _endpoint,
        "Day can have only one value or DayOfWeek or Date");
    
    VerifyOrReturnLogSend((day.transitions.size() > 0 || day.transitions.size() <= 48), _endpoint,
        "Day transinions list must have from 1 to 48 records");
    
    uint32_t time = 0;
    for (auto & transition : day.transitions)
    {
        auto tr_time = transition.transitionTime;
        VerifyOrReturnLogSend(tr_time > 1499, _endpoint, "Day transitions must be less 1499");
        VerifyOrReturnLogSend(tr_time > time, _endpoint, "Day transitions must be order");
/*
        if (feature.Has(Feature::kPricingTier))
            VerifyOrReturnLogSend(transition.priceTier.HasValue(), _endpoint, "Transition must have PriceTier value");
        if (feature.Has(Feature::kFriendlyCredit))
            VerifyOrReturnLogSend(transition.friendlyCredit.HasValue(), _endpoint, "Transition must have FriendlyCredit value");
        if (feature.Has(Feature::kAuxiliaryLoad))
            VerifyOrReturnLogSend(transition.auxiliaryLoad.HasValue(), _endpoint, "Transition must have AuxiliaryLoad value");
*/
        time = tr_time;
    }

    return true;
}

bool CalendarProvider::CheckPeakPeriod(const Structs::PeakPeriodStruct::Type & peakPeriod)
{
    VerifyOrReturnLogSend(peakPeriod.severity >= PeakPeriodSeverityEnum::kUnknownEnumValue,
        _endpoint, "Wrong PeakPeriod Severity value %d", static_cast<int>(PeakPeriodSeverityEnum::kUnknownEnumValue));

    return true;
}

CHIP_ERROR CalendarProvider::UpdateDays(uint64_t day)
{
    CHIP_ERROR status;
    DataModel::Nullable<Structs::DayStruct::Type> currentDay = GetDay(day);
    DataModel::Nullable<Structs::DayStruct::Type> nextDay = GetDay(day + kSecInOneDay);

    if (status == CHIP_NO_ERROR)
    {
        status = SetCurrentAndNextDays(currentDay, nextDay);
    }

    return status;
}

EnergyCalendarServer::EnergyCalendarServer() : AttributeAccessInterface(NullOptional, EnergyCalendar::Id), feature(0)
{
    //uint32_t time = GetCurrentTime();

    chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5 /*kSecInOneDay - time*/), MidnightTimerCallback, this);
}

EnergyCalendarServer::EnergyCalendarServer(Feature aFeature) :
    AttributeAccessInterface(NullOptional, EnergyCalendar::Id), feature(aFeature)
{
    //uint32_t time = GetCurrentTime();

    chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5 /*kSecInOneDay - time*/), MidnightTimerCallback, this);
}

bool EnergyCalendarServer::HasFeature(Feature aFeature) const
{
    return feature.Has(aFeature);
}

CHIP_ERROR EnergyCalendarServer::AddCalendarProvider(CalendarProvider * provider)
{
    for (auto i = 0; i < kNumSupportedEndpoints; ++i)
    {
        if (calendars[i] == nullptr)
        {
            calendars[i] = provider;
            return CHIP_NO_ERROR;
        }
    }
    return CHIP_ERROR_NO_MEMORY;
}

CalendarProvider * EnergyCalendarServer::GetProvider(EndpointId ep)
{
    for (auto i = 0; i < kNumSupportedEndpoints; ++i)
    {
        if (calendars[i] != nullptr && calendars[i]->endpoint() == ep)
        {
            return calendars[i];
        }
    }
    return nullptr;
}

DataModel::Nullable<Structs::TransitionStruct::Type> EnergyCalendarServer::GetTransition(EndpointId ep)
{
    CalendarProvider * provider = GetProvider(ep);
    if (provider == nullptr || provider->GetCurrentDay().IsNull())
    {
        return DataModel::Nullable<Structs::TransitionStruct::Type>();
    }

    Structs::DayStruct::Type & currentDay = provider->GetCurrentDay().Value();

    uint32_t time = GetCurrentTime();

    auto transition       = currentDay.transitions.begin();
    uint32_t next_tr_time = kSecInOneDay;

    const Structs::TransitionStruct::Type * current = nullptr;

    while (transition != currentDay.transitions.end())
    {
        auto tr_time = transition->transitionTime;
        if (tr_time <= time && (current == nullptr || current->transitionTime < tr_time))
        {
            current = transition;
        }
        if ((time > tr_time) && (time < next_tr_time))
        {
            next_tr_time = time;
        }
    }
    return DataModel::Nullable<Structs::TransitionStruct::Type>(*current);
}

// AttributeAccessInterface
CHIP_ERROR EnergyCalendarServer::Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder)
{
    ChipLogProgress(Zcl, " EnergyCalendarServer::Read");
    //ChipLogProgress(Zcl, "current date %ld time %d", GetCurrentDay(), GetCurrentTime());
    CalendarProvider * provider = GetProvider(aPath.mEndpointId);
    switch (aPath.mAttributeId)
    {
    case CalendarID::Id:
        return aEncoder.Encode(provider->GetCalendarID());
    case Name::Id:
        return aEncoder.Encode(provider->GetName());
    case ProviderID::Id:
        return aEncoder.Encode(provider->GetProviderID());
    case EventID::Id:
        return aEncoder.Encode(provider->GetEventID());
    case StartDate::Id:
        return aEncoder.Encode(provider->GetStartDate());
    case CalendarPeriods::Id:
        return aEncoder.Encode(provider->GetCalendarPeriods());
    case SpecialDays::Id:
        return aEncoder.Encode(provider->GetSpecialDays());
    /* Date relative attributes */
    case CurrentDay::Id:
        return aEncoder.Encode(provider->GetCurrentDay());
    case NextDay::Id:
        return aEncoder.Encode(provider->GetNextDay());
    case CurrentTransition::Id:
        return aEncoder.Encode(GetTransition(aPath.mEndpointId));
    case CurrentPeakPeriod::Id:
        if (feature.Has(Feature::kPeakPeriod))
            return aEncoder.Encode(provider->GetCurrentPeakPeriod());
        else
            return CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute);
    case NextPeakPeriod::Id:
        if (feature.Has(Feature::kPeakPeriod))
            return aEncoder.Encode(provider->GetNextPeakPeriod());
        else
            return CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute);
    /* FeatureMap - is held locally */
    case FeatureMap::Id:
        return aEncoder.Encode(feature);
    }

    /* Allow all other unhandled attributes to fall through to Ember */
    return CHIP_NO_ERROR;
}

CHIP_ERROR EnergyCalendarServer::Write(const ConcreteDataAttributePath & aPath, AttributeValueDecoder & aDecoder)
{
    switch (aPath.mAttributeId)
    {
    default:
        // Unknown attribute; return error.  None of the other attributes for
        // this cluster are writable, so should not be ending up in this code to
        // start with.
        return CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute);
    }
}

// void EnergyCalendarServer::InvokeCommand(HandlerContext & handlerContext)
//{
//     //using namespace Commands;
//
//     //switch (handlerContext.mRequestPath.mCommandId)
//     //{
//     //}
//     return;
// }

void EnergyCalendarServer::MidnightTimerCallback(chip::System::Layer *, void * callbackContext)
{
    EnergyCalendarServer * instance = (EnergyCalendarServer *) callbackContext;

    uint32_t day = GetCurrentDay();
    if (day != mCurrentDate)
    {
        mCurrentDate = day;
        for (int i = 0; i < kNumSupportedEndpoints; ++i)
        {
            if (instance->calendars[i] != nullptr)
            {
                instance->calendars[i]->UpdateDays(mCurrentDate);
            }
        }
    }

    //uint32_t time = GetCurrentTime();
    chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5 /*kSecInOneDay - time*/), MidnightTimerCallback,
                                                callbackContext);
}

} // namespace EnergyCalendar
} // namespace Clusters
} // namespace app
} // namespace chip

// -----------------------------------------------------------------------------
// Plugin initialization

void MatterEnergyCalendarPluginServerInitCallback() {}
