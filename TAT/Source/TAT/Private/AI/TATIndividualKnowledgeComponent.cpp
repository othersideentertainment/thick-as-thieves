// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/TATIndividualKnowledgeComponent.h"

// tat
#include "AI/Perception/TATAIPerceptionSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATIndividualKnowledgeComponent)

void UTATIndividualKnowledgeComponent::BeginPlay()
{
   Super::BeginPlay();
   UTATAIPerceptionSystem::RegisterResettableKnowledgeContainer(GetWorld(),this);
}

void UTATIndividualKnowledgeComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   UTATAIPerceptionSystem::UnregisterResettableKnowledgeContainer(GetWorld(),this);
   Super::EndPlay(endPlayReason);
}

void UTATIndividualKnowledgeComponent::ResetKnowledgeOfActor(AActor* actor)
{
   ClearAllTags(actor);
}
