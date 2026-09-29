// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// self
#include "UtilityAIManagerWorldSubsystem.generated.h"

class UUtilityAISettings;
class UUtilityAIComponent;

UCLASS(BlueprintType)
class OSEAI_API UUtilityAIManagerWorldSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   void RegisterAIGoalComponent(UUtilityAIComponent* component);
   void DeregisterAIGoalComponent(UUtilityAIComponent* component);
   
   void RegisterAIBehaviorComponent(UUtilityAIComponent* component);
   void DeregisterAIBehaviorComponent(UUtilityAIComponent* component);
   
   virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UUtilityAIManagerComponent, STATGROUP_Tickables); }
   
private:
   UPROPERTY(Transient)
   TArray<UUtilityAIComponent*>  _goalComponents;
   int _nextGoalIndex { 0 };

   UPROPERTY(Transient)
   TArray<UUtilityAIComponent*>  _behaviorComponents;
   int _nextBehaviorIndex { 0 };

   UPROPERTY(Transient)
   const UUtilityAISettings* _utilityAISettings;
};
