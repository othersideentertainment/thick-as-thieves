// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSEIndividualKnowledgeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEIndividualKnowledgeComponent)

UOSEIndividualKnowledgeComponent::UOSEIndividualKnowledgeComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UOSEIndividualKnowledgeComponent::AddTag(const AActor* actor, const FGameplayTag tag, const float expirationTime)
{
   FIndividualKnowledge& knowledge = FindOrAddKnowledgeForActor(actor);
   knowledge.GameplayTagContainer.AddTag(tag);
   if(expirationTime > 0)
   {
      FTimerHandle tagRemovalTimer;
      GetWorld()->GetTimerManager().SetTimer(
         tagRemovalTimer,
         FTimerDelegate::CreateUObject(this, &ThisClass::OnRemoveAfterTime, actor, tag),
         expirationTime,
         false
      );
   }
}

bool UOSEIndividualKnowledgeComponent::HasTag(const AActor* actor, const FGameplayTag tag)
{
   const FIndividualKnowledge& knowledge = FindOrAddKnowledgeForActor(actor);
   return knowledge.GameplayTagContainer.HasTag(tag);
}

void UOSEIndividualKnowledgeComponent::RemoveTag(const AActor* actor, const FGameplayTag tag)
{
   FIndividualKnowledge& knowledge = FindOrAddKnowledgeForActor(actor);
   knowledge.GameplayTagContainer.RemoveTag(tag);
}

void UOSEIndividualKnowledgeComponent::ClearAllTags(const AActor* actor)
{
   FIndividualKnowledge& knowledge = FindOrAddKnowledgeForActor(actor);
   knowledge.GameplayTagContainer.Reset();
}

const FIndividualKnowledge* UOSEIndividualKnowledgeComponent::FindKnowledgeForActor(const AActor* actor) const
{
   return _knowledgeMap.Find(actor);
}

FIndividualKnowledge& UOSEIndividualKnowledgeComponent::FindOrAddKnowledgeForActor(const AActor* actor)
{
   FIndividualKnowledge& knowledge = _knowledgeMap.FindOrAdd(actor);
   return knowledge;
}

void UOSEIndividualKnowledgeComponent::OnRemoveAfterTime(const AActor* actor, FGameplayTag gameplayTag)
{
   RemoveTag(actor, gameplayTag);
}

