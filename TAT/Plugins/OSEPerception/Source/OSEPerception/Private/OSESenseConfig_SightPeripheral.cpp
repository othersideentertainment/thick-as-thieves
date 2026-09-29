// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSESenseConfig_SightPeripheral.h"

// unreal


// ose
#include "OSEPerceptionComponent.h"
#include "OSESense_SightPeripheral.h"
#include "AI/Perception/OSEAIPerceptionHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESenseConfig_SightPeripheral)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif

UOSESenseConfig_SightPeripheral::UOSESenseConfig_SightPeripheral(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   DebugColor = FColor::Green;
   Implementation = UOSESense_SightPeripheral::StaticClass();
}

TSubclassOf<UOSESense> UOSESenseConfig_SightPeripheral::GetSenseImplementation() const
{
   return *Implementation;
}

#if WITH_EDITOR
void UOSESenseConfig_SightPeripheral::PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent)
{
   static const FName NAME_AutoSuccessRangeFromLastSeenLocation = GET_MEMBER_NAME_CHECKED(UOSESenseConfig_SightPeripheral, AutoSuccessRangeFromLastSeenLocation);
   static const FName NAME_AutoSuccessLOSRange = GET_MEMBER_NAME_CHECKED(UOSESenseConfig_SightPeripheral, AutoSuccessLOSRange);
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
      if (propName == NAME_AutoSuccessLOSRange)
      {
         if (AutoSuccessLOSRange< 0)
         {
            AutoSuccessLOSRange = FAISystem::InvalidRange;
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

void UOSESenseConfig_SightPeripheral::DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const
{
   if (perceptionComponent == nullptr || debuggerCategory == nullptr)
   {
      return;
   }

   FColor sightRangeColor = FColor::Green;
   FColor loseSightRangeColor = FColorList::NeonPink;

   // don't call Super implementation on purpose, replace color description line
   debuggerCategory->AddTextLine(
      FString::Printf(TEXT("%s: {%s}%s {white}rangeIN:{%s}%s {white} rangeOUT:{%s}%s"), *GetSenseName(),
         *GetDebugColor().ToString(), *DescribeColorHelper(GetDebugColor()),
         *sightRangeColor.ToString(), *DescribeColorHelper(sightRangeColor),
         *loseSightRangeColor.ToString(), *DescribeColorHelper(loseSightRangeColor))
   );

   const AActor* bodyActor = perceptionComponent->GetBodyActor();
   if (bodyActor != nullptr)
   {
      FVector bodyLocation, bodyFacing;
      perceptionComponent->GetLocationAndDirection(bodyLocation, bodyFacing);

      const float sightRange = SightRadius + PointOfViewBackwardOffset;
      const float loseSightRange = LoseSightRadius + PointOfViewBackwardOffset;

      const FVector rootLocation = bodyLocation - (bodyFacing * PointOfViewBackwardOffset);
      const FVector sightCenter = rootLocation + (bodyFacing * sightRange);
      const FVector loseSightCenter = rootLocation + (bodyFacing * loseSightRange);

      debuggerCategory->AddShape(FGameplayDebuggerShape::MakeCapsule(sightCenter, sightRange, 0, sightRangeColor));
      debuggerCategory->AddShape(FGameplayDebuggerShape::MakeCapsule(loseSightCenter, loseSightRange, 0, loseSightRangeColor));

   }
}
#endif // WITH_GAMEPLAY_DEBUGGER

