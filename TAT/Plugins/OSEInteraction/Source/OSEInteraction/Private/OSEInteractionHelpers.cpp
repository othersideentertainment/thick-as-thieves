// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/OSEInteractionHelpers.h"

#include "Interactables/InteractableInterface.h"
#include "Interactables/InteractAbilityInstantAnimationInterface.h"
#include "Interactables/InteractHoldAbilityInterface.h"
#include "Interactables/InteractPromptAbilityInterface.h"

// ose
#include "OSECoreCollision.h"
#include "OSEProjectSettings.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/OSEAbilitySystemGlobals.h"
#include "Abilities/OSEAbilitySystemComponent.h"

// ue4
#include "Components/TimelineComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEInteractionHelpers)

namespace InteractCVars
{
   static float TimelineServerTolerance = 0.5f;
   FAutoConsoleVariableRef CVarTimelineServerTolerance(
      TEXT("OSE.Interact.TimelineServerTolerance"),
      TimelineServerTolerance,
      TEXT("Tolerance on server when considering timeline complete for toggling purposes"),
      ECVF_Default);
}

const FName UOSEInteractionHelpers::kInteractTag("Interact");
const FName UOSEInteractionHelpers::kInteractNoHighlightTag("InteractNoHighlight");

bool FOSEToggleState::IsOld(const UObject* worldContext, float threshold) const
{
   float now = UOSEInteractionHelpers::GetServerTimeForComparison(worldContext);
   return ChangedServerTime == 0 || (now - ChangedServerTime) >= threshold;
}

float UOSEInteractionHelpers::GetServerTimeForWrite(const UObject* WorldContext)
{
   AGameStateBase* gameState = WorldContext->GetWorld()->GetGameState();
   return gameState ? gameState->GetServerWorldTimeSeconds() : 0;
}

float UOSEInteractionHelpers::GetServerTimeForComparison(const UObject* worldContext)
{
   // If no game state found, use large number so comparisons against it will fail
   AGameStateBase* gameState = worldContext->GetWorld()->GetGameState();
   return gameState ? gameState->GetServerWorldTimeSeconds() : (float)MAX_uint32;
}

bool UOSEInteractionHelpers::IsOld(const UObject* worldContext, float serverTimestamp, float threshold /*= 0.75f*/)
{
   const float age = GetServerTimeForComparison(worldContext) - serverTimestamp;
   return serverTimestamp == 0 || age > threshold;
}

void UOSEInteractionHelpers::SyncTimelineWithToggle(const FOSEToggleState& state, UTimelineComponent* timeline)
{
   check(timeline);
   const float length = timeline->GetTimelineLength();
   const float stateAge = GetServerTimeForComparison(timeline) - state.ChangedServerTime;

   if(state.ChangedServerTime == 0 || stateAge > length)
   {
      float newPosition = state.bIsOn ? length : 0;
      // TODO: is it correct to fire events?
      timeline->SetPlaybackPosition(newPosition, true);
      timeline->Stop();
   }
   else if(state.bIsOn)
   {
      timeline->Play();
   }
   else
   {
      timeline->Reverse();
   }
}

bool UOSEInteractionHelpers::IsTimelineCompleteForTransition(UTimelineComponent* timeline)
{
   check(timeline);

   // if timeline is not playing, it is complete
   if (!timeline->IsPlaying())
   {
      return true;
   }

   // if on server, give more tolerance
   if (timeline->GetOwnerRole() == ROLE_Authority)
   {
      float secondsUntilComplete = timeline->IsReversing() ?
         timeline->GetPlaybackPosition() :
         timeline->GetTimelineLength() - timeline->GetPlaybackPosition();
      return secondsUntilComplete < InteractCVars::TimelineServerTolerance;
   }

   return false;
}

