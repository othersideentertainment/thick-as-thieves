// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATGameUserSettings.h"

// UE
#include <Blueprint/BlueprintExceptionInfo.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameUserSettings)

// Add a new version to mark significant changes that should invalidate user settings
enum class ETATGameUserSettingsVersion : uint32
{
   Initial = 0,
   AddModularSettings,

   VersionPlusOne,
   Latest = VersionPlusOne - 1
};

UTATGameUserSettings* UTATGameUserSettings::Get()
{
   return GEngine ? Cast<UTATGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

UTATGameUserSettings::UTATGameUserSettings()
{
   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      UTATUserSettingsCollection::OnSettingChanged.AddUObject(this, &ThisClass::HandleSettingChanged);
      UTATUserSettingsCollection::OnSettingApplied.AddUObject(this, &ThisClass::HandleSettingApplied);

      TArray<UClass*> collectionClasses;
      GetDerivedClasses(UTATUserSettingsCollection::StaticClass(), collectionClasses);

      for (UClass* collectionClass : collectionClasses)
      {
         if (collectionClass->HasAnyClassFlags(CLASS_Abstract))
         {
            continue;
         }

         UTATUserSettingsCollection* collectionCDO = collectionClass->GetDefaultObject<UTATUserSettingsCollection>();
         if (ensureMsgf(collectionCDO->GetTag().IsValid(), TEXT("Settings collection '%s' has an invalid tag"), *GetName()))
         {
            Collections.Add(collectionCDO);
         }
      }
   }
}

void UTATGameUserSettings::LoadSettings(bool bForceReload)
{
   Super::LoadSettings(bForceReload);

#if !UE_SERVER
   // Check scalability version to determine if we should invalidate user scalability settings and run automated hardware benchmark
   static int32 ScalabilityVersionFromIni = GConfig->GetIntOrDefault(TEXT("ScalabilitySettings"), TEXT("Version"), ScalabilityVersion, GScalabilityIni);
   if (ScalabilityVersion != ScalabilityVersionFromIni)
   {
      ScalabilityVersion = ScalabilityVersionFromIni;

      // Due to concerns about the hardware benchmark inducing a crash, save ScalabilityVersion immediately
      // so that the next run can skip the automated benchmark regardless if its completed in the last run
      SaveSettings();

      RunHardwareBenchmark();
      ApplyHardwareBenchmarkResults();
   }
#endif

   for (UTATUserSettingsCollection* collection : Collections)
   {
      collection->LoadSettings();
   }
}

void UTATGameUserSettings::SaveSettings()
{
   Super::SaveSettings();

   for (UTATUserSettingsCollection* collection : Collections)
   {
      collection->SaveSettings();
   }
}

void UTATGameUserSettings::ApplyNonResolutionSettings()
{
   Super::ApplyNonResolutionSettings();

   for (UTATUserSettingsCollection* collection : Collections)
   {
      collection->ApplySettings();
   }
}

void UTATGameUserSettings::ResetToCurrentSettings()
{
   Super::ResetToCurrentSettings();

   for (UTATUserSettingsCollection* collection : Collections)
   {
      collection->ResetSettings();
   }
}

bool UTATGameUserSettings::IsDirty() const
{
   if (Super::IsDirty())
   {
      return true;
   }

   for (UTATUserSettingsCollection* collection : Collections)
   {
      if (collection->IsDirty())
      {
         return true;
      }
   }

   return false;
}

bool UTATGameUserSettings::IsVersionValid()
{
   return Super::IsVersionValid() && CustomVersion == static_cast<uint32>(ETATGameUserSettingsVersion::Latest);
}

void UTATGameUserSettings::UpdateVersion()
{
   Super::UpdateVersion();
   CustomVersion = static_cast<uint32>(ETATGameUserSettingsVersion::Latest);
}

UTATUserSettingsCollection* UTATGameUserSettings::FindCollectionForSetting(FGameplayTag tag) const
{
   for (UTATUserSettingsCollection* collection : Collections)
   {
      if (tag.MatchesTag(collection->GetTag()))
      {
         return collection;
      }
   }

   return nullptr;
}

