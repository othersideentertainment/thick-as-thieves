// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"

#include "TATAnimSetTagTriggers.generated.h"

struct FTATAnimSetOverrides;

USTRUCT()
struct FTATAnimSetTagTrigger
{
   GENERATED_BODY()

   // Tag on the character that would trigger the anim-set
   UPROPERTY(EditAnywhere)
   FGameplayTag RequiredTag;

   // Tag of the anim set to request
   UPROPERTY(EditAnywhere, meta = (Category="AnimSet"))
   FGameplayTag AnimSetTag;
   
   // Higher is more important, can be negative
   // Tools are zero, so probably don't use that
   UPROPERTY(EditAnywhere)
   int Priority = 1;

   bool operator==(const FGameplayTag& requiredTag) const { return RequiredTag == requiredTag; }
};

// An asset that describes anim sets that are triggered when a character
// has a given gameplay tag
UCLASS()
class TAT_API UTATAnimSetTagTriggerSet : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category=Triggers, meta=(TitleProperty="{RequiredTag} -> {AnimSetTag}"))
   TArray<FTATAnimSetTagTrigger> AnimSetTriggers;

   // Should be called when tag is added or removed (e.g. EGameplayTagEventType::NewOrRemoved)
   void ApplyTagCountChange(const FGameplayTag tag, int32 newTagCount, FTATAnimSetOverrides& overrides) const;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
