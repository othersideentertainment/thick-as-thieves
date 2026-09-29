// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "Common/TATPropertyBagUtils.h"
#include "Settings/TATUserSettingsTags.h"

// UE
#include <Delegates/Delegate.h>
#include <StructUtils/PropertyBag.h>
#include <UObject/Object.h>

#include "TATUserSettingsCollection.generated.h"

class UTATUserSettingsCollection;

DECLARE_LOG_CATEGORY_EXTERN(LogTATUserSettingsCollection, Log, All);

DECLARE_MULTICAST_DELEGATE_TwoParams(FTATUserSettingDelegate, UTATUserSettingsCollection* collection, FGameplayTag tag);

template<typename T>
using TTATUserSettingValueDelegate = TDelegate<void(T& value)>;

template<typename T>
using TTATUserSettingOptionsDelegate = TDelegate<void(TMap<T, FText>& options)>;

template<typename T>
struct TTATUserSettingParams
{
   FText DisplayName;

   // Callback to get the setting's discrete options
   TTATUserSettingOptionsDelegate<T> GetOptions;

   // Callback to execute when the setting is changed
   TTATUserSettingValueDelegate<T> OnSetValue;

   // Callback to execute when the setting is applied
   TTATUserSettingValueDelegate<T> OnApplyValue;
};

USTRUCT()
struct FTATUserSetting
{
   GENERATED_BODY()

   UPROPERTY()
   FName PropertyName;

   UPROPERTY()
   EPropertyBagPropertyType PropertyValueType = EPropertyBagPropertyType::None;

   UPROPERTY()
   TObjectPtr<const UObject> PropertyValueTypeObject;

   UPROPERTY()
   bool bIsDirty = false;

   UPROPERTY()
   FText DisplayName;

   TFunction<void(void* OptionsPtr)> GetOptions;

   TFunction<void(FInstancedPropertyBag& propertyBag, FName name)> OnSetValue;
   TFunction<void(FInstancedPropertyBag& propertyBag, FName name)> OnApplyValue;

   template<typename T>
   bool CompatibleType() const;
   bool CompatibleType(const FProperty* property) const;

   bool ExecuteOnSetValue(FInstancedPropertyBag& propertyBag) const;
   bool ExecuteOnApplyValue(FInstancedPropertyBag& propertyBag) const;
};

UCLASS(MinimalAPI, Abstract)
class UTATUserSettingsCollection : public UObject
{
   GENERATED_BODY()

public:
   FGameplayTag GetTag() const;

   virtual void LoadSettings();
   virtual void SaveSettings();
   virtual void ApplySettings();
   virtual void ResetSettings();
   virtual bool IsDirty() const;

   template<typename T>
   bool GetSetting(FGameplayTag tag, T& value) const;

   template<typename T>
   bool GetSettingOptions(FGameplayTag tag, TMap<T, FText>& options) const;

   template<typename T>
   bool SetSetting(FGameplayTag tag, const T& value);

   bool ApplySetting(FGameplayTag tag);

   bool GetSettingGeneric(FGameplayTag tag, const FProperty* property, void* address);
   bool GetSettingOptionsGeneric(FGameplayTag tag, const FMapProperty* property, void* address);
   bool SetSettingGeneric(FGameplayTag tag, const FProperty* property, const void* address);

   bool GetSettingAsString(FGameplayTag tag, FString& value) const;
   bool SetSettingFromString(FGameplayTag tag, const FString& value);

   void ForEachSetting(TFunctionRef<bool(FGameplayTag tag, const FTATUserSetting& setting)> functor) const;

   static FTATUserSettingDelegate OnSettingChanged;
   static FTATUserSettingDelegate OnSettingApplied;

protected:
   template<typename T>
   void CreateSetting(FGameplayTag tag, TTATUserSettingParams<T>&& params);

   FName GetSettingsStringTableId() const;

   UPROPERTY()
   FGameplayTag CollectionTag;

   UPROPERTY()
   TMap<FGameplayTag, FTATUserSetting> Settings;

   UPROPERTY()
   FInstancedPropertyBag AppliedSettings;

   UPROPERTY()
   FInstancedPropertyBag UnappliedSettings;
};

