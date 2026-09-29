// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


// ose
#include "OSEIndividualKnowledgeBlueprintFunctionLibrary.h"

#include "OSEIndividualKnowledgeInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEIndividualKnowledgeBlueprintFunctionLibrary)

UOSEIndividualKnowledgeComponent* UOSEIndividualKnowledgeBlueprintFunctionLibrary::GetIndividualKnowledgeComponent(
   AActor* actor)
{
   const IOSEIndividualKnowledgeInterface* individualKnowledgeInterface = Cast<IOSEIndividualKnowledgeInterface>(actor);
   if(individualKnowledgeInterface == nullptr)
      return nullptr;   
   return individualKnowledgeInterface->GetIndividualKnowledgeComponent();
}

bool UOSEIndividualKnowledgeBlueprintFunctionLibrary::SetIndividualKnowledge(AActor* actor,
                                                                             AActor* target,
                                                                             FGameplayTag tag)
{
   UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = GetIndividualKnowledgeComponent(actor);
   if(individualKnowledgeComponent == nullptr)
      return false;
   individualKnowledgeComponent->AddTag(target, tag);
   return true;
}

bool UOSEIndividualKnowledgeBlueprintFunctionLibrary::HasIndividualKnowledge(AActor* actor,
   AActor* target,
   FGameplayTag tag)
{
   UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = GetIndividualKnowledgeComponent(actor);
   if(individualKnowledgeComponent == nullptr)
      return false;
   return individualKnowledgeComponent->HasTag(target, tag);
}

bool UOSEIndividualKnowledgeBlueprintFunctionLibrary::UnSetIndividualKnowledgeForActor(
   AActor* actor,
   AActor* target,
   const FGameplayTag tag)
{
   UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = GetIndividualKnowledgeComponent(actor);
   if(individualKnowledgeComponent == nullptr)
      return false;
   individualKnowledgeComponent->RemoveTag(target, tag);
   return true;
}

void UOSEIndividualKnowledgeBlueprintFunctionLibrary::ClearIndividualKnowledgeOfActor(AActor* actor, const AActor* target)
{
   UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = GetIndividualKnowledgeComponent(actor);
   if(individualKnowledgeComponent == nullptr)
      return;
   individualKnowledgeComponent->ClearAllTags(target);
}
