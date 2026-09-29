// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/OSEAISenseConfig_Sight.h"

// unreal
#include "Perception/AIPerceptionComponent.h"

// ose
#include "OSECommon.h"
#include "AI/OSEAIController.h"
#include "AI/Perception/OSEAIPerceptionHelpers.h"
#include "AI/Perception/OSEAISenseSharedConfigData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAISenseConfig_Sight)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif

UOSEAISenseConfig_Sight::UOSEAISenseConfig_Sight(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   DebugColor = FColor::Green;
   Implementation = UOSEAISense_Sight::StaticClass();
}

const FOSEPerAlertLevelSettings& UOSEAISenseConfig_Sight::GetSettingsForAlertLevel(const EAlertnessLevel alertnessLevel) const
{
   switch (alertnessLevel)
   {
   case EAlertnessLevel::Neutral:    return NeutralValues;
   case EAlertnessLevel::Suspicious: return SuspiciousValues;
   case EAlertnessLevel::Alerted:    return AlertedValues;
   case EAlertnessLevel::Combat:     return CombatValues;
   default:
      checkNoEntry();
   }

   checkNoEntry();
   return NeutralValues;
}

TSubclassOf<UAISense> UOSEAISenseConfig_Sight::GetSenseImplementation() const
{
   return *Implementation;
}

#if WITH_EDITOR
void UOSEAISenseConfig_Sight::PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent)
{
   static const FName NAME_AutoSuccessRangeFromLastSeenLocation = GET_MEMBER_NAME_CHECKED(UOSEAISenseConfig_Sight, AutoSuccessRangeFromLastSeenLocation);
   
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

void UOSEAISenseConfig_Sight::DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const
{
   if (perceptionComponent == nullptr || debuggerCategory == nullptr)
   {
      return;
   }

   const FColor sightRangeColor = FColor::Green;
   const FColor loseSightRangeColor = FColorList::NeonPink;

   // don't call Super implementation on purpose, replace color description line
   debuggerCategory->AddTextLine(
      FString::Printf(TEXT("%s: {%s}%s {white}rangeIN:{%s}%s {white} rangeOUT:{%s}%s"), *GetSenseName(),
         *GetDebugColor().ToString(), *DescribeColorHelper(GetDebugColor()),
         *sightRangeColor.ToString(), *DescribeColorHelper(sightRangeColor),
         *loseSightRangeColor.ToString(), *DescribeColorHelper(loseSightRangeColor))
   );

   if (const AActor* bodyActor = perceptionComponent->GetBodyActor())
   {
      FVector bodyLocation, bodyFacing;
      perceptionComponent->GetLocationAndDirection(bodyLocation, bodyFacing);

      EAlertnessLevel alertnessLevel = EAlertnessLevel::Neutral;
      if (const IOSEAlertnessInterface* alertnessInterface = Cast<const IOSEAlertnessInterface>(bodyActor))
      {
         alertnessLevel = alertnessInterface->GetAlertnessLevel();
      }

      const FOSEPerAlertLevelSettings& currentAlertSettings = GetSettingsForAlertLevel(alertnessLevel);
      
      float sightRadius = currentAlertSettings.SightRadius + currentAlertSettings.PointOfViewBackwardOffset;
      float loseSightRadius = currentAlertSettings.LoseSightRadius + currentAlertSettings.PointOfViewBackwardOffset;
      
      if(SharedConfigData)
      {
         sightRadius *= SharedConfigData->CalculateRangePerceptionModifiers(perceptionComponent);
         loseSightRadius *= SharedConfigData->CalculateRangePerceptionModifiers(perceptionComponent);
      }
      const FVector rootLocation = bodyLocation - (bodyFacing * currentAlertSettings.PointOfViewBackwardOffset);

      OSEAIPerceptionHelpers::DrawDebugTargetFrustum(GetWorld(),
         rootLocation,
         bodyFacing.Rotation(),
         currentAlertSettings.PeripheralVisionAngleDegrees,
         currentAlertSettings.NearClippingRadius,
         sightRadius,
         currentAlertSettings.FrustumPitch,
         currentAlertSettings.FrustumAspectRatio,
         sightRangeColor);
      
      OSEAIPerceptionHelpers::DrawDebugTargetFrustum(GetWorld(),
         rootLocation,
         bodyFacing.Rotation(),
         currentAlertSettings.PeripheralVisionAngleDegrees,
         currentAlertSettings.NearClippingRadius,
         loseSightRadius,
         currentAlertSettings.FrustumPitch,
         currentAlertSettings.FrustumAspectRatio,
         loseSightRangeColor);
   }
}
#endif // WITH_GAMEPLAY_DEBUGGER