namespace InteractionCasterImpl
{
   static void CastForAllInteractables(ACharacter* interactingCharacter, FVector startPoint, FVector endPoint, TArray<FHitResult>& hitResults, const FCollisionResponseParams& responseParams)
   {
      // Ignore myself
      FCollisionQueryParams queryParams(SCENE_QUERY_STAT(Interact));
      queryParams.AddIgnoredActor(interactingCharacter);
      interactingCharacter->GetWorld()->LineTraceMultiByChannel(hitResults, startPoint, endPoint, COLLISION_INTERACT, queryParams, responseParams);
   }
}

TScriptInterface<IInteractableInterface> UOSEInteractionHelpers::CastForInteractableHits(ACharacter* interactingCharacter, FVector startPoint, FVector endPoint, FHitResult& outHitResult, const FCollisionResponseParams& responseParams)
{
   TArray<FHitResult> hits;
   InteractionCasterImpl::CastForAllInteractables(interactingCharacter, startPoint, endPoint, hits, responseParams);

   TScriptInterface<IInteractableInterface> result;
   for (const FHitResult& hit : hits)
   {
      TScriptInterface<IInteractableInterface> Found = GetInteractableFromHit(interactingCharacter, hit);
      if (Found.GetObject())
      {
         outHitResult = hit;
         return Found;
      }
   }

   if (hits.Num() > 0)
   {
      outHitResult = hits.Last();
   }
   else
   {
      outHitResult.TraceStart = startPoint;
      outHitResult.TraceEnd = endPoint;
      outHitResult.Location = endPoint;
   }
   return nullptr;
}

TScriptInterface<IInteractableInterface>
UOSEInteractionHelpers::CastForInteractables(ACharacter* interactingCharacter, FVector startPoint, FVector endPoint, const FCollisionResponseParams& responseParams)
{
   TArray<FHitResult> hits;
   InteractionCasterImpl::CastForAllInteractables(interactingCharacter, startPoint, endPoint, hits, responseParams);
   return SelectNearestInteractableFromArray(interactingCharacter, hits);
}

bool UOSEInteractionHelpers::SphereOverlapInteractables(ACharacter* interactingCharacter, const FVector spherePos, const float sphereRadius, TArray<TScriptInterface<IInteractableInterface>>* outInteractables, TArray<FOverlapResult>* outOverlapResults, const FCollisionResponseParams& responseParams)
{
   if (!interactingCharacter)
   {
      return false;
   }

   UWorld* world = interactingCharacter->GetWorld();
   if (!world)
   {
      return false;
   }

   const FCollisionShape sphere = FCollisionShape::MakeSphere(sphereRadius);
   
   int numFound = 0;
   TArray<FOverlapResult> overlaps;
   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(SphereOverlapInteractables));
   if (world->OverlapMultiByChannel(overlaps, spherePos, FQuat::Identity, COLLISION_INTERACT, sphere, queryParams, responseParams))
   {
      for (const FOverlapResult& overlap : overlaps)
      {
         if (!IsComponentTargetableForInteraction(overlap.GetComponent()))
         {
            continue;
         }

         AActor* actor = overlap.GetActor();
         if (actor && 
             actor->Implements<UInteractableInterface>() && 
             IInteractableInterface::Execute_IsInteractable(actor, interactingCharacter))
         {
            if (outInteractables)
               outInteractables->Add(actor);
            if (outOverlapResults)
               outOverlapResults->Add(overlap);
            ++numFound;
         }
         else
         {
            UPrimitiveComponent* component = overlap.GetComponent();
            if (component &&
                component->Implements<UInteractableInterface>() &&
                IInteractableInterface::Execute_IsInteractable(component, interactingCharacter))
            {
               if (outInteractables)
                  outInteractables->Add(component);
               if (outOverlapResults)
                  outOverlapResults->Add(overlap);
               ++numFound;
            }
         }
      }
      return numFound > 0;
   }

   return false;
}

