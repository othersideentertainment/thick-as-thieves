// (c) 2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Analytics/TATAnalyticsManager.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "TATGameInstance.h"
#include "Settings/TATGameUserSettings.h"
#include "Tools/TATToolComponent.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue4
#include "Analytics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnalyticsManager)

DEFINE_LOG_CATEGORY(LogAnalyticsManager)
DEFINE_LOG_CATEGORY(LogAnalyticsManagerAlt)


FString FTATCustomValue::ToString() const
{
   switch (ValueType)
   {
   case ETATAnalyticsCustomValueType::AsNumber:
      return FString::Printf(TEXT("%f"), ValueNumber);
   case ETATAnalyticsCustomValueType::AsString:
      return ValueString;
   case ETATAnalyticsCustomValueType::AsBool:
      return ValueBool ? TEXT("true") : TEXT("false");
   }
   return TEXT("NOT SET");
}

FString FTATAnalyticsCustomFields::ToString() const
{
   TStringBuilder<128> builder;
   for (const FTATCustomValue& value : Values)
   {
      builder.Appendf(TEXT("[%s : %s]\r\n"), *value.Key, *value.ToString());
   }
   return builder.ToString();
}

void FTATAnalyticsCustomFields::Set(FString const& Key, const double Value)
{
   FTATCustomValue& newValue = Values.AddDefaulted_GetRef();
   newValue.Key = Key;
   newValue.ValueType = ETATAnalyticsCustomValueType::AsNumber;
   newValue.ValueNumber = Value;
}

void FTATAnalyticsCustomFields::Set(FString const& Key, FString const& Value)
{
   FTATCustomValue& newValue = Values.AddDefaulted_GetRef();
   newValue.Key = Key;
   newValue.ValueType = ETATAnalyticsCustomValueType::AsString;
   newValue.ValueString = Value;
}

void FTATAnalyticsCustomFields::Set(FString const& Key, const FGameplayTag& Value)
{
   FTATCustomValue& newValue = Values.AddDefaulted_GetRef();
   newValue.Key = Key;
   newValue.ValueType = ETATAnalyticsCustomValueType::AsString;
   newValue.ValueString = Value.ToString();
}

void FTATAnalyticsCustomFields::Set(FString const& Key, const bool Value)
{
   FTATCustomValue& newValue = Values.AddDefaulted_GetRef();
   newValue.Key = Key;
   newValue.ValueType = ETATAnalyticsCustomValueType::AsBool;
   newValue.ValueBool = Value;
}

void UTATAnalyticsManager::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   //Test(TEXT("Hello World"));

   UE_LOG(LogAnalyticsManager, Log, TEXT("GameAnalytics is %s"), IsEnabled()? TEXT("enabled") : TEXT("disabled"));

   //if (!UFeatureFlagLibrary::IsGameAnalyticsEnabled()) // Note that UFeatureFlagLibrary is more for Blueprints, as C++ cannot make use of compile time stripping
   if (!IsEnabled()) 
   {
      return;
   }

   if (UTATGameUserSettings* userSettings = UTATGameUserSettings::Get())
   {
      _HandleAnalyticsSettingApplied(Tag_Settings_General_AllowAnalytics);
      userSettings->OnSettingApplied.AddUniqueDynamic(this, &ThisClass::_HandleAnalyticsSettingApplied);
   }
}

void UTATAnalyticsManager::Deinitialize()
{
   _HandleGameAnalyticsShutdown();
   Super::Deinitialize();
}


FString UTATAnalyticsManager::OnProviderGetValue(const FString& KeyName, bool bIsRequired)
{
#ifdef TEST_GAME_ANALYTICS_LOCAL_GAME
   const FString SectionName = FString(TEXT("/Script/GameAnalyticsEditor.GameAnalyticsProjectSettings"));
#else
   const FString SectionName = FString::Printf(TEXT("GameAnalytics-%s"), *UTATAnalyticsManager::GetEnvironment());
#endif
    
   FString StrOut;
   GConfig->GetString(*SectionName, *KeyName, StrOut, GEngineIni);

   return StrOut;
}

