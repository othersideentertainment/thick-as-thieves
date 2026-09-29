// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/TATUtilityAIGoalComponent.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "AI/TATKnowledgeComponent.h"
#include "AI/Utility/TATConsiderationInputs.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "AI/Nodes/OSESearchNode.h"

// ue4
#include "Engine/Level.h"
#include "Engine/LevelBounds.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayBehaviorSmartObjectBehaviorDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "SmartObjectComponent.h"
#include "TimerManager.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUtilityAIGoalComponent)

UTATUtilityAIGoalComponent::UTATUtilityAIGoalComponent()
   : Super()
{
}

void UTATUtilityAIGoalComponent::BeginPlay()
{
   Super::BeginPlay();

   _potentialTargetsCache.SetWorld(GetWorld());

   _tatAIController = Cast<ATATAIController>(_aiController);
}

void UTATUtilityAIGoalComponent::AddAllowedEnemyActor(AActor* actor)
{
   _potentialTargetsCache.AddAllowedEnemyActor(actor, _tatAIController);
}

void UTATUtilityAIGoalComponent::RemoveAllowedEnemyActor(AActor* actor)
{
   _potentialTargetsCache.RemoveAllowedEnemyActor(actor);
}

bool UTATUtilityAIGoalComponent::_HasCachedPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   return _potentialTargetsCache.HasCachedPotentialTargetsFor(targeting, targetingGroup);
}

void UTATUtilityAIGoalComponent::_CachePotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup)
{
   _potentialTargetsCache.CachePotentialTargetsFor(targeting, targetingGroup, _tatAIController);
}

const TArray<FUtilityStateTarget>& UTATUtilityAIGoalComponent::_GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   return _potentialTargetsCache.GetPotentialTargetsFor(targeting, targetingGroup);
}

void UTATUtilityAIGoalComponent::_ResetPotentialTargetCache()
{
   _potentialTargetsCache.ResetPotentialTargetCache();
}

