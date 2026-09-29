// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATUserSettingsCollection.h"

// TAT
#include "Developer/TATProjectSettings.h"

// UE
#include <Internationalization/StringTable.h>
#include <Logging/StructuredLog.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUserSettingsCollection)

DEFINE_LOG_CATEGORY(LogTATUserSettingsCollection);

bool FTATUserSetting::CompatibleType(const FProperty* property) const
{
   const FPropertyBagPropertyDesc settingDesc(PropertyName, PropertyValueType, PropertyValueTypeObject);
   const FPropertyBagPropertyDesc otherDesc(PropertyName, property);

   return settingDesc.CompatibleType(otherDesc);
}

bool FTATUserSetting::ExecuteOnSetValue(FInstancedPropertyBag& propertyBag) const
{
   if (UE_LOG_ACTIVE(LogTATUserSettingsCollection, Verbose))
   {
      FString propertyValue;
      TAT::PropertyBag::TryGetValueAsString(propertyBag, PropertyName, propertyValue);

      UE_LOGFMT(LogTATUserSettingsCollection, Verbose, "SetValue({PropertyName}) = {PropertyValue}", PropertyName, propertyValue);
   }

   if (!OnSetValue)
   {
      return false;
   }

   OnSetValue(propertyBag, PropertyName);

   return true;
}

bool FTATUserSetting::ExecuteOnApplyValue(FInstancedPropertyBag& propertyBag) const
{
   if (UE_LOG_ACTIVE(LogTATUserSettingsCollection, Verbose))
   {
      FString propertyValue;
      TAT::PropertyBag::TryGetValueAsString(propertyBag, PropertyName, propertyValue);

      UE_LOGFMT(LogTATUserSettingsCollection, Verbose, "ApplyValue({PropertyName}) = {PropertyValue}", PropertyName, propertyValue);
   }

   if (!OnApplyValue)
   {
      return false;
   }

   OnApplyValue(propertyBag, PropertyName);

   return true;
}

FTATUserSettingDelegate UTATUserSettingsCollection::OnSettingChanged;
FTATUserSettingDelegate UTATUserSettingsCollection::OnSettingApplied;

FGameplayTag UTATUserSettingsCollection::GetTag() const
{
   return CollectionTag;
}

void UTATUserSettingsCollection::LoadSettings()
{
   if (!ensureMsgf(CollectionTag.IsValid(), TEXT("Settings collection '%s' has an invalid tag"), *GetName()))
   {
      return;
   }

   TArray<FPropertyBagPropertyDesc> settingDescs;
   settingDescs.Reserve(Settings.Num());

   for (const TTuple<FGameplayTag, FTATUserSetting>& settingPair : Settings)
   {
      const FTATUserSetting& setting = settingPair.Value;
      settingDescs.Emplace(setting.PropertyName, setting.PropertyValueType, setting.PropertyValueTypeObject);
   }

   AppliedSettings.Reset();
   AppliedSettings.AddProperties(settingDescs);

   UnappliedSettings.Reset();
   UnappliedSettings.AddProperties(settingDescs);

   // Load config into unapplied settings and invoke set callbacks while updating dirty flag; this helps maintain
   // a two-stage set -> apply flow allowing different settings to react to either stage as they find appropriate
   TAT::PropertyBag::LoadConfig(UnappliedSettings, *CollectionTag.ToString(), GGameUserSettingsIni);

   for (TPair<FGameplayTag, FTATUserSetting>& settingPair : Settings)
   {
      settingPair.Value.bIsDirty = true;
      settingPair.Value.ExecuteOnSetValue(UnappliedSettings);
   }
}

void UTATUserSettingsCollection::SaveSettings()
{
   if (!ensureMsgf(CollectionTag.IsValid(), TEXT("Settings collection '%s' has an invalid tag"), *GetName()))
   {
      return;
   }

   TAT::PropertyBag::SaveConfig(AppliedSettings, *CollectionTag.ToString(), GGameUserSettingsIni);
}

void UTATUserSettingsCollection::ApplySettings()
{
   AppliedSettings = UnappliedSettings;

   for (TPair<FGameplayTag, FTATUserSetting>& settingPair : Settings)
   {
      if (settingPair.Value.bIsDirty)
      {
         settingPair.Value.bIsDirty = false;
         settingPair.Value.ExecuteOnApplyValue(AppliedSettings);

         OnSettingApplied.Broadcast(this, settingPair.Key);
      }
   }
}