bool UTATAnalyticsManager::IsInitialized() const
{
   return _isInitialized;
}

bool UTATAnalyticsManager::IsEnabled() const
{
   return false;
}

void UTATAnalyticsManager::OnFTUEStarted() const
{
   TStringBuilder<EventBufLength> strBuf;
   strBuf << TEXT("ftue");
   FString eventId = strBuf.ToString();
   UE_LOG(LogAnalyticsManager, Log, TEXT("Progression Start '%s'."), *strBuf);
}

void UTATAnalyticsManager::OnFTUECompleted() const
{
   TStringBuilder<EventBufLength> strBuf;
   strBuf << TEXT("ftue");
   UE_LOG(LogAnalyticsManager, Log, TEXT("Progression Completed '%s'."), *strBuf);
}

void UTATAnalyticsManager::OnContractStarted() const
{
   TStringBuilder<EventBufLength> strBuf;
   strBuf << TEXT("contract");
   UE_LOG(LogAnalyticsManager, Log, TEXT("Progression Start '%s'."), *strBuf);
}

void UTATAnalyticsManager::OnContractCompleted() const
{
   TStringBuilder<EventBufLength> strBuf;
   strBuf << TEXT("contract");
   UE_LOG(LogAnalyticsManager, Log, TEXT("Progression Completed '%s'."), *strBuf);
}


void UTATAnalyticsManager::_HandleGameAnalyticsInit()
{
}

void UTATAnalyticsManager::_HandleGameAnalyticsShutdown()
{
}

void UTATAnalyticsManager::_HandleAnalyticsSettingApplied(const FGameplayTag tag)
{
   if (tag == Tag_Settings_General_AllowAnalytics)
   {
      bool allowAnalytics = false;
      UTATGameUserSettings::Get()->GetSetting(Tag_Settings_General_AllowAnalytics, allowAnalytics);
      UE_LOG(LogTemp, Warning, TEXT("Allow Analytics %s "), allowAnalytics ? TEXT("Yes") : TEXT("No"));
      if (_isInitialized && allowAnalytics == false)
      {
         _HandleGameAnalyticsShutdown();
      }
      else if (_isInitialized == false && allowAnalytics)
      {
         _HandleGameAnalyticsInit();
      }
   }
}

FString UTATAnalyticsManager::_GetEventIDForToolEvent(FString eventName)
{
   TStringBuilder<EventBufLength> strBuf;
   strBuf << TEXT("tool_event_") << eventName;
   FString eventId = strBuf.ToString();
   return eventId;
}


void UTATAnalyticsManager::OnToolEquipped(const FString& equipmentName)
{
   _HandleToolDesignEvent(true, equipmentName);
}

void UTATAnalyticsManager::OnToolUnEquipped(const FString& equipmentName)
{
   _HandleToolDesignEvent(false, equipmentName);
}

void UTATAnalyticsManager::OnDesignEvent(const FString& eventName)
{
   OnDesignEventWithCustomFields(eventName, FTATAnalyticsCustomFields());
}


void UTATAnalyticsManager::OnDesignEventWithCustomFields(const FStringView& eventName, const FTATAnalyticsCustomFields& customFields)
{
}

void UTATAnalyticsManager::OnAbilityTriggered(UOSEGameplayAbility* ability, AActor* instigator)
{
   if (ability == nullptr || instigator == nullptr)
      return;
   
   if (ability->IsLocallyControlled() == false)
      return;
   
   FTATAnalyticsCustomFields fields;
   fields.Set(TEXT("Ability"), GetNameSafe(ability->GetClass()));
   fields.Set(TEXT("Location"), instigator->GetActorLocation().ToCompactString());
   
   OnDesignEventWithCustomFields(TEXT("AbilityActivated"), fields);
}

