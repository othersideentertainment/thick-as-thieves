// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"

#include "TATMatchSettingsPropertyDef.generated.h"

/// All supported match settings property types
UENUM(BlueprintType)
enum class ETATMatchSettingsPropertyType : uint8
{
   Invalid = 0,
   Bool,
   Integer,
   Float,
   Enum,
   GameplayTag,
};

/// Used to provide enum value metadata for match settings properties with enum types.
USTRUCT(BlueprintType)
struct TAT_API FTATMatchSettingsEnumValue
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Enum Value")
   FText Label;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Enum Value")
   uint8 ValueAsByte = 0;
};

/// Used to provide value metadata for gameplay tag match settings properties.
USTRUCT(BlueprintType)
struct TAT_API FTATMatchSettingsGameplayTag
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Gameplay Tag")
   FGameplayTag Tag;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Gameplay Tag")
   FText Label;
};

/// Extra context to pass through for some match settings queries (eg. custom gameplay tag groups)
USTRUCT(BlueprintType)
struct TAT_API FTATMatchSettingsQueryContext
{
   GENERATED_BODY()

   static const FTATMatchSettingsQueryContext NullContext;

   FTATMatchSettingsQueryContext() = default;
   explicit FTATMatchSettingsQueryContext(const TSoftObjectPtr<UWorld>& map, const UObject* worldContext) : Map(map), WorldContextObject(worldContext) {}

   static FTATMatchSettingsQueryContext MakeFromWorldContext(const UObject* contextObject);

   /// What level is selected (either the selected level in the match setup screen, or the current level if in a match)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Query Context")
   TSoftObjectPtr<UWorld> Map;

   UPROPERTY(BlueprintReadWrite, Transient, Category = "TAT Match Settings Query Context")
   TWeakObjectPtr<const UObject> WorldContextObject = nullptr;
};

/// Allows providing custom functionality for gameplay tag groups
UCLASS(Blueprintable)
class TAT_API UTATMatchSettingsGameplayTagQuery : public UObject
{
   GENERATED_BODY()

public:
   /// Gets all valid gameplay tags for a gameplay tag group
   UFUNCTION(BlueprintNativeEvent)
   void GetGameplayTags(const FTATMatchSettingsQueryContext& context, TArray<FTATMatchSettingsGameplayTag>& gameplayTags) const;
   virtual void GetGameplayTags_Implementation(const FTATMatchSettingsQueryContext& context, TArray<FTATMatchSettingsGameplayTag>& gameplayTags) const {}

   /// Checks if a given gameplay tag is valid for this tag group
   UFUNCTION(BlueprintNativeEvent)
   bool ContainsGameplayTag(const FTATMatchSettingsQueryContext& context, FGameplayTag tag) const;
   virtual bool ContainsGameplayTag_Implementation(const FTATMatchSettingsQueryContext& context, FGameplayTag tag) const { return true; }
};

/// Data table row used to define all valid gameplay tags for a match settings type.
USTRUCT(BlueprintType)
struct TAT_API FTATMatchSettingsGameplayTagGroup : public FTableRowBase
{
   GENERATED_BODY()

public:
   /// Add a "None" entry that clears the gameplay tag value
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT Match Settings Gameplay Tag Group")
   bool AllowNone = false;

   /// When allowing a "None" entry, what label should it use in the UI?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT Match Settings Gameplay Tag Group", meta = (EditCondition = "AllowNone"))
   FText NoneLabel;

   /// List of all valid gameplay tags for this group
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT Match Settings Gameplay Tag Group")
   TArray<FTATMatchSettingsGameplayTag> GameplayTags;

   /// If specified, you can use this implement a function to find additional valid tags for this tag group
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT Match Settings Gameplay Tag Group")
   TSubclassOf<UTATMatchSettingsGameplayTagQuery> CustomQuery;

   /// Only relevant for match settings validation. If assigned, assume that any tag with this prefix is a valid value.
   /// It is strongly recommended you set this if using a custom query.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT Match Settings Gameplay Tag Group")
   FGameplayTag ValidationTagPrefix;

   /// Gets the CDO of the CustomQuery object, if any
   const UTATMatchSettingsGameplayTagQuery* GetCustomQueryObject() const;

   /// Gets all gameplay tags in this tag group, including those returned by the custom query object (if any)
   void GetAllGameplayTags(TArray<FTATMatchSettingsGameplayTag>& outTags, const FTATMatchSettingsQueryContext& queryContext = FTATMatchSettingsQueryContext::NullContext, bool includeNone = true) const;

   /// Checks if this group contains a specific gameplay tag
   bool ContainsGameplayTag(FGameplayTag gameplayTag, const FTATMatchSettingsQueryContext& queryContext = FTATMatchSettingsQueryContext::NullContext) const;

   /// Returns all valid tags (including None if AllowNone is true) as a comma-separated string.
   /// Mostly just useful for logging error messages and cheats
   FString ToTagListDebugString(const TCHAR* separator = TEXT(", ")) const;
};

/// Property metadata for match settings fields. Used to construct a UMG widget for the property.
USTRUCT(BlueprintType)
struct TAT_API FTATMatchSettingsPropertyDef
{
   GENERATED_BODY()

   /// Name of the UPROPERTY this is metadata for
   UPROPERTY(BlueprintReadWrite, Category = "TAT Match Settings Property")
   FName Name = NAME_None;

   /// Type of the UPROPERTY field
   UPROPERTY(BlueprintReadWrite, Category = "TAT Match Settings Property")
   ETATMatchSettingsPropertyType Type = ETATMatchSettingsPropertyType::Invalid;

   /// Player-facing field label
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property")
   FText Label;

   /// Player-facing field description
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property")
   FText Description;

   /// If enabled, the default value of this setting will be a random valid value.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property")
   bool RandomizeDefaultValue = false;

   /// For GameplayTag property types, this is the data table row that defines all gameplay tags (and their player-facing labels) that are valid for this property.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property")
   FDataTableRowHandle GameplayTagGroup;

   /// Enable field minimum value limit?
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property", meta = (InlineEditConditionToggle))
   bool UseMinValue = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property", meta = (EditCondition = "UseMinValue"))
   double MinValue = 0;

   /// Enable field maximum value limit?
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property", meta = (InlineEditConditionToggle))
   bool UseMaxValue = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TAT Match Settings Property", meta = (EditCondition = "UseMaxValue"))
   double MaxValue = 0;

   template<typename T>
   FORCEINLINE TOptional<T> GetMinValue() const { return UseMinValue ? TOptional(static_cast<T>(MinValue)) : NullOpt; }

   template<typename T>
   FORCEINLINE TOptional<T> GetMaxValue() const { return UseMaxValue ? TOptional(static_cast<T>(MaxValue)) : NullOpt; }
};
