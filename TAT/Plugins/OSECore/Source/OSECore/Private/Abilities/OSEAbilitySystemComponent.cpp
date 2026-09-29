// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEAbilitySystemComponent.h"

// ose
#include "Abilities/OSEAbilityInputBinds.h"
#include "Abilities/OSEAbilityTypes.h"
#include "Abilities/OSEGameplayAbility.h"
#include "Player/OSEPlayerController.h"
#include "Input/OSEInputSettings.h"

// wwise
#include "AkSwitchValue.h"

// ue4
#include "AbilitySystemGlobals.h"
#include "EnhancedInputComponent.h"
#include "TimerManager.h"
#include "Animation/AnimMontage.h"
#include "Components/InputComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilitySystemComponent)


DEFINE_LOG_CATEGORY_STATIC(LogOSEAbilitySystemComponent, Log, All);

UOSEAbilitySystemComponent::UOSEAbilitySystemComponent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   for(int idx = 0; idx < int(EAbilityInputType::None); ++idx)
   {
      EAbilityInputType inputType = EAbilityInputType(idx);
      _isAbilityInputTriggered.Add(inputType, false);
   }
}

void UOSEAbilitySystemComponent::InitializeComponent()
{
   Super::InitializeComponent();
   AbilityFailedCallbacks.AddUObject(this, &UOSEAbilitySystemComponent::OnAbilityFailed);
}

void UOSEAbilitySystemComponent::BindToInputComponent(UInputComponent* inputComponent)
{
   FGameplayAbilityInputBinds bindInfo = FOSEAbilityInputBinds<EAbilityInputType>::Get();

   //DO THESE FIRST
   BindAbilityActivationToInputComponent(inputComponent, bindInfo);

   // Bind to enhanced input actions
   UEnhancedAbilityInputActionsAsset* enhancedAbilityInputActionsAsset = _GetEnhancedAbilityInputActionsAsset();
   if (enhancedAbilityInputActionsAsset)
   {
      if (UEnhancedInputComponent* enhancedInputComponent = Cast<UEnhancedInputComponent>(inputComponent))
      {
         if (enhancedAbilityInputActionsAsset->ConfirmInputAction)
         {
            enhancedInputComponent->BindAction(enhancedAbilityInputActionsAsset->ConfirmInputAction, ETriggerEvent::Triggered, this, &UOSEAbilitySystemComponent::_OnConfirmInputTriggered);
            enhancedInputComponent->BindAction(enhancedAbilityInputActionsAsset->ConfirmInputAction, ETriggerEvent::Completed, this, &UOSEAbilitySystemComponent::_OnConfirmInputCompleted);
         }

         if (enhancedAbilityInputActionsAsset->CancelInputAction)
         {
            enhancedInputComponent->BindAction(enhancedAbilityInputActionsAsset->CancelInputAction, ETriggerEvent::Triggered, this, &UOSEAbilitySystemComponent::_OnCancelInputTriggered);
            enhancedInputComponent->BindAction(enhancedAbilityInputActionsAsset->CancelInputAction, ETriggerEvent::Completed, this, &UOSEAbilitySystemComponent::_OnCancelInputCompleted);
         }

         for (auto it = enhancedAbilityInputActionsAsset->AbilityInputToEnhancedInputActionMapping.CreateConstIterator(); it; ++it)
         {
            UInputAction* inputAction = it.Value();
            if (inputAction)
            {
               enhancedInputComponent->BindAction(inputAction, ETriggerEvent::Triggered, this, &UOSEAbilitySystemComponent::_OnInputTriggered);
               enhancedInputComponent->BindAction(inputAction, ETriggerEvent::Completed, this, &UOSEAbilitySystemComponent::_OnInputCompleted);
            }
         }
      }
   }
}

// Returns true if the ability system component is locally controlled
bool UOSEAbilitySystemComponent::IsLocallyControlled() const
{
   if (!AbilityActorInfo.IsValid())
      return false;

   return AbilityActorInfo.Get()->IsLocallyControlled();
}

