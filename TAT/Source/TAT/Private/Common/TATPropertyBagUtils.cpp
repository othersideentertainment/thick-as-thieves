// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Common/TATPropertyBagUtils.h"

FName TAT::PropertyBag::SanitizePropertyName(FStringView name)
{
   FString sanitizedName(name);

   // Invalid character list matches UE::StructUtils::Private::Constants::InvalidNameCharacters in UE 5.6
   static constexpr TCHAR kInvalidNameCharacters[] = TEXT(" \"',/.:|&!?~\\\n\r\t@#(){}[]<>=;^%$`+*");
   for (const TCHAR invalidChar : kInvalidNameCharacters)
   {
      sanitizedName.ReplaceCharInline(invalidChar, TEXT('_'));
   }

   return FName(sanitizedName);
}

void TAT::PropertyBag::LoadConfig(FInstancedPropertyBag& propertyBag, const TCHAR* sectionName, const FString& fileName, FConfigCacheIni* config)
{
   const FStructView view = propertyBag.GetMutableValue();
   if (!view.IsValid())
   {
      return;
   }

   const UScriptStruct* structType = view.GetScriptStruct();
   uint8* structMemory = view.GetMemory();

   for (const FProperty* property = structType->PropertyLink; property; property = property->PropertyLinkNext)
   {
      const FString key = property->GetName();

      checkf(!property->IsA<FArrayProperty>() && !property->IsA<FSetProperty>(), TEXT("Container properties are not supported (%s)"), *key);
      checkf(property->ArrayDim == 1, TEXT("Array properties are not supported (%s)"), *key);

      uint8* valueAddress = structMemory + property->GetOffset_ForInternal();

      FString value;
      if (config->GetValue(sectionName, *key, value, fileName))
      {
         property->ImportText_Direct(*value, valueAddress, nullptr, PPF_SerializedAsImportText);
      }
   }
}

void TAT::PropertyBag::SaveConfig(const FInstancedPropertyBag& propertyBag, const TCHAR* sectionName, const FString& fileName, FConfigCacheIni* config)
{
   const FConstStructView view = propertyBag.GetValue();
   if (!view.IsValid())
   {
      return;
   }

   const UScriptStruct* structType = view.GetScriptStruct();
   const uint8* structMemory = view.GetMemory();

   for (const FProperty* property = structType->PropertyLink; property; property = property->PropertyLinkNext)
   {
      const FString key = property->GetName();

      checkf(!property->IsA<FArrayProperty>() && !property->IsA<FSetProperty>(), TEXT("Container properties are not supported (%s)"), *key);
      checkf(property->ArrayDim == 1, TEXT("Array properties are not supported (%s)"), *key);

      const uint8* valueAddress = structMemory + property->GetOffset_ForInternal();

      FString value;
      if (property->ExportText_Direct(value, valueAddress, valueAddress, nullptr, PPF_SerializedAsImportText))
      {
         config->SetString(sectionName, *key, *value, fileName);
      }
   }

   if (config == GConfig)
   {
      config->Flush(false, fileName);
   }
}

bool TAT::PropertyBag::TryGetValueAsString(const FInstancedPropertyBag& propertyBag, const FName name, FString& value)
{
   TValueOrError<FString, EPropertyBagResult> Result = propertyBag.GetValueSerializedString(name);
   if (Result.HasValue())
   {
      value = Result.GetValue();
      return true;
   }

   return false;
}

bool TAT::PropertyBag::TrySetValueFromString(FInstancedPropertyBag& propertyBag, const FName name, const FString& value)
{
   const EPropertyBagResult result = propertyBag.SetValueSerializedString(name, value);
   return result == EPropertyBagResult::Success;
}

bool TAT::PropertyBag::GetValueForProperty(const FInstancedPropertyBag& propertyBag, const FName name, const FProperty* dstProperty, void* dstAddress)
{
   const FConstStructView view = propertyBag.GetValue();
   if (!view.IsValid())
   {
      return false;
   }

   const FPropertyBagPropertyDesc* srcPropertyDesc = propertyBag.FindPropertyDescByName(name);
   if (!srcPropertyDesc)
   {
      return false;
   }

   const FPropertyBagPropertyDesc dstPropertyDesc(name, dstProperty);
   if (!dstPropertyDesc.CompatibleType(*srcPropertyDesc))
   {
      return false;
   }

   const uint8* srcAddress = view.GetMemory() + srcPropertyDesc->CachedProperty->GetOffset_ForInternal();
   dstProperty->CopyCompleteValue(dstAddress, srcAddress);

   return true;
}

bool TAT::PropertyBag::SetValueForProperty(FInstancedPropertyBag& propertyBag, const FName name, const FProperty* srcProperty, const void* srcAddress)
{
   const FStructView view = propertyBag.GetMutableValue();
   if (!view.IsValid())
   {
      return false;
   }

   const FPropertyBagPropertyDesc* dstPropertyDesc = propertyBag.FindPropertyDescByName(name);
   if (!dstPropertyDesc)
   {
      return false;
   }

   const FPropertyBagPropertyDesc srcPropertyDesc(name, srcProperty);
   if (!dstPropertyDesc->CompatibleType(srcPropertyDesc))
   {
      return false;
   }

   uint8* dstAddress = view.GetMemory() + dstPropertyDesc->CachedProperty->GetOffset_ForInternal();
   srcProperty->CopyCompleteValue(dstAddress, srcAddress);

   return true;
}

bool TAT::PropertyBag::CopyValue(FInstancedPropertyBag& dstPropertyBag, const FInstancedPropertyBag& srcPropertyBag, FName name)
{
   const FStructView dstView = dstPropertyBag.GetMutableValue();
   const FConstStructView srcView = srcPropertyBag.GetValue();

   if (!dstView.IsValid() || !srcView.IsValid())
   {
      return false;
   }

   const FPropertyBagPropertyDesc* dstPropertyDesc = dstPropertyBag.FindPropertyDescByName(name);
   const FPropertyBagPropertyDesc* srcPropertyDesc = srcPropertyBag.FindPropertyDescByName(name);

   if (!dstPropertyDesc || !srcPropertyDesc)
   {
      return false;
   }

   if (!dstPropertyDesc->CompatibleType(*srcPropertyDesc))
   {
      return false;
   }

   uint8* dstAddress = dstView.GetMemory() + dstPropertyDesc->CachedProperty->GetOffset_ForInternal();
   const uint8* srcAddress = srcView.GetMemory() + srcPropertyDesc->CachedProperty->GetOffset_ForInternal();

   dstPropertyDesc->CachedProperty->CopyCompleteValue(dstAddress, srcAddress);

   return true;
}
