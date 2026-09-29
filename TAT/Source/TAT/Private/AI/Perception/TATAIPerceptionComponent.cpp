// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Perception/TATAIPerceptionComponent.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"
#include "Interactables/TATVisionPerceptionDevice.h"
#include "Character/TATCharacterAIBase.h"
#include "AI/Perception/TATAISense_Sight.h"
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "AI/TATKnowledgeComponent.h"

// ose
#include "OSECommon.h"
#include "Character/OSECharacterBase.h"
#include "AI/Perception/OSEAISense_Sight.h"

// ue4
#include "AIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIPerceptionComponent)

#if WITH_GAMEPLAY_DEBUGGER
void UTATAIPerceptionComponent::DescribeSelfToGameplayDebugger(FGameplayDebuggerCategory* debuggerCategory) const
{
   Super::DescribeSelfToGameplayDebugger(debuggerCategory);
}
#endif // WITH_GAMEPLAY_DEBUGGER

namespace CVarsTATAIPerceptionComponent
{
   static int DrawInformationSightDebug = 0;
   FAutoConsoleVariableRef CVarDebugDrawInformationSightDebug(
      TEXT("TAT.Perception.DebugInformationSharingFromSight"),
      DrawInformationSightDebug,
      TEXT("Draw debug lines for information shared from sight - Red = Agent shared to, Green = Target Shared, Yellow = Location Shared"),
      ECVF_Default);
}

void UTATAIPerceptionComponent::_ShareKnowledgeWithActorFromSight(const AActor* actor) const
{
   const ATATAIController* ownController = Cast<ATATAIController>(GetOwner());
   const ATATAIController* otherController = actor->GetInstigatorController<ATATAIController>();
   if(ownController != nullptr && otherController != nullptr)
   {
      EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(ownController->GetPawn(), otherController->GetPawn());
      if(attitude != EOSETeamAttitude::Friendly)
      {
         // Don't visually share hostile targets to anyone who isn't friendly.
         // For example, we only want guards sharing with other guards and civilians SHOULDN'T share their hostility as
         // they may see some guards as hostile after being beaten down when told to leave an area.
         return;
      }
      UOSEIndividualAttitudeComponent* ownAttitudeComponent = ownController->GetAttitudeComponent();
      UOSEIndividualAttitudeComponent* otherAttitudeComponent = otherController->GetAttitudeComponent();
      const ATATCharacterAIBase* characterAIBase = otherController->GetPawn<ATATCharacterAIBase>();
      if(characterAIBase != nullptr && characterAIBase->IsUnconscious() == false)
      {
         UTATKnowledgeComponent* ownKnowledgeComp = ownController->GetTATKnowledgeComponent();
         const UTATKnowledgeComponent* otherKnowledgeComp = otherController->GetTATKnowledgeComponent();
         bool attitudeChanged = false;
         for (const FTATActorKnowledge& knownActorFromOtherKnowledge : otherKnowledgeComp->GetKnownActors())
         {
            if(knownActorFromOtherKnowledge.IsEnemy() &&
               knownActorFromOtherKnowledge.GetDetectionState() == EActorDetectionState::Identified)
            {
               // force identification of the target that our ally is chasing
               ownKnowledgeComp->ForceActorDetectionStateIdentified(knownActorFromOtherKnowledge.GetActor(),
                                                                    knownActorFromOtherKnowledge.GetLastKnownLocation(),
                                                                    knownActorFromOtherKnowledge. GetLastStimTimestamp());
               if(CVarsTATAIPerceptionComponent::DrawInformationSightDebug != 0)
               {
                  FVector sharedFromLocation = ownController->GetPawn()->GetActorLocation(); 
                  DrawDebugLine(GetWorld(), sharedFromLocation, characterAIBase->GetActorLocation(), FColor::Red, false, 10);
                  DrawDebugLine(GetWorld(), sharedFromLocation, knownActorFromOtherKnowledge.GetActor()->GetActorLocation(), FColor::Green, false, 10);
                  DrawDebugLine(GetWorld(), sharedFromLocation, knownActorFromOtherKnowledge.GetLastKnownLocation(), FColor::Yellow, false, 10);
               }
               attitudeChanged |= otherAttitudeComponent->ShareSpecificIndividualAttitudeWithTarget(
                  knownActorFromOtherKnowledge.GetActor(),
                  ownAttitudeComponent,
                  EOSEAttitudeCopyRules::PreferMostHostile,
                  EOSEExpirationTimeCopyRules::PreferGreater,
                  false
               );
            }
         }
         if(attitudeChanged)
         {
            otherAttitudeComponent->BroadcastAttitudeChangeFromSharing(ownAttitudeComponent);
         }
      }
   }
}

void UTATAIPerceptionComponent::_OnTargetPerceptionUpdated(AActor* actor, FAIStimulus stimulus)
{
   Super::_OnTargetPerceptionUpdated(actor, stimulus);

   // Ignore stims from us or instigated by us.
   AActor* owningPawn = AIOwner ? AIOwner->GetPawn() : nullptr;
   if (owningPawn && actor && (actor == owningPawn || actor->GetInstigator() == owningPawn))
   {
      return;
   }
   if(stimulus.Type == UAISense::GetSenseID<UTATAISense_Sight>())
   {
      OnTargetSightPerceptionUpdated.Broadcast(actor, stimulus);
      _ShareKnowledgeWithActorFromSight(actor);
   }
   if (stimulus.Type == UAISense::GetSenseID<UTATAISense_Hearing>())
   {
      OnTATHearingEvent.Broadcast(actor, stimulus);
   }
}

void UTATAIPerceptionComponent::_OnTargetPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& updateInfo)
{
   Super::_OnTargetPerceptionInfoUpdated(updateInfo);

   // TODO: Should we be using this?
}

