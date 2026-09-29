// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATMatchSettingsBase.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/DemoHubMissionMgr.h"
#include "TATGameInstance.h"

// ue
#include "Blueprint/BlueprintExceptionInfo.h"
#include "EngineUtils.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchSettingsBase)

DEFINE_LOG_CATEGORY_STATIC(LogTATMatchSettingsBase, Log, All);

// For non-shipping builds, when serializing match settings, include extra validation data to ensure correctness
static constexpr bool kMatchSettingsDebugValidationMode = !static_cast<bool>(UE_BUILD_SHIPPING);

// Can set this to 1 to turn on some match settings serialization/deserialization debug messages
#define TAT_DEBUG_MATCH_SETTINGS_SERIALIZATION 0

#if TAT_DEBUG_MATCH_SETTINGS_SERIALIZATION
/// String builder helper for debugging match settings serialize/deserialize methods
struct FTATDebugPropList
{
   TStringBuilder<127> Props;
   int32 NumProps = 0;
   const TCHAR* operator*() const { return *Props; }
   void Add(const FString& propName, const FString& propValue)
   {
      if (NumProps++ > 0) { Props << TEXT(", "); }
      Props << TEXT("'") << propName << TEXT("' = ") << propValue;
   }
};
#define TAT_ADD_PROP_TO_DEBUG_LIST(PROP_LIST, PROP_NAME, PROP_VALUE) (PROP_LIST).Add((PROP_NAME), (PROP_VALUE))
#else
#define TAT_ADD_PROP_TO_DEBUG_LIST(PROP_LIST, PROP_NAME, PROP_VALUE)
#endif

//NB. Several parts of this file require an entry per supported property type.
// If you are adding a new supported type, you can Ctrl-F for "TAT_ForEachPropertyType" to locate them all.

namespace MatchSettingsHelpers
{
   // Suffix used to indicate that a property stores metadata for another property
   static const TCHAR* kMetadataSuffix = TEXT("_Metadata");

   /// Type trait to map C++ types to FProperty subclasses.
   /// Each trait should define the following items:
   /// using PropType: The FProperty subclass used by the type.
   /// using AllowImplicitConvFrom: C++ type that can be converted to/from. Eg. to accept float for a double property or vice versa.
   /// static constexpr bool AllowImplicitEnumConv: Should this type be implicitly convertable to/from enum types? Only supported on integral properties.
   /// static constexpr ETATMatchSettingsPropertyType UIType: The enum value that will be passed to the UI to help decide which UMG widget to use for editing.
   template<typename T>
   struct TCppTypeToProperty {};

   // TAT_ForEachPropertyType (There should be exactly one template specialization per supported C++ type)
   template<>
   struct TCppTypeToProperty<bool>
   {
      using PropType = FBoolProperty;
      using AllowImplicitConvFrom = void;
      static constexpr bool AllowImplicitEnumConv = false;
      static constexpr ETATMatchSettingsPropertyType UIType = ETATMatchSettingsPropertyType::Bool;
   };
   template<>
   struct TCppTypeToProperty<int32>
   {
      using PropType = FIntProperty;
      using AllowImplicitConvFrom = void;
      static constexpr bool AllowImplicitEnumConv = false;
      static constexpr ETATMatchSettingsPropertyType UIType = ETATMatchSettingsPropertyType::Integer;
   };
   template<>
   struct TCppTypeToProperty<float>
   {
      using PropType = FFloatProperty;
      using AllowImplicitConvFrom = double;
      static constexpr bool AllowImplicitEnumConv = false;
      static constexpr ETATMatchSettingsPropertyType UIType = ETATMatchSettingsPropertyType::Float;
   };
   template<>
   struct TCppTypeToProperty<double>
   {
      using PropType = FDoubleProperty;
      using AllowImplicitConvFrom = float;
      static constexpr bool AllowImplicitEnumConv = false;
      static constexpr ETATMatchSettingsPropertyType UIType = ETATMatchSettingsPropertyType::Float;
   };
   template<>
   struct TCppTypeToProperty<uint8>
   {
      // Unfortunately, UE does not expose UEnum* to blueprints like it does with UStruct* and UClass*,
      // so we'll just use byte values in blueprints and allow implicit conversion to typed enum properties.
      using PropType = FByteProperty;
      using AllowImplicitConvFrom = void;
      static constexpr bool AllowImplicitEnumConv = true;
      static constexpr ETATMatchSettingsPropertyType UIType = ETATMatchSettingsPropertyType::Enum;
   };
   template<>
   struct TCppTypeToProperty<FGameplayTag>
   {
      using PropType = FStructProperty;
      using AllowImplicitConvFrom = void;
      static constexpr bool AllowImplicitEnumConv = false;
      static constexpr ETATMatchSettingsPropertyType UIType = ETATMatchSettingsPropertyType::GameplayTag;
   };

   template<typename T>
   bool PropertyMatchesType(const FProperty* prop)
   {
      using Traits = TCppTypeToProperty<T>;

      // first check that the property type matches
      if (prop == nullptr || !prop->IsA<typename Traits::PropType>())
      {
         return false;
      }

      // for struct properties, make sure the struct type matches
      if constexpr (std::is_same_v<typename Traits::PropType, FStructProperty>)
      {
         return CastField<FStructProperty>(prop)->Struct == T::StaticStruct();
      }
      else
      {
         return true;
      }
   }

   /// Finds the match settings type enum value for a given FProperty, or Invalid if the FProperty is not supported.
   FORCEINLINE ETATMatchSettingsPropertyType GetMatchSettingsType(const FProperty* prop)
   {
      check(prop != nullptr);

      // NB. This could be converted into a lambda once we're using C++20 (lambdas in C++17 and earlier can't be templates)
#define TAT_TRY_GET_PROPERTY_TYPE(CPP_TYPE) do { \
         if (PropertyMatchesType<CPP_TYPE>(prop)) { return TCppTypeToProperty<CPP_TYPE>::UIType; } \
      } while(0)

      // TAT_ForEachPropertyType
      TAT_TRY_GET_PROPERTY_TYPE(bool);
      TAT_TRY_GET_PROPERTY_TYPE(int32);
      TAT_TRY_GET_PROPERTY_TYPE(float);
      TAT_TRY_GET_PROPERTY_TYPE(double);
      TAT_TRY_GET_PROPERTY_TYPE(uint8);
      TAT_TRY_GET_PROPERTY_TYPE(FGameplayTag);

#undef TAT_TRY_GET_PROPERTY_TYPE

