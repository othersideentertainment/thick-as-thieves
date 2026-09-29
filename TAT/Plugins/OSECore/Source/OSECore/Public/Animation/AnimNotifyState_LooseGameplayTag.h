// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotifyState_LooseGameplayTag.generated.h"

class UOSEAbilitySystemComponent;

/// Adds a loose gameplay tag to a character for the duration of this notify state
UCLASS(Blueprintable, meta = (DisplayName = "Loose Gameplay Tag"))
class OSECORE_API UAnimNotifyState_LooseGameplayTag : public UAnimNotifyState
{
   GENERATED_BODY()

public:

   UAnimNotifyState_LooseGameplayTag(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());
   virtual FString GetNotifyName_Implementation() const override;
   virtual void NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyTick(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float frameDeltaTime, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyEnd(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

protected:
   UPROPERTY(EditAnywhere, meta = (Categories = "AnimNotify"))
   FGameplayTag GameplayTag;

   UPROPERTY(EditAnywhere)
   int TagCount = 1;

private:
   UOSEAbilitySystemComponent* _GetAbilitySystemComponentFromMesh(USkeletalMeshComponent* meshComp);
};
