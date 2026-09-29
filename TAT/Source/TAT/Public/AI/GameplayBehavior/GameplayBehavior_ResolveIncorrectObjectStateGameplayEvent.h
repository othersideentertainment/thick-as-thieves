// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/GameplayBehavior/GameplayBehavior_GameplayEvent.h"

// ue
#include "GameplayBehavior.h"

#include "GameplayBehavior_ResolveIncorrectObjectStateGameplayEvent.generated.h"

class UBehaviorTree;
class AAIController;

UCLASS()
class TAT_API UGameplayBehavior_ResolveIncorrectObjectStateGameplayEvent : public UGameplayBehavior_GameplayEvent
{
	GENERATED_BODY()

public:
   UGameplayBehavior_ResolveIncorrectObjectStateGameplayEvent(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

protected:
   // from UGameplayBehavior_GameplayEvent
   virtual FGameplayTag _GetGameplayEventTag(AActor& inAvatar, const UGameplayBehaviorConfig* config, AActor* smartObjectOwner) const override;
};