      if (prop->IsA<FEnumProperty>())
      {
         return ETATMatchSettingsPropertyType::Enum;
      }
      return ETATMatchSettingsPropertyType::Invalid;
   }

   /// Calls the callback for each FProperty* in the settings object that is not marked transient and where the property name does not end with the metadata suffix
   /// Usage: ForEachMatchSettingsProperty(this, [](FProperty* prop, const FString& propName) {})
   template<typename Lambda>
   void ForEachMatchSettingsProperty(const UObject* settings, Lambda&& callback)
   {
      check(settings != nullptr);
      FString propName;
      for (TPropertyValueIterator<FProperty> propIter(settings->GetClass(), settings, EPropertyValueIteratorFlags::NoRecursion); propIter; ++propIter)
      {
         const FProperty* prop = propIter->Key;
         check(prop != nullptr);
         if ((prop->PropertyFlags & CPF_Transient) != 0)
         {
            continue;
         }
         prop->GetName(propName);
         if (propName.EndsWith(kMetadataSuffix))
         {
            // The check above this should have already skipped over all the metadata properties because they should all be marked as transient
            UE_LOG(LogTATMatchSettingsBase, Error, TEXT("Match settings property '%s' looks like a metadata value but is not marked as transient!"), *propName);
            continue;
         }
         callback(prop, propName);
      }
   }

   /// Safely extract the value of an FProperty with type checks.
   /// NB. The IsImplicitConversion template param is just to avoid an accidental recursive loop
   template<typename T, bool IsImplicitConversion = false>
   FORCEINLINE bool TryGetPropertyValue(FProperty* baseProperty, const void* valuePtr, T& outValue)
   {
      using Traits = TCppTypeToProperty<T>;
      check(valuePtr != nullptr);
      auto* prop = CastField<typename Traits::PropType>(baseProperty);
      if (prop != nullptr && prop->ArrayDim == 1)
      {
         if constexpr (std::is_same_v<typename Traits::PropType, FStructProperty>)
         {
            if (prop->Struct == T::StaticStruct())
            {
               outValue = *static_cast<const T*>(valuePtr);
               return true;
            }
            else
            {
               // wrong struct type for this property
               return false;
            }
         }
         else
         {
            outValue = prop->GetPropertyValue(valuePtr);
            return true;
         }
      }

      static_assert(std::is_integral_v<T> || (!std::is_integral_v<T> && !Traits::AllowImplicitEnumConv),
         "Only integral types can have AllowImplicitEnumConv set to true");

      // Allow implicit conversions to enum types if specified
      if constexpr (Traits::AllowImplicitEnumConv && std::is_integral_v<T>)
      {
         if (FEnumProperty* enumProperty = CastField<FEnumProperty>(baseProperty))
         {
            if (FNumericProperty* numericProp = enumProperty->GetUnderlyingProperty())
            {
               check(numericProp->GetElementSize() == sizeof(T));
               if constexpr (std::is_signed_v<T>)
               {
                  outValue = static_cast<T>(numericProp->GetSignedIntPropertyValue(valuePtr));
               }
               else
               {
                  outValue = static_cast<T>(numericProp->GetUnsignedIntPropertyValue(valuePtr));
               }
               return true;
            }
         }
      }

      // Allow implicit conversions if specified
      using ConvertedT = typename Traits::AllowImplicitConvFrom;
      if constexpr (!IsImplicitConversion && !std::is_void_v<ConvertedT>)
      {
         ConvertedT convertedOutValue{};
         const bool success = TryGetPropertyValue<ConvertedT, true>(baseProperty, valuePtr, convertedOutValue);
         if (success)
         {
            outValue = static_cast<T>(convertedOutValue);
         }
         return success;
      }
      else
      {
         return false;
      }
   }

   /// Safely set the value of an FProperty with type checks.
   /// NB. The IsImplicitConversion template param is just to avoid an accidental recursive loop
   template<typename T, bool IsImplicitConversion = false>
   FORCEINLINE bool TrySetPropertyValue(FProperty* baseProperty, void* outValuePtr, const T& newValue)
   {
      using Traits = TCppTypeToProperty<T>;
      check(outValuePtr != nullptr);
      auto* prop = CastField<typename Traits::PropType>(baseProperty);
      if (prop != nullptr && prop->ArrayDim == 1)
      {
         if constexpr (std::is_same_v<typename Traits::PropType, FStructProperty>)
         {
            if (prop->Struct == T::StaticStruct())
            {
               *static_cast<T*>(outValuePtr) = newValue;
               return true;
            }
            else
            {
               // wrong struct type for this property
               return false;
            }
         }
         else
         {
            prop->SetPropertyValue(outValuePtr, newValue);
            return true;
         }
      }

      // Allow implicit conversions to enum types if specified
      if constexpr (Traits::AllowImplicitEnumConv && std::is_integral_v<T>)
      {
         if (FEnumProperty* enumProperty = CastField<FEnumProperty>(baseProperty))
         {
            if (FNumericProperty* numericProp = enumProperty->GetUnderlyingProperty())
            {
               check(numericProp->GetElementSize() == sizeof(T));
               if constexpr (std::is_signed_v<T>)
               {
                  numericProp->SetIntPropertyValue(outValuePtr, static_cast<int64>(newValue));
               }
               else
               {
                  numericProp->SetIntPropertyValue(outValuePtr, static_cast<uint64>(newValue));
               }
               return true;
            }
         }
      }

      // Allow implicit conversions if specified
      using ConvertedT = typename Traits::AllowImplicitConvFrom;
      if constexpr (!IsImplicitConversion && !std::is_void_v<ConvertedT>)
      {
         return TrySetPropertyValue<ConvertedT, true>(baseProperty, outValuePtr, static_cast<ConvertedT>(newValue));
      }
      else
      {
         return false;
      }
   }

   /// Templated match settings getter, used in the blueprint getter's custom thunk.
   /// Theoretically this could just be a member function, but that would require moving all templates into the header file and it's just not needed at this time.
   template<typename T>
   FORCEINLINE bool TryGetMatchSettingsValue(const UObject* settings, FName propName, T& outValue)
   {
      check(settings != nullptr);
      UClass* cls = settings->GetClass();
      check(cls != nullptr);
      if (FProperty* prop = cls->FindPropertyByName(propName))
      {
         return TryGetPropertyValue<T>(prop, prop->ContainerPtrToValuePtr<uint8>(settings), outValue);
      }
      return false;
   }

   /// Templated match settings setter, used in the blueprint setter's custom thunk.
   /// Theoretically this could just be a member function, but that would require moving all templates into the header file and it's just not needed at this time.
   template<typename T>
   FORCEINLINE bool TrySetMatchSettingsValue(UObject* settings, FName propName, const T& newValue)
   {
      check(settings != nullptr);
      UClass* cls = settings->GetClass();
      check(cls != nullptr);
      if (FProperty* prop = cls->FindPropertyByName(propName))
      {
         return TrySetPropertyValue<T>(prop, prop->ContainerPtrToValuePtr<uint8>(settings), newValue);
      }
      return false;
   }

   // Helper for error handling in custom thunk functions
   void ThrowUnhandledParameterTypeBlueprintExceptionAndLogError(FName propName, FProperty* paramProperty, const UObject* activeObject, FFrame& stackFrame)
   {
      FString propType = TEXT("UNKNOWN");
      if (activeObject != nullptr && activeObject->GetClass() != nullptr)
      {
         if (FProperty* prop = activeObject->GetClass()->FindPropertyByName(propName))
         {
            propType = prop->GetCPPType();
         }
      }

      const FString paramType = (paramProperty != nullptr) ? paramProperty->GetCPPType() : TEXT("UNKNOWN");
      const FString errorMsg = FString::Printf(TEXT("Match settings property '%s' has type '%s', but got parameter of type '%s'"),
         *propName.ToString(), *propType, *paramType);

      UE_LOG(LogTATMatchSettingsBase, Error, TEXT("%s"), *errorMsg);

#if WITH_EDITOR
      const FBlueprintExceptionInfo exceptionInfo(EBlueprintExceptionType::NonFatalError, FText::AsCultureInvariant(errorMsg));
      FBlueprintCoreDelegates::ThrowScriptException(activeObject, stackFrame, exceptionInfo);
#endif
   }

   /// Sets the default data table for a data table row handle (mostly just useful when setting up metadata with UTATMatchSettingsBase::MakePropertyMetadata)
   void FixupGameplayTagGroupDataTableRowHandle(FDataTableRowHandle& handle)
   {
      if (handle.RowName != NAME_None && handle.DataTable == nullptr)
      {
         handle.DataTable = UTATProjectSettings::Get().MatchSettingsGameplayTagGroups.LoadSynchronous();
      }
   }

   /// Finds a field with the same name as another field but with a '_Metadata' suffix and with the type FTATMatchSettingsPropertyDef
   const FTATMatchSettingsPropertyDef* FindMatchSettingsPropertyMetadata(const UObject* obj, const FString& propName)
   {
      check(obj != nullptr);
      UClass* cls = obj->GetClass();
      check(cls != nullptr);
      const FName metadataPropName = FName(propName + kMetadataSuffix);
      FStructProperty* prop = CastField<FStructProperty>(cls->FindPropertyByName(metadataPropName));
      if (prop != nullptr && prop->Struct == FTATMatchSettingsPropertyDef::StaticStruct())
      {
         return prop->ContainerPtrToValuePtr<FTATMatchSettingsPropertyDef>(obj);
      }
      return nullptr;
   }

   /// Similar to FindMatchSettingsPropertyMetadata, but also fixes up the data first.
   /// The metadata attached to the settings object isn't always complete - eg. it may have used static initialization and/or doesn't have up-to-date Name or Type fields.
   bool FindAndPopulateMatchSettingsPropertyMetadata(const UObject* obj, const FString& propName, FTATMatchSettingsPropertyDef& outMetadata)
   {
      check(obj != nullptr);
      UClass* cls = obj->GetClass();
      check(cls != nullptr);
      const FProperty* prop = cls->FindPropertyByName(FName(propName));
      if (prop == nullptr)
      {
         return false;
      }

      // Init to any explicitly defined property metadata if we have it
      if (const FTATMatchSettingsPropertyDef* existingPropertyDef = FindMatchSettingsPropertyMetadata(obj, propName))
      {
         outMetadata = *existingPropertyDef;
      }
      else
      {
         outMetadata = FTATMatchSettingsPropertyDef{};
      }

      // Fill out any missing data where possible (this can happen if, for example, the metadata was initialized with MakePropertyMetadata).
      // For name and type we'll just overwrite any user-supplied values because because we want to keep these correct without requiring the metadata
      // to be manually kept in sync.
      outMetadata.Name = prop->GetFName();
      outMetadata.Type = GetMatchSettingsType(prop);

      // If the property has no player-facing label, set the label as the property name so it's at least readable.
      if (outMetadata.Label.IsEmpty())
      {
         outMetadata.Label = FText::AsCultureInvariant(prop->GetAuthoredName());
      }

      // For gameplay tag fields, make sure it has a valid tag group
      if (outMetadata.GameplayTagGroup.RowName != NAME_None)
      {
         ensureMsgf(outMetadata.Type == ETATMatchSettingsPropertyType::GameplayTag,
            TEXT("Match settings property '%s.%s' specifies a gameplay tag group, but the property type is %s"),
            *cls->GetName(), *propName, *StaticEnum<ETATMatchSettingsPropertyType>()->GetNameStringByValue(static_cast<int64>(outMetadata.Type)));

         // If no data table is specified, default to the one in project settings
         FixupGameplayTagGroupDataTableRowHandle(outMetadata.GameplayTagGroup);
      }

      return true;
   }

   const UEnum* GetMatchSettingsUEnum(const UObject* settings, FName propName)
   {
      if (FProperty* prop = settings->GetClass()->FindPropertyByName(propName))
      {
         if (FEnumProperty* enumProp = CastField<FEnumProperty>(prop))
         {
            return enumProp->GetEnum();
         }
      }
      return nullptr;
   }

   /// Helper for getting the data table row for a gameplay tag group
   const FTATMatchSettingsGameplayTagGroup* GetGameplayTagGroup(const UObject* settings, const FString& propName, const TCHAR* lookupContext)
   {
      FDataTableRowHandle tagGroupRowHandle{};
      check(settings != nullptr);
      FStructProperty* structProp = CastField<FStructProperty>(settings->GetClass()->FindPropertyByName(FName(propName)));
      if (structProp == nullptr || structProp->Struct != FGameplayTag::StaticStruct())
      {
         return nullptr;
      }
      if (const FTATMatchSettingsPropertyDef* propertyDef = FindMatchSettingsPropertyMetadata(settings, propName))
      {
         ensure(propertyDef->Type == ETATMatchSettingsPropertyType::GameplayTag);
         tagGroupRowHandle = propertyDef->GameplayTagGroup;
         FixupGameplayTagGroupDataTableRowHandle(tagGroupRowHandle);
      }
      return !tagGroupRowHandle.IsNull() ? tagGroupRowHandle.GetRow<FTATMatchSettingsGameplayTagGroup>(lookupContext) : nullptr;
   }

   template<typename T>
   TOptional<T> GetRandomMatchSettingsPropertyValue(const UObject* settings, const FString& propName, const FTATMatchSettingsQueryContext& context)
   {
      // TAT_ForEachPropertyType - this should handle returning a random value (or NullOpt in error cases) for all supported value types
      if constexpr (std::is_same_v<T, bool>)
      {
         return FMath::RandBool();
      }
      else if constexpr (std::is_same_v<T, int32> || std::is_same_v<T, int64> || std::is_floating_point_v<T>)
      {
         auto randRangeHelper = [](T minVal, T maxVal) -> T
         {
            if constexpr (std::is_integral_v<T>)
            {
               return FMath::RandRange(minVal, maxVal);
            }
            else
            {
               return FMath::FRandRange(minVal, maxVal);
            }
         };

         TOptional<T> minNumericValue = NullOpt;
         TOptional<T> maxNumericValue = NullOpt;
         if (const FTATMatchSettingsPropertyDef* propertyDef = FindMatchSettingsPropertyMetadata(settings, propName))
         {
            minNumericValue = propertyDef->GetMinValue<T>();
            maxNumericValue = propertyDef->GetMaxValue<T>();
         }

         if (minNumericValue && maxNumericValue)
         {
            return randRangeHelper(*minNumericValue, *maxNumericValue);
         }

         // Without a concrete min and max value, this is a bit tricky. Arguably we could just call it a validation error to have random unbounded values,
         // but to simplify things, we'll just pick some arbitrary bounds values that look reasonable to human players and work with any int or float type.
         static constexpr T minRandomValue = static_cast<T>(-1000000);
         static constexpr T maxRandomValue = static_cast<T>(1000000);

         if (minNumericValue && !maxNumericValue)
         {
            return randRangeHelper(*minNumericValue, maxRandomValue);
         }
         if (!minNumericValue && maxNumericValue)
         {
            return randRangeHelper(minRandomValue, *maxNumericValue);
         }
         return randRangeHelper(minRandomValue, maxRandomValue);
      }
      else if constexpr (std::is_enum_v<T> || std::is_same_v<T, uint8>)
      {
         const UEnum* enumType = GetMatchSettingsUEnum(settings, FName(propName));
         if (!ensure(enumType != nullptr))
         {
            return NullOpt;
         }
         int32 numEnumValues = enumType->NumEnums();
         if (enumType->ContainsExistingMax())
         {
            --numEnumValues;
         }
         const int32 randomEnumValueIndex = FMath::RandRange(0, numEnumValues - 1);
         const int64 value = enumType->GetValueByIndex(randomEnumValueIndex);
         if (ensure(value >= std::numeric_limits<uint8>::min() && value <= std::numeric_limits<uint8>::max()))
         {
            return static_cast<T>(value);
         }
         return NullOpt;
      }
      else if constexpr (std::is_same_v<T, FGameplayTag>)
      {
         static constexpr const TCHAR* dataTableLookupContext = TEXT("MatchSettingsHelpers::GetRandomMatchSettingsPropertyValue");
         if (const FTATMatchSettingsGameplayTagGroup* tagGroup = GetGameplayTagGroup(settings, propName, dataTableLookupContext))
         {
            constexpr bool includeNone = false;
            TArray<FTATMatchSettingsGameplayTag> allValidTags;
            tagGroup->GetAllGameplayTags(allValidTags, context, includeNone);
            if (allValidTags.Num() > 0)
            {
               return allValidTags[FMath::RandRange(0, allValidTags.Num() - 1)].Tag;
            }
         }
         return NullOpt;
      }
      else
      {
         UE_LOG(LogTATMatchSettingsBase, Error, TEXT("MatchSettingsHelpers::GetRandomMatchSettingsPropertyValue (with propName = '%s') called with unhandled value type"),
            *propName);
         return NullOpt;
      }
   }

   /// Sets a match settings value to the default value from the CDO
   template<typename T>
   bool TrySetMatchSettingsValueToCDOValue(UTATMatchSettingsBase* settings, FName propName)
   {
      const UTATMatchSettingsBase* cdo = settings->GetClass()->GetDefaultObject<UTATMatchSettingsBase>();
      check(cdo != nullptr);
      T value = T{};
      if (!TryGetMatchSettingsValue<T>(cdo, propName, value))
      {
         return false;
      }
      return TrySetMatchSettingsValue<T>(settings, propName, value);
   }

   /// Templated match settings value initializer - just sets the property value to the CDO's property value, or a random value if specified by the property's metadata
   template<typename T>
   bool TryInitMatchSettingsValue(UTATMatchSettingsBase* settings, const FString& propName, const FTATMatchSettingsQueryContext& context)
   {
      check(settings != nullptr);

      const FName propFName{ propName };

      // If this property is supposed to be randomized on init, try to generate a random value and assign that
      const FTATMatchSettingsPropertyDef* propertyDef = FindMatchSettingsPropertyMetadata(settings, propName);
      if (propertyDef != nullptr && propertyDef->RandomizeDefaultValue)
      {
         const TOptional<T> randomValue = GetRandomMatchSettingsPropertyValue<T>(settings, propName, context);
         if (randomValue && TrySetMatchSettingsValue<T>(settings, propFName, *randomValue))
         {
            return true;
         }
      }

      // Otherwise just set the value to whatever value the CDO has for this property
      return TrySetMatchSettingsValueToCDOValue<T>(settings, propFName);
   }

   /// Clamp, adjust, or reset a match settings value to make it valid
   /// Returns true if the value was valid, false if it was invalid and was reset/clamped to a valid value.
   template<typename T>
   bool ValidateMatchSettingsValue(const UObject* settings, const FString& propName, T& value)
   {
      check(settings != nullptr);
      const UObject* defaultSettings = settings->IsDefaultSubobject() ? settings : GetDefault<UObject>(settings->GetClass());
      check(defaultSettings != nullptr);

      auto getCDOPropertyValueOrFallback = [defaultSettings, &propName](T defaultValue) -> T
      {
         T result;
         if (TryGetMatchSettingsValue<T>(defaultSettings, FName(propName), result))
         {
            return result;
         }
         return defaultValue;
      };

      // TAT_ForEachPropertyType - not all types require validation, but you can add validation here

      // Note that the order of these if branches are important, because bool and enum types are also integral types, so we need to check those first.
      if constexpr (std::is_same_v<T, bool>)
      {
         // Make sure if we have an int value, it's either 0 or 1. The only reason this could fail is if the source data is corrupt or malicious.
         const int32 boolValue = static_cast<int32>(value);
         if (ensure(boolValue == 0 || boolValue == 1))
         {
            return true;
         }

         // Get the default value for this field and if that fails, default to false.
         value = getCDOPropertyValueOrFallback(false);
         return false;
      }
      else if constexpr (std::is_enum_v<T>)
      {
         UEnum* enumMeta = StaticEnum<T>();
         check(enumMeta != nullptr);
         const int64 enumValue = static_cast<int64>(value);
         if (ensure(enumMeta->IsValidEnumValue(enumValue) && enumValue != enumMeta->GetMaxEnumValue()))
         {
            return true;
         }
         value = getCDOPropertyValueOrFallback(static_cast<T>(0));
         return false;
      }
      else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
      {
         if (const FTATMatchSettingsPropertyDef* propertyDef = FindMatchSettingsPropertyMetadata(settings, propName))
         {
            // If the value is out of range, clamp it
            if (propertyDef->UseMinValue && value < static_cast<T>(propertyDef->MinValue))
            {
               value = static_cast<T>(propertyDef->MinValue);
               return false;
            }
            if (propertyDef->UseMaxValue && value > static_cast<T>(propertyDef->MaxValue))
            {
               value = static_cast<T>(propertyDef->MaxValue);
               return false;
            }
         }
         return true;
      }
      else if constexpr (std::is_same_v<T, FGameplayTag>)
      {
         if (const FTATMatchSettingsGameplayTagGroup* tagGroup = GetGameplayTagGroup(settings, propName, TEXT("ValidateMatchSettingsValue")))
         {
            if (tagGroup->ContainsGameplayTag(value))
            {
               return true;
            }
            // If we have a tag prefix for validation and the tag starts with that prefix, assume we have a valid value
            if (tagGroup->ValidationTagPrefix.IsValid() && value.MatchesTag(tagGroup->ValidationTagPrefix))
            {
               return true;
            }
            // if an empty tag is valid for this tag group, use that as the fallback value
            if (tagGroup->AllowNone)
            {
               value = getCDOPropertyValueOrFallback(FGameplayTag::EmptyTag);
               return false;
            }
            // use the first tag in the group as the default
            if (tagGroup->GameplayTags.Num() > 0)
            {
               value = getCDOPropertyValueOrFallback(tagGroup->GameplayTags[0].Tag);
               return false;
            }
         }

         // If we get here, we don't have enough information to validate a gameplay tag, so assume that all valid tags are fine
         return true;
      }
      else
      {
         return true;
      }
   }

   /// Helper for logging values
   template<typename T>
   FString MatchSettingValueToDebugString(const T& value, UEnum* enumType = nullptr)
   {
      if constexpr (std::is_enum_v<T> || std::is_same_v<T, uint8>)
      {
         if (enumType == nullptr)
         {
            return FString::Printf(TEXT("(UnknownEnum)%d"), static_cast<int32>(value));
         }
         FString valueName;
         if (enumType->FindNameStringByValue(valueName, static_cast<int64>(value)))
         {
            return FString::Printf(TEXT("(%s)%s"), *enumType->GetName(), *valueName);
         }
         return FString::Printf(TEXT("(%s)%d"), *enumType->GetName(), static_cast<int32>(value));
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
         if (static_cast<int32>(value) == 0)
         {
            return TEXT("False");
         }
         if (static_cast<int32>(value) == 1)
         {
            return TEXT("True");
         }
         return FString::Printf(TEXT("(bool)%d"), static_cast<int32>(value));
      }
      else if constexpr (std::is_same_v<T, FGameplayTag>)
      {
         return value.ToString();
      }
      else
      {
         // assume the type can be formatted with FString::Format
         FStringFormatOrderedArguments args;
         args.Add(value);
         return FString::Format(TEXT("{0}"), args);
      }
   }
} // namespace MatchSettingsHelpers

