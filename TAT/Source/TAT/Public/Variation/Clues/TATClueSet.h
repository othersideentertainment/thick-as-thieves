// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATClueSetBase.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"

#include "TATClueSet.generated.h"

struct FTATClueInfo;
struct FInstancedStruct;

USTRUCT()
struct TAT_API FTATClueSetEntry
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category="Clues")
   bool Enabled = true;
   
   UPROPERTY(EditDefaultsOnly, Category="Clues", meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATClueInfo"))
   FInstancedStruct Clue;

   bool SerializeFromMismatchedTag(const FPropertyTag& tag, FStructuredArchive::FSlot slot);
   bool ImportTextItem(const TCHAR*& buffer, int32 portFlags, UObject* parent, FOutputDevice* errorText);
};

template<>
struct TStructOpsTypeTraits<FTATClueSetEntry> : public TStructOpsTypeTraitsBase2<FTATClueSetEntry>
{
   enum
   {
      WithImportTextItem = true,
      WithStructuredSerializeFromMismatchedTag = true,
   };
};

// A data asset with a set of clues for the same thing
// (e.g. a jewel is spawned at TBD)
UCLASS()
class TAT_API UTATClueSet : public UTATClueSetBase
{
   GENERATED_BODY()

public:
   // eventually may do something interesting
   virtual void AddRelevantClueViews(const FTATClueSetContext& context, TArray<FConstStructView>& result) const override final;

   using FClueSetArray = TArray<FSoftObjectPath, TInlineAllocator<4>>;
   static FClueSetArray FindMatchingSets(FGameplayTag sourceTag, FGameplayTag locationTag);
   static FSoftObjectPath FindRandomMatchingSet(FGameplayTag sourceTag, FGameplayTag locationTag, int32 seed);
   
   virtual void GetAssetRegistryTags(FAssetRegistryTagsContext context) const override;
   
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& context) const override;
   virtual void VisitClueSetDependencies(TFunctionRef<void(const UTATClueSetBase*)> visitor) const override;
#endif

   // Whether the clue set should be looked up via its tags
   // This could be disabled for two general reasons:
   // 1. A clue set that is meant to be directly referenced (either by other clue sets, or scene variants, ...)
   // 2. The clue set is disabled
   UPROPERTY(EditDefaultsOnly, Category="ClueSet")
   bool UseForLookup = false;
   
   // Tag representing what the clues are about
   UPROPERTY(EditDefaultsOnly, Category="ClueSet", meta = (Categories="ClueSourceCategory", EditCondition="UseForLookup"))
   FGameplayTag ClueSource;

   // Tag representing the location that the target of the clues appear is
   UPROPERTY(EditDefaultsOnly, Category="ClueSet", meta = (Categories="ClueLocation", EditCondition="UseForLookup"))
   FGameplayTag ClueLocation;

   // Optional ClueSet whose clues will also be used when this clue set is used
   // Useful for a common set of shared clues (e.g. generic electrotypes)
   // CONSIDER: should be clue set base? CLUESET-AUDIT
   UPROPERTY(EditDefaultsOnly, Category="ClueSet")
   TObjectPtr<UTATClueSet> ParentClueSet = nullptr;
   
   UPROPERTY(EditDefaultsOnly, Category="Clues")
   TArray<FTATClueSetEntry> Clues;

   // CLUE-WIP: Likely a quick-and-dirty Map of location tags to more specific clues (possibly only readable ones)
};
