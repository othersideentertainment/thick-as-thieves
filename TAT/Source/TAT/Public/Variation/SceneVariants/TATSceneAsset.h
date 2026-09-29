// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATSceneAsset.generated.h"


class UTATSceneVariantConfig;

// An asset describing a scene which can have different variants (e.g. a specific room mansion)
UCLASS()
class TAT_API UTATSceneAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   TPair<UTATSceneVariantConfig*, int32> SelectRandomVariant(int32 seed, const FGameplayTagContainer& requiredTraits, const FGameplayTagContainer& allowedTraits) const;
   bool HasVariant(const UTATSceneVariantConfig* variant) const;
   bool HasMatchingVariant(const FGameplayTagContainer& requiredTraits, const FGameplayTagContainer& allowedTraits) const;
   bool HasVariantWith(const FGameplayTagContainer& requiredTraits) const;

   virtual void PostLoad() override;

#if WITH_EDITOR
   virtual void PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent) override;
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   UPROPERTY(EditDefaultsOnly, Category = Scenes)
   TArray<TObjectPtr<UTATSceneVariantConfig>> Variants;

private:
   void _AssignVariantParents();
};