// Searches for an ability in the ability system component, optionally matching the source object
const FGameplayAbilitySpec* UOSEAbilitySystemComponent::FindAbilitySpec(TSubclassOf<UGameplayAbility> abilityClass, const UObject* sourceObject /* = nullptr */) const
{
   const TArray<FGameplayAbilitySpec>& AllAbilities = GetActivatableAbilities();
   for (const FGameplayAbilitySpec& Spec : AllAbilities)
   {
      if (Spec.Ability->GetClass() == abilityClass)
      {
         if ((sourceObject == nullptr) || (sourceObject == Spec.SourceObject))
            return &Spec;
      }
   }
   return nullptr;
}

// Can be called from the server or locally to get the ability spec handle that matches the ability and optional source object
FGameplayAbilitySpecHandle UOSEAbilitySystemComponent::FindAbilitySpecHandle(TSubclassOf<UGameplayAbility> abilityClass, const UObject* sourceObject /* = nullptr */) const
{
   const FGameplayAbilitySpec* spec = FindAbilitySpec(abilityClass, sourceObject);
   if (spec != nullptr)
      return spec->Handle;
   else
      return FGameplayAbilitySpecHandle();
}

// Returns true if this object can initiate an ability activation. Essentially compares the ENetRole of
// the items's owner with the NetExecutionPolicy of the ability.
bool UOSEAbilitySystemComponent::HasAuthorityToActivateAbility(const FGameplayAbilitySpec& spec) const
{
   // Needed to wrap this to expose the protected method
   return Super::HasNetworkAuthorityToActivateTriggeredAbility(spec);
}

bool UOSEAbilitySystemComponent::HasAuthorityToActivateAbility(TSubclassOf<UGameplayAbility> abilityClass) const
{
   const FGameplayAbilitySpec* spec = FindAbilitySpec(abilityClass);
   if (spec == nullptr)
      return false;

   return HasAuthorityToActivateAbility(*spec);
}

// Attempts to activate an ability given the input mapping index
bool UOSEAbilitySystemComponent::TryActivateAbilityByInputID(int32 inputID, float holdDuration /* = 0.0f*/, bool allowRemoteActivation /* = false */)
{
   if (!FindAndActivateAbilityByInputID(static_cast<EAbilityInputType>(inputID), allowRemoteActivation))
   {
      return false;
   }

   /// Positive values of HoldDuration will delay releasing the input, otherwise we'll release it
   /// on the next frame.
   FTimerDelegate delegate = FTimerDelegate::CreateUObject(this, &UOSEAbilitySystemComponent::AbilityLocalInputReleased, inputID);
   if (holdDuration > 0.0f)
   {
      FTimerHandle timerHandle;
      GetWorld()->GetTimerManager().SetTimer(timerHandle, delegate, holdDuration, false);
   }
   else
   {
      GetWorld()->GetTimerManager().SetTimerForNextTick(delegate);
   }

   return true;
}

bool UOSEAbilitySystemComponent::TryActivateAbilityByInputID(EAbilityInputType inputType, bool allowRemoteActivation)
{
   const FGameplayAbilitySpec* spec = FindAbilitySpecFromInputID(static_cast<int32>(inputType));
   if (spec == nullptr)
      return false;
   
   if (!allowRemoteActivation && !HasAuthorityToActivateAbility(*spec))
      return false;

   // For the cases where triggered abilities can end immediately, inject a lambda callback
   // so we know if our ability was triggered
   bool activated = false;
   const FGameplayAbilitySpecHandle specHandle = spec->Handle;
   const FDelegateHandle lambdaHandle = AbilityActivatedCallbacks.AddLambda([specHandle, &activated](UGameplayAbility* ability)
   {
      // Make sure the ability we care about was the one activated
      if (specHandle == ability->GetCurrentAbilitySpecHandle())
      {
         activated = true;
      }
   });
   
   bool allowMultiPress = true;
   _SendAbilityLocalInputPressed(inputType, allowMultiPress);

   AbilityActivatedCallbacks.Remove(lambdaHandle);
   return activated;
}

