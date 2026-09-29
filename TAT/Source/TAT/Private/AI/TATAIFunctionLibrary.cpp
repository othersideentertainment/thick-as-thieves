// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/TATAIFunctionLibrary.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"

// ose
#include "OSECommon.h"

// ue4
#include "AIController.h"
#include "AISystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BrainComponent.h"
#include "GameplayTagAssetInterface.h"
#include "NavigationSystem.h"
#include "NavigationSystemTypes.h"
#include "NavLinkCustomComponent.h"
#include "SmartObjectSubsystem.h"
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIFunctionLibrary)
DEFINE_LOG_CATEGORY_STATIC(LogTATAIFunctionLibrary, Log, All);

int UTATAIFunctionLibrary::AuthorityForEachPlayerKnowledge(const AActor* actor, const TFunctionRef<void(const FTATActorKnowledge&, const APawn&)>& cb)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATAIFunctionLibrary_AuthorityForEachPlayerKnowledge);

   int numPlayers = 0;

   if (const UTATKnowledgeComponent* knowledgeComponent = UTATKnowledgeComponent::TryGet(actor))
   {
      const TArray<FTATActorKnowledge>& knownActors = knowledgeComponent->GetKnownActors();
      for (const FTATActorKnowledge& actorKnowledge : knownActors)
      {
         if (actorKnowledge.IsPlayer())
         {
            if (APawn* playerPawn = Cast<APawn>(actorKnowledge.GetActor()))
            {
               cb(actorKnowledge, *playerPawn);
               ++numPlayers;
            }
         }
      }
   }

   return numPlayers;
}

int UTATAIFunctionLibrary::AuthorityForEachEnemyKnowledge(const AActor* actor, const TFunctionRef<void(const FTATActorKnowledge&)>& cb)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATAIFunctionLibrary_AuthorityForEachEnemyKnowledge);

   int numEnemies = 0;

   if (!actor)
      return numEnemies;

   const ATATAIController* aiController = UOSECommon::GetController<ATATAIController>(actor);
   if (!aiController)
      return numEnemies;

   UTATKnowledgeComponent* knowledgeComponent = aiController->GetTATKnowledgeComponent();
   if (!knowledgeComponent)
      return numEnemies;

   const TArray<FTATActorKnowledge>& knownActors = knowledgeComponent->GetKnownActors();
   for (const FTATActorKnowledge& actorKnowledge : knownActors)
   {
      if (actorKnowledge.IsEnemy())
      {
         cb(actorKnowledge);
         ++numEnemies;
      }
   }

   return numEnemies;
}

const APawn* UTATAIFunctionLibrary::AuthorityFindClosestVisibleDetectedPlayerPawn(const AActor* actor)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_TATAIFunctionLibrary_AuthorityFindClosestVisibleDetectedPlayerPawn);

   FVector myLocation = actor->GetActorLocation();

   const APawn* closestPlayer = nullptr;
   float closestPlayerDistance = 0.0f;
   float closestPlayerDistanceSq = 0.0f;

   UTATAIFunctionLibrary::AuthorityForEachPlayerKnowledge(actor,
      [&myLocation, &closestPlayer, &closestPlayerDistance, &closestPlayerDistanceSq](const FTATActorKnowledge& actorKnowledge, const APawn& playerPawn)
      {
         // requires visibility
         if (!actorKnowledge.GetIsVisible())
            return;

         // requires some detection first
         if (actorKnowledge.GetDetectionValue() == 0.0f)
            return;

         // should be our visible location
         FVector visibleLocation = actorKnowledge.GetLastKnownVisibleLocation();
         float playerDistanceSq = FVector::DistSquared(myLocation, visibleLocation);

         if (!closestPlayer)
         {
            closestPlayer = &playerPawn;
            closestPlayerDistance = FVector::Distance(myLocation, visibleLocation);
            closestPlayerDistanceSq = playerDistanceSq;
         }
         else if (playerDistanceSq < closestPlayerDistanceSq)
         {
            closestPlayer = &playerPawn;
            closestPlayerDistance = FVector::Distance(myLocation, visibleLocation);
         }
      }
   );

   return closestPlayer;
}

bool UTATAIFunctionLibrary::IsBamboozled(const AActor* actor)
{
   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(actor))
   {
      const UTATAISettings& settings = UTATAISettings::Get();
      return tagInterface->HasMatchingGameplayTag(settings.BamboozledTag);
   }
   return false;
}

bool UTATAIFunctionLibrary::CancelBamboozledEffects(const AActor* actor)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      const UTATAISettings& settings = UTATAISettings::Get();
      const int numRemoved = asc->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(settings.BamboozledTag));
      return numRemoved > 0;
   }
   return false;
}

void UTATAIFunctionLibrary::FindNavLinkStartingPoints(const TArray<UNavLinkCustomComponent*>& navLinkComponents, TArray<FVector>& outStartingPoints)
{
   outStartingPoints.Reset();
   outStartingPoints.Reserve(navLinkComponents.Num() * 2);
   for (UNavLinkCustomComponent* navLinkComponent : navLinkComponents)
   {
      FVector unusedLeft, unusedRight;
      ENavLinkDirection::Type direction;
      navLinkComponent->GetLinkData(unusedLeft, unusedRight, direction);

      // Starting point for navlink is left point (aka "start point")
      if (direction == ENavLinkDirection::LeftToRight || direction == ENavLinkDirection::BothWays)
      {
         outStartingPoints.Add(navLinkComponent->GetStartPoint());
      }

      // Starting point for navlink is right point (aka "end point")
      if (direction == ENavLinkDirection::RightToLeft || direction == ENavLinkDirection::BothWays)
      {
         outStartingPoints.Add(navLinkComponent->GetEndPoint());
      }
   }
}