TScriptInterface<IInteractableInterface> UOSEInteractionHelpers::GetInteractableFromHit(ACharacter* interactingCharacter, const FHitResult& hit)
{
   if (!IsComponentTargetableForInteraction(hit.Component.Get()))
   {
      return nullptr;
   }

   USceneComponent* possibleComponent = hit.Component.Get();
   while (possibleComponent)
   {
      if (possibleComponent->Implements<UInteractableInterface>() && IInteractableInterface::Execute_IsInteractable(possibleComponent, interactingCharacter))
      {
         return possibleComponent;
      }
      possibleComponent = possibleComponent->GetAttachParent();
   }

   AActor* hitActor = hit.GetActor();

   if (hitActor)
   {
      if (hitActor->Implements<UInteractableInterface>() && IInteractableInterface::Execute_IsInteractable(hitActor, interactingCharacter))
      {
         return hitActor;
      }

      // check non-scene components last (figure out priority later)
      const TSet<UActorComponent*>& components = hitActor->GetComponents();
      for (UActorComponent* component : components)
      {
         if (!component->IsA<USceneComponent>() && component->Implements<UInteractableInterface>() && IInteractableInterface::Execute_IsInteractable(component, interactingCharacter))
         {
            return component;
         }
      }
   }

   return nullptr;
}

AActor* UOSEInteractionHelpers::GetActorForInteractable(TScriptInterface<IInteractableInterface> taggable)
{
   UObject* interactObject = taggable.GetObject();
   if (AActor* actor = Cast<AActor>(interactObject))
   {
      return actor;
   }
   else if (UActorComponent* component = Cast<UActorComponent>(interactObject))
   {
      return component->GetOwner();
   }

   return nullptr;
}

bool UOSEInteractionHelpers::IsComponentTargetableForInteraction(USceneComponent* component)
{
   return component && (component->ComponentHasTag(kInteractTag) || component->ComponentHasTag(kInteractNoHighlightTag));
}

TScriptInterface<IInteractableInterface> UOSEInteractionHelpers::SelectNearestInteractableFromArray(ACharacter* interactingCharacter, const TArray<FHitResult>& interactablesArray)
{
   for (const FHitResult& hit : interactablesArray)
   {
      TScriptInterface<IInteractableInterface> found = GetInteractableFromHit(interactingCharacter, hit);
      if (found.GetObject())
      {
         return found;
      }
   }

   return nullptr;

   // TODO: Support interaction triggers and sort them less than solids
   // Waiting on this until we determine how much interaction/rigid-bodies we want in TAT
   // It would prefer objects as the following: (highest number is highest priority)
   // 4 - Interaction colliders
   // 3 - Direct Hit on non-trigger RB
   // 2 - Hit on Trigger RB
   // 1 - Hit on Collider (Non-RB)

   // TODO: Also support selecting the closest interactable to your ray (includes left+right)
   // The benefit of this is to be able to easily select/interact with objects on tables that are also interactable. (Sword On a table Immersive Sim problem)
}

FGameplayAbilityTargetDataHandle UOSEInteractionHelpers::MakeTargetDataFromInteractable(
   TScriptInterface<IInteractableInterface> interactable)
{
   FGameplayAbilityTargetDataHandle result;

   if (interactable.GetObject())
   {
      FGameplayAbilityTargetData_Interactable* targetData = new FGameplayAbilityTargetData_Interactable();
      targetData->Interactable = interactable.GetObject();
      result.Add(targetData);
   }

   return result;
}

TScriptInterface<IInteractableInterface> UOSEInteractionHelpers::GetInteractableFromTargetData(
   const FGameplayAbilityTargetDataHandle& targetDataHandle)
{
   const FGameplayAbilityTargetData* targetData = targetDataHandle.Get(0);
   if (targetData == nullptr)
   {
      return nullptr;
   }

   if (targetData->GetScriptStruct() == FGameplayAbilityTargetData_Interactable::StaticStruct())
   {
      const auto interactableTargetData = static_cast<const FGameplayAbilityTargetData_Interactable*>(targetData);
      UObject* possibleInteractable = interactableTargetData->Interactable.Get();
      if (possibleInteractable && possibleInteractable->Implements<UInteractableInterface>())
      {
         return possibleInteractable;
      }
   }

   return nullptr;
}