// static
const UTATMatchSettingsBase* UTATMatchSettingsBase::GetTATMatchSettings(const UObject* contextObject)
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
   {
      if(UTATGameInstance* TATGameInstance = world->GetGameInstance<UTATGameInstance>())
      {
         // Use the version that tolerates null
         // Could make a new method, but :shrug:
         return TATGameInstance->BP_GetMatchSettings();
      }
   }

   return nullptr;
}

// static
FTATMatchSettingsPropertyDef UTATMatchSettingsBase::MakePropertyMetadata(const FString& label, const FString& desc, const TOptional<double>& minValue, const TOptional<double>& maxValue, bool randomizeDefaultValue)
{
   FTATMatchSettingsPropertyDef metadata{};
   metadata.Label = FText::AsCultureInvariant(label);
   metadata.Description = FText::AsCultureInvariant(desc);
   metadata.RandomizeDefaultValue = randomizeDefaultValue;
   if (minValue)
   {
      metadata.UseMinValue = true;
      metadata.MinValue = *minValue;
   }
   if (maxValue)
   {
      metadata.UseMaxValue = true;
      metadata.MaxValue = *maxValue;
   }
   return metadata;
}

// static
FTATMatchSettingsPropertyDef UTATMatchSettingsBase::MakePropertyMetadata(const FString& label, const FString& desc, FName gameplayTagGroupName, bool randomizeDefaultValue)
{
   FTATMatchSettingsPropertyDef metadata{};
   metadata.Type = ETATMatchSettingsPropertyType::GameplayTag;
   metadata.Label = FText::AsCultureInvariant(label);
   metadata.Description = FText::AsCultureInvariant(desc);
   metadata.RandomizeDefaultValue = randomizeDefaultValue;
   // Ideally we would set DataTable to UTATProjectSettings::Get().MatchSettingsGameplayTagGroups, but this is intended to be run at static initialization,
   // so we need to defer until later.
   // See also: MatchSettingsHelpers::FixupGameplayTagGroupDataTableRowHandle()
   metadata.GameplayTagGroup.DataTable = nullptr;
   metadata.GameplayTagGroup.RowName = gameplayTagGroupName;
   return metadata;
}