void UTATGameUserSettings::HandleSettingChanged(UTATUserSettingsCollection* collection, FGameplayTag tag)
{
   OnSettingChanged.Broadcast(tag);
}

void UTATGameUserSettings::HandleSettingApplied(UTATUserSettingsCollection* collection, FGameplayTag tag)
{
   OnSettingApplied.Broadcast(tag);
}

bool UTATGameUserSettings::ApplySetting(FGameplayTag tag) const
{
   UTATUserSettingsCollection* collection = FindCollectionForSetting(tag);
   return collection && collection->ApplySetting(tag);
}

bool UTATGameUserSettings::GetSettingGeneric(FGameplayTag tag, uint8& value)
{
   checkNoEntry();
   return false;
}

DEFINE_FUNCTION(UTATGameUserSettings::execGetSettingGeneric)
{
   P_GET_STRUCT(FGameplayTag, tag);

   Stack.StepCompiledIn<FProperty>(nullptr);
   const FProperty* valueProperty = Stack.MostRecentProperty;
   void* valueAddress = Stack.MostRecentPropertyAddress;

   P_FINISH;

   if (valueProperty == nullptr || valueAddress == nullptr)
   {
      const FBlueprintExceptionInfo exceptionInfo(
         EBlueprintExceptionType::AbortExecution,
         INVTEXT("Failed to resolve value for GetSetting")
      );

      FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, exceptionInfo);
   }
   else
   {
      P_NATIVE_BEGIN;

         UTATUserSettingsCollection* collection = P_THIS->FindCollectionForSetting(tag);
         *static_cast<bool*>(RESULT_PARAM) = collection && collection->GetSettingGeneric(tag, valueProperty, valueAddress);

      P_NATIVE_END;
   }
}

bool UTATGameUserSettings::GetSettingOptionsGeneric(FGameplayTag tag, TMap<int32, int32>& options)
{
   checkNoEntry();
   return false;
}

DEFINE_FUNCTION(UTATGameUserSettings::execGetSettingOptionsGeneric)
{
   P_GET_STRUCT(FGameplayTag, tag);

   Stack.StepCompiledIn<FMapProperty>(nullptr);
   FMapProperty* optionsProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
   void* optionsAddress = Stack.MostRecentPropertyAddress;

   if (!optionsProperty)
   {
      Stack.bArrayContextFailed = true;
      return;
   }

   P_FINISH;

   P_NATIVE_BEGIN;

      UTATUserSettingsCollection* collection = P_THIS->FindCollectionForSetting(tag);
      *static_cast<bool*>(RESULT_PARAM) = collection && collection->GetSettingOptionsGeneric(tag, optionsProperty, optionsAddress);

   P_NATIVE_END;
}

bool UTATGameUserSettings::SetSettingGeneric(FGameplayTag tag, const uint8& value)
{
   checkNoEntry();
   return false;
}

DEFINE_FUNCTION(UTATGameUserSettings::execSetSettingGeneric)
{
   P_GET_STRUCT(FGameplayTag, tag);

   Stack.StepCompiledIn<FProperty>(nullptr);
   const FProperty* valueProperty = Stack.MostRecentProperty;
   const void* valueAddress = Stack.MostRecentPropertyAddress;

   P_FINISH;

   if (valueProperty == nullptr || valueAddress == nullptr)
   {
      const FBlueprintExceptionInfo exceptionInfo(
         EBlueprintExceptionType::AbortExecution,
         INVTEXT("Failed to resolve value for SetSetting")
      );

      FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, exceptionInfo);
   }
   else
   {
      P_NATIVE_BEGIN;

         UTATUserSettingsCollection* collection = P_THIS->FindCollectionForSetting(tag);
         *static_cast<bool*>(RESULT_PARAM) = collection && collection->SetSettingGeneric(tag, valueProperty, valueAddress);

      P_NATIVE_END;
   }
}
