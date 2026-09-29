// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayBehavior_AnimationBased.h"
#include "TATGameplayBehaviorConfig_Animation.h"
#include "UObject/Object.h"

#include "TATGameplayBehavior_AnimationBased.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATGameplayBehavior_AnimationBased, Warning, All);

UCLASS()
class TAT_API UTATGameplayBehavior_AnimationBased : public UGameplayBehavior_AnimationBased
{
   GENERATED_BODY()

public:
   virtual bool Trigger(AActor& avatar, const UGameplayBehaviorConfig* config, AActor* smartObjectOwner) override;
   virtual void EndBehavior(AActor& Avatar, const bool bInterrupted) override;
   
protected:
   void AnimationLoadedForAvatar(AActor& actor, UAnimMontage& animMontage, const UGameplayBehaviorConfig* config, AActor* smartObjectOwner);
   
   UFUNCTION(BlueprintNativeEvent)
   void AboutToTrigger(AActor* avatar, AActor* smartObjectOwner);

   bool _bHasEndedBehavior { false };
};
