// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/TATConsiderationInputs.h"

// tat
#include "AI/LivingWorld/TATLivingWorldAgentComponent.h"
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "AI/TATKnowledgeComponent.h"
#include "AI/Reactions/TATAIReactionCoordinator.h"
#include "AI/Reactions/TATAIReactionTarget.h"
#include "AI/SmartObjects/TATActionNodeComponent.h"
#include "AI/Squad/TATSquadAlarmStation.h"
#include "Character/TATCharacterAIBase.h"
#include "Combat/TATAICombatComponent.h"
#include "Player/TATCharacter.h"
#include "Loot/TATLootInventory.h"
#include "Environment/TATPrivateSpaceVolume.h"

// ose
#include "OSECoreCollision.h"
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/OSEAIFunctionLibrary.h"
#include "AI/Utility/UtilityAIComponent.h"
#include "Character/OSECharacterBase.h"
#include "OSEIndividualKnowledgeInterface.h"

// ue4
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "EngineDefines.h"
#include "Logging/LogVerbosity.h"
#if ENABLE_VISUAL_LOG
#include "VisualLogger/VisualLogger.h"
#endif

DEFINE_LOG_CATEGORY(LogTATConsiderationInputs);

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATConsiderationInputs)

namespace TATConsiderationInputHelpers
{
   static const AActor* TryRetrieveActorFromContext(const FConsiderationContext& ctx)
   {
      if (const AActor* actor = ctx.Target.Actor.Get())
      {
         return actor;
      }
      else if (const UActorComponent* component = ctx.Target.Component.Get())
      {
         return component->GetOwner();
      }
      else if (const AActor* stimInstigator = ctx.Target.Stim.Instigator.Get())
      {
         return stimInstigator;
      }
      else
      {
         return nullptr;
      }
   }

   static const FTATAIReactionTarget GenerateReactionTargetFromContext(const FConsiderationContext& ctx)
   {
      if (const AActor* targetActor = ctx.Target.Actor.Get())
      {
         return FTATAIReactionTarget::GenerateForActor(targetActor);;
      }
      else if (ctx.Target.Stim.IsValid())
      {
         ATATCharacterAIBase* aiCharacter = Cast<ATATCharacterAIBase>(ctx.Character);
         return FTATAIReactionTarget::GenerateForStim(aiCharacter, ctx.Target.Stim);
      }
      else if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(ctx.Target.GetTargetUObject()))
      {
         return FTATAIReactionTarget::GenerateForActor(smartObjComp->GetOwner());
      }
      
      return FTATAIReactionTarget();
   }
}

UConsiderationInput* TATConsiderationFixup::FixupConsideration(UConsiderationInput* input, UObject* outerAsset)
{
   /*
   if (UConsiderationInput_DistanceToLastKnownLocationComparison* oldInput = Cast<UConsiderationInput_DistanceToLastKnownLocationComparison>(input))
   {
      UConsiderationInput_NavMeshDistanceToLastKnownLocationComparison* newInput = NewObject<UConsiderationInput_NavMeshDistanceToLastKnownLocationComparison>(outerAsset);
      newInput->Distance = oldInput->Distance;
      newInput->ComparisonMethod = oldInput->ComparisonMethod;
      return newInput;
   }
   */

   return input;
}

float UConsiderationInput_IdentificationValueMatches::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(CanSeeTarget);
   
   const AActor* target = TATConsiderationInputHelpers::TryRetrieveActorFromContext(ctx);
   const ATATAIController* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   const UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   const EActorDetectionState detectionState = knowledge->GetActorDetectionState(target);
   if(detectionState == EActorDetectionState::Identified || detectionState == EActorDetectionState::Identifying)
   {
      const float detectionValue = knowledge->GetDetectionValue(target);
      return UOSEMathFunctionLibrary::CompareFloats(detectionValue, RequiredValue, ComparisonMethod) ? 1.0f : 0.0f;
   }
   return 0.f;
}

float UConsiderationInput_CanSeeTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(CanSeeTarget);

   const AActor* target = TATConsiderationInputHelpers::TryRetrieveActorFromContext(ctx);
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   const bool canSeeTarget = knowledge ? knowledge->IsActorVisible(target) : false;
   return canSeeTarget ? 1.0f : 0.0f;
}

float UConsiderationInput_MyDetectionStateForTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyDetectionStateForTarget);

   AActor* target = ctx.Target.Actor.Get();
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   if (UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent())
   {
      return UOSEMathFunctionLibrary::CompareInts((int)knowledge->GetActorDetectionState(target), (int)State, ComparisonMethod) ? 1.0f : 0.0f;
   }
   return 0.0f;
}

float UConsiderationInput_MyDetectionForTargetIsNonZero::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyDetectionForTargetIsNonZero);
   AActor* target = ctx.Target.Actor.Get();
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   const float detectionValue = knowledge ? knowledge->GetDetectionValue(target) : 0;
   return detectionValue > 0 ? 1.0 : 0.0;
}

