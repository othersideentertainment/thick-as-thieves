// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayBehavior.h"

#include "GameplayBehavior_GameplayEvent.generated.h"

class UBehaviorTree;
class AAIController;

UCLASS()
class OSEAI_API UGameplayBehavior_GameplayEvent : public UGameplayBehavior
{
	GENERATED_BODY()

public:
   UGameplayBehavior_GameplayEvent(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

protected:
   // from UGameplayBehavior
	virtual bool Trigger(AActor& inAvatar, const UGameplayBehaviorConfig* config = nullptr, AActor* smartObjectOwner = nullptr) override;
	virtual void EndBehavior(AActor& inAvatar, const bool interrupted) override;

   // for our subclasses
   virtual FGameplayTag _GetGameplayEventTag(AActor& inAvatar, const UGameplayBehaviorConfig* config, AActor* smartObjectOwner) const;
};
