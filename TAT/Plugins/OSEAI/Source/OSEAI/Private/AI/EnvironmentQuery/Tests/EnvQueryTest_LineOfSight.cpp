// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Tests/EnvQueryTest_LineOfSight.h"

// ue
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryTest_LineOfSight) 

#define LOCTEXT_NAMESPACE "EnvQueryTest"

UEnvQueryTest_LineOfSight::UEnvQueryTest_LineOfSight(const FObjectInitializer& ObjectInitializer)
{
   context = UEnvQueryContext_Querier::StaticClass();
   Cost = EEnvTestCost::High;
   ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

   SetWorkOnFloatValues(false);
}

ANavigationData* FindNavigationData(UNavigationSystemV1& NavSys, UObject* Owner)
{
   INavAgentInterface* NavAgent = Cast<INavAgentInterface>(Owner);
   if (NavAgent)
   {
      return NavSys.GetNavDataForProps(NavAgent->GetNavAgentPropertiesRef(), NavAgent->GetNavAgentLocation());
   }

   return NavSys.GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
}

void UEnvQueryTest_LineOfSight::RunTest(FEnvQueryInstance& queryInstance) const
{
   APawn* queryOwner = Cast<APawn>(queryInstance.Owner.Get());
   if (queryOwner == nullptr) { return; }

   BoolValue.BindData(queryOwner, queryInstance.QueryID);

   UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(queryInstance.World);
   if (navSys == nullptr) { return; }

   ANavigationData* navData = FindNavigationData(*navSys, queryOwner);
   if (navData == nullptr) { return; }

   TArray<FVector> contextLocations;
   if (!queryInstance.PrepareContext(context, contextLocations))
   {
      return;
   }

   navData->BeginBatchQuery();
   for (FEnvQueryInstance::ItemIterator it(this, queryInstance); it; ++it)
   {
      const FVector itemLocation = GetItemLocation(queryInstance, it.GetIndex());
      for (int32 contextIndex = 0; contextIndex < contextLocations.Num(); contextIndex++)
      {
         FVector hitLocation = FVector::Zero();
         const bool hasLOS = navSys->NavigationRaycast(queryInstance.World, itemLocation, contextLocations[contextIndex], hitLocation, filterClass, queryOwner->GetController());
         it.SetScore(TestPurpose, FilterType, hasLOS != hide, false);
      }
   }
   navData->FinishBatchQuery();
}

FText UEnvQueryTest_LineOfSight::GetDescriptionTitle() const
{
   return LOCTEXT("LineOfSightTest", "Line Of Sight Test");
}

FText UEnvQueryTest_LineOfSight::GetDescriptionDetails() const
{
   FString inverseString = hide ? "hide" : "watch";

   FFormatNamedArguments args;
   args.Add(TEXT("Behaviour"), FText::FromString(inverseString));

   FText desc = FText::Format(LOCTEXT("LineOfSightTest", "Desired Behaviour: {inverseString}"), args);
   return desc;
}

#undef LOCTEXT_NAMESPACE