float UConsiderationInput_ShouldInvestigateTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(ShouldInvestigateTarget);

   AActor* target = ctx.Target.Actor.Get();
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   if (UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent())
   {
      if (const FTATActorKnowledge* actorKnowledge = knowledge->GetActorKnowledge(target))
      {
         // investigate a target if we've detected it, or if we're already investigating it (so we can keep investigating even once our detection level depletes)
         // TODO: add squad stuff here
         if (actorKnowledge->GetDetectionValue() > 0.0f || actorKnowledge->GetIsInvestigatingActor())
         {
            return 1.0f;
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_IsInvestigatingTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsInvestigatingTarget);

   AActor* target = ctx.Target.Actor.Get();
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   if (UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent())
   {
      if (const FTATActorKnowledge* actorKnowledge = knowledge->GetActorKnowledge(target))
      {
         return actorKnowledge->GetIsInvestigatingActor() ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_MyAlertnessIsNeutral::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyAlertnessIsNeutral);
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!aiController)
   {
      return false;
   }

   return aiController->GetAlertnessLevel() == EAlertnessLevel::Neutral;
}

float UConsiderationInput_MyAlertnessIsExactly::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyAlertnessIsExactly);
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!aiController)
   {
      return false;
   }

   return aiController->GetAlertnessLevel() == Level;
}

float UConsiderationInput_MyAlertnessIsGreaterThanOrEqualTo::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyAlertnessIsGreaterThanOrEqualTo);
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!aiController)
   {
      return false;
   }

   return aiController->GetAlertnessLevel() >= Level;
}

float UConsiderationInput_MyAlertnessValue::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyAlertnessValue);
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!aiController)
   {
      return false;
   }

   return UOSEMathFunctionLibrary::CompareInts((int)aiController->GetAlertnessLevel(), (int)Level, ComparisonMethod) ? 1.0f : 0.0f;
}

float UConsiderationInput_TargetAlertnessValue::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyAlertnessValue);
   const auto* aiCharacter = Cast<ATATCharacterAIBase>(ctx.Target.Actor);
   if (!aiCharacter)
   {
      return false;
   }

   const auto* aiController = aiCharacter->GetController<ATATAIController>();
   if (!aiController)
   {
      return false;
   }

   return UOSEMathFunctionLibrary::CompareInts(static_cast<int>(aiController->GetAlertnessLevel()), static_cast<int>(Level), ComparisonMethod) ? 1.0f : 0.0f;
}


float UConsiderationInput_HasPathToLastKnownTargetLocation::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasPathToLastKnownTargetLocation);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         if (const AActor* targetActor = ctx.Target.Actor.Get())
         {
            return knowledgeComp->DoesFullPathToLastKnownLocationExist(targetActor) ? 1.0f : 0.0f;
         }
         if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(ctx.Target.GetTargetUObject()))
         {
            const FSmartObjectSlotHandle& smartObjSlot = ctx.Target.SmartObjectRequestTarget.SlotHandle;
            return knowledgeComp->DoesFullPathToSmartObjectSlotLocationExist(smartObjComp, smartObjSlot) ? 1.0f : 0.0f;
         }
      }
      if(bUsePathFinding)
      {
         const FVector targetLocation = ctx.Target.GetTargetWorldLocation();
         if(FAISystem::IsValidLocation(targetLocation) == false)
            return 0.f;
         constexpr bool allowPartial = false;
         return UOSEAIFunctionLibrary::HasPathToLocation(ctx.AIController, targetLocation, allowPartial) ? 1.0f : 0.0f;
      }
   }
   UE_LOG(LogTATConsiderationInputs, Warning, TEXT("Attempting to find full path to target with no valid actor - %s"), *GetNameSafe(ctx.StateInstance))
   return 0.0f;
}

float UConsiderationInput_NavMeshDistanceToLocationBase::_InternalGetValue(const AActor* const fromActor, FVector targetLocation) const
{
   const FOSEAINavMeshCalcPathResult result = UOSEAIFunctionLibrary::CalcNavMeshPathLengthFromCurrentLocation(fromActor, targetLocation);
   if (result.HasPartialOrFullPath())
   {
      return UOSEMathFunctionLibrary::CompareFloats(result.PathLength, Distance, ComparisonMethod) ? 1.0f : 0.0f;
   }
   return 0.f;
}

float UConsiderationInput_NavMeshDistanceToLastKnownLocationComparison::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(NavMeshDistanceToLastKnownLocationComparison);

   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         if (const AActor* targetActor = ctx.Target.Actor.Get())
         {
            if (const FTATActorKnowledge* knowledge = knowledgeComp->GetActorKnowledge(targetActor))
            {
              return _InternalGetValue(ctx.AIController, knowledge->GetLastKnownLocation());
            }
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_HasFullPathToTargetLocation::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasFullPathToTargetLocation);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         if (const AActor* targetActor = ctx.Target.Actor.Get())
         {
            return knowledgeComp->DoesFullPathToLastKnownLocationExist(targetActor) ? 1.f : 0.f;
         }
         if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(ctx.Target.GetTargetUObject()))
         {
            const FSmartObjectSlotHandle& smartObjSlot = ctx.Target.SmartObjectRequestTarget.SlotHandle;
            return knowledgeComp->DoesFullPathToSmartObjectSlotLocationExist(smartObjComp, smartObjSlot) ? 1.0f : 0.0f;
         }
      }
      if(bUsePathFinding)
      {
         const FVector targetLocation = ctx.Target.GetTargetWorldLocation();
         if(FAISystem::IsValidLocation(targetLocation) == false)
            return 0.f;
         constexpr bool allowPartial = false;
         return UOSEAIFunctionLibrary::HasPathToLocation(ctx.AIController, targetLocation, allowPartial) ? 1.0f : 0.0f;
      }
      UE_LOG(LogTATConsiderationInputs, Warning, TEXT("Attempting to find full path to target with no valid actor - %s"), *GetNameSafe(ctx.StateInstance))
   }
   return 0.0f;
}

