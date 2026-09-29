// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Perception/TATPerceptionFunctionLibrary.h"

// tat
#include "AI/Perception/TATHearingTypes.h"

// ose
#include "AI/OSEAIFunctionLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPerceptionFunctionLibrary)

bool UTATPerceptionFunctionLibrary::GetStimInfoByGameplayTag(
   const UDataTable* dataTable,
   const FGameplayTag& tag,
   FTATHearingEventStimSettings& stimSettings,
   FName& stimName)
{
   if(dataTable == nullptr)
      return false;
   // I don't like that we have to iterate over the entire data table just to return one value.
   // Making this function so we have _one_ place to change the implementation in the future instead of multiple.
   dataTable->ForeachRow<FTATHearingEventStimSettings>(
                  TEXT("HearingEvent"),
                  [&tag, &stimSettings, &stimName](
                     const FName& key,
                     const FTATHearingEventStimSettings& value)
                  {
                     if (tag == value.Tag)
                     {
                        stimName = key;
                        stimSettings = value;
                     }
                  }
               );
   return stimSettings.IsValid();
}

bool UTATPerceptionFunctionLibrary::CanActorBoundingBoxBeSeenFromLocation(
   const AActor* ownerActor,
   const FBox& bounds,
   const FVector& observerLocation,
   FVector& outSeenLocation,
   int32& numberOfLoSChecksPerformed,
   float& outSightStrength,
   const AActor* ignoreActor)
{
   check(ownerActor);
   const FVector closestPointOnBox = bounds.GetClosestPointTo(observerLocation);
   FHitResult hitResult;
   const bool canBeSeen = UOSEAIFunctionLibrary::SightSenseLineTrace(hitResult, observerLocation, ownerActor, ignoreActor, &closestPointOnBox);
   numberOfLoSChecksPerformed = 1;
   if (canBeSeen)
   {
      outSeenLocation = ownerActor->GetActorLocation();
      outSightStrength = 1.0f; 
      return true;
   }

   outSightStrength = 0;
   return false;
}
