// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Perception/TATAISenseConfig_Sight.h"

// TAT
#include "AI/Perception/TATAISense_Sight.h"
#include "Character/TATCharacterAIBase.h"

// OSE
#include "AI/Perception/OSEAIPerceptionHelpers.h"
#include "AI/Perception/OSEAISenseSharedConfigData.h"
#include "Perception/AIPerceptionComponent.h"

// UE
#include "GameFramework/PlayerController.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAISenseConfig_Sight)

UTATAISenseConfig_Sight::UTATAISenseConfig_Sight() 
{
   Implementation = UTATAISense_Sight::StaticClass();
}

const FTATPerEscalationLevelSettings& UTATAISenseConfig_Sight::GetSettingsForEscalation(const ETATEscalationState state) const
{
   if(const FTATPerEscalationLevelSettings* foundState = _EscalationStateToSettings.Find(state))
   {
      return *foundState;
   }
   return TATAISenseConfig_Sight::INVALID_SETTINGS;
}

TSubclassOf<UAISense> UTATAISenseConfig_Sight::GetSenseImplementation() const
{
   return Implementation.Get();
}


#if WITH_GAMEPLAY_DEBUGGER
namespace TATAISenseConfig_Sight
{
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
}
void UTATAISenseConfig_Sight::DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const
{
   if (perceptionComponent == nullptr || debuggerCategory == nullptr)
   {
      return;
   }

   const FColor sightRangeColor = FColor::Green;
   const FColor sightForLocalPawnRangeColor = FColor::Yellow;
   const FColor loseSightRangeColor = FColorList::NeonPink;

   FVector bodyLocation, bodyFacing;
   perceptionComponent->GetLocationAndDirection(bodyLocation, bodyFacing);

   const ATATCharacterAIBase* bodyActor = Cast<ATATCharacterAIBase>(perceptionComponent->GetBodyActor());
   const ETATEscalationState escalation = bodyActor ? bodyActor->GetCurrentEscalationState() : ETATEscalationState::None;
   const FTATPerEscalationLevelSettings& currentAlertSettings = GetSettingsForEscalation(escalation);
      
   float sightRadius = currentAlertSettings.SightRadius + currentAlertSettings.PointOfViewBackwardOffset;
   float loseSightRadius = currentAlertSettings.LoseSightRadius + currentAlertSettings.PointOfViewBackwardOffset;
   float sightRadiusAgainstLocalPawn = sightRadius + currentAlertSettings.PointOfViewBackwardOffset;

   if(SharedConfigData)
   {
      sightRadius *= SharedConfigData->CalculateRangePerceptionModifiers(perceptionComponent);
      loseSightRadius *= SharedConfigData->CalculateRangePerceptionModifiers(perceptionComponent);
   }

   FString descSightRangeModification = TEXT("");
   if(const APlayerController* localPlayerController = GetWorld()->GetFirstPlayerController<APlayerController>())
   {
      if (const IOSEAISightInterface* sightInterface = Cast<IOSEAISightInterface>(bodyActor))
      {
         const APawn* localPawn = localPlayerController->GetPawn();
         sightInterface->ModifySightRangeForSpecificActor(localPawn, sightRadiusAgainstLocalPawn);
         descSightRangeModification = sightInterface->DescribeSightRangeModificationForSpecificActor(localPawn);
      }
   }
   
   const FVector rootLocation = bodyLocation - (bodyFacing * currentAlertSettings.PointOfViewBackwardOffset);
   
   sightRadius = FMath::Max(currentAlertSettings.MinimumSightRadius + currentAlertSettings.PointOfViewBackwardOffset,sightRadius);
   loseSightRadius = FMath::Max(currentAlertSettings.MinimumSightRadius + currentAlertSettings.PointOfViewBackwardOffset,loseSightRadius);
   sightRadiusAgainstLocalPawn = FMath::Max(currentAlertSettings.MinimumSightRadius + currentAlertSettings.PointOfViewBackwardOffset,sightRadiusAgainstLocalPawn);
   
   debuggerCategory->AddTextLine(
      FString::Printf(
         TEXT("%s: {%s} [%s] Sight Range: %f"),
         *GetSenseName(),
         *sightRangeColor.ToString(),
         *TATAISenseConfig_Sight::DescribeColorHelper(sightRangeColor),
         sightRadius
         ));
   debuggerCategory->AddTextLine(
      FString::Printf(
         TEXT("%s: {%s} [%s]Lost Sight Range: %f"),
         *GetSenseName(),
         *loseSightRangeColor.ToString(),
         *TATAISenseConfig_Sight::DescribeColorHelper(loseSightRangeColor),
         loseSightRadius
         ));
   
   debuggerCategory->AddTextLine(
      FString::Printf(
         TEXT("%s: {%s} [%s] Local Player calculated sight range: %f\n   %s"),
         *GetSenseName(),
         *sightForLocalPawnRangeColor.ToString(),
         *TATAISenseConfig_Sight::DescribeColorHelper(sightForLocalPawnRangeColor),
         sightRadiusAgainstLocalPawn,
         *descSightRangeModification
         ));

   constexpr float frustumThicknesses = 2.0f;
   TArray<FVector> frustumSegmentList;

   const FMatrix localPawnSightProjection = OSEAIPerceptionHelpers::CreateProjectionMatrix(
      rootLocation,
      bodyFacing.Rotation(),
      currentAlertSettings.PeripheralVisionAngleDegrees,
      currentAlertSettings.NearClippingRadius,
      sightRadiusAgainstLocalPawn,
      currentAlertSettings.FrustumPitch,
      currentAlertSettings.FrustumAspectRatio);
   OSEAIPerceptionHelpers::GenerateFrustumSegmentList(localPawnSightProjection.Inverse(), frustumSegmentList);
   debuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegmentList(frustumSegmentList, frustumThicknesses, sightForLocalPawnRangeColor));

   const FMatrix sightProjection = OSEAIPerceptionHelpers::CreateProjectionMatrix(
      rootLocation, 
      bodyFacing.Rotation(), 
      currentAlertSettings.PeripheralVisionAngleDegrees,
      currentAlertSettings.NearClippingRadius,
      sightRadius,
      currentAlertSettings.FrustumPitch,
      currentAlertSettings.FrustumAspectRatio);
   OSEAIPerceptionHelpers::GenerateFrustumSegmentList(sightProjection.Inverse(), frustumSegmentList);
   debuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegmentList(frustumSegmentList, frustumThicknesses, sightRangeColor));

   const FMatrix loseSightProjection = OSEAIPerceptionHelpers::CreateProjectionMatrix(
      rootLocation, 
      bodyFacing.Rotation(), 
      currentAlertSettings.PeripheralVisionAngleDegrees,
      sightRadius,
      loseSightRadius,
      currentAlertSettings.FrustumPitch,
      currentAlertSettings.FrustumAspectRatio);
   OSEAIPerceptionHelpers::GenerateFrustumSegmentList(loseSightProjection.Inverse(), frustumSegmentList);
   debuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegmentList(frustumSegmentList, frustumThicknesses, loseSightRangeColor));
}
#endif // WITH_GAMEPLAY_DEBUGGER
