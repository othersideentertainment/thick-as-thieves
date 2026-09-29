// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "AnimNotify_Footstep.generated.h"


UCLASS(Blueprintable, meta = (DisplayName = "Footstep"))
class OSECORE_API UAnimNotify_Footstep : public UAnimNotify
{
   GENERATED_BODY()
public:
    UAnimNotify_Footstep(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   /// Overriden to customize the text shown in the AnimNotify keyframing UI.
   virtual FString GetNotifyName_Implementation() const override;

   /// Gets called when the animation reaches the AnimNotify, we post our AkEvent in here.
   virtual void Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

   UPROPERTY(EditAnywhere, meta = (Categories = "SimulatedFootstep"))
   FGameplayTag StrideTag;
};