template<typename T>
void UTATUserSettingsCollection::CreateSetting(FGameplayTag tag, TTATUserSettingParams<T>&& params)
{
   if (!ensureMsgf(CollectionTag.IsValid(), TEXT("Settings collection '%s' has an invalid tag"), *GetName()))
   {
      return;
   }

   if (!ensureMsgf(tag.MatchesTag(CollectionTag), TEXT("Setting '%s' has an invalid tag for collection (%s)"), *tag.ToString(), *CollectionTag.ToString()))
   {
      return;
   }

   FTATUserSetting& setting = Settings.Emplace(tag);

   // Make a short identifier from the setting tag excluding the collection tag prefix
   const FString shortSettingName = tag.ToString().RightChop(CollectionTag.GetTagName().GetStringLength() + 1);

   setting.PropertyName = TAT::PropertyBag::SanitizePropertyName(shortSettingName);
   setting.PropertyValueType = TAT::PropertyBag::TTraits<T>::ValueType;
   setting.PropertyValueTypeObject = TAT::PropertyBag::TTraits<T>::GetValueTypeObject();

   setting.DisplayName = params.DisplayName;

   if (params.GetOptions.IsBound())
   {
      setting.GetOptions = [Callback = MoveTemp(params.GetOptions)](void* optionsPtr)
      {
         TMap<T, FText>& options = *static_cast<TMap<T, FText>*>(optionsPtr);
         Callback.Execute(options);
      };
   }

   if (params.OnSetValue.IsBound())
   {
      setting.OnSetValue = [callback = MoveTemp(params.OnSetValue)](FInstancedPropertyBag& propertyBag, FName name)
      {
         T value = TAT::PropertyBag::GetValueChecked<T>(propertyBag, name);

         // Ensure the callback is still valid, otherwise we're modifying a setting without effect
         verifyf(callback.ExecuteIfBound(value), TEXT("Setting callback for '%s' is no longer valid"), *name.ToString());

         TAT::PropertyBag::SetValueChecked<T>(propertyBag, name, value);
      };
   }

   if (params.OnApplyValue.IsBound())
   {
      setting.OnApplyValue = [callback = MoveTemp(params.OnApplyValue)](FInstancedPropertyBag& propertyBag, FName name)
      {
         T value = TAT::PropertyBag::GetValueChecked<T>(propertyBag, name);

         // Ensure the callback is still valid, otherwise we're modifying a setting without effect
         verifyf(callback.ExecuteIfBound(value), TEXT("Setting callback for '%s' is no longer valid"), *name.ToString());

         TAT::PropertyBag::SetValueChecked<T>(propertyBag, name, value);
      };
   }
}

template<typename T>
bool FTATUserSetting::CompatibleType() const
{
   const FPropertyBagPropertyDesc settingDesc(PropertyName, PropertyValueType, PropertyValueTypeObject);
   const FPropertyBagPropertyDesc otherDesc(PropertyName, TAT::PropertyBag::TTraits<T>::ValueType, TAT::PropertyBag::TTraits<T>::GetValueTypeObject());

   return settingDesc.CompatibleType(otherDesc);
}

template<typename T>
bool UTATUserSettingsCollection::GetSetting(FGameplayTag tag, T& value) const
{
   const FTATUserSetting* setting = Settings.Find(tag);
   if (!setting || !setting->CompatibleType<T>())
   {
      return false;
   }

   return TAT::PropertyBag::TryGetValue(setting->bIsDirty ? UnappliedSettings : AppliedSettings, setting->PropertyName, value);
}

template<typename T>
bool UTATUserSettingsCollection::GetSettingOptions(FGameplayTag tag, TMap<T, FText>& options) const
{
   const FTATUserSetting* setting = Settings.Find(tag);
   if (!setting || !setting->CompatibleType<T>())
   {
      return false;
   }

   if (!setting->GetOptions)
   {
      return false;
   }

   setting->GetOptions(&options);

   return true;
}

template<typename T>
bool UTATUserSettingsCollection::SetSetting(FGameplayTag tag, const T& value)
{
   FTATUserSetting* setting = Settings.Find(tag);
   if (!setting || !setting->CompatibleType<T>())
   {
      return false;
   }

   if (!TAT::PropertyBag::TrySetValue(UnappliedSettings, setting->PropertyName, value))
   {
      return false;
   }

   setting->bIsDirty = true;
   setting->ExecuteOnSetValue(UnappliedSettings);

   OnSettingChanged.Broadcast(this, tag);

   return true;
}
