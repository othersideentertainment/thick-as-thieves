// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "CoreMinimal.h"

// tat
#include "TATMatchSettingsPropertyDef.h"

#include "TATMatchSettingsBase.generated.h"

///
/// Base class for player-facing match settings
///
/// Unlike normal UPROPERTY fields that only have their metadata available to the editor, fields added to subclasses of UTATMatchSettingsBase need to expose
/// metadata available in a shipping build.
///
/// How to add a field:
///  * Add a UPROPERTY field with a BlueprintReadOnly flag (they're not intended to be modified during a match)
///  * (Optional) Add a second UPROPERTY field with a Transient flag of type FTATMatchSettingsPropertyDef, and named the same as the field, but with the
///    suffix "_Metadata". You can use the MakePropertyMetadata helper function to set a player-facing label and description as well as other relevant metadata
///    if needed.
///
UCLASS(BlueprintType, Blueprintable)
class TAT_API UTATMatchSettingsBase : public UObject
{
   GENERATED_BODY()

public:
   // statics
   UFUNCTION(BlueprintPure, Category = "TAT Match Settings", DisplayName = "Get TAT Match Settings", meta = (WorldContext = "contextObject"))
   static const UTATMatchSettingsBase* GetTATMatchSettings(const UObject* contextObject);

   template<class T>
   static const T* GetTATMatchSettings(const UObject* contextObject) { return Cast<T>(GetTATMatchSettings(contextObject));}

   // Only change match settings in place if you are careful
   template<class T>
   static T* GetMutableMatchSettings(const UObject* contextObject) { return const_cast<T*>(Cast<T>(GetTATMatchSettings(contextObject)));}
   
   /// Quick and dirty helper for adding property metadata from native
   static FTATMatchSettingsPropertyDef MakePropertyMetadata(const FString& label, const FString& desc, const TOptional<double>& minValue = NullOpt, const TOptional<double>& maxValue = NullOpt, bool randomizeDefaultValue = false);
   /// Quick and dirty helper for adding property metadata from native (gameplay tag version)
   static FTATMatchSettingsPropertyDef MakePropertyMetadata(const FString& label, const FString& desc, FName gameplayTagGroupName, bool randomizeDefaultValue = false);

   /// Makes a query context for the current world, using any object in that world as a context object
   UFUNCTION(BlueprintPure, Category = "TAT Match Settings")
   static FTATMatchSettingsQueryContext MakeMatchSettingsQueryContextFromWorldContext(const UObject* contextObject);

   /// Sets up match settings values for a new match, resetting values to their defaults.
   UFUNCTION(BlueprintCallable, Category = "TAT Match Settings")
   void InitMatchSettingsForNewMatch(const FTATMatchSettingsQueryContext& context);

   /// Sets a match settings property to a random value
   UFUNCTION(BlueprintCallable, Category = "TAT Match Settings")
   bool RandomizeMatchSettingsProperty(FName propName, const FTATMatchSettingsQueryContext& context);

   /// Creates a new match settings object that's a copy of this one
   UTATMatchSettingsBase* CopyMatchSettingsIntoNewObject(UObject* outer, FName name = NAME_None, EObjectFlags flags = RF_NoFlags) const;