// static
FTATMatchSettingsQueryContext UTATMatchSettingsBase::MakeMatchSettingsQueryContextFromWorldContext(const UObject* contextObject)
{
   return FTATMatchSettingsQueryContext::MakeFromWorldContext(contextObject);
}

void UTATMatchSettingsBase::InitMatchSettingsForNewMatch(const FTATMatchSettingsQueryContext& context)
{
   MatchSettingsHelpers::ForEachMatchSettingsProperty(this, [&](const FProperty* prop, const FString& propName)
   {
      const ETATMatchSettingsPropertyType propType = MatchSettingsHelpers::GetMatchSettingsType(prop);
      const FTATMatchSettingsPropertyDef* propertyDef = MatchSettingsHelpers::FindMatchSettingsPropertyMetadata(this, propName);

   // TODO: Convert this to a templated lambda once we're using C++20
#define TAT_TRY_INIT_PROPERTY(CPP_TYPE) do { \
      using Traits = MatchSettingsHelpers::TCppTypeToProperty<CPP_TYPE>; \
      if (propType == Traits::UIType) \
      { \
         MatchSettingsHelpers::TryInitMatchSettingsValue<CPP_TYPE>(this, propName, context); \
         return; \
      } \
   } while(0)

      // TAT_ForEachPropertyType
      TAT_TRY_INIT_PROPERTY(bool);
      TAT_TRY_INIT_PROPERTY(int32);
      TAT_TRY_INIT_PROPERTY(float);
      TAT_TRY_INIT_PROPERTY(double);
      TAT_TRY_INIT_PROPERTY(uint8);
      TAT_TRY_INIT_PROPERTY(FGameplayTag);

#undef TAT_TRY_INIT_PROPERTY
   });
}

