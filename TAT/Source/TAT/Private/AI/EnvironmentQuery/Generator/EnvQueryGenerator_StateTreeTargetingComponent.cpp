// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Generator/EnvQueryGenerator_ActorKnowledge.h"

// tat
#include "AI/TATAIController.h"

// ue
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryGenerator_ActorKnowledge)

#define LOCTEXT_NAMESPACE "EnvQueryGenerator_ConditionalRoleAISet"

UEnvQueryGenerator_ActorKnowledge::UEnvQueryGenerator_ActorKnowledge(const FObjectInitializer& objectInitializer)
{
   ItemType = UEnvQueryItemType_Actor::StaticClass();
}

void UEnvQueryGenerator_ActorKnowledge::GenerateItems(FEnvQueryInstance& queryInstance) const
{
   UObject* queryOwner = queryInstance.Owner.Get();
   if (queryOwner == nullptr)
      return;

   const ATATAIController* aiController = Cast<ATATAIController>(queryOwner);
   if(aiController == nullptr)
      return;

   const UTATKnowledgeComponent* knowledgeComponent = aiController->GetTATKnowledgeComponent();
   if(knowledgeComponent == nullptr)
      return; 

   for (const FTATActorKnowledge& element : knowledgeComponent->GetKnownActors())
   {
      if (!RequireActorIdentified || element.GetDetectionState() == EActorDetectionState::Identified)
      {
         queryInstance.AddItemData<UEnvQueryItemType_Actor>(element.GetActor());
      }
   }
}

FText UEnvQueryGenerator_ActorKnowledge::GetDescriptionTitle() const
{
   return LOCTEXT("EnvQueryGenerator_ActorKnowledge_Title", "Generate items for all known actors in knowledge");
}

#undef LOCTEXT_NAMESPACE