void UTATUserSettingsCollection::ResetSettings()
{
   UnappliedSettings = AppliedSettings;

   for (TPair<FGameplayTag, FTATUserSetting>& settingPair : Settings)
   {
      if (settingPair.Value.bIsDirty)
      {
         settingPair.Value.bIsDirty = false;
         settingPair.Value.ExecuteOnSetValue(UnappliedSettings);

         OnSettingChanged.Broadcast(this, settingPair.Key);
      }
   }
}

bool UTATUserSettingsCollection::IsDirty() const
{
   for (const TPair<FGameplayTag, FTATUserSetting>& settingPair : Settings)
   {
      if (settingPair.Value.bIsDirty)
      {
         return true;
      }
   }

   return false;
}

bool UTATUserSettingsCollection::ApplySetting(FGameplayTag tag)
{
   FTATUserSetting* setting = Settings.Find(tag);
   if (!setting)
   {
      return false;
   }

   if (!TAT::PropertyBag::CopyValue(AppliedSettings, UnappliedSettings, setting->PropertyName))
   {
      return false;
   }

   setting->bIsDirty = false;
   setting->ExecuteOnApplyValue(AppliedSettings);

   OnSettingApplied.Broadcast(this, tag);

   return true;
}

bool UTATUserSettingsCollection::GetSettingGeneric(FGameplayTag tag, const FProperty* property, void* address)
{
   const FTATUserSetting* setting = Settings.Find(tag);
   if (!setting || !setting->CompatibleType(property))
   {
      return false;
   }

   return TAT::PropertyBag::GetValueForProperty(setting->bIsDirty ? UnappliedSettings : AppliedSettings, setting->PropertyName, property, address);
}

bool UTATUserSettingsCollection::GetSettingOptionsGeneric(FGameplayTag tag, const FMapProperty* property, void* address)
{
   if (!property || !address)
   {
      return false;
   }

   if (!property->ValueProp->IsA<FTextProperty>())
   {
      return false;
   }

   const FTATUserSetting* setting = Settings.Find(tag);
   if (!setting || !setting->CompatibleType(property->KeyProp))
   {
      return false;
   }

   if (!setting->GetOptions)
   {
      return false;
   }

   setting->GetOptions(address);

   return true;
}

bool UTATUserSettingsCollection::SetSettingGeneric(FGameplayTag tag, const FProperty* property, const void* address)
{
   FTATUserSetting* setting = Settings.Find(tag);
   if (!setting || !setting->CompatibleType(property))
   {
      return false;
   }

   if (!TAT::PropertyBag::SetValueForProperty(UnappliedSettings, setting->PropertyName, property, address))
   {
      return false;
   }

   setting->bIsDirty = true;
   setting->ExecuteOnSetValue(UnappliedSettings);

   OnSettingChanged.Broadcast(this, tag);

   return true;
}

bool UTATUserSettingsCollection::GetSettingAsString(FGameplayTag tag, FString& value) const
{
   const FTATUserSetting* setting = Settings.Find(tag);
   if (!setting)
   {
      return false;
   }

   return TAT::PropertyBag::TryGetValueAsString(setting->bIsDirty ? UnappliedSettings : AppliedSettings, setting->PropertyName, value);
}

bool UTATUserSettingsCollection::SetSettingFromString(FGameplayTag tag, const FString& value)
{
   FTATUserSetting* setting = Settings.Find(tag);
   if (!setting)
   {
      return false;
   }

   if (!TAT::PropertyBag::TrySetValueFromString(UnappliedSettings, setting->PropertyName, value))
   {
      return false;
   }

   setting->bIsDirty = true;
   setting->ExecuteOnSetValue(UnappliedSettings);

   OnSettingChanged.Broadcast(this, tag);

   return true;
}

void UTATUserSettingsCollection::ForEachSetting(TFunctionRef<bool(FGameplayTag tag, const FTATUserSetting& setting)> functor) const
{
   for (const TPair<FGameplayTag, FTATUserSetting>& settingPair : Settings)
   {
      if (!functor(settingPair.Key, settingPair.Value))
      {
         break;
      }
   }
}

FName UTATUserSettingsCollection::GetSettingsStringTableId() const
{
   const UTATProjectSettings& settings = *GetDefault<UTATProjectSettings>();
   if (UStringTable* stringTable = settings.SettingsStringTable.Get())
   {
      return stringTable->GetStringTableId();
   }
   return NAME_None;
}