/// Attempts to ativate an ability given he input mapping index and returns the ability activated on success.
UGameplayAbility* UOSEAbilitySystemComponent::FindAndActivateAbilityByInputID(EAbilityInputType inputType, bool allowRemoteActivation /*= false*/)
{
   const FGameplayAbilitySpec* spec = FindAbilitySpecFromInputID(static_cast<int32>(inputType));
   if (spec == nullptr)
      return nullptr;

   if (!allowRemoteActivation && !HasAuthorityToActivateAbility(*spec))
      return nullptr;

   bool allowMultiPress = true;
   _SendAbilityLocalInputPressed(inputType, allowMultiPress);

   return _TryGetActiveAbilityInstanceFromSpec(*spec);
}

UGameplayAbility* UOSEAbilitySystemComponent::FindAndActivateAbilityByClass(TSubclassOf<UGameplayAbility> abilityToActivate, bool allowRemoteActivation /*= false*/)
{
   if (const UGameplayAbility* abilityCDO = abilityToActivate.GetDefaultObject())
   {
      for (const FGameplayAbilitySpec& spec : ActivatableAbilities.Items)
      {
         if (spec.Ability == abilityCDO)
         {
            UGameplayAbility* ability = nullptr;
            if (TryActivateAbility(spec.Handle, allowRemoteActivation))
            {
               ability = _TryGetActiveAbilityInstanceFromSpec(spec);
            }
            return ability;
         }
      }
   }
   return nullptr;
}

bool UOSEAbilitySystemComponent::FindAndCancelAbilityByClass(TSubclassOf<UGameplayAbility> abilityToActivate)
{
   if (UGameplayAbility* abilityCDO = abilityToActivate.GetDefaultObject())
   {
      CancelAbility(abilityCDO);
      return true;
   }
   return false;
}

bool UOSEAbilitySystemComponent::IsLocalInputPressed(EAbilityInputType inputCommand) const
{
   if (IsLocallyControlled())
   {
      return _isAbilityInputTriggered[inputCommand];
   }
   return false;
}

void UOSEAbilitySystemComponent::SendAbilityLocalInputPressed(EAbilityInputType inputCommand)
{
   bool allowMultiPress = true;
   _SendAbilityLocalInputPressed(inputCommand, allowMultiPress);
}

void UOSEAbilitySystemComponent::SendAbilityLocalInputReleased(EAbilityInputType inputCommand)
{
   _SendAbilityLocalInputReleased(inputCommand);
}

UOSEAbilitySystemComponent* UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(const AActor* actor)
{
   return Cast<UOSEAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor));
}

//delegates for LocalInput of Confirm and Cancel Releases
void UOSEAbilitySystemComponent::LocalInputConfirmReleased()
{
   FAbilityConfirmOrCancelReleased temp = GenericLocalConfirmReleasedCallbacks;
   GenericLocalConfirmReleasedCallbacks.Clear();
   temp.Broadcast();
}

void UOSEAbilitySystemComponent::LocalInputCancelReleased()
{
   FAbilityConfirmOrCancelReleased temp = GenericLocalCancelReleasedCallbacks;
   GenericLocalCancelReleasedCallbacks.Clear();
   temp.Broadcast();
}

//Add our confirm && cancel release to general ability released - this is an override
void UOSEAbilitySystemComponent::AbilityLocalInputReleased(int32 inputID)
{
   // Consume the input if this InputID is overloaded with GenericConfirm/CancelReleased and the GenericConfim/Cancel callback is bound
   if (IsGenericConfirmInputReleaseBound(inputID))
   {
      LocalInputConfirmReleased();
      return;
   }

   if (IsGenericCancelInputReleaseBound(inputID))
   {
      LocalInputCancelReleased();
      return;
   }

   // ---------------------------------------------------------

   Super::AbilityLocalInputReleased(inputID);
}

void UOSEAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
   Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

   QUICK_SCOPE_CYCLE_COUNTER(STAT_UOSEAbilitySystemComponent_NotifyAbilityFailed);

   if (ReceiveAbilityFailedWithSpec.IsBound())
   {
      const FGameplayAbilitySpec* abilitySpec = FindAbilitySpecFromHandle(Handle);
      check(abilitySpec != nullptr);
      ReceiveAbilityFailedWithSpec.Broadcast(*abilitySpec, Ability, FailureReason);
   }
   
   // Fire blueprint hook to trigger UX feedback
   ReceiveAbilityFailed.Broadcast(Ability, FailureReason);
}