float UConsiderationInput_IsThisMyProblematicTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsThisMyProblematicTarget);
   
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (const UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         const FTATSharedTarget& sharedKnowledge = knowledgeComp->GetTargetToShare();
         if(const AActor* currentProblematicActor = sharedKnowledge.TargetActor.Get())
         {
            if(currentProblematicActor == ctx.Target.Actor)
            {
               return 1.f;
            }
         }
         if(ctx.Target.Stim.IsValid() == false)
         {
            return 0.f;
         }
         return sharedKnowledge.TargetStimId == ctx.Target.Stim.Id ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_IsTargetACharacter::GetValue(const FConsiderationContext& ctx) const
{
   if(ctx.Target.Actor.IsValid() == false)
   {
      return 0.f;
   }
   const ACharacter* character = Cast<ACharacter>(ctx.Target.Actor.Get());
   if(character == nullptr)
      return 0.f;

   return 1.f;
}

float UConsiderationInput_IsTargetNPC::GetValue(const FConsiderationContext& ctx) const
{
   if(ctx.Target.Actor.IsValid() == false)
   {
      return 0.f;
   }
   const ACharacter* character = Cast<ACharacter>(ctx.Target.Actor.Get());
   if(character == nullptr)
      return 0.f;

   const AAIController* aiController = Cast<AAIController>(character->GetController());
   if(aiController == nullptr)
      return 0.f;

   return 1.f;
}

float UConsiderationInput_IsActorBroken::GetValue(const FConsiderationContext& ctx) const
{
   if(ctx.Target.Actor.IsValid() == false)
   {
      return 0.f;
   }
   if(ITATBreakableActorInfoInterface* breakableActorInfoInterface = Cast<ITATBreakableActorInfoInterface>(ctx.Target.Actor.Get()))
   {
      return breakableActorInfoInterface->IsBroken();
   }
   return 0.f;
}

float UConsiderationInput_DoIHaveAProblematicTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(DoIHaveAProblematicTarget);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (const UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         const FTATSharedTarget& sharedKnowledge = knowledgeComp->GetTargetToShare();
         if(sharedKnowledge.TargetActor.IsValid())
         {
            return 1.f;
         }
         return sharedKnowledge.TargetStimId != INDEX_NONE ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_CanIUseMagicKnowledgeToFindHelperAlly::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(CanIUseMagicKnowledgeToFindHelperAlly);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (const UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         const FTATSharedTarget& sharedKnowledge = knowledgeComp->GetTargetToShare();
         return sharedKnowledge.CanPartnerSearchUseMagicKnowledge ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_DoIHaveAHelperAlly::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(DoIHaveAHelperAlly);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         const FTATSharedTarget& sharedKnowledge = knowledgeComp->GetTargetToShare();
         return sharedKnowledge.Partner.IsValid() ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_IsThisMyHelperAlly::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsThisMyHelperAlly);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (const AActor* targetActor = ctx.Target.Actor.Get())
      {
         if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
         {
            const FTATSharedTarget& sharedKnowledge = knowledgeComp->GetTargetToShare();

            if (UTATKnowledgeComponent* sharedKnowledgeComp = sharedKnowledge.Partner.Get())
            {
               return sharedKnowledgeComp->GetAICharacter() == targetActor ? 1.0f : 0.0f;
            }
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_AmIAHelperAlly::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsThisMyHelperAlly);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         return knowledgeComp->GetTargetSharedWithMe().IsValid() ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_HasProblematicTargetBeenDestroyed::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasProblematicTargetBeenDestroyed);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (const UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         return knowledgeComp->HasSharedTargetBeenHandled() ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_DoINeedHelp::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(DoINeedHelp);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (const UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         const FTATSharedTarget& sharedKnowledge = knowledgeComp->GetTargetToShare();
         return sharedKnowledge.State == FTATSharedTarget::EState::PartnerNeeded ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_DoIAlreadyHaveHelp::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(DoIAlreadyHaveHelp);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (const UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         const FTATSharedTarget& sharedKnowledge = knowledgeComp->GetTargetToShare();
         return sharedKnowledge.State != FTATSharedTarget::EState::PartnerNeeded ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_TimeSinceLastAttackedByTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceLastAttackedByTarget);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent();
      if (knowledgeComp)
      {
         const AActor* targetActor = ctx.Target.Actor.Get();
         if (targetActor)
         {
            const FTATActorKnowledge* knowledge = knowledgeComp->GetActorKnowledge(targetActor);
            if (knowledge)
            {
               float lastAttackTime = knowledge->GetLastAttackTimestamp();
               if (lastAttackTime >= 0)
               {
                  float now = knowledgeComp->GetWorld()->GetTimeSeconds();
                  return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, (float)now - lastAttackTime);
               }
               else
               {
                  // The value initializes to -1. That means he's never attacked us so we
                  // should return the maximum value.
                  return 1.0;
               }
            }
         }
      }
   }
   return 1.0f;
}

float UConsiderationInput_KnowledgeSource::GetValue(const FConsiderationContext& ctx) const
{
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         if (const AActor* targetActor = ctx.Target.Actor.Get())
         {
            if (const FTATActorKnowledge* knowledge = knowledgeComp->GetActorKnowledge(targetActor))
            {
               return knowledge->GetKnowledgeSource() == KnowledgeSource ? 1.f : 0.f;
            }
         }
      }
   }
   return 0.f;
}

