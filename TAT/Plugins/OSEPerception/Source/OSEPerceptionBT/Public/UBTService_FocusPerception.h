// Copyright Epic Games, Inc. All Rights Reserved.
// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "UBTService_FocusPerception.generated.h"



class AActor;
class UBlackboardComponent;
class UOSEPerceptionComponent;

struct FBTFocusPerceptionMemory
{
	AActor* FocusActorSet;
	FVector FocusLocationSet;
	bool bActorSet;

	void Reset()
	{
		FocusActorSet = nullptr;
		FocusLocationSet = FAISystem::InvalidLocation;
		bActorSet = false;
	}
};

/**
 * 
 */
UCLASS(hidecategories=(Service))
class OSEPERCEPTIONBT_API UUBTService_FocusPerception : public UBTService_BlackboardBase
{
	GENERATED_BODY()
	
   protected:
	// not exposed to users on purpose. Here to make reusing focus-setting mechanics by derived classes possible
	UPROPERTY()
	uint8 FocusPriority;

	UUBTService_FocusPerception(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTFocusPerceptionMemory); }
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

   void SetFocus(AAIController* OwnerController, const UBlackboardComponent* MyBlackboard, const UOSEPerceptionComponent* MyPerception, FBTFocusPerceptionMemory* MyMemory);

   virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

	EBlackboardNotificationResult OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID);

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
};