void UOSEAbilitySystemComponent::SetUserAbilityActivationInhibited(bool newInhibit)
{
   if (AbilityActorInfo->IsLocallyControlled())
   {
      const int32 delta = newInhibit ? 1 : -1;
      _userAbilityInhibitionCounter = FMath::Max<int32>(_userAbilityInhibitionCounter + delta, 0);

      // This field is inexplicably public, so don't need to call super here
      UserAbilityActivationInhibited = _userAbilityInhibitionCounter > 0;
   }
}

void UOSEAbilitySystemComponent::SetUserInputInhibited(bool newInhibit)
{
   if (AbilityActorInfo->IsLocallyControlled())
   {
      const bool wasInhibited = IsUserInputInhibited();
      const int32 delta = newInhibit ? 1 : -1;
      _userInputInihibitionCounter = FMath::Max<int32>(_userInputInihibitionCounter + delta, 0);
   }
}

bool UOSEAbilitySystemComponent::IsUserInputInhibited() const
{
   if (_userInputInihibitionCounter > 0)
   {
      return true;
   }

   // This caution is probably excessive
   if (!AbilityActorInfo.IsValid())
   {
      return false;
   }

   // TODO: if the hard dependency on AOSEPlayerController becomes a problem, introduce an
   //       interface with just the IsAbilityInputIgnored method
   if(auto sourceController = Cast<AOSEPlayerController>(AbilityActorInfo->PlayerController.Get()))
   {
      return sourceController->IsAbilityInputIgnored();
   }

   return false;
}

bool UOSEAbilitySystemComponent::HasAbilityForGameplayEventTag(FGameplayTag eventTag) const
{
   return GameplayEventTriggeredAbilities.Contains(eventTag);
}

const FGameplayAbilitySpec* UOSEAbilitySystemComponent::FindFirstActivatableAbilityForGameplayEvent(const FGameplayEventData& eventData) const
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_OSEAbilitySystemComponent_FindFirstActivatableAbilityForGameplayEvent);
   FGameplayAbilityActorInfo* actorInfo = AbilityActorInfo.Get();

   // explicitly not searching down the chain of tags for now
   if (const TArray<FGameplayAbilitySpecHandle>* triggeredAbilityHandles =  GameplayEventTriggeredAbilities.Find(eventData.EventTag))
   {

      for (const FGameplayAbilitySpecHandle& abilityHandle : *triggeredAbilityHandles)
      {
         // if this is a linear search, at what point is it better just to iterate over abilities
         const FGameplayAbilitySpec* spec = FindAbilitySpecFromHandle(abilityHandle);
         if(!ensure(spec)) continue;

         const UGameplayAbility* ability = spec->Ability;
         if (!ability->ShouldAbilityRespondToEvent(actorInfo, &eventData))
         {
            continue;
         }

         if (!ability->CanActivateAbility(abilityHandle, actorInfo, &eventData.InstigatorTags, &eventData.TargetTags))
         {
            continue;
         }

         return spec;
      }
   }

    return nullptr;
}

void UOSEAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& abilitySpec)
{
   Super::OnGiveAbility(abilitySpec);

   if (UOSEGameplayAbility* oseAbility = Cast<UOSEGameplayAbility>(abilitySpec.Ability))
   {
      const FOSEAbilityInfo& info = oseAbility->GetAbilityInfo();
      if (!info.HideInUI)
      {
         // create
         FOSEAbilityInfoRuntime runtimeInfo;
         runtimeInfo.Ability = oseAbility;
         runtimeInfo.Info = info;
         runtimeInfo.AbilityBinding = static_cast<EAbilityInputType>(abilitySpec.InputID);

         // add
         _abilityInfoRuntime.Add(runtimeInfo);

         // broadcast
         OnAbilityInfoRuntimeAdded.Broadcast(runtimeInfo);
      }
   }
}

void UOSEAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& abilitySpec)
{
   Super::OnRemoveAbility(abilitySpec);

   if (UOSEGameplayAbility* oseAbility = Cast<UOSEGameplayAbility>(abilitySpec.Ability))
   {
      const FOSEAbilityInfo& info = oseAbility->GetAbilityInfo();
      if (!info.HideInUI)
      {
         // create
         FOSEAbilityInfoRuntime runtimeInfo;
         runtimeInfo.Ability = oseAbility;
         runtimeInfo.Info = info;
         runtimeInfo.AbilityBinding = static_cast<EAbilityInputType>(abilitySpec.InputID);

         // remove
         int index = INDEX_NONE;
         if (_abilityInfoRuntime.Find(runtimeInfo, index))
         {
            _abilityInfoRuntime.RemoveAt(index);
         }

         // broadcast
         if (index != INDEX_NONE)
         {
            OnAbilityInfoRuntimeRemoved.Broadcast(runtimeInfo, index);
         }
      }
   }
}

void UOSEAbilitySystemComponent::OnTagUpdated(const FGameplayTag& tag, bool tagExists)
{
   if (tagExists)
   {
      if (!_tagAddedWorldTime.Contains(tag))
      {
         _tagAddedWorldTime.Emplace(tag, GetWorld()->GetTimeSeconds());
         UE_LOG(LogOSEAbilitySystemComponent, Verbose, TEXT("[%s %s] Gained tag %s at world time %.02f"), 
            GetOwner()->HasAuthority() ? TEXT("Authority") : TEXT("Client"),
            *AActor::GetDebugName(GetOwner()),
            *tag.ToString(), _tagAddedWorldTime[tag]);
      }
      _tagRemovedWorldTime.Remove(tag);
   }
   else
   {
      if (!_tagRemovedWorldTime.Contains(tag))
      {
         _tagRemovedWorldTime.Emplace(tag, GetWorld()->GetTimeSeconds());
         UE_LOG(LogOSEAbilitySystemComponent, Verbose, TEXT("[%s %s] Lost tag %s at world time %.02f"), 
            GetOwner()->HasAuthority() ? TEXT("Authority") : TEXT("Client"), 
            *AActor::GetDebugName(GetOwner()),
            *tag.ToString(), _tagAddedWorldTime[tag]);
      }
      _tagAddedWorldTime.Remove(tag);
   }
}

void UOSEAbilitySystemComponent::SetUpgradeState(const FUpgradeState& state)
{
   // If we had existing upgrade state tags, remove them.
   for (const auto& pair : _upgradeState.Values)
   {
      RemoveLooseGameplayTag(pair.Key);
   }

   _upgradeState = state;

   // Add the new tags to the ability system component.
   // Note that this is only moderately useful since you can only check for the existence of an upgrade tag and not its level,
   // but there are use-cases for checking if any level of an upgrade is available (especially for upgrades with only one level).
   for (const auto& pair : _upgradeState.Values)
   {
      AddLooseGameplayTag(pair.Key);
   }
}

bool UOSEAbilitySystemComponent::GetTimeSinceTagAdded(const FGameplayTag& tag, float& timeSinceAdded) const
{
   if(_tagAddedWorldTime.Contains(tag))
   {
      timeSinceAdded = GetWorld()->GetTimeSeconds() - _tagAddedWorldTime[tag];
      return true;
   }

   // failed
   timeSinceAdded = float(INDEX_NONE);
   return false;
}

bool UOSEAbilitySystemComponent::GetTimeSinceTagRemoved(const FGameplayTag& tag, float& timeSinceRemoved) const
{
   if (_tagRemovedWorldTime.Contains(tag))
   {
      timeSinceRemoved = GetWorld()->GetTimeSeconds() - _tagRemovedWorldTime[tag];
      return true;
   }

   // failed
   timeSinceRemoved = float(INDEX_NONE);
   return false;
}