float UConsiderationInput_TimeSinceTargetLastSeen::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceTargetLastSeen);
   if (auto* aiController = Cast<ATATAIController>(ctx.AIController))
   {
      if (UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent())
      {
         if (AActor* targetActor = ctx.Target.Actor.Get())
         {
            // 3/23: GetTimeSinceActorSeen is only used here, and was including a GetLastKnownActorLocation() internally.  
            // I have moved that out into this consideration, but, do we really need it?!  I am erroring on the safe side for MS10
            float timeSinceVisible = 1.0;
            FVector lastKnownLocation;
            const bool actorSeen = knowledgeComp->GetTimeSinceActorVisible(targetActor, timeSinceVisible);
            const bool actorLastKnownLocationIsValid = RequiresValidLastKnownLocation ? knowledgeComp->GetLastKnownActorLocation(targetActor, lastKnownLocation) : true;
            if (actorSeen && actorLastKnownLocationIsValid)
            {
               return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, timeSinceVisible);
            }
            else
            {
               // Target has never been seen, return maximum value.
               return timeSinceVisible;
            }
         }
      }
   }
   return 1.0f;
}

float UConsiderationInput_HasLineOfSightToTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasLineOfSightToTarget);

   // ASSUMPTION: I assume we're not using this in a lot of cases, and therefore it's cheap enough here in a consideration.
   // But if we were using it a lot, we could pre-calc it in the knowledge component.  I suspect it's just less frequent
   // to do right here, though.

   if (!ctx.Character)
      return 0.0f;

   FVector targetLocation = ctx.Target.GetTargetWorldLocation();
   if (targetLocation == FAISystem::InvalidLocation)
      return 0.0f;

   FVector eyesLoc;
   FRotator eyesRot;
   ctx.Character->GetActorEyesViewPoint(eyesLoc, eyesRot);

   FHitResult hit;
   FCollisionQueryParams queryParams(FName("ConsiderationInput_HasLineOfSightToTarget"), SCENE_QUERY_STAT_ONLY(ConsiderationInput_HasLineOfSightToTarget));
   queryParams.AddIgnoredActor(ctx.Character);

   FCollisionResponseParams responseParam(ECR_Ignore);
   responseParam.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
   GetWorld()->LineTraceSingleByChannel(hit, eyesLoc, targetLocation, COLLISION_AOE, queryParams, responseParam);
   return hit.bBlockingHit ? 0.0f : 1.0f;
}

float UConsiderationInput_NavMeshDistanceToTargetNearbyActor::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(NavMeshDistanceToTargetNearbyActor);

   if (const ATATAIController* aiController = Cast<const ATATAIController>(ctx.AIController))
   {
      UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent();
      if (knowledgeComp == nullptr)
      {
         return 0.0f;
      }

      float pathLength = -1.0f;

      if (const AActor* targetActor = ctx.Target.Actor.Get())
      {
         if (knowledgeComp->DoesAnyPathToLastKnownLocationExist(targetActor))
         {
            pathLength = knowledgeComp->GetPathLengthToLastKnownLocation(targetActor);
         }
      }
      else if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(ctx.Target.GetTargetUObject()))
      {
         const FSmartObjectSlotHandle& smartObjSlot = ctx.Target.SmartObjectRequestTarget.SlotHandle;
         if (knowledgeComp->DoesAnyPathToSmartObjectSlotLocationExist(smartObjComp, smartObjSlot))
         {
            pathLength = knowledgeComp->GetPathLengthToSmartObjectSlotLocation(smartObjComp, smartObjSlot);
         }
      }

      if (pathLength >= 0.0f)
      {
         return FMath::GetMappedRangeValueClamped(FVector2D(0.0f, FMath::Square(knowledgeComp->NearbyActorsSphereRadius)), kOutputRange, pathLength);
      }
   }

   return 0.0f;
}

float UConsiderationInput_CachedNavMeshDistanceToTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(CachedNavMeshDistanceToTarget);
   if (const ATATAIController* aiController = Cast<const ATATAIController>(ctx.AIController))
   {
      UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent();
      if (knowledgeComp == nullptr)
      {
         return 1.0f;
      }

      float pathLength = -1.0f;

      if (const AActor* targetActor = ctx.Target.Actor.Get())
      {
         if (knowledgeComp->DoesAnyPathToLastKnownLocationExist(targetActor))
         {
            pathLength = knowledgeComp->GetPathLengthToLastKnownLocation(targetActor);
         }
      }
      else if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(ctx.Target.GetTargetUObject()))
      {
         const FSmartObjectSlotHandle& smartObjSlot = ctx.Target.SmartObjectRequestTarget.SlotHandle;
         if (knowledgeComp->DoesAnyPathToSmartObjectSlotLocationExist(smartObjComp, smartObjSlot))
         {
            pathLength = knowledgeComp->GetPathLengthToSmartObjectSlotLocation(smartObjComp, smartObjSlot);
         }
      }

      if (pathLength >= 0.0f)
      {
         if (!IsFullStrengthAtGreaterThanMax && pathLength > Max)
         {
            return 0.0f;
         }

         return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, pathLength);
      }
   }

   return 1.0f;
}

