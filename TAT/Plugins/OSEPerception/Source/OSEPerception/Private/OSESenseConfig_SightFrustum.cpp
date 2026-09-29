// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSESenseConfig_SightFrustum.h"

// unreal


// ose
//#include "OSECommon.h"
#include "OSEPerceptionComponent.h"
#include "OSESense_SightFrustum.h"
#include "AI/Perception/OSEAIPerceptionHelpers.h"
//#include "AI/OSEAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESenseConfig_SightFrustum)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif


UOSESenseConfig_SightFrustum::UOSESenseConfig_SightFrustum(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , AutoSuccessRangeFromLastSeenLocation(FAISystem::InvalidRange)
{
   DebugColor = FColor::Green;
   Implementation = UOSESense_SightFrustum::StaticClass();
}

TSubclassOf<UOSESense> UOSESenseConfig_SightFrustum::GetSenseImplementation() const
{
   return *Implementation;
}

#if WITH_EDITOR
void UOSESenseConfig_SightFrustum::PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent)
{
   static const FName NAME_AutoSuccessRangeFromLastSeenLocation = GET_MEMBER_NAME_CHECKED(UOSESenseConfig_SightFrustum, AutoSuccessRangeFromLastSeenLocation);
   static const FName NAME_AutoSuccessLOSRange = GET_MEMBER_NAME_CHECKED(UOSESenseConfig_SightFrustum, AutoSuccessLOSRange);
   
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

void UOSESenseConfig_SightFrustum::DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const
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

      const FVector rootLocation = bodyLocation - (bodyFacing * FrustumSettings.PointOfViewBackwardOffset);

      FMatrix sightProjection = OSEAIPerceptionHelpers::CreateProjectionMatrix(rootLocation, bodyFacing.Rotation(), FrustumSettings.VisionHorizontalAngleDegrees, FrustumSettings.NearClippingRadius, FrustumSettings.SightRadius, FrustumSettings.FrustumPitch, FrustumSettings.VisionHorizontalAngleDegrees / FrustumSettings.VisionVerticalAngleDegrees);
      FMatrix loseSightProjection = OSEAIPerceptionHelpers::CreateProjectionMatrix(rootLocation, bodyFacing.Rotation(), FrustumSettings.VisionHorizontalAngleDegrees, FrustumSettings.SightRadius, FrustumSettings.LoseSightRadius, FrustumSettings.FrustumPitch, FrustumSettings.VisionHorizontalAngleDegrees / FrustumSettings.VisionVerticalAngleDegrees);

      TArray<FVector> frustumSegmentList;

      OSEAIPerceptionHelpers::GenerateFrustumSegmentList(sightProjection.Inverse(), frustumSegmentList);
      debuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegmentList(frustumSegmentList, 3, sightRangeColor));

      OSEAIPerceptionHelpers::GenerateFrustumSegmentList(loseSightProjection.Inverse(), frustumSegmentList);
      debuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegmentList(frustumSegmentList, 1, loseSightRangeColor));
   }
}
#endif // WITH_GAMEPLAY_DEBUGGER

