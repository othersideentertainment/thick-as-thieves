// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_WaitTargetEvent.h"
#include "Abilities/OSEAbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitTargetEvent)

UAbilityTask_WaitTargetEvent* UAbilityTask_WaitTargetEvent::WaitTargetEvent(UGameplayAbility* owningAbility, FName taskInstanceName, FGameplayTag eventTag, TEnumAsByte<EGameplayTargetingConfirmation::Type> confirmationType, TSubclassOf<AGameplayAbilityTargetActor> classType)
{
   UAbilityTask_WaitTargetEvent* myObj = NewAbilityTask<UAbilityTask_WaitTargetEvent>(owningAbility, taskInstanceName);      //Register for task list here, providing a given FName as a key
   myObj->_targetClass = classType;
   myObj->_targetActor = nullptr;
   myObj->_confirmationType = confirmationType;
   myObj->_eventTag = eventTag;
   return myObj;
}

void UAbilityTask_WaitTargetEvent::Activate()
{
   // Need to handle case where target actor was passed into task
   if (Ability && (_targetClass == nullptr))
   {
      if (_targetActor)
      {
         AGameplayAbilityTargetActor* SpawnedActor = _targetActor;
         _targetClass = SpawnedActor->GetClass();

         RegisterTargetDataCallbacks();


         if (!IsValid(this))
         {
            return;
         }

         if (ShouldSpawnTargetActor())
         {
            InitializeTargetActor(SpawnedActor);
            FinalizeTargetActor(SpawnedActor);

            // Note that the call to FinalizeTargetActor, this task could finish and our owning ability may be ended.
         }
         else
         {
            _targetActor = nullptr;

            // We may need a better solution here.  We don't know the target actor isn't needed till after it's already been spawned.
            SpawnedActor->Destroy();
            SpawnedActor = nullptr;
         }
      }
      // Change from WaitTargetData: Custom and Custom Multi work without a targeting actor
      else if (_confirmationType != EGameplayTargetingConfirmation::Custom && _confirmationType != EGameplayTargetingConfirmation::CustomMulti)
      {
         UE_LOG(LogAbilitySystemComponent, Warning, TEXT("UAbilityTask_WaitTargetEvent on ability %s aborted because there is no way to get target information!"), *Ability->GetPathName());
         EndTask();
      }
   }
}

bool UAbilityTask_WaitTargetEvent::BeginSpawningActor(UGameplayAbility* owningAbility, TSubclassOf<AGameplayAbilityTargetActor> inTargetClass, AGameplayAbilityTargetActor*& spawnedActor)
{
   spawnedActor = nullptr;

   if (Ability)
   {
      if (ShouldSpawnTargetActor())
      {
         if (*inTargetClass)
         {
            if (UWorld* world = GEngine->GetWorldFromContextObject(owningAbility, EGetWorldErrorMode::LogAndReturnNull))
            {
               // Change from WaitTargetData: Handle pooling
               UOSEAbilitySystemGlobals& globals = UOSEAbilitySystemGlobals::OSEGet();
               spawnedActor = globals.GetTargetActor(world, inTargetClass, _confirmationType);
            }
         }

         if (spawnedActor)
         {
            _targetActor = spawnedActor;
            InitializeTargetActor(spawnedActor);
         }
      }

      RegisterTargetDataCallbacks();
   }

   return (spawnedActor != nullptr);
}

