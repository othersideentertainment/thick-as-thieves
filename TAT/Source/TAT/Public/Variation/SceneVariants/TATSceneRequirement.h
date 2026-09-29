// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATSceneRequirement.generated.h"

class IMessageToken;
class UTATSceneAsset;
class UTATSceneSetAsset;
class UTATSceneVariantConfig;


UENUM()
enum class ETATSceneRequirementType : uint8
{
   None,
   // Requires the scene element to be set in the active variant for the given scene
   RequireSceneElement,
   // Requires the given variant to be active
   RequireVariant
};

USTRUCT(BlueprintType)
struct TAT_API FTATSceneRequirement
{
   GENERATED_BODY()

   bool IsNone() const { return Type == ETATSceneRequirementType::None; }

#if WITH_EDITOR
   FString ToDebugString() const;

   void ValidateRequirement(FMessageLog& msgLog, const AActor* owningActor, TFunctionRef<TSharedRef<IMessageToken> ()> createOwnerToken) const;
   void ValidateRequirement(FMessageLog& msgLog, TFunctionRef<TConstArrayView<TObjectPtr<UTATSceneSetAsset>>()> getSceneSets, TFunctionRef<TSharedRef<IMessageToken> ()> createOwnerToken) const;
#endif

   UPROPERTY(EditInstanceOnly, meta = (DisplayThumbnail = false, DisplayName="Scene Requirement"))
   ETATSceneRequirementType Type = ETATSceneRequirementType::None;

   UPROPERTY(EditAnywhere, meta = (DisplayAfter = Scene, Categories = "MapVariation.SceneElement"))
   FGameplayTag ElementTag;

   UPROPERTY(EditInstanceOnly, meta = (EditCondition = "Type == ETATSceneRequirementType::RequireSceneElement", EditConditionHides, DisplayThumbnail = false))
   TObjectPtr<UTATSceneAsset> Scene;

   UPROPERTY(EditInstanceOnly, meta = (EditCondition = "Type == ETATSceneRequirementType::RequireVariant", EditConditionHides, DisplayThumbnail = false))
   TObjectPtr<UTATSceneVariantConfig> Variant;
   

   // using hard pointers for now, since there isn't a situation where they wouldn't already be loaded anyways
};
