// (c) 2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "Common/TATVersionEdition.h"

#include "Unlockables/TATUnlockableContent.h"

#include "TATAnalyticsManager.generated.h"

class UTATToolComponent;
class UOSEGameplayAbility;
struct FGACustomFields;
DECLARE_LOG_CATEGORY_EXTERN(LogAnalyticsManager, Log, All)

DECLARE_LOG_CATEGORY_EXTERN(LogAnalyticsManagerAlt, Log, All)

// NOTE: These are mirroring the GameAnalytics module, as that module doesn't exist on servers
// We use our own and convert in place when needed
UENUM()
enum class ETATAnalyticsCustomValueType
{
   AsString,
   AsNumber,
   AsBool
};

USTRUCT(BlueprintType)
struct FTATCustomValue
{
   GENERATED_BODY();

   UPROPERTY()
   FString Key;

   UPROPERTY()
   FString ValueString;

   UPROPERTY()
   double ValueNumber = 0.0;

   UPROPERTY()
   bool ValueBool = false;

   UPROPERTY()
   ETATAnalyticsCustomValueType ValueType = ETATAnalyticsCustomValueType::AsNumber;
   FString ToString() const;
};

USTRUCT(BlueprintType)
struct TAT_API FTATAnalyticsCustomFields
{
   GENERATED_BODY();

   UPROPERTY()
   TArray<FTATCustomValue> Values;

   bool IsEmpty() const { return Values.IsEmpty(); }
   FString ToString() const;

   void Set(FString const& Key, double Value);
   void Set(FString const& Key, FString const& Value);
   void Set(FString const& Key, const FGameplayTag& Value);
   void Set(FString const& Key, bool Value);
};


UCLASS()
class TAT_API UTATAnalyticsManager : public UGameInstanceSubsystem
{
   GENERATED_BODY()   

public:
    // Custom data provision to supply environment specific values (type of FAnalyticsProviderConfigurationDelegate)
   static FString OnProviderGetValue(const FString& keyName, bool bIsRequired);

   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   virtual void Deinitialize() override;

   UFUNCTION(BluePrintCallable, Category = "Analytics")
   bool IsInitialized() const;

   UFUNCTION(BluePrintCallable, Category = "Analytics")
   bool IsEnabled() const;
   
   UFUNCTION(BluePrintCallable, Category = "Analytics")
   void OnFTUEStarted() const;

   UFUNCTION(BluePrintCallable, Category = "Analytics")
   void OnFTUECompleted() const;

   UFUNCTION(BluePrintCallable, Category = "Analytics")
   void OnContractStarted() const;
 
   UFUNCTION(BluePrintCallable, Category = "Analytics")
   void OnContractCompleted() const;

   UFUNCTION(BluePrintCallable, Category = "Analytics")
   void OnToolEquipped(const FString& equipmentName);
   UFUNCTION(BluePrintCallable, Category = "Analytics")
   void OnToolUnEquipped(const FString& equipmentName);

   UFUNCTION(BluePrintCallable, Category = "Analytics")
   void OnDesignEvent(const FString& eventName);

   void OnDesignEventWithCustomFields(const FStringView& eventName, const FTATAnalyticsCustomFields& customFields);
   
   UFUNCTION(BlueprintCallable, Category = "Analytics")
   void OnAbilityTriggered(UOSEGameplayAbility* ability, AActor* instigator);
   UFUNCTION(BlueprintCallable, Category = "Analytics")
   void OnAbilityTriggeredWithTool(UOSEGameplayAbility* ability, AActor* instigator, UTATToolComponent* tool);
      
   void OnFTUEProgressionEventStarted(const FString& eventSection, const FString& eventName);
   void OnFTUEProgressionEventCompleted(const FString& eventSection, const FString& eventName);
   void HandleContentUnlocked(const FTATUnlockableContent& unlockableContent);
   void HandleContentUnlocked(const FGameplayTag& tag);
   void HandlePlayerLevelChanged(int level);

protected:
   static constexpr int32 EventBufLength = 256;

   static const FString GetEnvironment();
   static const FString GetGameVersion();
   static const FString GetPlatform();
   static const FString GetSDKIntegration();

   bool _isInitialized = false;
   bool _hasEverPreviouslyBeenInitialized = false;

   bool _inSession = false;

   FString _GetEventIDForToolEvent(FString eventName);
   void _HandleToolDesignEvent(bool started, const FString& eventName);
   
   void _HandleGameAnalyticsInit();
   void _HandleGameAnalyticsShutdown();
   
   UFUNCTION()
   void _HandleAnalyticsSettingApplied(FGameplayTag tag);
   static FString _GetEventIDForFTUEEvent(FString eventName);   
   void _HandleFTUEProgressionEvent(bool started, const FString& eventSection, const FString& eventName);
   TMap<FString, float> _EventTimeStartedMap;
};


inline
const FString UTATAnalyticsManager::GetEnvironment()
{
   return FString(TEXT("DEV"));
}

inline
const FString UTATAnalyticsManager::GetGameVersion()
{
  return UTATVersion::GetBuildVersionString();
}

inline 
const FString UTATAnalyticsManager::GetPlatform()
{
   return FPlatformProperties::IniPlatformName();
}

inline
const FString UTATAnalyticsManager::GetSDKIntegration()
{
   TStringBuilder<EventBufLength> strBuf;
   strBuf << TEXT("UE ") << ENGINE_MAJOR_VERSION << ENGINE_MINOR_VERSION << ENGINE_PATCH_VERSION;
   return strBuf.ToString();
}
