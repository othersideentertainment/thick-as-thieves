// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Generators/EnvQueryGenerator_SearchNodes.h"

// ose
#include "AI/Nodes/OSESearchNode.h"
#include "AI/Nodes/OSESearchNodeManagerComponent.h"

// ue4
#include "EngineGlobals.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryGenerator_SearchNodes)

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryGenerator_SearchNodes::UEnvQueryGenerator_SearchNodes(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   ItemType = UEnvQueryItemType_Actor::StaticClass();

   GenerateOnlyActorsInRadius.DefaultValue = true;
   SearchRadius.DefaultValue = 1500.0f;
   SearchCenter = UEnvQueryContext_Querier::StaticClass();
}

void UEnvQueryGenerator_SearchNodes::GenerateItems(FEnvQueryInstance& queryInstance) const
{
   UObject* queryOwner = queryInstance.Owner.Get();
   if (!queryOwner)
      return;
   
   UWorld* world = GEngine->GetWorldFromContextObject(queryOwner, EGetWorldErrorMode::LogAndReturnNull);
   if (!world)
      return;

   UOSESearchNodeManagerComponent* searchNodeManager = world->GetSubsystem<UOSESearchNodeManagerComponent>();

   GenerateOnlyActorsInRadius.BindData(queryOwner, queryInstance.QueryID);
   bool useRadius = GenerateOnlyActorsInRadius.GetValue();

   TArray<AActor*> matchingActors;
   if (useRadius)
   {
      TArray<FVector> contextLocations;
      queryInstance.PrepareContext(SearchCenter, contextLocations);

      SearchRadius.BindData(queryOwner, queryInstance.QueryID);
      const float radiusValue = SearchRadius.GetValue();
      const float radiusSq = FMath::Square(radiusValue);

      if (searchNodeManager)
      {
         for (const TWeakObjectPtr<AOSESearchNode>& searchNodePtr : searchNodeManager->GetSearchNodes())
         {
            if (AOSESearchNode* searchNode = searchNodePtr.Get())
            {
               // NOTE: Intentionally not feeding search node cooldowns into this check here, since we want this to happen somewhat infrequently.  The
               // utility state that drives an AI to a search node should check the cooldown in real-time.

               for (int32 contextIndex = 0; contextIndex < contextLocations.Num(); ++contextIndex)
               {
                  if (FVector::DistSquared(contextLocations[contextIndex], searchNode->GetActorLocation()) < radiusSq)
                  {
                     matchingActors.Add(searchNode);
                  }
               }
            }
         }
      }
      else
      {
#if WITH_EDITOR
         // useful for the EQS testing pawn to fall back to FindActorsOfClass at edit-time
         if (GEngine->IsEditor())
         {
            for (TActorIterator<AActor> itActor = TActorIterator<AActor>(world, AOSESearchNode::StaticClass()); itActor; ++itActor)
            {
               for (int32 contextIndex = 0; contextIndex < contextLocations.Num(); ++contextIndex)
               {
                  if (FVector::DistSquared(contextLocations[contextIndex], itActor->GetActorLocation()) < radiusSq)
                  {
                     matchingActors.Add(*itActor);
                     break;
                  }
               }
            }
         }
#endif // WITH_EDITOR
      }
   }
   else
   {
      // If radius is not positive, ignore Search Center and Search Radius and just return all actors of class.
      if (searchNodeManager)
      {
         for (const TWeakObjectPtr<AOSESearchNode>& searchNodePtr : searchNodeManager->GetSearchNodes())
         {
            if (AOSESearchNode* searchNode = searchNodePtr.Get())
            {
               matchingActors.Add(searchNode);
            }
         }
      }
      else
      {
#if WITH_EDITOR
         // useful for the EQS testing pawn to fall back to FindActorsOfClass at edit-time
         if (GEngine->IsEditor())
         {
            for (TActorIterator<AActor> itActor = TActorIterator<AActor>(world, AOSESearchNode::StaticClass()); itActor; ++itActor)
            {
               matchingActors.Add(*itActor);
            }
         }
#endif // WITH_EDITOR
      }
   }

   queryInstance.AddItemData<UEnvQueryItemType_Actor>(matchingActors);
}

FText UEnvQueryGenerator_SearchNodes::GetDescriptionTitle() const
{
   FFormatNamedArguments args;
   args.Add(TEXT("DescriptionTitle"), Super::GetDescriptionTitle());

   if (!GenerateOnlyActorsInRadius.IsDynamic() && !GenerateOnlyActorsInRadius.GetValue())
   {
      return FText::Format(LOCTEXT("DescriptionGenerateSearchNodes", "{DescriptionTitle}: generate set of search nodes"), args);
   }

   args.Add(TEXT("DescribeContext"), UEnvQueryTypes::DescribeContext(SearchCenter));
   return FText::Format(LOCTEXT("DescriptionGenerateSearchNodesAroundContext", "{DescriptionTitle}: generate set of search nodes around {DescribeContext}"), args);
};

FText UEnvQueryGenerator_SearchNodes::GetDescriptionDetails() const
{
   FFormatNamedArguments args;
   args.Add(TEXT("Radius"), FText::FromString(SearchRadius.ToString()));

   FText desc = FText::Format(LOCTEXT("SearchNodeDescription", "radius: {Radius}"), args);
   return desc;
}

#undef LOCTEXT_NAMESPACE