float UConsiderationInput_CachedNavMeshDistanceToTargetComparison::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(CachedNavMeshDistanceToTargetComparison);
   if (const ATATAIController* aiController = Cast<const ATATAIController>(ctx.AIController))
   {
      UTATKnowledgeComponent* knowledgeComp = aiController->GetTATKnowledgeComponent();
      if (knowledgeComp == nullptr)
      {
         return 0.0f;
      }

      float pathLength = -1.0f;

      if (const AActor* targetActor = ctx.Target.Actor.Get())
      {
         if(knowledgeComp->DoesAnyPathToLastKnownLocationExist(targetActor))
         {
            pathLength = knowledgeComp->GetPathLengthToLastKnownLocation(targetActor);
         }
      }
      else if (const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(ctx.Target.GetTargetUObject()))
      {
         const FSmartObjectSlotHandle& smartObjSlot = ctx.Target.SmartObjectRequestTarget.SlotHandle;
         if (knowledgeComp->DoesAnyPathToSmartObjectSlotLocationExist(smartObjComp, smartObjSlot))
         {
            pathLength = knowledgeComp->GetPathLengthToSmartObjectSlotLocation(smartObjComp, smartObjSlot);
         }
      }

      if (pathLength >= 0.0f)
      {
         return UOSEMathFunctionLibrary::CompareFloats(pathLength, ComparisonValue, ComparisonMethod) ? 1.0f : 0.0f;
      }
   }

   return 0.0f;
}

//---------------------------------------------------------------------------------------
// UConsiderationInput_FakeCyclicalIntelligence
//---------------------------------------------------------------------------------------

float UConsiderationInput_FakeCyclicalIntelligence::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(FakeCyclicalIntelligence);
   
   auto* aiController = Cast<const ATATAIController>(ctx.AIController);
   if (!aiController)
   {
      return 0.0f;
   }

   UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   const float worldTime = aiController->GetWorld()->GetTimeSeconds();
   const float offsetWorldTime = worldTime + knowledge->GetFakeCylicalIntelligenceTimeOffset();
   const float cos = FMath::Cos((2.0f * PI) * (1.0f / SecondsPerCycle) * offsetWorldTime);
   if (cos >= MinSmartness && cos <= MaxSmartness)
   {
      return FMath::GetMappedRangeValueClamped(FVector2D(MinSmartness, MaxSmartness), kOutputRange, cos);
   }
   return 0.0f;
}

#if WITH_EDITOR
void UConsiderationInput_FakeCyclicalIntelligence::PostLoad()
{
   Super::PostLoad();
   _ClampValuesInRange();
   _RefreshUptime();
}

void UConsiderationInput_FakeCyclicalIntelligence::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _ClampValuesInRange();
   _RefreshUptime();
}

void UConsiderationInput_FakeCyclicalIntelligence::PostEditUndo()
{
   Super::PostEditUndo();
   _ClampValuesInRange();
   _RefreshUptime();
}
#endif // WITH_EDITOR

void UConsiderationInput_FakeCyclicalIntelligence::_ClampValuesInRange()
{
   MinSmartness = FMath::Clamp(MinSmartness, -1.0f, 1.0f);
   MaxSmartness = FMath::Clamp(MaxSmartness, -1.0f, 1.0f);
   
   MinSmartness = FMath::Min(MinSmartness, MaxSmartness);
   MaxSmartness = FMath::Max(MinSmartness, MaxSmartness);
}

void UConsiderationInput_FakeCyclicalIntelligence::_RefreshUptime()
{
   const float totalDistance = 2.0f; // -1 => 1
   float totalSmartnessPercent = FMath::Max(0.0f, MaxSmartness - MinSmartness) / totalDistance;
   SecondsUpTime = (totalSmartnessPercent * SecondsPerCycle);
   SecondsDownTime = SecondsPerCycle - SecondsUpTime;
}

//---------------------------------------------------------------------------------------
// UConsiderationInput_AICombatCombo
//---------------------------------------------------------------------------------------

float UConsiderationInput_AICombatCombo::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(AICombatCombo);
   if (ctx.Character)
   {
      if (UTATAICombatComponent* aiCombatComp = Cast<UTATAICombatComponent>(ctx.Character->GetCombatComponent()))
      {
         FGameplayTag lookingForAbilityTag = aiCombatComp->FindCurrentComboAbilityTag();
         return (lookingForAbilityTag.IsValid() && lookingForAbilityTag == AbilityTag) ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_TargetAllyIsBehavingSuspiciously::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetAllyIsBehavingSuspiciously);

   ATATCharacterAIBase* ai = Cast<ATATCharacterAIBase>(ctx.Character);
   if (ai)
   {
      ATATCharacter* target = Cast<ATATCharacter>(ctx.Target.Actor.Get());
      if (target)
      {
         return target->IsBehavingSuspiciously(ai->GetAlertnessLevel()) ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float GetTargetIdentificationStateValue(const FConsiderationContext& ctx, const EActorDetectionState requiredState)
{
   AActor* target = ctx.Target.Actor.Get();
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   if (const UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent())
   {
      return knowledge->GetActorDetectionState(target) == requiredState ? 1.0f : 0.0f;
   }
   return 0.0f;
}
float UConsiderationInput_TargetIsInIdentificationState::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetIsInIdentificationState);
   return GetTargetIdentificationStateValue(ctx, RequiredState);   
}
float UConsiderationInput_TargetIsIdentified::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetIsIdentified);
   return GetTargetIdentificationStateValue(ctx, EActorDetectionState::Identified);
}