   /// Gets all match settings fields, their types, and their value restrictions.
   UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "TAT Match Settings")
   void GetAllMatchSettingsProperties(TArray<FTATMatchSettingsPropertyDef>& outProperties) const;

   /// Gets the type of a given match settings field by property name.
   UFUNCTION(BlueprintPure, Category = "TAT Match Settings")
   ETATMatchSettingsPropertyType GetMatchSettingPropertyType(FName propName) const;

   /// Gets the list of possible enum values for an enum property.
   /// Returns false if the given property is not an enum.
   UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "TAT Match Settings")
   bool GetMatchSettingsEnumValueList(FName propName, TArray<FTATMatchSettingsEnumValue>& enumValues) const;

   /// Gets the list of valid tags for a gameplay tag property.
   /// Returns false if the given property is not a gameplay tag or the property does not define a list of valid tags (in which case all tags are technically valid)
   UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "TAT Match Settings")
   bool GetMatchSettingsGameplayTagGroup(FName propName, FTATMatchSettingsGameplayTagGroup& gameplayTagGroup) const;

   /// Gets the list of valid tags for a gameplay tag property.
   /// Returns false if the given property is not a gameplay tag or the property does not define a list of valid tags (in which case all tags are technically valid)
   UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "TAT Match Settings")
   bool GetMatchSettingsGameplayTagList(FName propName, const FTATMatchSettingsQueryContext& context, TArray<FTATMatchSettingsGameplayTag>& tagValues, bool& allowNone) const;

   /// Checks if a gameplay tag is valid for a match settings property.
   /// If the property has a defined gameplay tag group, check to see if the tag is in that group. Otherwise returns true (because all tags are technically valid in that case)
   UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "TAT Match Settings")
   bool IsValidGameplayTagForProperty(FName propName, FGameplayTag gameplayTag, const FTATMatchSettingsQueryContext& context) const;

   /// Retrieves a match settings value by property name.
   /// You need to connect the outValue field to specify the field's type (eg. set a variable of the appropriate type).
   /// The type must match the field's actual type, and it must be a type referenced in the ETATMatchSettingsPropertyType enum.
   /// (If unsure, you can call GetMatchSettingPropertyType to get the correct type)
   UFUNCTION(BlueprintCallable, CustomThunk, Category = "TAT Match Settings", meta = (CustomStructureParam = "outValue", DisplayName = "Get Match Settings Value (By Name)"))
   bool BP_GetMatchSettingsValue(FName propName, uint8& outValue);
   DECLARE_FUNCTION(execBP_GetMatchSettingsValue);

   /// Sets a match settings value by property name.
   /// The new value must match the field's actual type, and it must be a type referenced in the ETATMatchSettingsPropertyType enum.
   /// (If unsure, you can call GetMatchSettingPropertyType to get the correct type)
   ///
   /// NB. This function is intended for use in match settings UI widgets. Match settings are _not_ intended to be writable during a match.
   UFUNCTION(BlueprintCallable, CustomThunk, Category = "TAT Match Settings", meta = (CustomStructureParam = "newValue", DisplayName = "Set Match Settings Value (By Name)"))
   bool BP_SetMatchSettingsValue(FName propName, uint8 newValue);
   DECLARE_FUNCTION(execBP_SetMatchSettingsValue);

   /// Gets the enum type of a match settings property, or nullptr if the property does not exist or is not an enum.
   UEnum* GetMatchSettingsEnumType(FName propName) const;

   /// Gets a match settings value. Only returns true if the property exists and is a bool.
   bool GetMatchSettingsValueAsBool(FName propName, bool& outValue) const;

   /// Gets a match settings value. Only returns true if the property exists and is an int.
   bool GetMatchSettingsValueAsInt(FName propName, int32& outValue) const;

   /// Gets a match settings value. Only returns true if the property exists and is a float or double.
   bool GetMatchSettingsValueAsFloat(FName propName, float& outValue) const;

   /// Gets a match settings value. Only returns true if the property exists and is a float or double.
   bool GetMatchSettingsValueAsDouble(FName propName, double& outValue) const;

   /// Gets a match settings value. Only returns true if the property exists and is a byte (this is intended for untyped enum values).
   bool GetMatchSettingsValueAsByte(FName propName, uint8& outValue) const;

   /// Gets a match settings value. Only returns true if the property exists and is an enum with the specified type.
   /// NB. The enum type must be a UENUM().
   template<typename T>
   FORCEINLINE bool GetMatchSettingsValueAsEnum(FName propName, T& outValue) const
   {
      static_assert(std::is_same_v<std::underlying_type_t<T>, uint8>, "A byte-sized enum type is required");
      UEnum* enumMetadata = StaticEnum<T>();
      if (enumMetadata != nullptr && enumMetadata == GetMatchSettingsEnumType(propName))
      {
         return GetMatchSettingsValueAsByte(propName, reinterpret_cast<uint8&>(outValue));
      }
      return false;
   }

   /// Gets a match settings value. Only returns true if the property exists and is a gameplay tag.
   bool GetMatchSettingsValueAsGameplayTag(FName propName, FGameplayTag& outValue) const;

   /// Gets a match settings value by serializing it as a text value. Primarily intended for use by cheats.
   bool GetMatchSettingsValueAsString(FName propName, FString& outPropValueString) const;

   /// Sets a match settings value by deserializing a text value. Primarily intended for use by cheats.
   bool SetMatchSettingsValueAsString(FName propName, FString newPropValue, const FTATMatchSettingsQueryContext& queryContext = FTATMatchSettingsQueryContext::NullContext, FString* outErrorMessage = nullptr);

   /// Serializes all settings to a byte array that can be sent over the network to sync settings
   void SerializeToByteArray(TArray<uint8>& outByteArray);

   /// Loads settings from a byte array, replacing any existing values in this object
   bool DeserializeFromByteArray(const TArray<uint8>& byteArray);

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

};
