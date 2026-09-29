// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Tests/EnvQueryTest_HasLineOfSight.h"

#include "AIController.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryTest_HasLineOfSight) 

#define LOCTEXT_NAMESPACE "EnvQueryTest"


UEnvQueryTest_HasLineOfSight::UEnvQueryTest_HasLineOfSight(const FObjectInitializer& ObjectInitializer)
{
   Context = UEnvQueryContext_Querier::StaticClass();
   Cost = EEnvTestCost::High;
   ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

   SetWorkOnFloatValues(false);
}

void UEnvQueryTest_HasLineOfSight::RunTest(FEnvQueryInstance& queryInstance) const
{
   APawn* queryOwner = Cast<APawn>(queryInstance.Owner.Get());
   if (queryOwner == nullptr) { return; }

   AController* ownerController = queryOwner->GetController();
   if (ownerController == nullptr) { return; }

   BoolValue.BindData(queryOwner, queryInstance.QueryID);

   TArray<AActor*> contextActors;
   if (!queryInstance.PrepareContext(Context, contextActors))
   {
      return;
   }

   //ignore the query owner in the collision, that way if the owner is standing on the EQS location it wont block its own LOS.
   FCollisionQueryParams CollisionParms(SCENE_QUERY_STAT(EQSTestHasLineOfSight), true, queryOwner);

   for (FEnvQueryInstance::ItemIterator it(this, queryInstance); it; ++it)
   {
      const FVector itemLocation = GetItemLocation(queryInstance, it.GetIndex());
      for (int32 contextIndex = 0; contextIndex < contextActors.Num(); contextIndex++)
      {
         APawn* other = Cast<APawn>(contextActors[contextIndex]);
         if(other == nullptr) { continue; }

         FVector hitLocation = FVector::Zero();
         //Requesting your own target location is a bit odd, but thinking if we override target location it would by default would return center mass
         // and having the Querier request target location might produce unexpected results depending on what logic we add in future.
         //(as the querier isn't at the location of the LOS yet) 
         FVector TargetLocation = other->GetTargetLocation(other);
         FVector eyeViewPoint;
         FRotator eyeRotation;
         ownerController->GetActorEyesViewPoint(eyeViewPoint, eyeRotation);
         FVector potentialViewPoint = itemLocation;
         //add height of eye offset of the character to potential EQS location.
         potentialViewPoint.Z += eyeViewPoint.Z;

         //Line trace looking for a collision against the target.  If we have a collision we have LOS. 
         FHitResult OutHit;
         GetWorld()->LineTraceSingleByChannel(OutHit, potentialViewPoint, TargetLocation, ECC_Visibility, CollisionParms);
         const bool hasLOS = OutHit.GetActor() == other;

         it.SetScore(TestPurpose, FilterType, hasLOS != Hide, true);
      }
   }
}

FText UEnvQueryTest_HasLineOfSight::GetDescriptionTitle() const
{
   return LOCTEXT("HasLineOfSightTest", "Has Line Of Sight Test");
}

FText UEnvQueryTest_HasLineOfSight::GetDescriptionDetails() const
{
   FString inverseString = Hide ? "hide" : "watch";

   FFormatNamedArguments args;
   args.Add(TEXT("Behaviour"), FText::FromString(inverseString));

   FText desc = FText::Format(LOCTEXT("HasLineOfSightTest", "Desired Behaviour: {inverseString}"), args);
   return desc;
}