struct FCharacterInteractContext
{
   FCharacterInteractContext(ACharacter* interactor, const ACharacter* target, FGameplayEventData& eventData)
      : _interactor(interactor)
      , _target(target)
      , _interactorAsc(UOSEAbilitySystemGlobals::GetOSEAbilitySystemComponentFromActor(interactor))
      , _targetAsc(UOSEAbilitySystemGlobals::GetOSEAbilitySystemComponentFromActor(target))
      , _eventData(eventData)
   {
      if (IsValid())
      {
         _eventData.Target = target;
         _eventData.Instigator = interactor;
         _interactorAsc->GetOwnedGameplayTags(_eventData.InstigatorTags);
         _targetAsc->GetOwnedGameplayTags(_eventData.TargetTags);
      }
   }

   bool IsValid()
   {
      return _interactorAsc && _targetAsc;
   }

   const FGameplayAbilitySpec* FindHold()
   {
      return Find(UOSEProjectSettings::Get().CharacterInteractHoldTag);
   }

   const FGameplayAbilitySpec* FindPress()
   {
      return Find(UOSEProjectSettings::Get().CharacterInteractInstantTag);
   }

   const FGameplayAbilitySpec* Find(const FGameplayTag& tag)
   {
      _eventData.EventTag = tag;
      return _interactorAsc->FindFirstActivatableAbilityForGameplayEvent(_eventData);
   }

   bool TriggerAbility(const FGameplayTag& tag)
   {
      const FGameplayAbilitySpec* spec = Find(tag);
      if (spec == nullptr)
      {
         return false;
      }

      // Note: this may be false for network authority reasons, and so is not an error if false
      const FGameplayAbilitySpecHandle handle = spec->Handle;
      return _interactorAsc->TriggerAbilityFromGameplayEvent(handle, _interactorAsc->AbilityActorInfo.Get(), tag, &_eventData, *_interactorAsc);
   }
private:
   ACharacter* _interactor;
   const ACharacter* _target;
   UOSEAbilitySystemComponent* _interactorAsc;
   UOSEAbilitySystemComponent* _targetAsc;
   FGameplayEventData& _eventData;
};

FText UOSEInteractionHelpers::GetPromptForAbility(const UGameplayAbility* ability)
{
   if (!ability)
   {
      return FText::GetEmpty();
   }
   else if (ability->Implements<UInteractPromptAbilityInterface>())
   {
      return IInteractPromptAbilityInterface::Execute_GetInteractPromptVerb(ability);
   }
   else
   {
      // just fall back to something so it is obvious what is happening
      return FText::FromString(ability->GetName());
   }
}

FGameplayTag UOSEInteractionHelpers::GetPromptActionTagForAbility(const UGameplayAbility* ability)
{
   if (ability && ability->Implements<UInteractPromptAbilityInterface>())
   {
      return IInteractPromptAbilityInterface::Execute_GetInteractPromptActionTag(ability);
   }

   return FGameplayTag();
}

bool UOSEInteractionHelpers::HasCharacterInteractionAbility(ACharacter* interactor, const ACharacter* target)
{
   FGameplayEventData eventData;
   FCharacterInteractContext context(interactor, target, eventData);
   if (!context.IsValid()) return false;

   return context.FindHold() || context.FindPress();
}

bool UOSEInteractionHelpers::HasHoldCharacterInteraction(ACharacter* interactor, const ACharacter* target)
{
   FGameplayEventData eventData;
   FCharacterInteractContext context(interactor, target, eventData);
   if (!context.IsValid()) return false;

   return context.FindHold() != nullptr;
}

