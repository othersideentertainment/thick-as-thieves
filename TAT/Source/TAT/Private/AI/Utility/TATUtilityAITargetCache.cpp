// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/TATUtilityAITargetCache.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATKnowledgeComponent.h"
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "Character/TATCharacterAIBase.h"
#include "Developer/TATProjectSettings.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "AI/OSEAIController.h"
#include "AI/Utility/OSESmartObjectCacheSubsystem.h"
#include "Character/OSETeamInterface.h"

// ue
#include "BlackboardKeyType_SOClaimHandle.h"
#include "SmartObjectComponent.h"
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUtilityAITargetCache)

bool FPotentialTargetsCache::HasCachedPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   return _FindPotentialTargets(targeting, targetingGroup) != nullptr;
}

void FPotentialTargetsCache::CachePotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup, ATATAIController* aiController)
{
   check(!HasCachedPotentialTargetsFor(targeting, targetingGroup));

   // always add before early-out
   TArray<FPotentialTargetsGroup>& targetGroups = _potentialTargets.FindOrAdd(targeting);
   FPotentialTargetsGroup& targetGroup = targetGroups.Emplace_GetRef(FPotentialTargetsGroup(targetingGroup));
   TArray<FUtilityStateTarget>& targets = targetGroup.Targets;

   if (!aiController)
      return;

   const UTATKnowledgeComponent* knowledge = aiController->GetTATKnowledgeComponent();
   if (!knowledge)
      return;

   const ATATCharacterAIBase* myCharacter = Cast<ATATCharacterAIBase>(aiController->GetOSECharacter());
   if (!myCharacter)
      return;

   switch (targeting)
   {
   case EUtilityStateTargeting::Actors:
      {
         // Ask knowledge component for actors I know of.
         for (const FTATActorKnowledge& info : knowledge->GetKnownActors())
         {
            if (AActor* actor = info.GetActor())
            {
               _TryAddToCache(actor, targetingGroup, targets);
            }
         }
      }
      break;
   case EUtilityStateTargeting::Allies:
      {
         // Ask knowledge component for actors I know of and store off ones I'm friendly targets.
         for (const FTATActorKnowledge& info : knowledge->GetKnownActors())
         {
            if (info.GetAttitude() == EOSETeamAttitude::Friendly)
            {
               // An ally!
               if (AActor* actor = info.GetActor())
               {
                  _TryAddToCache(actor, targetingGroup, targets);
               }
            }
         }
      }
      break;
   case EUtilityStateTargeting::Enemies:
      {
         // Ask knowledge component for actors I know of and store off ones I'm hostile targets.
         for (const FTATActorKnowledge& info : knowledge->GetKnownActors())
         {
            if (info.GetAttitude() == EOSETeamAttitude::Hostile)
            {
               // An enemy!
               AActor* actor = info.GetActor();
               if (actor && _IsAllowedToTargetEnemyActor(actor))
               {
                  _TryAddToCache(actor, targetingGroup, targets);
               }
            }
         }
      }
      break;
   case EUtilityStateTargeting::Stims:
      {
         // Ask the stim database for stims!
         if (const IOSEStimDatabaseInterface* dbOwner = Cast<IOSEStimDatabaseInterface>(aiController))
         {
            if (const UOSEStimDatabase* db = dbOwner->AuthorityGetStimDatabase())
            {
               for (const FStimInfo& stim : db->GetKnownStims())
               {
                  targets.Emplace(stim);
               }
            }
         }
      }
      break;
   case EUtilityStateTargeting::SuspiciousAllies:
      {
         // Ask knowledge component for actors I know of and store off ones I'm friendly towards.
         for (const FTATActorKnowledge& info : knowledge->GetKnownActors())
         {
            if (info.IsSupiciousAlly())
            {
               _TryAddToCache(info.GetActor(), targetingGroup, targets);
            }
         }
      }
      break;
   case EUtilityStateTargeting::SmartObjects:
      {
         const USmartObjectSubsystem* smartObjectSubsystem = _world->GetSubsystem<USmartObjectSubsystem>();

         for (const FSmartObjectRequestResult& smartObjectRequestResult : _smartObjects)
         {
            // If we already own the claim to this smart object OR the smart object is available to claim, try adding it
            if (smartObjectSubsystem->CanBeClaimed(
               smartObjectRequestResult.SlotHandle))
            {
               _TryAddToCache(smartObjectRequestResult, targetingGroup, targets);
            }
         }
         // try and add any smart object that we're aware of, not just ones that are included in the local scan
         // potentially we want to fully remove the regular scan and just rely on the perception system for nearby "known" nodes
         // however currently some nodes have no visual representation so they won't show up in the actor knowledge list (search nodes for example)
         for (const FTATActorKnowledge& info : knowledge->GetKnownActors())
         {
            if (AActor* actor = info.GetActor())
            {
               if(const ITATSmartObjectOwnerInterface* smartObjectOwnerInterface = Cast<ITATSmartObjectOwnerInterface>(actor))
               {
                  _TryAddToCache(actor, smartObjectOwnerInterface->GetSmartObjectComponent(), targetingGroup, targets);
               }
            }
         }

         FSmartObjectClaimHandle claimHandle = FSmartObjectClaimHandle::InvalidHandle;
         if(const UBlackboardComponent* blackboard = aiController->GetBlackboardComponent())
         {
            claimHandle = blackboard->GetValue<UBlackboardKeyType_SOClaimHandle>(FName(TEXT("SmartObjectClaimHandleKey")));
         }
         // make sure to grab the current claimed smart object handle from the blackboard,
         // the above code will not find ones that are currently claimed (even by the agent 
         if(claimHandle.IsValid())
         {
            if(USmartObjectComponent* smartObjectComponent = smartObjectSubsystem->GetSmartObjectComponentByHandle(
               claimHandle.SmartObjectHandle))
            {
               const ITATUtilityAITargetingGroupInterface* targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(smartObjectComponent);
               if(!targetingGroupInterface)
               {
                  // If the component doesn't have the interface, the owner might
                  targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(smartObjectComponent->GetOwner());
               }
               
               // early out for smart objects that don't have a targeting group interface, ideally this should be a check as all SO should have this
               if(!targetingGroupInterface)
                  break;

               if (targetingGroupInterface->GetUtilityAITargetingGroup() != targetingGroup)
                  break;
               
               FSmartObjectRequestResult result = FSmartObjectRequestResult(claimHandle.SmartObjectHandle, claimHandle.SlotHandle);
               targets.Emplace(result, _world.Get(), smartObjectComponent);
            }
         }
         
      }
      break;
   default:
      {
         checkNoEntry();
      }
      break;
   }
}

