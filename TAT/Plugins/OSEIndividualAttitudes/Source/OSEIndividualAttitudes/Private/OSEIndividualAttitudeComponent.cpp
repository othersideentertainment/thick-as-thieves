// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "OSEIndividualAttitudeComponent.h"

// ose
#include "IndividualAttitudeTypes.h"
#include "OSEIndividualAttitudeReceiverInterface.h"

// ue
#include "GameplayDebuggerCategory.h"
#include "VisualLogger/VisualLoggerTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEIndividualAttitudeComponent)


UOSEIndividualAttitudeComponent::UOSEIndividualAttitudeComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.TickInterval = 1.f;
   
}

EOSEIndividualAttitude UOSEIndividualAttitudeComponent::GetAttitudeTowardsActor(const AActor* target)
{
   const FIndividualAttitude* foundAttitude = FindAttitude(target);
   if(foundAttitude == nullptr)
      return EOSEIndividualAttitude::Unknown;
   return foundAttitude->GetAttitude();
}

void UOSEIndividualAttitudeComponent::SetAttitudeTowardsActor(AActor* target,
                                                              const EOSEIndividualAttitude individualAttitude,
                                                              const float timeUntilExpiration,
                                                              const bool triggerEvent)
{
   check(GetOwner());
   check(GetOwner()->HasAuthority());
   FIndividualAttitude& attitude = FindOrAddAttitude(target);
   attitude.SetAttitude(individualAttitude);

   InformTargetOfAttitudeChange(target, individualAttitude);
   
   if(timeUntilExpiration > 0)
   {
      const UWorld* world = GetWorld();
      check(world);
      attitude.SetWorldTimeToRemove(world->GetTimeSeconds() + timeUntilExpiration);
   }
   if(triggerEvent)
   {
      BroadcastAttitudeChangeSingleTarget(target);
   }
}

void UOSEIndividualAttitudeComponent::ClearAttitudeTowardsActor(AActor* target,
                                                                bool triggerEvent)
{
   check(GetOwner());
   check(GetOwner()->HasAuthority());
   const bool clearSuccess = ClearAttitude(target);
   if (clearSuccess && triggerEvent)
   {
      BroadcastAttitudeChangeSingleTarget(target);
   }
   InformTargetOfAttitudeChange(target, EOSEIndividualAttitude::Unknown);
}

void UOSEIndividualAttitudeComponent::TickComponent(
   const float deltaTime,
   const ELevelTick tickType,
   FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
   
   if(GetOwnerRole() != ROLE_Authority)
      return;
   
   const UWorld* world = GetWorld();
   if(world == nullptr)
      return;
   
   const float worldTime = world->GetTimeSeconds();
   for(auto it = _individualAttitudes.CreateIterator(); it; ++it)
   {
      FIndividualAttitude& attitude = *it;
      if(attitude.GetTarget() == nullptr)
      {
         it.RemoveCurrent();
      }
      else if(attitude.IsExpired(worldTime))
      {
         InformTargetOfAttitudeChange(attitude.GetTarget(), EOSEIndividualAttitude::Unknown);
         it.RemoveCurrent();
      }
   }
}

void UOSEIndividualAttitudeComponent::InformTargetOfAttitudeChange(AActor* target, EOSEIndividualAttitude attitude)
{
   if(IOSEIndividualAttitudeReceiverInterface* targetReceiver = Cast<IOSEIndividualAttitudeReceiverInterface>(target))
   {
      targetReceiver->AttitudeChangedFromActor(GetOwner(), attitude);
   }
}

void UOSEIndividualAttitudeComponent::ShareAllIndividualAttitudes(
   UOSEIndividualAttitudeComponent* attitudeComponent,
   const EOSEAttitudeCopyRules& attitudeCopyRules,
   const EOSEExpirationTimeCopyRules& expirationTimeCopyRules,
   const bool triggerEvent)
{
   check(GetOwner());
   check(GetOwner()->HasAuthority());
   bool changed = false;
   for(auto it = _individualAttitudes.CreateConstIterator(); it; ++it)
   {
      const FIndividualAttitude& attitude = *it;
      AActor* target = attitude.GetTarget();
      if(target == nullptr)
         continue;
      FIndividualAttitude& otherAttitude = attitudeComponent->FindOrAddAttitude(target);
      // Should this take the original time offset? Right now we're expiring the world time from the original attitude.
      if(otherAttitude.Copy(attitude, attitudeCopyRules, expirationTimeCopyRules))
      {
         changed = true;
      }
   }
   if(changed && triggerEvent)
   {
      attitudeComponent->BroadcastAttitudeChangeFromSharing(this);
   }
}

bool UOSEIndividualAttitudeComponent::ShareSpecificIndividualAttitudeWithTarget(
   AActor* target,
   UOSEIndividualAttitudeComponent* attitudeComponent,
   const EOSEAttitudeCopyRules& attitudeCopyRules,
   const EOSEExpirationTimeCopyRules& expirationTimeCopyRules,
   const bool triggerEvent)
{
   check(GetOwner());
   check(GetOwner()->HasAuthority());
   const FIndividualAttitude* attitude = FindAttitude(target);
   if(attitude == nullptr)
      return false;
   FIndividualAttitude& otherAttitude = attitudeComponent->FindOrAddAttitude(target);
   bool changed = false;
   if(otherAttitude.Copy(*attitude))
   {
      changed = true;
   }
   if(changed && triggerEvent) 
   {
      attitudeComponent->BroadcastAttitudeChangeFromSharing(this);
   }
   return changed;
}

