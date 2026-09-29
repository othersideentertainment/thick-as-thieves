// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

//ue4
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"

#include "AnimNotifyState_OSEToolVisualState.generated.h"


UCLASS(Blueprintable, meta = (DisplayName = "OSE Tool Visual State"))
class OSECORE_API UAnimNotifyState_OSEToolVisualState : public UAnimNotifyState
{
   GENERATED_BODY()
public:

   UPROPERTY(EditAnywhere, meta=(Categories="ToolVisualState"))
   FGameplayTag VisualStateTag;

   virtual void NotifyBegin(class USkeletalMeshComponent* meshComp, class UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyEnd(class USkeletalMeshComponent* meshComp, class UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;
};
