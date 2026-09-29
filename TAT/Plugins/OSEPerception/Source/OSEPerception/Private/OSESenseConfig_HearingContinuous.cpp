// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSESenseConfig_HearingContinuous.h"

// unreal


// ose
#include "OSEPerceptionComponent.h"
#include "OSESense_HearingContinuous.h"
#include "AI/Perception/OSEAIPerceptionHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESenseConfig_HearingContinuous)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif

UOSESenseConfig_HearingContinuous::UOSESenseConfig_HearingContinuous(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   DebugColor = FColor::Green;
   Implementation = UOSESense_HearingContinuous::StaticClass();
}

TSubclassOf<UOSESense> UOSESenseConfig_HearingContinuous::GetSenseImplementation() const
{
   return *Implementation;
}

#if WITH_EDITOR
void UOSESenseConfig_HearingContinuous::PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent)
{
   static const FName NAME_AutoSuccessRangeFromLastSeenLocation = GET_MEMBER_NAME_CHECKED(UOSESenseConfig_HearingContinuous, AutoSuccessRangeFromLastSeenLocation);
   
   Super::PostEditChangeProperty(propertyChangedEvent);
   
   if (propertyChangedEvent.Property)
   {
      const FName propName = propertyChangedEvent.Property->GetFName();
      if (propName == NAME_AutoSuccessRangeFromLastSeenLocation)
      {
         if (AutoSuccessRangeFromLastSeenLocation < 0)
         {
            AutoSuccessRangeFromLastSeenLocation = FAISystem::InvalidRange;
         }
      }
   }
}
#endif // WITH_EDITOR

#if WITH_GAMEPLAY_DEBUGGER
static FString DescribeColorHelper(const FColor& color)
{
   const int32 maxColors = GColorList.GetColorsNum();
   for (int32 idx = 0; idx < maxColors; idx++)
   {
      if (color == GColorList.GetFColorByIndex(idx))
      {
         return GColorList.GetColorNameByIndex(idx);
      }
   }

   return FString(TEXT("color"));
}

void UOSESenseConfig_HearingContinuous::DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const
{
   if (perceptionComponent == nullptr || debuggerCategory == nullptr)
   {
      return;
   }

   FColor HearingRangeColor = FColor::Green;
   FColor loseHearingRangeColor = FColorList::NeonPink;

   // don't call Super implementation on purpose, replace color description line
   debuggerCategory->AddTextLine(
      FString::Printf(TEXT("%s: {%s}%s {white}rangeIN:{%s}%s {white} rangeOUT:{%s}%s"), *GetSenseName(),
         *GetDebugColor().ToString(), *DescribeColorHelper(GetDebugColor()),
         *HearingRangeColor.ToString(), *DescribeColorHelper(HearingRangeColor),
         *loseHearingRangeColor.ToString(), *DescribeColorHelper(loseHearingRangeColor))
   );

   const AActor* bodyActor = perceptionComponent->GetBodyActor();
   if (bodyActor != nullptr)
   {
      FVector bodyLocation, bodyFacing;
      perceptionComponent->GetLocationAndDirection(bodyLocation, bodyFacing);

      const float HearingRange = HearingRadius;
      const float loseHearingRange = LoseHearingRadius;

      const FVector HearingCenter = bodyLocation ;
      const FVector loseHearingCenter = bodyLocation;

      debuggerCategory->AddShape(FGameplayDebuggerShape::MakeCapsule(HearingCenter, HearingRange, 0, HearingRangeColor));
      debuggerCategory->AddShape(FGameplayDebuggerShape::MakeCapsule(loseHearingCenter, loseHearingRange, 0, loseHearingRangeColor));

   }
}
#endif // WITH_GAMEPLAY_DEBUGGER

