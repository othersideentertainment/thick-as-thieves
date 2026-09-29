// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Perception/TATAISense_Sight.h"

#include "AI/TATAIController.h"
#include "AI/Escalation/TATEscalationComponent.h"
#include "AI/Escalation/TATEscalationState.h"
#include "AI/Perception/OSEAISenseConfig_Sight.h"
#include "AI/Perception/OSEAISenseSharedConfigData.h"
#include "AI/Perception/TATAISenseConfig_Sight.h"
#include "Character/TATCharacterAIBase.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAISense_Sight)

class UTATAISenseConfig_Sight;

ETATEscalationState UTATAISense_Sight::GetEscalationStateForActor(const AActor* actor)
{
   // If there are non-characters that need escalation states, can introduce an interface
   // for now, just assume escalation is NA in that case
   const ATATAIController* aiCharacterBase = Cast<ATATAIController>(actor);
   return aiCharacterBase
      ? aiCharacterBase->GetTATEscalationComponent()->GetCurrentState()
      : ETATEscalationState::None;
}

const FDigestedSightProperties& UTATAISense_Sight::_SetupDigestedPropertiesForListener(
   const UAIPerceptionComponent& perceptionComponent)
{
   const UTATAISenseConfig_Sight* senseConfig = Cast<const UTATAISenseConfig_Sight>(perceptionComponent.GetSenseConfig(GetSenseID()));
   check(senseConfig);

   const ETATEscalationState escalationState = GetEscalationStateForActor(perceptionComponent.GetOwner());
  
   FDigestedSightProperties& propertyDigest = DigestedProperties.FindOrAdd(perceptionComponent.GetListenerId());
   
   const FTATPerEscalationLevelSettings settings = senseConfig->GetSettingsForEscalation(escalationState);
   propertyDigest.SightInterface = Cast<IOSEAISightInterface>(perceptionComponent.GetBodyActor());
   propertyDigest.SightRadiusSq = FMath::Square(settings.SightRadius + settings.PointOfViewBackwardOffset);
   propertyDigest.LoseSightRadiusSq = FMath::Square(settings.LoseSightRadius + settings.PointOfViewBackwardOffset);
   propertyDigest.MinimumSightRadiusSq = FMath::Square(settings.MinimumSightRadius + settings.PointOfViewBackwardOffset);
   propertyDigest.HalfFOVInDegrees = settings.PeripheralVisionAngleDegrees;
   propertyDigest.PointOfViewBackwardOffset = settings.PointOfViewBackwardOffset;
   propertyDigest.NearClip = settings.NearClippingRadius;
   propertyDigest.FarClip = settings.SightRadius;
   propertyDigest.LoseSightFarClip = settings.LoseSightRadius;
   propertyDigest.MinSightFarClip = settings.MinimumSightRadius;
   propertyDigest.FrustumPitch = settings.FrustumPitch;
   propertyDigest.FrustumAspectRatio = settings.FrustumAspectRatio;
   
   if (senseConfig->AutoSuccessRangeFromLastSeenLocation == FAISystem::InvalidRange)
   {
      propertyDigest.AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
   }
   else
   {
      propertyDigest.AutoSuccessRangeSqFromLastSeenLocation = FMath::Square(senseConfig->AutoSuccessRangeFromLastSeenLocation);
   }
   
   propertyDigest.SharedConfigData = senseConfig->SharedConfigData;
   propertyDigest.AffiliationFlags = senseConfig->DetectionByAffiliation.GetAsFlags();
   return propertyDigest;
}
