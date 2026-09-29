// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include <Concepts/BaseStructureProvider.h>
#include <StructUtils/PropertyBag.h>

// #TODO: Support for UObject/UClass types
// #TODO: Support for container types

namespace TAT::PropertyBag
{
   FName SanitizePropertyName(FStringView name);

   void LoadConfig(FInstancedPropertyBag& propertyBag, const TCHAR* sectionName, const FString& fileName, FConfigCacheIni* config = GConfig);
   void SaveConfig(const FInstancedPropertyBag& propertyBag, const TCHAR* sectionName, const FString& fileName, FConfigCacheIni* config = GConfig);

   template<typename T>
   static bool TryGetValue(const FInstancedPropertyBag& propertyBag, const FName name, T& value);

   template<typename T>
   static bool TrySetValue(FInstancedPropertyBag& propertyBag, const FName name, const T& value);

   template<typename T>
   static T GetValueChecked(const FInstancedPropertyBag& propertyBag, const FName name);

   template<typename T>
   static void SetValueChecked(FInstancedPropertyBag& propertyBag, const FName name, const T& value);

   bool TryGetValueAsString(const FInstancedPropertyBag& propertyBag, const FName name, FString& value);
   bool TrySetValueFromString(FInstancedPropertyBag& propertyBag, const FName name, const FString& value);

   bool GetValueForProperty(const FInstancedPropertyBag& propertyBag, const FName name, const FProperty* dstProperty, void* dstContainerAddress);
   bool SetValueForProperty(FInstancedPropertyBag& propertyBag, const FName name, const FProperty* srcProperty, const void* srcContainerAddress);

   bool CopyValue(FInstancedPropertyBag& dstPropertyBag, const FInstancedPropertyBag& srcPropertyBag, FName name);

   template<typename T, typename Enable = void>
   struct TTraits
   {
      static constexpr EPropertyBagPropertyType ValueType = EPropertyBagPropertyType::None;
      static const UObject* GetValueTypeObject() = delete;

      static TValueOrError<T, EPropertyBagResult> GetValue(const FInstancedPropertyBag& propertyBag, const FName name) = delete;
      static EPropertyBagResult SetValue(FInstancedPropertyBag& propertyBag, const FName name, const T& value) = delete;
   };
}

template<typename T>
bool TAT::PropertyBag::TryGetValue(const FInstancedPropertyBag& propertyBag, const FName name, T& value)
{
   TValueOrError<T, EPropertyBagResult> result = TTraits<T>::GetValue(propertyBag, name);
   if (result.HasValue())
   {
      value = result.GetValue();
      return true;
   }

   return false;
}

template<typename T>
bool TAT::PropertyBag::TrySetValue(FInstancedPropertyBag& propertyBag, const FName name, const T& value)
{
   const EPropertyBagResult result = TTraits<T>::SetValue(propertyBag, name, value);
   return result == EPropertyBagResult::Success;
}

template<typename T>
T TAT::PropertyBag::GetValueChecked(const FInstancedPropertyBag& propertyBag, const FName name)
{
   TValueOrError<T, EPropertyBagResult> result = TTraits<T>::GetValue(propertyBag, name);
   check(result.HasValue());

   return result.GetValue();
}

template<typename T>
void TAT::PropertyBag::SetValueChecked(FInstancedPropertyBag& propertyBag, const FName name, const T& value)
{
   const EPropertyBagResult result = TTraits<T>::SetValue(propertyBag, name, value);
   check(result == EPropertyBagResult::Success);
}

#define TAT_DEFINE_PROPERTY_BAG_TYPE(T, VT, Getter, Setter) \
   template<> struct TAT::PropertyBag::TTraits<T> { \
      static constexpr EPropertyBagPropertyType ValueType = VT; \
      static const UObject* GetValueTypeObject() { return nullptr; } \
      static TValueOrError<T, EPropertyBagResult> GetValue(const FInstancedPropertyBag& propertyBag, const FName name) { return propertyBag.Getter(name); } \
      static EPropertyBagResult SetValue(FInstancedPropertyBag& propertyBag, const FName name, const T& value) { return propertyBag.Setter(name, value); } \
   }

