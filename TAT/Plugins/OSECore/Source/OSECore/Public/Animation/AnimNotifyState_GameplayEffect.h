// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "GameplayEffectTypes.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotifyState_GameplayEffect.generated.h"

class UGameplayEffect;
class UOSEAbilitySystemComponent;

// Adds a gameplay effect to a character for the duration of this notify state
UCLASS(Blueprintable, meta = (DisplayName = "GameplayEffect"))
class OSECORE_API UAnimNotifyState_GameplayEffect : public UAnimNotifyState
{
   GENERATED_BODY()

public:

   UAnimNotifyState_GameplayEffect(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());
   virtual FString GetNotifyName_Implementation() const override;
   virtual void NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyTick(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float frameDeltaTime, const FAnimNotifyEventReference& eventReference) override;
   virtual void NotifyEnd(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference) override;

protected:
   UPROPERTY(EditAnywhere)
   TSubclassOf<UGameplayEffect> GameplayEffectClass;

private:
   UOSEAbilitySystemComponent* _GetAbilitySystemComponentFromMesh(USkeletalMeshComponent* meshComp);
};
