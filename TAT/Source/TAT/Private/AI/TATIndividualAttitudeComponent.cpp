// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/TATIndividualAttitudeComponent.h"

// tat
#include "AI/Perception/TATAIPerceptionSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATIndividualAttitudeComponent)

void UTATIndividualAttitudeComponent::BeginPlay()
{
   Super::BeginPlay();
   UTATAIPerceptionSystem::RegisterResettableKnowledgeContainer(GetWorld(),this);
}

void UTATIndividualAttitudeComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   UTATAIPerceptionSystem::UnregisterResettableKnowledgeContainer(GetWorld(),this);
   Super::EndPlay(endPlayReason);
}

void UTATIndividualAttitudeComponent::ResetKnowledgeOfActor(AActor* actor)
{
   ClearAttitudeTowardsActor(actor, true);
}