void UTATAIFunctionLibrary::SetNavLinkEnabled(UNavLinkCustomComponent* navlink, bool enabled)
{
   if (!navlink)
   {
      UE_LOG(LogTATAIFunctionLibrary, Error, TEXT("SetNavLinkEnabled() called with invalid navlink!"));
      return;
   }

   navlink->SetEnabled(enabled);
}

bool UTATAIFunctionLibrary::GetNavLinkEnabled(const UNavLinkCustomComponent* navlink)
{
   if (!navlink)
   {
      UE_LOG(LogTATAIFunctionLibrary, Error, TEXT("GetNavLinkEnabled() called with invalid navlink!"));
      return false;
   }

   return navlink->IsEnabled();
}

FVector UTATAIFunctionLibrary::FindPositionOfInteractableComponents(AActor* interactableActor)
{
   static const FName kInteractName(TEXT("Interact"));
   TArray<UActorComponent*> interactbleComponents = interactableActor->GetComponentsByTag(USceneComponent::StaticClass(), kInteractName);

   if (interactbleComponents.Num() == 0)
   {
      return interactableActor->GetActorLocation();
   }

   FVector totalInteractablePosition = FVector::ZeroVector;
   int32 numInteractableComponents = 0;
   for (UActorComponent* interactbleComponent : interactbleComponents)
   {
      if (USceneComponent* interactbleSceneComponent = Cast<USceneComponent>(interactbleComponent))
      {
         totalInteractablePosition += interactbleSceneComponent->GetComponentLocation();
         numInteractableComponents++;
      }
   }

   check(numInteractableComponents > 0);

   return totalInteractablePosition / numInteractableComponents;
}

bool UTATAIFunctionLibrary::IsKnowledgeAboutActor(const FTATActorKnowledge& knowledge, const AActor* actor)
{
   return knowledge.GetActor() == actor;
}

EActorDetectionState UTATAIFunctionLibrary::GetKnowledgeDetectionState(const FTATActorKnowledge& knowledge)
{
   return knowledge.GetDetectionState();
}

void UTATAIFunctionLibrary::LockAIResourcesOnPawn(APawn* pawn, bool bLockMovement, bool bLockLogic)
{
   if (pawn == nullptr)
   {
      return;
   }

   if (ATATAIController* aiController = Cast<ATATAIController>(pawn->GetController()))
   {
      aiController->LockAIResources(bLockMovement, bLockLogic);
   }
}

void UTATAIFunctionLibrary::UnlockAIResourcesOnPawn(APawn* pawn, bool bLockMovement, bool bLockLogic)
{
   if (pawn == nullptr)
   {
      return;
   }

   if (ATATAIController* aiController = Cast<ATATAIController>(pawn->GetController()))
   {
      aiController->UnlockAIResources(bLockMovement, bLockLogic);
   }
}

void UTATAIFunctionLibrary::GetAIResourceLockStates(APawn* pawn, bool& isMovementLocked, bool& isLogicLocked)
{
   if (pawn == nullptr)
   {
      return;
   }

   if (ATATAIController* aiController = Cast<ATATAIController>(pawn->GetController()))
   {
      aiController->GetAIResourceLockStates(isMovementLocked, isLogicLocked);
   }
}

bool UTATAIFunctionLibrary::FindClosestSmartObjectSlotToActor(
   const AActor* queryingActor, 
   const TArray<FSmartObjectRequestResult>& smartObjectSlots,
   FSmartObjectRequestResult& outClosestSlot)
{
   outClosestSlot = FSmartObjectRequestResult();
   
   const ATATAIController* aiController = UOSECommon::GetController<ATATAIController>(queryingActor);
   if (aiController == nullptr)
   {
      UE_LOG(LogTATAIFunctionLibrary, Error, TEXT("FindClosestSmartObjectSlotToActor could not find a TATAIController on %s!"),
         *GetNameSafe(queryingActor));
      return false;
   }

   UTATKnowledgeComponent* knowledgeComponent = aiController->GetTATKnowledgeComponent();
   if (knowledgeComponent == nullptr)
   {
      UE_LOG(LogTATAIFunctionLibrary, Error, TEXT("FindClosestSmartObjectSlotToActor could not find a TATKnowledgeComponent on %s!"),
         *aiController->GetName());
      return false;
   }

   const USmartObjectSubsystem* smartObjectSubsystem = queryingActor->GetWorld()->GetSubsystem<USmartObjectSubsystem>();
   if (smartObjectSubsystem == nullptr)
   {
      UE_LOG(LogTATAIFunctionLibrary, Error, TEXT("FindClosestSmartObjectSlotToActor failed to grab the smart object subsystes!"));
      return false;
   }

   float closestSlotDistance = FLT_MAX;
   for (const FSmartObjectRequestResult& slot : smartObjectSlots)
   {
      const USmartObjectComponent* smartObjectComponent = smartObjectSubsystem->GetSmartObjectComponentByHandle(slot.SmartObjectHandle);
      if (ensureMsgf(smartObjectComponent != nullptr, TEXT("smartObjectSlots contained an invalid request result.")))
      {
         const bool hasPath = knowledgeComponent->DoesFullPathToSmartObjectSlotLocationExist(smartObjectComponent, slot.SlotHandle);
         const float slotDistance = knowledgeComponent->GetPathLengthToSmartObjectSlotLocation(smartObjectComponent, slot.SlotHandle);
         if (hasPath && slotDistance < closestSlotDistance)
         {
            outClosestSlot = slot;
            closestSlotDistance = slotDistance;
         }
      }
   }

   return outClosestSlot.IsValid();
}