bool UTATMatchSettingsBase::RandomizeMatchSettingsProperty(FName propName, const FTATMatchSettingsQueryContext& context)
{
   FProperty* prop = GetClass()->FindPropertyByName(propName);
   if (prop == nullptr)
   {
      return false;
   }

   const ETATMatchSettingsPropertyType propType = MatchSettingsHelpers::GetMatchSettingsType(prop);
   const FString propNameString = propName.ToString();

   // TODO: Convert this to a templated lambda once we're using C++20
#define TAT_TRY_RANDOMIZE_PROPERTY(CPP_TYPE) do { \
      using Traits = MatchSettingsHelpers::TCppTypeToProperty<CPP_TYPE>; \
      if (propType == Traits::UIType) \
      { \
         if (const TOptional<CPP_TYPE> randomValue = MatchSettingsHelpers::GetRandomMatchSettingsPropertyValue<CPP_TYPE>(this, propNameString, context)) \
         { \
            return MatchSettingsHelpers::TrySetMatchSettingsValue<CPP_TYPE>(this, propName, *randomValue); \
         } \
         return false; \
      } \
   } while(0)

      // TAT_ForEachPropertyType
      TAT_TRY_RANDOMIZE_PROPERTY(bool);
      TAT_TRY_RANDOMIZE_PROPERTY(int32);
      TAT_TRY_RANDOMIZE_PROPERTY(float);
      TAT_TRY_RANDOMIZE_PROPERTY(double);
      TAT_TRY_RANDOMIZE_PROPERTY(uint8);
      TAT_TRY_RANDOMIZE_PROPERTY(FGameplayTag);

#undef TAT_TRY_RANDOMIZE_PROPERTY

   return false;
}

UTATMatchSettingsBase* UTATMatchSettingsBase::CopyMatchSettingsIntoNewObject(UObject* outer, FName name, EObjectFlags flags) const
{
   check(outer != nullptr);
   TArray<uint8> data;
   const_cast<UTATMatchSettingsBase*>(this)->SerializeToByteArray(data);
   UTATMatchSettingsBase* selfCopy = NewObject<UTATMatchSettingsBase>(outer, GetClass(), name, flags);
   const bool matchSettingsDeserializeSuccess = selfCopy->DeserializeFromByteArray(data);
   ensure(matchSettingsDeserializeSuccess);
   return selfCopy;
}

void UTATMatchSettingsBase::GetAllMatchSettingsProperties(TArray<FTATMatchSettingsPropertyDef>& outProperties) const
{
   outProperties.Reset();
   FTATMatchSettingsPropertyDef propertyDef{};
   MatchSettingsHelpers::ForEachMatchSettingsProperty(this, [&](const FProperty* prop, const FString& propName)
   {
      if (MatchSettingsHelpers::FindAndPopulateMatchSettingsPropertyMetadata(this, propName, propertyDef))
      {
         outProperties.Add(propertyDef);
      }
   });
}

ETATMatchSettingsPropertyType UTATMatchSettingsBase::GetMatchSettingPropertyType(FName propName) const
{
   if (FProperty* prop = GetClass()->FindPropertyByName(propName))
   {
      return MatchSettingsHelpers::GetMatchSettingsType(prop);
   }
   return ETATMatchSettingsPropertyType::Invalid;
}

bool UTATMatchSettingsBase::GetMatchSettingsEnumValueList(FName propName, TArray<FTATMatchSettingsEnumValue>& enumValues) const
{
   enumValues.Reset();
   if (const UEnum* enumData = MatchSettingsHelpers::GetMatchSettingsUEnum(this, propName))
   {
      const int64 maxValue = enumData->GetMaxEnumValue();
      enumValues.Reserve(enumData->NumEnums());
      for (int32 i = 0; i < enumData->NumEnums(); i++)
      {
         const int64 enumValue = enumData->GetValueByIndex(i);
         if (maxValue == enumValue)
         {
            continue;
         }
         ensure(enumValue >= 0 && enumValue <= static_cast<int64>(std::numeric_limits<uint8>::max()));
         const uint8 enumValueByte = static_cast<uint8>(enumValue);
         enumValues.Add(FTATMatchSettingsEnumValue{ enumData->GetDisplayNameTextByIndex(i), enumValueByte });
      }
      return true;
   }
   return false;
}

bool UTATMatchSettingsBase::GetMatchSettingsGameplayTagGroup(FName propName, FTATMatchSettingsGameplayTagGroup& gameplayTagGroup) const
{
   static constexpr const TCHAR* dataTableLookupContext = TEXT("UTATMatchSettingsBase::GetMatchSettingsGameplayTagValueList");
   if (const FTATMatchSettingsGameplayTagGroup* tagGroup = MatchSettingsHelpers::GetGameplayTagGroup(this, propName.ToString(), dataTableLookupContext))
   {
      gameplayTagGroup = *tagGroup;
      return true;
   }
   gameplayTagGroup = FTATMatchSettingsGameplayTagGroup{};
   return false;
}

bool UTATMatchSettingsBase::GetMatchSettingsGameplayTagList(FName propName, const FTATMatchSettingsQueryContext& context, TArray<FTATMatchSettingsGameplayTag>& tagValues, bool& allowNone) const
{
   static constexpr const TCHAR* dataTableLookupContext = TEXT("UTATMatchSettingsBase::GetMatchSettingsGameplayTagValueList");
   tagValues.Reset();
   if (const FTATMatchSettingsGameplayTagGroup* tagGroup = MatchSettingsHelpers::GetGameplayTagGroup(this, propName.ToString(), dataTableLookupContext))
   {
      allowNone = tagGroup->AllowNone;
      tagGroup->GetAllGameplayTags(tagValues, context);
      return true;
   }
   allowNone = true; // If we can't find metadata, allow all tags
   return false;
}

