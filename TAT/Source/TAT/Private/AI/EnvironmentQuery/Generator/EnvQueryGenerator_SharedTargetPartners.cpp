// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Generator/EnvQueryGenerator_SharedTargetPartners.h"

// tat
#include "AI/TATKnowledgeComponent.h"
#include "Character/TATCharacterAIBase.h"

// ue
#include "EngineUtils.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryGenerator_SharedTargetPartners)

DEFINE_LOG_CATEGORY_STATIC(LogEnvQueryGenerator_SharedTargetPartners, Log, All);

#define LOCTEXT_NAMESPACE "UEnvQueryGenerator_SharedTargetPartners"

UEnvQueryGenerator_SharedTargetPartners::UEnvQueryGenerator_SharedTargetPartners(const FObjectInitializer& ObjectInitializer) :
   Super(ObjectInitializer)
{
   ItemType = UEnvQueryItemType_Actor::StaticClass();

   GenerateItemsOnlyInRadius.DefaultValue = true;
   SearchRadius.DefaultValue = 500.0f;
   SearchCenter = UEnvQueryContext_Querier::StaticClass();
}

void UEnvQueryGenerator_SharedTargetPartners::GenerateItems(FEnvQueryInstance& queryInstance) const
{
   UObject* queryOwner = queryInstance.Owner.Get();
   if (queryOwner == nullptr)
   {
      return;
   }

   const UTATKnowledgeComponent* knowledgeComponent = UTATKnowledgeComponent::TryGet(Cast<AActor>(queryOwner));
   if (knowledgeComponent == nullptr)
   {
      return;
   }

   const FTATSharedTarget& sharedTarget = knowledgeComponent->GetTargetToShare();
   const TSubclassOf<ATATCharacterAIBase> searchActorClass = sharedTarget.PartnerClass.Get();
   if (searchActorClass == nullptr)
   {
      return;
   }

   UWorld* world = GEngine->GetWorldFromContextObject(queryOwner, EGetWorldErrorMode::LogAndReturnNull);
   if (world == nullptr)
   {
      return;
   }

   GenerateItemsOnlyInRadius.BindData(queryOwner, queryInstance.QueryID);
   const bool useRadius = GenerateItemsOnlyInRadius.GetValue();

   TArray<AActor*> matchingActors;
   if (useRadius)
   {
      TArray<FVector> contextLocations;
      queryInstance.PrepareContext(SearchCenter, contextLocations);

      SearchRadius.BindData(queryOwner, queryInstance.QueryID);
      const float radiusValue = SearchRadius.GetValue();
      const float radiusSq = FMath::Square(radiusValue);

      for (TActorIterator<AActor> itActor = TActorIterator<AActor>(world, searchActorClass); itActor; ++itActor)
      {
         for (int32 ContextIndex = 0; ContextIndex < contextLocations.Num(); ++ContextIndex)
         {
            if (FVector::DistSquared(contextLocations[ContextIndex], itActor->GetActorLocation()) < radiusSq 
               && knowledgeComponent->IsActorValidSharePartner(*itActor))
            {
               matchingActors.Add(*itActor);
               break;
            }
         }
      }
   }
   else
   {	// If radius is not positive, ignore Search Center and Search Radius and just return all actors of class.
      for (TActorIterator<AActor> itActor = TActorIterator<AActor>(world, searchActorClass); itActor; ++itActor)
      {
         if (knowledgeComponent->IsActorValidSharePartner(*itActor))
         {
            matchingActors.Add(*itActor);
         }
      }
   }

   queryInstance.AddItemData<UEnvQueryItemType_Actor>(matchingActors);
}

FText UEnvQueryGenerator_SharedTargetPartners::GetDescriptionTitle() const
{
   FFormatNamedArguments Args;
   Args.Add(TEXT("DescriptionTitle"), Super::GetDescriptionTitle());

   if (!GenerateItemsOnlyInRadius.IsDynamic() && !GenerateItemsOnlyInRadius.GetValue())
   {
      return FText::Format(LOCTEXT("DescriptionGenerateActors", "{DescriptionTitle}: generate set of partner actors"), Args);
   }

   Args.Add(TEXT("DescribeContext"), UEnvQueryTypes::DescribeContext(SearchCenter));
   return FText::Format(LOCTEXT("DescriptionGenerateActorsAroundContext", "{DescriptionTitle}: generate set of partner actors around {DescribeContext}"), Args);
}

FText UEnvQueryGenerator_SharedTargetPartners::GetDescriptionDetails() const
{
   FFormatNamedArguments Args;
   Args.Add(TEXT("Radius"), FText::FromString(SearchRadius.ToString()));

   FText Desc = FText::Format(LOCTEXT("ActorsOfClassDescription", "radius: {Radius}"), Args);

   return Desc;
}

#undef LOCTEXT_NAMESPACE
