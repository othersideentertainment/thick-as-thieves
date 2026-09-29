// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueLocationInterface.h"

// ue5
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATSceneVariantConfig.generated.h"

class UTATClueSet;
class UTATSceneAsset;
enum class ETATPrivateSpaceType : uint8;

UENUM()
enum class ETATSceneSpawnerOverrideType : uint8
{
   // Use the default spawn settings on the spawner
   Default,
   // Always spawn
   Always,
   // Never Spawn
   Never,
   // Override the chance on the spawner
   OverrideChance
};

USTRUCT()
struct TAT_API FTATSceneSpawnerOverride
{
   GENERATED_BODY()

   FTATSceneSpawnerOverride() = default;

   explicit FTATSceneSpawnerOverride(ETATSceneSpawnerOverrideType type)
      : Type(type)
   {}

   bool IsDefault() const { return Type == ETATSceneSpawnerOverrideType::Default; }

   UPROPERTY(EditAnywhere)
   ETATSceneSpawnerOverrideType Type = ETATSceneSpawnerOverrideType::Default;

   UPROPERTY(EditAnywhere, meta = (Units = "Percent", ClampMin = "0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", EditCondition = "Type == ETATSceneSpawnerOverrideType::OverrideChance", EditConditionHides))
   float SpawnChancePercent = 100.0f;

   static const FTATSceneSpawnerOverride Default;
   static const FTATSceneSpawnerOverride Never;
};

// An asset representing the the variant of a given scene (e.g. a room with lights out)
UCLASS()
class TAT_API UTATSceneVariantConfig : public UDataAsset, public ITATClueLocationInterface
{
   GENERATED_BODY()

 public:
   const FTATSceneSpawnerOverride& ResolveSpawner(FGameplayTag key) const;
   bool ResolveBool(FGameplayTag key) const;
   ETATPrivateSpaceType GetSpacePrivacy() const { return _spacePrivacy; }

   const FGameplayTagContainer& GetTraits() const { return _sceneTraits; }
   bool Matches(const FGameplayTagContainer& requiredTraits, const FGameplayTagContainer& allowedTraits) const;
   bool HasTraits(const FGameplayTagContainer& requiredTraits) const;

   void Test_SetTraits(FGameplayTagContainer newTraits) { _sceneTraits = MoveTemp(newTraits);}
   // Should only be called by parent scene
   void SetParentScene(const UTATSceneAsset* parentScene) { _parentScene = parentScene; }
   const UTATSceneAsset* GetParentScene() const { return _parentScene; }

   // ITATClueLocationInterface
   virtual const FText& GetClueLocationName() const override final;
   virtual const FGameplayTag& GetClueLocationTag() const override final;
   // ITATClueLocationInterface end
   const TSoftObjectPtr<UTATClueSet>& GetClueSet() const { return _clueSet; }
   
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

private:
   // metadata about the variant, used in variant selection
   UPROPERTY(EditAnywhere, Category = Traits, meta = (Categories = "SceneTraitCategory"))
   FGameplayTagContainer _sceneTraits;

   // overrides for spawners, addressed by element tag
   UPROPERTY(EditAnywhere, Category = Spawners, meta = (ForceInlineRow, Categories = "MapVariation.SceneElement"))
   TMap<FGameplayTag, FTATSceneSpawnerOverride> _spawnerOverrides;

   // Tag of elements that should be enabled (largely non-spawners, but equivalent to spawners with Default)
   UPROPERTY(EditAnywhere, Category = SimpleElements, meta = (Categories = "MapVariation.SceneElement"))
   FGameplayTagContainer _simpleFlags;

   // Specifies the privacy of space volumes associated with the parent scene
   UPROPERTY(EditAnywhere, Category = PrivateSpaces)
   ETATPrivateSpaceType _spacePrivacy = static_cast<ETATPrivateSpaceType>(0);

   // A set of clues that get injected when this variant is active
   // (if you want to have multiple possible sets of clues, ask)
   // CLUESET-AUDIT
   UPROPERTY(EditAnywhere, Category = Clues)
   TSoftObjectPtr<UTATClueSet> _clueSet;

   // OPTIONAL: Clue location tag, only needed if it may want to use clue spawners that are limited to a given location
   UPROPERTY(EditAnywhere, Category = Clues, meta=(Categories = "ClueLocation"))
   FGameplayTag _clueLocationTag;

   UPROPERTY(Transient)
   TObjectPtr<const UTATSceneAsset> _parentScene;
};