void UAbilityTask_WaitTargetEvent::FinishSpawningActor(UGameplayAbility* owningAbility, AGameplayAbilityTargetActor* spawnedActor)
{
   if (spawnedActor && IsValid(spawnedActor))
   {
      check(_targetActor == spawnedActor);

      const FTransform spawnTransform = AbilitySystemComponent->GetOwner()->GetTransform();

      // Change from WaitTargetData: Handle pooling
      if (spawnedActor->HasActorBegunPlay())
      {
         spawnedActor->SetActorTransform(spawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
      }
      else
      {
         spawnedActor->FinishSpawning(spawnTransform);
      }

      FinalizeTargetActor(spawnedActor);
   }
}

bool UAbilityTask_WaitTargetEvent::ShouldSpawnTargetActor() const
{
   check(Ability);

   if (!_targetClass)
   {
      // Change from WaitTargetData: No class is fine
      return false;
   }

   // Spawn the actor if this is a locally controlled ability (always) or if this is a replicating targeting mode.
   // (E.g., server will spawn this target actor to replicate to all non owning clients)

   const AGameplayAbilityTargetActor* cdo = CastChecked<AGameplayAbilityTargetActor>(_targetClass->GetDefaultObject());

   const bool replicates = cdo->GetIsReplicated();
   const bool isLocallyControlled = Ability->GetCurrentActorInfo()->IsLocallyControlled();
   const bool shouldProduceTargetDataOnServer = cdo->ShouldProduceTargetDataOnServer;

   return (replicates || isLocallyControlled || shouldProduceTargetDataOnServer);
}

void UAbilityTask_WaitTargetEvent::InitializeTargetActor(AGameplayAbilityTargetActor* spawnedActor)
{
   check(spawnedActor);
   check(Ability);

   spawnedActor->PrimaryPC = Ability->GetCurrentActorInfo()->PlayerController.Get();

   // If we spawned the target actor, always register the callbacks for when the data is ready.
   spawnedActor->TargetDataReadyDelegate.AddUObject(this, &UAbilityTask_WaitTargetEvent::OnTargetDataReadyCallback);
   spawnedActor->CanceledDelegate.AddUObject(this, &UAbilityTask_WaitTargetEvent::OnTargetDataCancelledCallback);
}

void UAbilityTask_WaitTargetEvent::FinalizeTargetActor(AGameplayAbilityTargetActor* spawnedActor)
{
   check(spawnedActor);
   check(Ability);

   // User ability activation is inhibited while this is active
   AbilitySystemComponent->SpawnedTargetActors.Push(spawnedActor);

   spawnedActor->StartTargeting(Ability);

   if (spawnedActor->ShouldProduceTargetData())
   {
      // If instant confirm, then stop targeting immediately.
      // Note this is kind of bad: we should be able to just call a static func on the CDO to do this. 
      // But then we wouldn't get to set ExposeOnSpawnParameters.
      if (_confirmationType == EGameplayTargetingConfirmation::Instant)
      {
         spawnedActor->ConfirmTargeting();
      }
      else if (_confirmationType == EGameplayTargetingConfirmation::UserConfirmed)
      {
         // Bind to the Cancel/Confirm Delegates (called from local confirm or from repped confirm)
         spawnedActor->BindToConfirmCancelInputs();
      }
   }
}

void UAbilityTask_WaitTargetEvent::RegisterTargetDataCallbacks()
{
   if (!ensure(IsValid(this) == true))
   {
      return;
   }

   check(_targetClass);
   check(Ability);

   const AGameplayAbilityTargetActor* cdo = CastChecked<AGameplayAbilityTargetActor>(_targetClass->GetDefaultObject());

   const bool isLocallyControlled = Ability->GetCurrentActorInfo()->IsLocallyControlled();
   const bool shouldProduceTargetDataOnServer = cdo->ShouldProduceTargetDataOnServer;

   // If not locally controlled (server for remote client), see if TargetData was already sent
   // else register callback for when it does get here.
   if (!isLocallyControlled)
   {
      // Register with the TargetData callbacks if we are expecting client to send them
      if (!shouldProduceTargetDataOnServer)
      {
         FGameplayAbilitySpecHandle specHandle = GetAbilitySpecHandle();
         FPredictionKey activationPredictionKey = GetActivationPredictionKey();

         AbilitySystemComponent->AbilityTargetDataSetDelegate(specHandle, activationPredictionKey ).AddUObject(this, &UAbilityTask_WaitTargetEvent::OnTargetDataReplicatedCallback);
         AbilitySystemComponent->AbilityTargetDataCancelledDelegate(specHandle, activationPredictionKey ).AddUObject(this, &UAbilityTask_WaitTargetEvent::OnTargetDataReplicatedCancelledCallback);

         AbilitySystemComponent->CallReplicatedTargetDataDelegatesIfSet(specHandle, activationPredictionKey );

         SetWaitingOnRemotePlayerData();
      }
   }
   else
   {
      // Change from WaitTargetData: Listen for local set/cancel events
      FGameplayAbilitySpecHandle specHandle = GetAbilitySpecHandle();
      FPredictionKey activationPredictionKey = GetActivationPredictionKey();

      AbilitySystemComponent->AbilityTargetDataSetDelegate(specHandle, activationPredictionKey ).AddUObject(this, &UAbilityTask_WaitTargetEvent::OnLocalTargetDataSetCallback);
      AbilitySystemComponent->AbilityTargetDataCancelledDelegate(specHandle, activationPredictionKey ).AddUObject(this, &UAbilityTask_WaitTargetEvent::OnLocalTargetDataCancelledCallback);
   }
}

/** Valid TargetData was replicated to use (we are server, was sent from client) */
void UAbilityTask_WaitTargetEvent::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& data, FGameplayTag activationTag)
{
   check(AbilitySystemComponent.IsValid());

   // Change from WaitTargetData: Check event tag
   if (_eventTag.IsValid() && !activationTag.MatchesTag(_eventTag))
   {
      return;
   }

   FGameplayAbilityTargetDataHandle mutableData = data;
   AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());

   /** 
    *  Call into the TargetActor to sanitize/verify the data. If this returns false, we are rejecting
    *   the replicated target data and will treat this as a cancel.
    *   
    *   This can also be used for bandwidth optimizations. OnReplicatedTargetDataReceived could do an actual
    *   trace/check/whatever server side and use that data. So rather than having the client send that data
    *   explicitly, the client is basically just sending a 'confirm' and the server is now going to do the work
    *   in OnReplicatedTargetDataReceived.
    */
   if (_targetActor && !_targetActor->OnReplicatedTargetDataReceived(mutableData))
   {
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         Cancelled.Broadcast(activationTag, mutableData);
      }
   }
   else
   {
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         ValidData.Broadcast(activationTag, mutableData);
      }
   }

   if (_confirmationType != EGameplayTargetingConfirmation::CustomMulti)
   {
      EndTask();
   }
}