bool UTATMatchSettingsBase::IsValidGameplayTagForProperty(FName propName, FGameplayTag gameplayTag, const FTATMatchSettingsQueryContext& context) const
{
   const FTATMatchSettingsGameplayTagGroup* tagGroup = MatchSettingsHelpers::GetGameplayTagGroup(this, propName.ToString(), TEXT("UTATMatchSettingsBase::IsValidGameplayTag"));
   return tagGroup == nullptr || tagGroup->ContainsGameplayTag(gameplayTag, context);
}

DEFINE_FUNCTION(UTATMatchSettingsBase::execBP_GetMatchSettingsValue)
{
   // Collect arguments
   P_GET_STRUCT(FName, propName);

   // Accept any type for the outValue param
   Stack.MostRecentPropertyAddress = nullptr;
   Stack.MostRecentPropertyContainer = nullptr;
   Stack.StepCompiledIn<FProperty>(nullptr);
   void* outValuePtr = Stack.MostRecentPropertyAddress;
   FProperty* outValueProperty = Stack.MostRecentProperty;
   P_FINISH;

   check(outValueProperty != nullptr && outValuePtr != nullptr);

   bool& outReturnValue = *static_cast<bool*>(RESULT_PARAM);
   outReturnValue = false; // default return value

   // NB. This could be converted into a lambda once we're using C++20 (lambdas in C++17 and earlier can't be templates)
#define TAT_TRY_GET_PROPERTY_VALUE(CPP_TYPE) do \
   { \
      if (MatchSettingsHelpers::PropertyMatchesType<CPP_TYPE>(outValueProperty)) \
      { \
         CPP_TYPE curValue = CPP_TYPE(); \
         if (MatchSettingsHelpers::TryGetMatchSettingsValue<CPP_TYPE>(P_THIS, propName, curValue)) \
         { \
            outReturnValue = MatchSettingsHelpers::TrySetPropertyValue<CPP_TYPE>(outValueProperty, outValuePtr, curValue); \
            return; \
         } \
      } \
   } while(0)

   // TAT_ForEachPropertyType
   TAT_TRY_GET_PROPERTY_VALUE(bool);
   TAT_TRY_GET_PROPERTY_VALUE(int32);
   TAT_TRY_GET_PROPERTY_VALUE(float);
   TAT_TRY_GET_PROPERTY_VALUE(double);
   TAT_TRY_GET_PROPERTY_VALUE(uint8);
   TAT_TRY_GET_PROPERTY_VALUE(FGameplayTag);

#undef TAT_TRY_GET_PROPERTY_VALUE

   // If we got this far, the specified type is not supported
   MatchSettingsHelpers::ThrowUnhandledParameterTypeBlueprintExceptionAndLogError(propName, outValueProperty, P_THIS, Stack);
}

DEFINE_FUNCTION(UTATMatchSettingsBase::execBP_SetMatchSettingsValue)
{
   // Collect arguments
   P_GET_STRUCT(FName, propName);

   // Accept any type for the newValue param
   Stack.MostRecentPropertyAddress = nullptr;
   Stack.MostRecentPropertyContainer = nullptr;
   Stack.StepCompiledIn<FProperty>(nullptr);
   void* newValuePtr = Stack.MostRecentPropertyAddress;
   FProperty* newValueProperty = Stack.MostRecentProperty;
   P_FINISH;

   // Assumption: This blueprint setter method will *only* be used in UMG widgets during match setup.
   // If we can get a reference to world settings, verify this is the case (trying to modify match settings during a mission is almost certainly a mistake).
   // Note that there are other ways to modify match settings during a match (eg. cheats), and we don't want to block those. This check is just to safeguard
   // against someone trying to do something fancy in blueprints.
   if (const UWorld* world = GEngine->GetWorldFromContextObject(P_THIS, EGetWorldErrorMode::ReturnNull))
   {
      if (ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings()))
      {
         if (!ensure(worldSettings->MapType != ETATMapType::Mission))
         {
            return;
         }
      }
   }

   check(newValueProperty != nullptr && newValuePtr != nullptr);

   bool& outReturnValue = *static_cast<bool*>(RESULT_PARAM);
   outReturnValue = false; // default return value

   // NB. This could be converted into a lambda once we're using C++20 (lambdas in C++17 and earlier can't be templates)
#define TAT_TRY_SET_PROPERTY_VALUE(CPP_TYPE) do \
   { \
      if (MatchSettingsHelpers::PropertyMatchesType<CPP_TYPE>(newValueProperty)) \
      { \
         CPP_TYPE newValue = CPP_TYPE(); \
         if (MatchSettingsHelpers::TryGetPropertyValue<CPP_TYPE>(newValueProperty, newValuePtr, newValue)) \
         { \
            outReturnValue = MatchSettingsHelpers::TrySetMatchSettingsValue<CPP_TYPE>(P_THIS, propName, newValue); \
            return; \
         } \
      } \
   } \
   while (0)

   // TAT_ForEachPropertyType
   TAT_TRY_SET_PROPERTY_VALUE(bool);
   TAT_TRY_SET_PROPERTY_VALUE(int32);
   TAT_TRY_SET_PROPERTY_VALUE(float);
   TAT_TRY_SET_PROPERTY_VALUE(double);
   TAT_TRY_SET_PROPERTY_VALUE(uint8);
   TAT_TRY_SET_PROPERTY_VALUE(FGameplayTag);

#undef TAT_TRY_SET_PROPERTY_VALUE

   // If we got this far, the specified type is not supported
   MatchSettingsHelpers::ThrowUnhandledParameterTypeBlueprintExceptionAndLogError(propName, newValueProperty, P_THIS, Stack);
}

UEnum* UTATMatchSettingsBase::GetMatchSettingsEnumType(FName propName) const
{
   if (FEnumProperty* enumProperty = CastField<FEnumProperty>(GetClass()->FindPropertyByName(propName)))
   {
      return enumProperty->GetEnum();
   }
   return nullptr;
}

bool UTATMatchSettingsBase::GetMatchSettingsValueAsBool(FName propName, bool& outValue) const
{
   return MatchSettingsHelpers::TryGetMatchSettingsValue(this, propName, outValue);
}

bool UTATMatchSettingsBase::GetMatchSettingsValueAsInt(FName propName, int32& outValue) const
{
   return MatchSettingsHelpers::TryGetMatchSettingsValue(this, propName, outValue);
}

bool UTATMatchSettingsBase::GetMatchSettingsValueAsFloat(FName propName, float& outValue) const
{
   return MatchSettingsHelpers::TryGetMatchSettingsValue(this, propName, outValue);
}

bool UTATMatchSettingsBase::GetMatchSettingsValueAsDouble(FName propName, double& outValue) const
{
   return MatchSettingsHelpers::TryGetMatchSettingsValue(this, propName, outValue);
}

bool UTATMatchSettingsBase::GetMatchSettingsValueAsByte(FName propName, uint8& outValue) const
{
   return MatchSettingsHelpers::TryGetMatchSettingsValue(this, propName, outValue);
}

bool UTATMatchSettingsBase::GetMatchSettingsValueAsGameplayTag(FName propName, FGameplayTag& outValue) const
{
   return MatchSettingsHelpers::TryGetMatchSettingsValue(this, propName, outValue);
}

bool UTATMatchSettingsBase::GetMatchSettingsValueAsString(FName propName, FString& outPropValueString) const
{
   outPropValueString.Reset();

   const FProperty* prop = GetClass()->FindPropertyByName(propName);
   if (prop == nullptr)
   {
      return false;
   }

   const void* valuePtr = prop->ContainerPtrToValuePtr<void>(this);
   check(valuePtr != nullptr);

   prop->ExportTextItem_Direct(outPropValueString, valuePtr, nullptr, const_cast<UTATMatchSettingsBase*>(this), 0);
   return true;
}