const TArray<FUtilityStateTarget>& FPotentialTargetsCache::GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   check(HasCachedPotentialTargetsFor(targeting, targetingGroup));
   const FPotentialTargetsGroup* targetGroup = _FindPotentialTargets(targeting, targetingGroup);
   check(targetGroup);
   return targetGroup->Targets;
}

void FPotentialTargetsCache::ResetPotentialTargetCache()
{
   _potentialTargets.Reset();
}

void FPotentialTargetsCache::SetWorld(UWorld* world)
{
   check(world);
   _world = world;
}

void FPotentialTargetsCache::AddAllowedEnemyActor(AActor* actor, ATATAIController* aiController)
{
   if (UOSEAbilityFunctionLibrary::IsHostileToActor(actor, aiController ? aiController->GetPawn() : nullptr))
   {
      _allowedEnemyActors.AddUnique(actor);
   }
}

void FPotentialTargetsCache::RemoveAllowedEnemyActor(AActor* actor)
{
   _allowedEnemyActors.Remove(actor);
}

void FPotentialTargetsCache::SetSmartObjects(TArray<FSmartObjectRequestResult>&& smartObjects)
{
   _smartObjects = MoveTemp(smartObjects);
}

bool FPotentialTargetsCache::_IsAllowedToTargetEnemyActor(AActor* actor) const
{
   return _allowedEnemyActors.Num() == 0 || _allowedEnemyActors.Contains(actor);
}

