// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_EventTimeLine.h"

// ose
#include "Abilities/OSEGameplayAbility_SyncedAnimationPlayer.h"
#include "Online/OSEGameState.h"

// ue
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_EventTimeLine)

DEFINE_LOG_CATEGORY(LogAbilityTaskTimeLine);

namespace EventTimeLineUtils
{
   void CopyActions(TArray<UEventTimelineAction*>& to, const TArray<UEventTimelineAction*>& from, const TCHAR* actionKind, const UAbilityTask_EventTimeline* templateTask)
   {
      // Manually copy elements to detect null values and print a warning when encountered.
      to.Reserve(from.Num());
      for (UEventTimelineAction* action : from)
      {
         if (action == nullptr)
         {
            UE_LOG(LogAbilityTaskTimeLine, Warning, TEXT("Null %s Action in %s"), actionKind, *templateTask->GetPathName());
            continue;
         }
         to.Add(action);
      }
   }
}

//////////////////////////////////////////////////////////////////////////
// Actions
//////////////////////////////////////////////////////////////////////////

void UEventTimelineActionApplyEffect::EvaluateAction(AActor* actor, AActor* otherActor) const
{
   if (GameplayEffectClass)
   {
      if (UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
      {
         if (abilitySystemComponent->IsOwnerActorAuthoritative())
         {
            FGameplayEffectContextHandle effectContext = abilitySystemComponent->MakeEffectContext();
            if (effectContext.IsValid())
            {
               // If we have a reference to the other actor in the synced ability player animation, and we want to use it as the instigator,
               // then plumb it through to the effect we apply
               if (UseOtherActorAsInstigator && otherActor)
               {
                  effectContext.AddInstigator(otherActor, otherActor);
               }

               FGameplayEffectSpecHandle specHandle = abilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, UGameplayEffect::INVALID_LEVEL, effectContext);
               if (specHandle.IsValid())
               {
                  FGameplayEffectSpec* spec = specHandle.Data.Get();
                  check(spec); // already checked that the handle is valid...

                  for (const auto& entry : SetByCallerTagMagnitudes)
                  {
                     const FGameplayTag& magnitudeTag = entry.Key;
                     float magnitude = entry.Value;
                     spec->SetSetByCallerMagnitude(magnitudeTag, magnitude);
                  }
                  abilitySystemComponent->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
               }
            }
         }
      }
   }
}

void UEventTimelineActionRemoveEffect::EvaluateAction(AActor* actor, AActor* otherActor) const
{
   if (GameplayEffectClass)
   {
      if (UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
      {
         if (abilitySystemComponent->IsOwnerActorAuthoritative())
         {
            abilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(GameplayEffectClass, nullptr);
         }
      }
   }
}

void UEventTimelineActionTriggerVO::EvaluateAction(AActor* actor, AActor* otherActor) const
{
   VoiceOverParams.AuthoritySubmitRequest(actor);
}

//////////////////////////////////////////////////////////////////////////
// Task
//////////////////////////////////////////////////////////////////////////


UAbilityTask_EventTimeline::UAbilityTask_EventTimeline(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   bTickingTask = true;
   bSimulatedTask = true;
}

void UAbilityTask_EventTimeline::TickTask(float deltaTime)
{
   Super::TickTask(deltaTime);

   float currentTime = GetWorld()->GetTimeSeconds() - _startTime;

   bool allActionsComplete = true;
   for (UEventTimelineAction* eventTimelineAction : TimelineActions)
   {
      if (eventTimelineAction->Time < currentTime && 
          eventTimelineAction->Time >= _timeStamp)
      {
         eventTimelineAction->EvaluateAction(GetAvatarActor(), OtherActor);
      }
      allActionsComplete &= eventTimelineAction->Time < currentTime;
   }
   _timeStamp = currentTime;

   if (allActionsComplete)
   {
      EndTask();
      OnCompleted.Broadcast();
   }
}

void UAbilityTask_EventTimeline::Activate()
{
   Super::Activate();

   _startTime = GetWorld()->GetTimeSeconds();
   for (UEventTimelineAction* eventTimelineAction : ActivationActions)
   {
      eventTimelineAction->EvaluateAction(GetAvatarActor(), OtherActor);
   }
}

void UAbilityTask_EventTimeline::OnDestroy(bool inOwnerFinished)
{
   for (UEventTimelineAction* eventTimelineAction : ShutdownActions)
   {
      eventTimelineAction->EvaluateAction(GetAvatarActor(), OtherActor);
   }

   Super::OnDestroy(inOwnerFinished);
}

UAbilityTask_EventTimeline* UAbilityTask_EventTimeline::StartEventTimeLine(UGameplayAbility* owningAbility, FName taskInstanceName, const UAbilityTask_EventTimeline* templateTask, AActor* otherActor)
{
   UAbilityTask_EventTimeline* myObj = NewAbilityTask<UAbilityTask_EventTimeline>(owningAbility, taskInstanceName);
   if (templateTask)
   {
      EventTimeLineUtils::CopyActions(myObj->ActivationActions, templateTask->ActivationActions, TEXT("Activation"), templateTask);
      EventTimeLineUtils::CopyActions(myObj->TimelineActions, templateTask->TimelineActions, TEXT("Timeline"), templateTask);
      EventTimeLineUtils::CopyActions(myObj->ShutdownActions, templateTask->ShutdownActions, TEXT("Shutdown"), templateTask);
   }

   myObj->OtherActor = otherActor;

   return myObj;
}