bool UTATMatchSettingsBase::SetMatchSettingsValueAsString(FName propName, FString newPropValue, const FTATMatchSettingsQueryContext& queryContext, FString* outErrorMessage)
{
   FProperty* prop = GetClass()->FindPropertyByName(propName);
   if (prop == nullptr)
   {
      if (outErrorMessage != nullptr)
      {
         *outErrorMessage = FString::Printf(TEXT("Unknown property '%s'"), *propName.ToString());
      }
      return false;
   }

   // Some limited data validation, just for the sake of better error messages when using cheats/editor settings
   switch (MatchSettingsHelpers::GetMatchSettingsType(prop))
   {
   case ETATMatchSettingsPropertyType::Invalid:
      if (outErrorMessage != nullptr)
      {
         *outErrorMessage = FString::Printf(TEXT("Property '%s' has type '%s' which is unsupported"), *propName.ToString(), *prop->GetClass()->GetName());
      }
      return false;
   case ETATMatchSettingsPropertyType::Bool:
      if (!newPropValue.Equals(TEXT("true"), ESearchCase::IgnoreCase)
         && !newPropValue.Equals(TEXT("false"), ESearchCase::IgnoreCase)
         && newPropValue != TEXT("0")
         && newPropValue != TEXT("1"))
      {
         if (outErrorMessage != nullptr)
         {
            *outErrorMessage = FString::Printf(TEXT("Invalid boolean value '%s'"), *newPropValue);
         }
         return false;
      }
      break;
   case ETATMatchSettingsPropertyType::GameplayTag:
      {
         static constexpr const TCHAR* dataTableLookupContext = TEXT("UTATMatchSettingsBase::SetMatchSettingsValueAsString");
         const FTATMatchSettingsGameplayTagGroup* tagGroup = MatchSettingsHelpers::GetGameplayTagGroup(this, propName.ToString(), dataTableLookupContext);

         // The tag group allows none and the new value is none - no need to do other validation
         if (tagGroup != nullptr && tagGroup->AllowNone && (newPropValue.Len() == 0 || newPropValue.Equals(TEXT("none"), ESearchCase::IgnoreCase)))
         {
            break;
         }

         FString fixedString;
         FText tagError;
         if (!FGameplayTag::IsValidGameplayTagString(newPropValue, &tagError, &fixedString))
         {
            if (fixedString.Len() > 0)
            {
               // no need to return false here, because we recovered from the error with a fixed-up tag
               newPropValue = MoveTemp(fixedString);
            }
            else
            {
               // the gameplay tag isn't valid
               if (outErrorMessage != nullptr)
               {
                  *outErrorMessage = tagError.ToString();
               }
               return false;
            }
         }

         constexpr bool errorIfNotFound = false;
         if (!IsValidGameplayTagForProperty(propName, FGameplayTag::RequestGameplayTag(FName(newPropValue), errorIfNotFound), queryContext))
         {
            // generate an error message that includes the complete list of valid tags for this property if possible
            if (outErrorMessage != nullptr)
            {
               TStringBuilder<96> tagErrorMessage;
               tagErrorMessage.Appendf(TEXT("Invalid tag '%s' for property '%s'"), *newPropValue, *propName.ToString());
               if (tagGroup != nullptr)
               {
                  tagErrorMessage.Appendf(TEXT(" (valid tags: %s)"), *tagGroup->ToTagListDebugString());
               }
               *outErrorMessage = *tagErrorMessage;
            }
            return false;
         }
         break;
      }
   default:
      break;
   }

   void* valuePtr = prop->ContainerPtrToValuePtr<void>(this);
   check(valuePtr != nullptr);

   // Set the value by string in the same way that the editor allows copying property values by string
   // (eg. if this property was an FVector, this would accept the string "(X=0,Y=0,Z=0)")
   FStringOutputDevice errors;
   prop->ImportText_Direct(*newPropValue, valuePtr, this, 0, &errors);
   if (outErrorMessage != nullptr)
   {
      *outErrorMessage = errors;
   }
   return errors.Len() == 0;
}

void UTATMatchSettingsBase::SerializeToByteArray(TArray<uint8>& outByteArray)
{
   outByteArray.Reset();
   FMemoryWriter writer(outByteArray);

   UClass* selfClass = GetClass();
   check(selfClass != nullptr);

   // First byte is a bool that determines if we're going to include debug validation data.
   uint8 includeDebugValidationData = kMatchSettingsDebugValidationMode;
   writer << includeDebugValidationData;

   if constexpr (kMatchSettingsDebugValidationMode)
   {
      // Serialize the hash of our class path as a failsafe against deserializing to a different type
      uint32 selfClassHash = GetTypeHash(FSoftClassPath(selfClass).ToString());
      writer << selfClassHash;
   }

#if TAT_DEBUG_MATCH_SETTINGS_SERIALIZATION
   FTATDebugPropList debugPropList;
#endif

   FString propName;
   FString propValueString;

   for (TPropertyValueIterator<FProperty> propIter(selfClass, this, EPropertyValueIteratorFlags::NoRecursion); propIter; ++propIter)
   {
      const FProperty* prop = propIter->Key;
      check(prop != nullptr);
      const void* valuePtr = propIter->Value;
      check(valuePtr != nullptr);

      if ((prop->PropertyFlags & CPF_Transient) != 0)
      {
         continue;
      }

      prop->GetName(propName);

      // Don't validate metadata properties, they're not even intended to be mutable
      if (propName.EndsWith(MatchSettingsHelpers::kMetadataSuffix))
      {
         continue;
      }

      const FName propFName = prop->GetFName();

      writer << propName;

      ETATMatchSettingsPropertyType propType = MatchSettingsHelpers::GetMatchSettingsType(prop);
      writer << propType;

      //TODO: Once we're using C++20, look into special-casing gameplay tags to use NetSerialize (with a null package map) to avoid serializing them as strings
#define TAT_TRY_SERIALIZE_PROPERTY_VALUE(CPP_TYPE) { \
      using Traits = MatchSettingsHelpers::TCppTypeToProperty<CPP_TYPE>; \
      if (propType == Traits::UIType) \
      { \
         CPP_TYPE propValue{}; \
         const bool success = MatchSettingsHelpers::TryGetMatchSettingsValue(this, propFName, propValue); \
         check(success); \
         writer << propValue; \
         TAT_ADD_PROP_TO_DEBUG_LIST(debugPropList, propName, MatchSettingsHelpers::MatchSettingValueToDebugString(propValue, GetMatchSettingsEnumType(propFName))); \
         continue; \
      } \
   }

      // TAT_ForEachPropertyType
      TAT_TRY_SERIALIZE_PROPERTY_VALUE(bool);
      TAT_TRY_SERIALIZE_PROPERTY_VALUE(int32);
      TAT_TRY_SERIALIZE_PROPERTY_VALUE(float);
      TAT_TRY_SERIALIZE_PROPERTY_VALUE(double);
      TAT_TRY_SERIALIZE_PROPERTY_VALUE(uint8);
      TAT_TRY_SERIALIZE_PROPERTY_VALUE(FGameplayTag);

#undef TAT_TRY_SERIALIZE_PROPERTY_VALUE
   }

#if TAT_DEBUG_MATCH_SETTINGS_SERIALIZATION
   // This formats the byte array as a Python3 bytestring for ease of debugging
   TStringBuilder<255> hexBytes;
   hexBytes << TEXT("b'");
   for (uint8 byte : outByteArray)
   {
      if ((byte >= 32 && byte <= 126) && byte != '\\') { hexBytes.Appendf(TEXT("%c"), byte); }
      else { hexBytes.Appendf(TEXT("\\x%02x"), byte); }
   }
   hexBytes << TEXT("'");
   UE_LOG(LogTATMatchSettingsBase, Log, TEXT("Serialized match settings: %s\nBytes: %s"), *debugPropList, *hexBytes);
#endif
}