#if ENABLE_VISUAL_LOG
void UOSEIndividualAttitudeComponent::DescribeSelfToVisLog(FVisualLogEntry* snapshot) const
{
   FVisualLogStatusCategory category;
   category.Category = TEXT("Individual Attitudes");
   category.Add(TEXT("Total"),FString::Printf(TEXT("Total Attitudes: %i"), _individualAttitudes.Num()));
   
   const float worldTime = GetWorld()->GetTimeSeconds();   
   for (const FIndividualAttitude& individualAttitude : _individualAttitudes)
   {
      const float secondsLeft = individualAttitude.GetWorldTimeToRemove() < 0
                                   ? INDEX_NONE
                                   : individualAttitude.GetWorldTimeToRemove() - worldTime;
      FString attitudeName = StaticEnum<EOSEIndividualAttitude>()->GetNameStringByValue(
         static_cast<int64>(individualAttitude.GetAttitude())
      );
      FVisualLogStatusCategory attitudeCategory;
      attitudeCategory.Category = GetNameSafe(individualAttitude.GetTarget());
      attitudeCategory.Add(TEXT("Attitude"), attitudeName);
      attitudeCategory.Add(TEXT("TimeLeft"), FString::Printf(TEXT("%f"),secondsLeft));
      category.AddChild(attitudeCategory);
   }
   snapshot->Status.Add(category);
}
#endif

#if WITH_GAMEPLAY_DEBUGGER
void UOSEIndividualAttitudeComponent::DescribeSelfToGameplayDebugger(FGameplayDebuggerCategory* debuggerCategory)
{
   debuggerCategory->AddTextLine(FString::Printf(TEXT("Total Attitudes: %i"), _individualAttitudes.Num()));
   const float worldTime = GetWorld()->GetTimeSeconds();   
   for (const FIndividualAttitude& individualAttitude : _individualAttitudes)
   {
      const float secondsLeft = individualAttitude.GetWorldTimeToRemove() < 0
                                   ? INDEX_NONE
                                   : individualAttitude.GetWorldTimeToRemove() - worldTime;
      FString attitudeName = StaticEnum<EOSEIndividualAttitude>()->GetNameStringByValue(
         static_cast<int64>(individualAttitude.GetAttitude())
      );
      debuggerCategory->AddTextLine(
         FString::Printf(TEXT("%s : %s : %f"),
            *GetNameSafe(individualAttitude.GetTarget()),
            *attitudeName,
            secondsLeft)
      );
   }
}
#endif

FIndividualAttitude* UOSEIndividualAttitudeComponent::FindAttitude(const AActor* target)
{
   FIndividualAttitude* returnValue = _individualAttitudes.FindByPredicate(
      [target](const FIndividualAttitude& attitude)
      {
         return attitude.MatchesTarget(target);
      });
   return returnValue;
}

FIndividualAttitude& UOSEIndividualAttitudeComponent::FindOrAddAttitude(AActor* target)
{
   FIndividualAttitude* returnValue = FindAttitude(target);
   if(returnValue == nullptr)
   {
      returnValue= &_individualAttitudes.AddDefaulted_GetRef();
      returnValue->SetTarget(target);
   }
   return *returnValue;
}

bool UOSEIndividualAttitudeComponent::ClearAttitude(const AActor* target)
{
   const int32 removalIndex = _individualAttitudes.IndexOfByPredicate(
      [target](const FIndividualAttitude& attitude)
      {
         return attitude.MatchesTarget(target);
      });
   if (removalIndex != INDEX_NONE)
   {
      _individualAttitudes.RemoveAt(removalIndex);
      return true;
   }
   return false;
}

void UOSEIndividualAttitudeComponent::BroadcastAttitudeChangeSingleTarget(const AActor* target)
{
   OnIndividualAttitudeChangedForActor.Broadcast(target);
   // empty until we know how we want to expose the hooks.
   // Danny Goodayle: I don't like this pattern of exposing the hooks on components. I also don't like it being routed onto an
   // actor class either because it just muddies everything.
   // When exposing things via components you need to have lots of mini "audio / vfx" components, making it hard to track
   // down exactly what is triggering a specific sound to play. On the other hand we can't also have monolith BP's (AK_Voice)

   // Instead I'd like to have the hooks triggered via something like Lyra's gameplay message router, which fires off an
   // event on a specific actor with an optional struct. This would let us have things exposed in a consistent manner, but also
   // leave the implementation of reactions to those hooks to be left upto the sound designer / vfx artist.

   // Also, by using the gameplay tags as the hooking method, we can very easily find references and backtrack
   // the implementation in the future if we ever need to.
}

void UOSEIndividualAttitudeComponent::BroadcastAttitudeChangeFromSharing(UOSEIndividualAttitudeComponent* source)
{
   // empty until we know how we want to expose the hooks. (see above)
}
