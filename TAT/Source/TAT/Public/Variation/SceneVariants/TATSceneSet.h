// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATSceneSet.generated.h"

class UTATSceneAsset;
class UTATSceneVariantConfig;
struct FTATSceneVariantSelectionParams;

USTRUCT()
struct TAT_API FTATSceneVariantSelectionPhase
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, meta = (Categories = "MapVariation.SceneTrait"))
   FGameplayTag RequiredTrait;
   
   UPROPERTY(EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
   FInt32Interval SceneCount = FInt32Interval(1,1);
};

USTRUCT()
struct TAT_API FTATSceneSetOverride
{
   GENERATED_BODY()
   
   // Replaces the selection phases for the scene set
   UPROPERTY(EditDefaultsOnly, meta = (TitleProperty="{RequiredTrait} {SceneCount}", EditCondition="OverrideSelectionPhases"))
   TArray<FTATSceneVariantSelectionPhase> SelectionPhases;

   // Additional scene traits that are able to be selected
   UPROPERTY(EditDefaultsOnly, meta = (Categories="SceneTraitCategory"))
   FGameplayTagContainer ExtraSceneTraits;

   UPROPERTY(EditDefaultsOnly, meta=(InlineEditConditionToggle))
   bool OverrideSelectionPhases = true;
};

// A collection of scenes in a map (e.g. all of the room scenes in a mansion)
// A map may have multiple of these
UCLASS()
class TAT_API UTATSceneSetAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   // Will call handler with chosen variants in the order of scenes in the data
   // (including nulls, but only in the case of bad data that would be caught by validation)
   void SelectVariants(const FTATSceneVariantSelectionParams& params, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*, int32)> handler) const;

   int32 EmitVariantsForIndices(TConstArrayView<uint8> indices, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*)> handler) const;

   void ForEachVariant(TFunctionRef<void(const UTATSceneVariantConfig*)> handler) const;

   bool HasScene(const UTATSceneAsset* scene) const;
   bool HasVariant(const UTATSceneVariantConfig* variant) const;
   const UTATSceneAsset* FindSceneForVariant(const UTATSceneVariantConfig* variant) const;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   static void ValidateDuplicates(TConstArrayView<TObjectPtr<UTATSceneSetAsset>> sceneSets, const UObject* owner, FDataValidationContext& context);
#endif

   UPROPERTY(EditDefaultsOnly, Category=Selection, meta = (TitleProperty="{RequiredTrait} {SceneCount}"))
   TArray<FTATSceneVariantSelectionPhase> SelectionPhases;

   UPROPERTY(EditDefaultsOnly, Category=Scenes)
   TArray<TObjectPtr<UTATSceneAsset>> Scenes;
};