void UTATAnalyticsManager::OnAbilityTriggeredWithTool(UOSEGameplayAbility* ability, AActor* instigator, UTATToolComponent* tool)
{
   if (ability == nullptr || instigator == nullptr || tool == nullptr)
   {
      return;
   }

   if (ability->IsLocallyControlled() == false)
      return;
   
   FTATAnalyticsCustomFields fields;
   fields.Set(TEXT("Ability"), GetNameSafe(ability->GetClass()));
   fields.Set(TEXT("Tool"), GetNameSafe(tool->GetClass()));
   fields.Set(TEXT("Location"), instigator->GetActorLocation().ToCompactString());
   
   OnDesignEventWithCustomFields(TEXT("ToolAbilityActivated"), fields);
}

void UTATAnalyticsManager::_HandleToolDesignEvent(
   const bool started,
   const FString& eventName)
{
   FString eventId = _GetEventIDForToolEvent(eventName);
   float timeTakenToComplete = -1.f;
   if (started)
   {
      _EventTimeStartedMap.FindOrAdd(eventId) = GetWorld()->GetRealTimeSeconds();
   }
   else
   {
      float timeStarted = 0.0f;
      if (_EventTimeStartedMap.RemoveAndCopyValue(eventId, timeStarted))
      {
         timeTakenToComplete = GetWorld()->GetRealTimeSeconds() - timeStarted;
      }
   }
   
   UE_LOG(LogAnalyticsManager, 
      Log,
      TEXT("_HandleToolDesignEvent Event '%s' started? '%s'. time taken '%f"),
      *eventId,
      started ? TEXT("true") : TEXT("false"),
      timeTakenToComplete);

   if (started == false)
   {
   }
}

FString UTATAnalyticsManager::_GetEventIDForFTUEEvent(FString eventName)
{
   TStringBuilder<EventBufLength> strBuf;
   strBuf << TEXT("ftue_event_") << eventName;
   FString eventId = strBuf.ToString();
   return eventId;
}

void UTATAnalyticsManager::_HandleFTUEProgressionEvent(const bool started, const FString& eventSection, const FString& eventName)
{
   FString eventId = _GetEventIDForFTUEEvent(eventName);
   UE_LOG(LogAnalyticsManager, 
      Log,
      TEXT("_HandleFTUEProgressionEvent Section '%s' Event '%s' started? '%s'."),
      *eventSection,
      *eventId,
      started ? TEXT("true") : TEXT("false"));

   float timeTakenToComplete = -1.f;
   if (started)
   {
      _EventTimeStartedMap.FindOrAdd(eventId) = GetWorld()->GetRealTimeSeconds();
   }
   else
   {
      float timeStarted = 0.0f;
      if (_EventTimeStartedMap.RemoveAndCopyValue(eventId, timeStarted))
      {
         timeTakenToComplete = GetWorld()->GetRealTimeSeconds() - timeStarted;
      }
   }
}

void UTATAnalyticsManager::OnFTUEProgressionEventStarted(const FString& eventSection, const FString& eventName)
{
   _HandleFTUEProgressionEvent(true, eventSection, eventName);
}

void UTATAnalyticsManager::OnFTUEProgressionEventCompleted(const FString& eventSection, const FString& eventName)
{
   _HandleFTUEProgressionEvent(false, eventSection, eventName);
}

void UTATAnalyticsManager::HandleContentUnlocked(const FTATUnlockableContent& unlockableContent)
{
   HandleContentUnlocked(unlockableContent.Tag);
}

void UTATAnalyticsManager::HandleContentUnlocked(const FGameplayTag& tag)
{
   FTATAnalyticsCustomFields analyticsFields;
   analyticsFields.Set(TEXT("Unlockable"), tag);
   OnDesignEventWithCustomFields(TEXT("ContentUnlocked"), analyticsFields);
}

void UTATAnalyticsManager::HandlePlayerLevelChanged(const int level)
{
   FTATAnalyticsCustomFields analyticsFields;
   analyticsFields.Set(TEXT("Level"), static_cast<double>(level));
   OnDesignEventWithCustomFields(TEXT("PlayerLevelChanged"), analyticsFields);
}