bool FPotentialTargetsCache::_TryAddToCache(AActor* actor, const FGameplayTag& targetingGroup, TArray<FUtilityStateTarget>& targets) const
{
   if (!IsValid(actor))
      return false;

   const ITATUtilityAITargetingGroupInterface* targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(actor);
   if (targetingGroupInterface && targetingGroupInterface->GetUtilityAITargetingGroup() != targetingGroup)
      return false;

   targets.Emplace(actor);
   return true;
}

bool FPotentialTargetsCache::_TryAddToCache(const FSmartObjectRequestResult& smartObjectRequest, const FGameplayTag& targetingGroup, TArray<FUtilityStateTarget>& targets) const
{
   if (!smartObjectRequest.IsValid() || !_world.IsValid())
      return false;
   
   UOSESmartObjectCacheSubsystem* smartObjectCacheSubsystem = _world->GetSubsystem<UOSESmartObjectCacheSubsystem>();
   if (!smartObjectCacheSubsystem)
      return false;

   USmartObjectComponent* smartObjComponent = smartObjectCacheSubsystem->GetSmartObjectComponentForHandle(smartObjectRequest.SmartObjectHandle);
   if (!smartObjComponent)
      return false;

   const ITATUtilityAITargetingGroupInterface* targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(smartObjComponent);
   if(!targetingGroupInterface)
   {
      // If the component doesn't have the interface, the owner might
      targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(smartObjComponent->GetOwner());
   }

   if(!targetingGroupInterface)
   {
      return false;
   }
   
   if (targetingGroupInterface->GetUtilityAITargetingGroup() != targetingGroup)
      return false;

   targets.Emplace(smartObjectRequest, _world.Get(), smartObjComponent);
   return true;
}


bool FPotentialTargetsCache::_TryAddToCache(const AActor* owner, USmartObjectComponent* smartObjectComponent, const FGameplayTag& targetingGroup, TArray<FUtilityStateTarget>& targets) const
{
   if (!smartObjectComponent || !owner)
      return false;
   
   const ITATUtilityAITargetingGroupInterface* targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(smartObjectComponent);
   if(!targetingGroupInterface)
   {
      // If the component doesn't have the interface, the owner might
      targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(owner);
   }

   // early out for smart objects that don't have a targeting group interface, ideally this should be a check as all SO should have this
   if(!targetingGroupInterface)
      return false;

   if (targetingGroupInterface->GetUtilityAITargetingGroup() != targetingGroup)
      return false;


   const USmartObjectSubsystem* smartObjectSubsystem = _world->GetSubsystem<USmartObjectSubsystem>();
   if(!smartObjectSubsystem)
      return false;
   
   const FSmartObjectRequestFilter filter;
   TArray<FSmartObjectSlotHandle> outSlots;
   smartObjectSubsystem->FindSlots(smartObjectComponent->GetRegisteredHandle(), filter, outSlots);
   
   for (const FSmartObjectSlotHandle& slot : outSlots)
   {
      FSmartObjectRequestResult result = FSmartObjectRequestResult(smartObjectComponent->GetRegisteredHandle(), slot);
      targets.Emplace(result, _world.Get(), smartObjectComponent);
   }
   return true;
}



const FPotentialTargetsGroup* FPotentialTargetsCache::_FindPotentialTargets(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   if (const TArray<FPotentialTargetsGroup>* targets = _potentialTargets.Find(targeting))
   {
      for(const FPotentialTargetsGroup& target : (*targets))
      {
         if (target.TargetGroup == targetingGroup)
            return &target;
      }
   }
   return nullptr;
}