FCharacterInteractHoldInfo UOSEInteractionHelpers::GetHoldInfoForCharacterInteraction(ACharacter* interactor, const ACharacter* target, float defaultHoldDuration)
{
   FGameplayEventData eventData;
   FCharacterInteractContext context(interactor, target, eventData);
   if (!context.IsValid()) return { false };

   const FGameplayAbilitySpec* holdSpec = context.FindHold();
   if (holdSpec == nullptr) return { false };

   float holdDuration = defaultHoldDuration;
   FGameplayTag animationTag;
   const UGameplayAbility* ability = holdSpec->Ability;
   TSubclassOf<UGameplayEffect> holdTargetEffect = nullptr;
   if (ability->Implements<UInteractHoldAbilityInterface>())
   {
      const float possibleHoldDuration = IInteractHoldAbilityInterface::Execute_GetInteractHoldTime(ability, interactor, target);
      holdDuration = possibleHoldDuration > 0 ? possibleHoldDuration : defaultHoldDuration;
      holdTargetEffect = IInteractHoldAbilityInterface::Execute_GetInteractHoldTargetEffect(ability, target);
      animationTag = IInteractHoldAbilityInterface::Execute_GetInteractHoldAnimation(ability, target);
   }

   return { true, holdDuration, animationTag, holdTargetEffect, ability };
}

void UOSEInteractionHelpers::GetPromptForCharacterInteraction(ACharacter* interactor, const ACharacter* target, FInteractPrompt& outPrompt)
{
   FInteractPrompt prompt;
   FGameplayEventData eventData;
   FCharacterInteractContext context(interactor, target, eventData);
   if (!context.IsValid()) return;

   if (const FGameplayAbilitySpec* spec = context.FindHold())
   {
      outPrompt.HoldAction = GetPromptForAbility(spec->Ability);
      outPrompt.HoldActionTag = GetPromptActionTagForAbility(spec->Ability);
   }

   if (const FGameplayAbilitySpec* spec = context.FindPress())
   {
      outPrompt.PressAction = GetPromptForAbility(spec->Ability);
      outPrompt.PressActionTag = GetPromptActionTagForAbility(spec->Ability);
   }
}

bool UOSEInteractionHelpers::TriggerAbilityForCharacterInteraction(ACharacter* interactor, const ACharacter* target, bool hold)
{
   FGameplayEventData eventData;
   FCharacterInteractContext context(interactor, target, eventData);
   if (!context.IsValid())
   {
      return false;
   }

   // Note: I considered some other alternatives, such as
   // 1. Trying to trigger each ability, and stopping on the first valid one
   //    (This would have been a bit more efficient, since it could avoid
   //    evaluating conditions twice for the one that actually triggered,
   //    but it might be a bit less flexible if there are ever priorities
   //    added, plus asserts that the two match would negate a pref diff on
   //    dev builds.
   // 2. Pass the ability explicitly, if something else had to look it up already

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();
   const FGameplayTag eventTag = hold ? settings.CharacterInteractHoldTag : settings.CharacterInteractInstantTag;
   
   return context.TriggerAbility(eventTag);
}

FCharacterInteractInstantAnimationInfo UOSEInteractionHelpers::GetInstantAnimationForCharacterInteraction(ACharacter* interactor, const ACharacter* target)
{
   FGameplayEventData eventData;
   FCharacterInteractContext context(interactor, target, eventData);
   if (!context.IsValid()) return { false };

   const FGameplayAbilitySpec* abilitySpec = context.FindPress();
   if (abilitySpec == nullptr) return { false };

   const UGameplayAbility* ability = abilitySpec->Ability;
   if (ability->Implements<UInteractAbilityInstantAnimationInterface>())
   {
      FGameplayTag animationTag = IInteractAbilityInstantAnimationInterface::Execute_GetInteractInstantAnimation(ability, target);
      return { true, animationTag };
   }

   return { false };
}

bool FGameplayAbilityTargetData_Interactable::NetSerialize(FArchive& ar, UPackageMap* map, bool& bOutSuccess)
{
   ar << Interactable;

   bOutSuccess = true;

   return true;
}