float UConsiderationInput_HasAnyIdentifiedTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasAnyIdentifiedTarget);

   AActor* target = ctx.Target.Actor.Get();
   auto* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!target || !aiController)
   {
      return 0.0f;
   }

   UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   if (!knowledge)
      return 0.0f;

   return knowledge->HasAnyIdentifiedTargetWithAttitude(Attitude) ? 1.0f : 0.0f;
}

float UConsiderationInput_HasAnyForcedStateTags::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasAnyIdentifiedTarget);

   if (const IGameplayTagAssetInterface* tagAssetInterface = Cast<IGameplayTagAssetInterface>(ctx.Character))
   {
      const UTATAISettings& aiSettings = UTATAISettings::Get();
      return tagAssetInterface->HasAnyMatchingGameplayTags(aiSettings.ForcedStateTags) ? 1.0f : 0.0f;
   }
   return 0.0f;   
}

float UConsiderationInput_HasBeenRequestedToMoveFromLocation::GetValue(const FConsiderationContext& ctx) const
{
   const ATATAIController* aiController = Cast<ATATAIController>(ctx.AIController);
   if(aiController == nullptr)
      return 0.f;
   return aiController->GetMoveFromLocationRequested() ? 1.f : 0.f;
}

float UConsiderationInput_IsStimFromKnownPlayer::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsStimFromKnownPlayer);

   AActor* stimInstigator = ctx.Target.Stim.Instigator.Get();
   if (!stimInstigator)
      return 0.0f;

   const ATATAIController* aiController = Cast<ATATAIController>(ctx.AIController);
   if (!aiController)
      return 0.0f;

   UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   if (!knowledge)
      return 0.0f;

   if (FTATActorKnowledge* actorKnowledge = knowledge->GetActorKnowledge(stimInstigator))
   {
      return actorKnowledge->IsPlayer() ? 1.0f : 0.0f;
   }

   return 0.0f;
}

float UConsiderationInput_StimHasTag::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(StimHasTag);
   // Counter-intuitively the Stim "Tag" is actually the Datatables RowName, not the gameplay tag.
   // TATAISense_Hearing.cpp:243. Undoing this would mean updating the FindRow logic in
   // UTATKnowledgeComponent::_GetHearingStimSeverity to look for the gameplay tag instead of the row name.
   return ctx.Target.Stim.Tag.IsEqual(StimInfoRow.RowName, ENameCase::IgnoreCase) ? 1.f : 0.f;
}

float UConsiderationInput_IsLocationWithinGuardedPrivateArea::GetValue(const FConsiderationContext& ctx) const
{
   const ATATAIController* aiController = Cast<ATATAIController>(ctx.AIController);
   if(aiController == nullptr)
   {
      return 0.f;
   }
   const ATATPrivateSpaceVolume* privateSpaceVolume = aiController->GetPrivateSpaceControllerAssignedTo();
   if(privateSpaceVolume == nullptr)
   {
      return 0.f;
   }
   const FVector location = ctx.Target.GetTargetWorldLocation();
   if(privateSpaceVolume->EncompassesPoint(location))
   {
      return 1.f;
   }
   return 0.f;
}

float UConsiderationInput_StimInstigatorTeamAttitudeComparison::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(StimInstigatorTeamAttitudeComparison);

   AActor* stimInstigator = ctx.Target.Stim.Instigator.Get();
   const EOSETeamAttitude relativeAttitude = UOSETeamFunctionLibrary::GetTeamAttitude(ctx.Character, stimInstigator);
   return UOSEMathFunctionLibrary::CompareInts(static_cast<int>(relativeAttitude), static_cast<int>(Attitude), ComparisonMethod) ? 1.0f : 0.0f;
}

float UConsiderationInput_TargetAttitudeComparison::GetValue(const FConsiderationContext& ctx) const
{
   if (ctx.Target.Actor.IsValid() == false)
      return 0.0f;
   
   const EOSETeamAttitude relativeAttitude = UOSETeamFunctionLibrary::GetTeamAttitude(ctx.Character, ctx.Target.Actor.Get());
   return UOSEMathFunctionLibrary::CompareInts(static_cast<int>(relativeAttitude), static_cast<int>(Attitude), ComparisonMethod) ? 1.0f : 0.0f;
}

float UConsiderationInput_HasTrait::GetValue(const FConsiderationContext& ctx) const
{
   const ATATAIController* aiController = Cast<ATATAIController>(ctx.AIController);
   if (aiController == nullptr)
      return 0.0f;

   if(aiController->HasTrait(Trait) == false)
      return 0.f;

   return 1.f;
}

float UConsiderationInput_TargetHasTrait::GetValue(const FConsiderationContext& ctx) const
{
   if(ctx.Target.TargetType != EBehaviorTargetType::Actor)
      return 0.f;

   const ATATCharacterAIBase* character = Cast<ATATCharacterAIBase>(ctx.Target.Actor);
   if(character == nullptr)
      return 0.f;
      
   const ATATAIController* aiController = Cast<ATATAIController>(character->GetController());
   if (aiController == nullptr)
      return 0.0f;

   if(aiController->HasTrait(Trait) == false)
      return 0.f;

   return 1.f;
}