/** Client canceled this Targeting Task (we are the server) */
void UAbilityTask_WaitTargetEvent::OnTargetDataReplicatedCancelledCallback()
{
   check(AbilitySystemComponent.IsValid());
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      Cancelled.Broadcast(_eventTag, FGameplayAbilityTargetDataHandle());
   }
   EndTask();
}

void UAbilityTask_WaitTargetEvent::OnLocalTargetDataSetCallback(const FGameplayAbilityTargetDataHandle& data, FGameplayTag activationTag)
{
   check(AbilitySystemComponent.IsValid());
   if (!Ability)
   {
      return;
   }

   // Change from WaitTargetData: Check event tag
   if (_eventTag.IsValid() && !activationTag.MatchesTag(_eventTag))
   {
      return;
   }

   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), ShouldReplicateDataToServer());
   
   const FGameplayAbilityActorInfo* info = Ability->GetCurrentActorInfo();
   if (IsPredictingClient())
   {
      if (!_targetActor->ShouldProduceTargetDataOnServer)
      {
         AbilitySystemComponent->CallServerSetReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey(), data, activationTag, AbilitySystemComponent->ScopedPredictionKey);
      }
      else if (_confirmationType == EGameplayTargetingConfirmation::UserConfirmed)
      {
         // We aren't going to send the target data, but we will send a generic confirmed message.
         AbilitySystemComponent->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::GenericConfirm, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
      }
   }

   if (ShouldBroadcastAbilityTaskDelegates())
   {
      ValidData.Broadcast(activationTag, data);
   }

   if (_confirmationType != EGameplayTargetingConfirmation::CustomMulti)
   {
      EndTask();
   }
}

void UAbilityTask_WaitTargetEvent::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& Data)
{
   OnLocalTargetDataSetCallback(Data, _eventTag);
}

void UAbilityTask_WaitTargetEvent::OnLocalTargetDataCancelledCallback()
{
   check(AbilitySystemComponent.IsValid());

   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());

   if (IsPredictingClient())
   {
      if (!_targetActor->ShouldProduceTargetDataOnServer)
      {
         AbilitySystemComponent->ServerSetReplicatedTargetDataCancelled(GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey );
      }
      else
      {
         // We aren't going to send the target data, but we will send a generic confirmed message.
         AbilitySystemComponent->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::GenericCancel, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
      }
   }
   Cancelled.Broadcast(_eventTag, FGameplayAbilityTargetDataHandle());
   EndTask();
}

void UAbilityTask_WaitTargetEvent::OnTargetDataCancelledCallback(const FGameplayAbilityTargetDataHandle& data)
{
   OnLocalTargetDataCancelledCallback();
}

/** Called when the ability is asked to confirm from an outside node. What this means depends on the individual task. By default, this does nothing other than ending if bEndTask is true. */
void UAbilityTask_WaitTargetEvent::ExternalConfirm(bool endTask)
{
   check(AbilitySystemComponent.IsValid());
   if (_targetActor)
   {
      if (_targetActor->ShouldProduceTargetData())
      {
         _targetActor->ConfirmTargetingAndContinue();
      }
   }
   Super::ExternalConfirm(endTask);
}

void UAbilityTask_WaitTargetEvent::ValidateTarget()
{
   check(AbilitySystemComponent.IsValid());
   if (_targetActor)
   {
      if (_targetActor->ShouldProduceTargetData())
      {
         _targetActor->ConfirmTargetingAndContinue();
      }
   }
}

/** Called when the ability is asked to confirm from an outside node. What this means depends on the individual task. By default, this does nothing other than ending if bEndTask is true. */
void UAbilityTask_WaitTargetEvent::ExternalCancel()
{
   check(AbilitySystemComponent.IsValid());
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      Cancelled.Broadcast(_eventTag, FGameplayAbilityTargetDataHandle());
   }
   Super::ExternalCancel();
}

void UAbilityTask_WaitTargetEvent::OnDestroy(bool abilityEnded)
{
   if (_targetActor)
   {
      // Change from WaitTargetData: Handle pooling
      UOSEAbilitySystemGlobals& Globals = UOSEAbilitySystemGlobals::OSEGet();
      Globals.DoneWithTargetActor(_targetActor);
   }

   Super::OnDestroy(abilityEnded);
}

bool UAbilityTask_WaitTargetEvent::ShouldReplicateDataToServer() const
{
   if (!Ability || !_targetActor)
   {
      return false;
   }

   // Send TargetData to the server IFF we are the client and this isn't a GameplayTargetActor that can produce data on the server   
   const FGameplayAbilityActorInfo* info = Ability->GetCurrentActorInfo();
   if (!info->IsNetAuthority() && !_targetActor->ShouldProduceTargetDataOnServer)
   {
      return true;
   }

   return false;
}