void UOSEAbilitySystemComponent::OnAbilityFailed(const UGameplayAbility* ability, const FGameplayTagContainer& tags)
{
   // Container should, at this point, contain tags relevant to failure reason,
   // (if such tags have been configured) e.g. Ability.Failure.Cost
   const UOSEGameplayAbility* abilityAsOSE = Cast<const UOSEGameplayAbility>(ability);
   if (abilityAsOSE)
   {
      FGameplayTag failureTag = abilityAsOSE->AbilityFailedEventTag;

      if (failureTag.IsValid())
      {
         // Trigger a GameplayEvent with the indicated tag, passing on the
         // tags descriptive of the reason for failure.
         FOSEGameplayEffectContext* context = new FOSEGameplayEffectContext();
         context->SetEventTags(tags);

         FGameplayEventData eventData;
         eventData.ContextHandle = FGameplayEffectContextHandle(context);

         HandleGameplayEvent(failureTag, &eventData);
      }
   }
}

UEnhancedAbilityInputActionsAsset* UOSEAbilitySystemComponent::_GetEnhancedAbilityInputActionsAsset() const
{
   const UOSEInputDeveloperSettings& inputSettings = UOSEInputDeveloperSettings::Get();
   return EnhancedAbilityInputActionsAsset ? EnhancedAbilityInputActionsAsset : inputSettings.DefaultEnhancedAbilityInputActionsAsset.Get();
}

void UOSEAbilitySystemComponent::_OnInputTriggered(const FInputActionInstance& actionInstance)
{
   check(_GetEnhancedAbilityInputActionsAsset());

   if (!IsUserInputInhibited())
   {
      EAbilityInputType abilityType;
      if (_GetEnhancedAbilityInputActionsAsset()->FindAbilityInputFromInputAction(actionInstance.GetSourceAction(), abilityType))
      {
         bool allowMultiPress = false;
         _SendAbilityLocalInputPressed(abilityType, allowMultiPress);
      }
   }
}

void UOSEAbilitySystemComponent::_OnInputCompleted(const FInputActionInstance& actionInstance)
{
   check(_GetEnhancedAbilityInputActionsAsset());
   EAbilityInputType abilityType;
   if (_GetEnhancedAbilityInputActionsAsset()->FindAbilityInputFromInputAction(actionInstance.GetSourceAction(), abilityType))
   {
      _SendAbilityLocalInputReleased(abilityType);
   }
}

void UOSEAbilitySystemComponent::_OnConfirmInputTriggered(const FInputActionInstance& actionInstance)
{
   if (!_isConfirmTriggered && !IsUserInputInhibited())
   {
      _isConfirmTriggered = true;
      LocalInputConfirm();
   }
}

void UOSEAbilitySystemComponent::_OnConfirmInputCompleted(const FInputActionInstance& actionInstance)
{
   _isConfirmTriggered = false;
   LocalInputConfirmReleased();
}

void UOSEAbilitySystemComponent::_OnCancelInputTriggered(const FInputActionInstance& actionInstance)
{
   if (!_isCancelTriggered && !IsUserInputInhibited())
   {
      _isCancelTriggered = true;
      LocalInputCancel();
   }
}

void UOSEAbilitySystemComponent::_OnCancelInputCompleted(const FInputActionInstance& actionInstance)
{
   _isCancelTriggered = false;
   LocalInputCancelReleased();
}

void UOSEAbilitySystemComponent::_SendAbilityLocalInputPressed(EAbilityInputType inputCommand, bool allowMultiPress)
{
   // allowMultiPress allows us to send a second input pressed for the same input type, which is a useful tool
   // when faking input in abilities or in AI usage etc.  when we call this with real user-defined input we shouldn't allow a second press
   bool& isTriggered = _isAbilityInputTriggered.FindChecked(inputCommand);
   if (!isTriggered || allowMultiPress)
   {
      isTriggered = true;
      AbilityLocalInputPressed(static_cast<int32>(inputCommand));
   }
}

void UOSEAbilitySystemComponent::_SendAbilityLocalInputReleased(EAbilityInputType inputCommand)
{
   bool& isTriggered = _isAbilityInputTriggered.FindChecked(inputCommand);
   isTriggered = false;
   AbilityLocalInputReleased(static_cast<int32>(inputCommand));
}

UGameplayAbility* UOSEAbilitySystemComponent::_TryGetActiveAbilityInstanceFromSpec(const FGameplayAbilitySpec& spec)
{
   if (spec.IsActive())
   {
      return spec.GetPrimaryInstance();
   }
   return nullptr;
}