float UConsiderationInput_DistanceFromGuardPositionComparision::GetValue(const FConsiderationContext& ctx) const
{
   if(const AAIController* aiController = ctx.AIController)
   {
      if(const UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
      {
         const FVector guardLocation = blackboard->GetValueAsVector(ATATAIController::GetGuardLocationBlackboardKey());
         if(guardLocation == UBlackboardKeyType_Vector::InvalidValue)
         {
            return 0.f;
         }
         const FVector targetLocation = ctx.Target.GetTargetWorldLocation();
         
         bool bOutput = false;
         if(bUsePathfinding)
         {
            FOSEAINavMeshCalcPathResult result;
            double unusedOutPathCost = 0.0f;
         
            result.QueryResult = UOSEAIFunctionLibrary::CalcNavMeshPathLengthAndCost(*aiController,
                                                         targetLocation,
                                                         guardLocation,
                                                         aiController->GetDefaultNavigationFilterClass(),
                                                         aiController->GetNavAgentPropertiesRef(),
                                                         result.PathLength,
                                                         unusedOutPathCost);

            if (result.HasPartialOrFullPath())
            {
               bOutput = UOSEMathFunctionLibrary::CompareFloats(result.PathLength, Distance, ComparisonMethod);
            }
#if ENABLE_VISUAL_LOG
            const FColor outputColor = bOutput ? FColor::Green : FColor::Red;
            UE_VLOG_LOCATION(ctx.AIController, LogTATConsiderationInputs, Log, guardLocation, 50.f, outputColor, TEXT("Guard Location"));
            UE_VLOG_LOCATION(ctx.AIController, LogTATConsiderationInputs, Log, targetLocation, 20.f, outputColor, TEXT("Target Location"));
            UE_VLOG_ARROW(ctx.AIController, LogTATConsiderationInputs, Log, guardLocation, targetLocation,outputColor, TEXT("Distance %f"), result.PathLength);
#endif // ENABLE_VISUAL_LOG
         }
         else
         {
            const float distanceFromTarget = FVector::Distance(targetLocation, guardLocation);
            bOutput = UOSEMathFunctionLibrary::CompareFloats(distanceFromTarget, Distance, ComparisonMethod);
#if ENABLE_VISUAL_LOG
            const FColor outputColor = bOutput ? FColor::Green : FColor::Red;
            UE_VLOG_LOCATION(ctx.AIController, LogTATConsiderationInputs, Log, guardLocation, 50.f, outputColor, TEXT("Guard Location"));
            UE_VLOG_LOCATION(ctx.AIController, LogTATConsiderationInputs, Log, targetLocation, 20.f, outputColor, TEXT("Target Location"));
            UE_VLOG_ARROW(ctx.AIController, LogTATConsiderationInputs, Log, guardLocation, targetLocation,outputColor, TEXT("Distance %f"), distanceFromTarget);
#endif // ENABLE_VISUAL_LOG
         }
         return bOutput ? 1.0f : 0.0f;
      }
   }
   return 0.f;
}

float UConsiderationInput_SmartObjectIsWithinRange::GetValue(const FConsiderationContext& ctx) const
{
   if (ctx.Target.TargetType != EBehaviorTargetType::SmartObjectRequest)
      return 0.0f;

   const UTATSmartObjectComponent* smartObjComp = Cast<UTATSmartObjectComponent>(ctx.Target.GetTargetUObject());
   if (!smartObjComp)
      return 0.0f;

   const float internalValue = _InternalGetValue(ctx.AIController, ctx.Target.GetTargetWorldLocation());
#if ENABLE_VISUAL_LOG
   UE_VLOG_CIRCLE(ctx.AIController, LogTATConsiderationInputs, Log, ctx.Target.GetTargetWorldLocation(), FVector::UpVector, Distance, FColor::Red, TEXT("Smart Object Check"));
#endif // ENABLE_VISUAL_LOG
   return internalValue;
}

float UConsiderationInput_SmartObjectHasTag::GetValue(const FConsiderationContext& ctx) const
{
   
   CONSIDERATION_SCOPE(SmartObjectHasTag);

   if(const USmartObjectSubsystem* smartObjectSubsystem = ctx.AIController->GetWorld()->GetSubsystem<USmartObjectSubsystem>())
   {
      const FGameplayTagContainer& gameplayTagContainer = smartObjectSubsystem->GetInstanceTags(
         ctx.Target.SmartObjectRequestTarget.SmartObjectHandle);
      return gameplayTagContainer.HasTag(GameplayTag) ? 1.0f : 0.0f;
   }
   return 0.f;
}

float UConsiderationInput_AlertnessValueChangedFromBlackboardValue::GetValue(const FConsiderationContext& ctx) const
{
   const UBlackboardComponent* bb = ctx.AIController->GetBlackboardComponent();
   const uint8 bbEnumValue = bb->GetValueAsEnum(ATATAIController::GetAlertnessLevelBlackboardKey());
   const uint8 currentEnumValue = static_cast<uint8>(ctx.AIController->GetAlertnessLevel());
   if(bbEnumValue != currentEnumValue)
   {
      return 1.f;
   }
   return 0.f;
}

float UConsiderationInput_HasMajorLoot::GetValue(const FConsiderationContext& ctx) const
{
   if(ctx.Character == nullptr)
      return 0.f;

   const ITATLootInventoryInterface* inventoryInterface = Cast<ITATLootInventoryInterface>(ctx.Character);
   if(inventoryInterface == nullptr)
      return 0.f;

   const UTATLootInventoryComponent* lootInventoryComponent = inventoryInterface->GetLootInventoryComponent();
   if(lootInventoryComponent == nullptr)
      return 0.f;

   return lootInventoryComponent->HasMajorLoot() ? 1.f : 0.f;
}

float UConsiderationInput_AlarmStationState::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(AlarmStationState);
   if (ctx.Target.Component.IsValid() == false)
      return 0.f;
   if (const ATATSquadAlarmStation* targetAlarmStation = Cast<ATATSquadAlarmStation>(ctx.Target.Component->GetOwner()))
   {
      return UOSEMathFunctionLibrary::CompareInts(static_cast<int>(targetAlarmStation->GetState()),
         static_cast<int>(AlarmState), ComparisonMethod)
         ? 1.0f
         : 0.0f;
   }

   return 0.0f;
}