bool UTATMatchSettingsBase::DeserializeFromByteArray(const TArray<uint8>& byteArray)
{
   if (byteArray.Num() == 0)
   {
      UE_LOG(LogTATMatchSettingsBase, Error, TEXT("Failed to deserialize match settings: data was empty"));
      return false;
   }

   UClass* selfClass = GetClass();
   check(selfClass != nullptr);

   FMemoryReader reader(byteArray);

   uint8 includeDebugValidationData = false;
   reader << includeDebugValidationData;

   if (includeDebugValidationData)
   {
      uint32 classHash = 0;
      reader << classHash;

      if constexpr (kMatchSettingsDebugValidationMode)
      {
         // Read the hash of our class path as a failsafe against deserializing to a different type
         const uint32 selfClassHash = GetTypeHash(FSoftClassPath(selfClass).ToString());
         if (classHash != selfClassHash)
         {
            UE_LOG(LogTATMatchSettingsBase, Error, TEXT("Failed to deserialize match settings: expected class hash %u, got %u (expected match settings class '%s')"),
               selfClassHash, classHash, *FSoftClassPath(selfClass).ToString());
            return false;
         }
      }
   }

   UEnum* propTypeEnum = StaticEnum<ETATMatchSettingsPropertyType>();
   check(propTypeEnum != nullptr);

#if TAT_DEBUG_MATCH_SETTINGS_SERIALIZATION
   FTATDebugPropList debugPropList;
#endif

   FString propName;
   ETATMatchSettingsPropertyType propType = ETATMatchSettingsPropertyType::Invalid;
   FString propValueString;
   while (!reader.AtEnd())
   {
      // Read and validate the property name
      reader << propName;
      if (propName.Len() == 0 || propName.Len() > 255)
      {
         UE_LOG(LogTATMatchSettingsBase, Error, TEXT("Failed to deserialize match settings: got invalid property name '%s'"), *propName);
         return false;
      }

      const FName propFName{ propName };

      // Make sure it represents an actual property
      FProperty* prop = selfClass->FindPropertyByName(propFName);
      if (prop == nullptr)
      {
         UE_LOG(LogTATMatchSettingsBase, Error, TEXT("Failed to deserialize match settings: got unexpected property '%s'"), *propName);
         return false;
      }

      // Read and validate the property type
      reader << propType;
      const ETATMatchSettingsPropertyType actualPropType = MatchSettingsHelpers::GetMatchSettingsType(prop);
      check(actualPropType != ETATMatchSettingsPropertyType::Invalid);
      if (propType != actualPropType)
      {
         UE_LOG(LogTATMatchSettingsBase, Error, TEXT("Failed to deserialize match settings: While reading property '%s', expected type %s, got %s"),
            *propName,
            *MatchSettingsHelpers::MatchSettingValueToDebugString(actualPropType, propTypeEnum),
            *MatchSettingsHelpers::MatchSettingValueToDebugString(propType, propTypeEnum));
         return false;
      }

#define TAT_TRY_DESERIALIZE_PROPERTY_VALUE(CPP_TYPE) { \
      using Traits = MatchSettingsHelpers::TCppTypeToProperty<CPP_TYPE>; \
      if (propType == Traits::UIType) \
      { \
         CPP_TYPE propValue{}; \
         reader << propValue; \
         const CPP_TYPE origValue = propValue; \
         if (!MatchSettingsHelpers::ValidateMatchSettingsValue(this, propName, propValue)) \
         { \
            UE_LOG(LogTATMatchSettingsBase, Warning, TEXT("Issue deserializing match settings: property '%s' with type '%s' failed validation and was changed from %s to %s"), \
               *propName, \
               TEXT(#CPP_TYPE), \
               *MatchSettingsHelpers::MatchSettingValueToDebugString(origValue, GetMatchSettingsEnumType(propFName)), \
               *MatchSettingsHelpers::MatchSettingValueToDebugString(propValue, GetMatchSettingsEnumType(propFName))); \
         } \
         const bool success = MatchSettingsHelpers::TrySetMatchSettingsValue(this, propFName, propValue); \
         check(success); \
         TAT_ADD_PROP_TO_DEBUG_LIST(debugPropList, propName, MatchSettingsHelpers::MatchSettingValueToDebugString(propValue, GetMatchSettingsEnumType(propFName))); \
         continue; \
      } \
   }

      // Read and validate the property value
      // TAT_ForEachPropertyType
      TAT_TRY_DESERIALIZE_PROPERTY_VALUE(bool);
      TAT_TRY_DESERIALIZE_PROPERTY_VALUE(int32);
      TAT_TRY_DESERIALIZE_PROPERTY_VALUE(float);
      TAT_TRY_DESERIALIZE_PROPERTY_VALUE(double);
      TAT_TRY_DESERIALIZE_PROPERTY_VALUE(uint8);
      TAT_TRY_DESERIALIZE_PROPERTY_VALUE(FGameplayTag);

#undef TAT_TRY_DESERIALIZE_PROPERTY_VALUE

      // If we get to here, none of the above blocks matched this property
      UE_LOG(LogTATMatchSettingsBase, Error, TEXT("Failed to deserialize match settings: property '%s' does not have a supported type"), *propName);
      return false;
   }

#if TAT_DEBUG_MATCH_SETTINGS_SERIALIZATION
   UE_LOG(LogTATMatchSettingsBase, Log, TEXT("Deserialized match settings: %s"), *debugPropList);
#endif

   return true;
}

#if WITH_EDITOR
EDataValidationResult UTATMatchSettingsBase::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = EDataValidationResult::Valid;
   UClass* cls = GetClass();
   check(cls != nullptr);

   // NB. This is actually validating that fields and metadata are set up correctly, which is really static data instead of runtime data.

   FString propName;
   for (TPropertyValueIterator<FProperty> propIter(cls, this, EPropertyValueIteratorFlags::NoRecursion); propIter; ++propIter)
   {
      const FProperty* prop = propIter->Key;
      check(prop != nullptr);
      const bool isTransient = (prop->PropertyFlags & CPF_Transient) != 0;

      prop->GetName(propName);

      if (propName.EndsWith(MatchSettingsHelpers::kMetadataSuffix))
      {
         // Validate metadata field
         if (!isTransient)
         {
            context.AddError(FText::AsCultureInvariant(FString::Printf(TEXT("Metadata field '%s' must be transient!"), *propName)));
            result = EDataValidationResult::Invalid;
         }

         // Make sure the the metadata field has the correct type
         const FStructProperty* structProperty = CastField<FStructProperty>(prop);
         if (structProperty == nullptr || structProperty->Struct != FTATMatchSettingsPropertyDef::StaticStruct())
         {
            context.AddError(FText::AsCultureInvariant(
               FString::Printf(TEXT("Match settings metadata field '%s' is not of type FTATMatchSettingsPropertyDef"), *propName)));
            result = EDataValidationResult::Invalid;
         }

         // Make sure that this metadata field references an existing field
         FStringView metadataForPropName = propName;
         metadataForPropName.RemoveSuffix(FStringView(MatchSettingsHelpers::kMetadataSuffix).Len());
         if (cls->FindPropertyByName(FName(metadataForPropName)) == nullptr)
         {
            context.AddError(FText::AsCultureInvariant(
               FString::Printf(TEXT("Match settings metadata field '%s' is valid, but field with name '%s' was not found!"), *propName, *FString(metadataForPropName))));
            result = EDataValidationResult::Invalid;
         }
      }
      else if (!isTransient)
      {
         // Validate non-metadata fields (must not be transient)
         const ETATMatchSettingsPropertyType type = MatchSettingsHelpers::GetMatchSettingsType(prop);
         if (type == ETATMatchSettingsPropertyType::Invalid)
         {
            context.AddError(FText::AsCultureInvariant(FString::Printf(TEXT("Match settings field '%s' has type '%s' which is not supported"),
               *propName, *prop->GetCPPType())));
            result = EDataValidationResult::Invalid;
         }
      }
      else
      {
         check(isTransient);
         // Assume that a non-metadata transient field is not intended to be part of the actual match settings.
         // It's fine to exist as long as there isn't a metadata field referring to it.
         if (MatchSettingsHelpers::FindMatchSettingsPropertyMetadata(this, propName) != nullptr)
         {
            context.AddError(FText::AsCultureInvariant(FString::Printf(TEXT("Match settings field '%s' is transient but also has a metadata field"), *propName)));
            result = EDataValidationResult::Invalid;
         }
      }
   }
   return result;
}
#endif
