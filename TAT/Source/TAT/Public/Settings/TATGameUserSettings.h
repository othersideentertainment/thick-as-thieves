// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "Settings/TATUserSettingsCollection.h"

// UE
#include <GameFramework/GameUserSettings.h>

#include "TATGameUserSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATUserSettingDynamicDelegate, FGameplayTag, tag);

UCLASS(BlueprintType, Config = GameUserSettings, ConfigDoNotCheckDefaults)
class TAT_API UTATGameUserSettings : public UGameUserSettings
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, DisplayName = "Get TAT Game User Settings")
   static UTATGameUserSettings* Get();

   UTATGameUserSettings();

   virtual void LoadSettings(bool bForceReload) override;
   virtual void SaveSettings() override;
   virtual void ApplyNonResolutionSettings() override;
   virtual void ResetToCurrentSettings() override;
   virtual bool IsDirty() const override;
   virtual bool IsVersionValid() override;
   virtual void UpdateVersion() override;

   template<typename T>
   bool GetSetting(FGameplayTag tag, T& value) const;

   template<typename T>
   bool GetSettingOptions(FGameplayTag tag, TMap<T, FText>& options) const;

   template<typename T>
   bool SetSetting(FGameplayTag tag, const T& value) const;

   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Settings", meta=(ExpandBoolAsExecs = "ReturnValue"))
   bool ApplySetting(FGameplayTag tag) const;

   UPROPERTY(BlueprintAssignable)
   FTATUserSettingDynamicDelegate OnSettingChanged;

   UPROPERTY(BlueprintAssignable)
   FTATUserSettingDynamicDelegate OnSettingApplied;

protected:
   UTATUserSettingsCollection* FindCollectionForSetting(FGameplayTag tag) const;

   void HandleSettingChanged(UTATUserSettingsCollection* collection, FGameplayTag tag);
   void HandleSettingApplied(UTATUserSettingsCollection* collection, FGameplayTag tag);

   UFUNCTION(BlueprintCallable, CustomThunk, DisplayName = "Get Setting", Category = "Settings", meta=(CustomStructureParam = "value", ExpandBoolAsExecs = "ReturnValue"))
   bool GetSettingGeneric(FGameplayTag tag, uint8& value);
   DECLARE_FUNCTION(execGetSettingGeneric);

   UFUNCTION(BlueprintCallable, CustomThunk, DisplayName = "Get Setting Options", Category = "Settings", meta=(MapParam = "options", ExpandBoolAsExecs = "ReturnValue"))
   bool GetSettingOptionsGeneric(FGameplayTag tag, TMap<int32, int32>& options);
   DECLARE_FUNCTION(execGetSettingOptionsGeneric);

   UFUNCTION(BlueprintCallable, CustomThunk, DisplayName = "Set Setting", Category = "Settings", meta=(CustomStructureParam = "value", ExpandBoolAsExecs = "ReturnValue"))
   bool SetSettingGeneric(FGameplayTag tag, const uint8& value);
   DECLARE_FUNCTION(execSetSettingGeneric);

   UPROPERTY(Config)
   uint32 CustomVersion = 0;

   UPROPERTY(Config)
   uint32 ScalabilityVersion = 0;

   UPROPERTY()
   TArray<TObjectPtr<UTATUserSettingsCollection>> Collections;
};

template<typename T>
bool UTATGameUserSettings::GetSetting(FGameplayTag tag, T& value) const
{
   UTATUserSettingsCollection* collection = FindCollectionForSetting(tag);
   return collection && collection->GetSetting<T>(tag, value);
}

template<typename T>
bool UTATGameUserSettings::GetSettingOptions(FGameplayTag tag, TMap<T, FText>& options) const
{
   UTATUserSettingsCollection* collection = FindCollectionForSetting(tag);
   return collection && collection->GetSettingOptions<T>(tag, options);
}

template<typename T>
bool UTATGameUserSettings::SetSetting(FGameplayTag tag, const T& value) const
{
   UTATUserSettingsCollection* collection = FindCollectionForSetting(tag);
   return collection && collection->SetSetting<T>(tag, value);
}