float UConsiderationInput_LivingWorldAgentHasClaimedHandles::GetValue(const FConsiderationContext& ctx) const
{
   if(ctx.Character == nullptr)
      return 0.f;

   const ITATLivingWorldAgentInterface* livingWorldAgentInterface = Cast<ITATLivingWorldAgentInterface>(ctx.Character);
   if(livingWorldAgentInterface == nullptr)
      return 0.f;

   const UTATLivingWorldAgentComponent* livingWorldAgentComponent = livingWorldAgentInterface->
      GetLivingWorldAgentComponent();
   if(livingWorldAgentComponent == nullptr)
      return 0.f;

   return livingWorldAgentComponent->HasValidClaimedSmartObjectHandles() ? 1.f : 0.f;
}

float UConsiderationInput_GetDistanceOfClosestActorOfKnowledgeTypeInRange::GetValue(const FConsiderationContext& ctx) const
{
   const ATATAIController* aiController = Cast<ATATAIController>(ctx.AIController);
   check(aiController);

   const UTATKnowledgeComponent* knowledgeComponent = aiController->GetTATKnowledgeComponent();
   check(knowledgeComponent);

   const FVector currentActorLocation = ctx.Character->GetActorLocation();
   float closestDistance = {UE_MAX_FLT};
   for (FTATActorKnowledge actorKnowledge : knowledgeComponent->GetKnownActors())
   {
      if(RequiredAttitude != actorKnowledge.GetAttitude())
         continue;
      FVector lastKnownLocation = actorKnowledge.GetLastKnownLocation();
      if(lastKnownLocation == FAISystem::InvalidLocation)
         continue;
      const float distance = FVector::Distance(currentActorLocation, lastKnownLocation);
      if(distance < MinDistanceForKnowledgeActor && distance < closestDistance)
      {
         closestDistance = distance;
      }
   }
   // Normalize return value to 0 - 1 where 1 == 0 Distance && 0 == MinDistanceFromKnowledgeActor.
   return FMath::GetMappedRangeValueClamped(FVector2D(0,MinDistanceForKnowledgeActor), FVector2D(1.f, 0.f), closestDistance );
}

float UConsiderationInput_HasIndividualKnowledgeOfTarget::GetValue(const FConsiderationContext& ctx) const
{
   const IOSEIndividualKnowledgeInterface* individualKnowledgeInterface = Cast<IOSEIndividualKnowledgeInterface>(
      ctx.AIController
   );
   if(const UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = individualKnowledgeInterface->
      GetIndividualKnowledgeComponent())
   {
      const AActor* target = TATConsiderationInputHelpers::TryRetrieveActorFromContext(ctx);
      const FIndividualKnowledge* individualKnowledge = individualKnowledgeComponent->FindKnowledgeForActor(target);
      if(individualKnowledge == nullptr)
      {
         return 0.f;
      }
      return QueryToRun.Matches(individualKnowledge->GameplayTagContainer) ? 1.f : 0.f;
   }
   return 0.f;
}

float UConsiderationInput_HasAnyReactionRoleClaimed::GetValue(const FConsiderationContext& ctx) const
{
   if (UTATAIReactionCoordinatorSubsystem* airc = ctx.AIController->GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>())
   {
      if (ATATCharacterAIBase* aiCharacter = Cast<ATATCharacterAIBase>(ctx.Character))
      {
         FTATAIReactionTarget reactionTarget = TATConsiderationInputHelpers::GenerateReactionTargetFromContext(ctx);
         if (reactionTarget.IsValid())
         {
            const bool conditionMet = airc->IsAIRegisteredWithEvent(aiCharacter, reactionTarget);
            return conditionMet ? 1.0f : 0.0f;
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_IsReactionRoleAvailableOrClaimed::GetValue(const FConsiderationContext& ctx) const
{
   if (UTATAIReactionCoordinatorSubsystem* airc = ctx.AIController->GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>())
   {
      if (ATATCharacterAIBase* aiCharacter = Cast<ATATCharacterAIBase>(ctx.Character))
      {
         FTATAIReactionTarget reactionTarget = TATConsiderationInputHelpers::GenerateReactionTargetFromContext(ctx);
         if (reactionTarget.IsValid())
         {
            bool eventConfigExists = true;
            const bool conditionMet = airc->IsRoleAvailableOrClaimedByAI(reactionTarget, RoleTag, aiCharacter, eventConfigExists);
            return (conditionMet || (SucceedIfNoEventConfig && !eventConfigExists)) ? 1.0f : 0.0f;
         }
      }
   }
   return 0.0f;
}
