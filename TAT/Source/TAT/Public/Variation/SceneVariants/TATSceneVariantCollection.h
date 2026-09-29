// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATSceneVariantCollection.generated.h"

class FMessageLog;
class UTATSceneAsset;
class UTATSceneSetAsset;
class UTATSceneVariantConfig;
struct FTATSceneRequirement;
struct FTATSceneSetOverride;
struct FTATSceneSpawnerOverride;

struct FTATScenePhaseSummary
{
   TWeakObjectPtr<const UTATSceneSetAsset> SceneSet = nullptr;
   FGameplayTag PhaseTag;
   int32 NumChosen = 0;
   int32 NumDesired = 0;
   int32 Min = 0;
   int32 Max = 0;
};

USTRUCT()
struct FTATSceneTraitWithLimit
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, meta = (Categories = "SceneTraitCategory"))
   FGameplayTag Trait;

   UPROPERTY(EditAnywhere, meta= (UIMin = 0, ClampMin = 0))
   int32 Limit = 0;

   bool operator==(const FGameplayTag& tag) const
   {
      return Trait == tag;
   }

   static void Add(TArray<FTATSceneTraitWithLimit>& limitedTraits, FGameplayTag tag, int32 count = 1)
   {
      if (FTATSceneTraitWithLimit* found = limitedTraits.FindByKey(tag))
      {
         found->Limit += count;
      }
      else
      {
         limitedTraits.Add({ tag, count });
      }
   }
};

struct FTATSceneVariantSelectionParams
{
   int32 Seed = 0;
   bool AllowWarningLogs = false;
   TConstArrayView<FGameplayTag> ExtraAllowedTraits;
   TConstArrayView<FTATSceneTraitWithLimit> LimitedTraits;
   TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> VariantOverrides;
   const TMap<TObjectPtr<UTATSceneSetAsset>, FTATSceneSetOverride>* SceneSetOverrides = nullptr; 
   TArray<FTATScenePhaseSummary>* OutPhaseSummary = nullptr;
};


// A resolved set of scene variants that have been chosen to be active
USTRUCT()
struct TAT_API FTATSceneVariantCollection
{
   GENERATED_BODY()

   static const FTATSceneVariantCollection& Empty();

   const FTATSceneSpawnerOverride& ResolveSpawner(const FTATSceneRequirement& requirement) const;
   bool ResolveBool(const FTATSceneRequirement& requirement) const;
   const UTATSceneVariantConfig* FindVariantForScene(const UTATSceneAsset* scene) const;

   void Reset();
   void AddVariant(const UTATSceneAsset* scene, const UTATSceneVariantConfig* variant);
   void WriteToMessageLog(FMessageLog& msgLog) const;
   void WriteToLog() const;
   void WriteToOutput(FOutputDevice& outputDevice) const;
   FString ToCompactString() const;

   int32 Num() const { return _variants.Num(); }
   const TArray<const UTATSceneVariantConfig*>& GetVariants() const { return _variants; }

private:
   UPROPERTY()
   TArray<const UTATSceneVariantConfig*> _variants;

   UPROPERTY()
   TMap<const UTATSceneAsset*, const UTATSceneVariantConfig*> _sceneToConfig;
};

// Indices of chosen variants for each scene in the order they appear in spawn data
// Should include indices even for invalid data (which should be ignored)
//
// Use-case is for replication to clients
USTRUCT()
struct TAT_API FTATSceneVariantIndices
{
   GENERATED_BODY()

   bool IsInitialized() const { return _initialized; }
   void MarkInitialized() { _initialized = true; }
   void AddVariant(int32 index)
   {
      checkf(index >= 0 && index <= TNumericLimits<uint8>::Max(), TEXT("Input value %d will exceed uint8 limits"), index);
      _variantSelections.Add(static_cast<uint8>(index));
   }

   bool NetSerialize(FArchive& ar, UPackageMap* packageMap, bool& outSuccess);

   const TArray<uint8>& GetVariantSelections() const { return _variantSelections; }

private:
   UPROPERTY()
   TArray<uint8> _variantSelections;

   UPROPERTY()
   bool _initialized = false;
};

template<>
struct TStructOpsTypeTraits<FTATSceneVariantIndices> : public TStructOpsTypeTraitsBase2<FTATSceneVariantIndices>
{
   enum
   {
      WithNetSerializer = true,
      WithNetSharedSerialization = true
   };
};
