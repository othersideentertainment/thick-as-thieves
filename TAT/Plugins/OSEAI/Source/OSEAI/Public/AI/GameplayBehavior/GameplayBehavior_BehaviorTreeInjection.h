// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayBehavior.h"

#include "GameplayBehavior_BehaviorTreeInjection.generated.h"

class UBehaviorTree;
class AAIController;

UCLASS()
class OSEAI_API UGameplayBehavior_BehaviorTreeInjection : public UGameplayBehavior
{
	GENERATED_BODY()

public:
   UGameplayBehavior_BehaviorTreeInjection(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

protected:
	virtual bool Trigger(AActor& inAvatar, const UGameplayBehaviorConfig* config = nullptr, AActor* smartObjectOwner = nullptr) override;
	virtual void EndBehavior(AActor& inAvatar, const bool interrupted) override;

	UPROPERTY()
	AAIController* _aiController;
};