// clang-format off
TAT_DEFINE_PROPERTY_BAG_TYPE(bool, EPropertyBagPropertyType::Bool, GetValueBool, SetValueBool);
TAT_DEFINE_PROPERTY_BAG_TYPE(uint8, EPropertyBagPropertyType::Byte, GetValueByte, SetValueByte);
TAT_DEFINE_PROPERTY_BAG_TYPE(int32, EPropertyBagPropertyType::Int32, GetValueInt32, SetValueInt32);
TAT_DEFINE_PROPERTY_BAG_TYPE(int64, EPropertyBagPropertyType::Int64, GetValueInt64, SetValueInt64);
TAT_DEFINE_PROPERTY_BAG_TYPE(uint32, EPropertyBagPropertyType::UInt32, GetValueUInt32, SetValueUInt32);
TAT_DEFINE_PROPERTY_BAG_TYPE(uint64, EPropertyBagPropertyType::UInt64, GetValueUInt64, SetValueUInt64);
TAT_DEFINE_PROPERTY_BAG_TYPE(float, EPropertyBagPropertyType::Float, GetValueFloat, SetValueFloat);
TAT_DEFINE_PROPERTY_BAG_TYPE(double, EPropertyBagPropertyType::Double, GetValueDouble, SetValueDouble);
TAT_DEFINE_PROPERTY_BAG_TYPE(FName, EPropertyBagPropertyType::Name, GetValueName, SetValueName);
TAT_DEFINE_PROPERTY_BAG_TYPE(FString, EPropertyBagPropertyType::String, GetValueString, SetValueString);
TAT_DEFINE_PROPERTY_BAG_TYPE(FText, EPropertyBagPropertyType::Text, GetValueText, SetValueText);
TAT_DEFINE_PROPERTY_BAG_TYPE(FSoftObjectPath, EPropertyBagPropertyType::SoftObject, GetValueSoftPath, SetValueSoftPath);
// clang-format on

// Traits specialization for struct types
template<typename T>
struct TAT::PropertyBag::TTraits<T, std::enable_if_t<TModels_V<CBaseStructureProvider, T>>>
{
   static constexpr EPropertyBagPropertyType ValueType = EPropertyBagPropertyType::Struct;

   static const UObject* GetValueTypeObject()
   {
      return TBaseStructure<T>::Get();
   }

   static TValueOrError<T, EPropertyBagResult> GetValue(const FInstancedPropertyBag& propertyBag, const FName name)
   {
      TValueOrError<T*, EPropertyBagResult> result = propertyBag.GetValueStruct<T>(name);
      if (result.HasValue())
      {
         return MakeValue(*result.GetValue());
      }

      return MakeError(result.GetError());
   }

   static EPropertyBagResult SetValue(FInstancedPropertyBag& propertyBag, const FName name, const T& value)
   {
      return propertyBag.SetValueStruct<T>(name, value);
   }
};

// Traits specialization for enum types
template<typename T>
struct TAT::PropertyBag::TTraits<T, std::enable_if_t<std::is_enum_v<T>>>
{
   static constexpr EPropertyBagPropertyType ValueType = EPropertyBagPropertyType::Enum;

   static const UObject* GetValueTypeObject()
   {
      return StaticEnum<T>();
   }

   static TValueOrError<T, EPropertyBagResult> GetValue(const FInstancedPropertyBag& propertyBag, const FName name)
   {
      return propertyBag.GetValueEnum<T>(name);
   }

   static EPropertyBagResult SetValue(FInstancedPropertyBag& propertyBag, const FName name, const T& value)
   {
      return propertyBag.SetValueEnum<T>(name, value);
   }
};
