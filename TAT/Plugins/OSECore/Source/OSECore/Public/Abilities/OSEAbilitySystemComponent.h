// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSEAbilityInputBinds.h"
#include "OSEAbilityInfo.h"
#include "Abilities/OSEUpgradeState.h"

// ue4
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"

#include "OSEAbilitySystemComponent.generated.h"

struct FInputActionInstance;
class UEnhancedAbilityInputActionsAsset;
class UOSEGameplayAbility;

/** Used to register callbacks to confirm/cancel input */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityConfirmOrCancelReleased);

/** Blueprint hook called when ability fails to activate, passes along the failed ability and a tag explaining why */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceiveAbilityFailedDelegate, const UGameplayAbility*, ability, const FGameplayTagContainer&, tags);

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAbilityInfoRuntime
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadOnly)
   UOSEGameplayAbility* Ability = nullptr;

   UPROPERTY(BlueprintReadOnly)
   FOSEAbilityInfo Info;

   UPROPERTY(BlueprintReadOnly)
   EAbilityInputType AbilityBinding = EAbilityInputType::None;

   bool operator==(const FOSEAbilityInfoRuntime& other) const
   {
      return (Info == other.Info) && (AbilityBinding == other.AbilityBinding) && (Ability == other.Ability);
   }
};

/// Derived gameplay ability system component
UCLASS(ClassGroup = (AbilitySystem), meta = (BlueprintSpawnableComponent))
class OSECORE_API UOSEAbilitySystemComponent : public UAbilitySystemComponent
{
   GENERATED_BODY()

public:

   UOSEAbilitySystemComponent(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   virtual void InitializeComponent() override;

   /// Bind to an input component with some default action names
   virtual void BindToInputComponent(UInputComponent* inputComponent) override;

   /// Returns true if the ability system component is locally controlled
   bool IsLocallyControlled() const;

   /// Searches for an ability in the ability system component, optionally matching the source object
   const FGameplayAbilitySpec* FindAbilitySpec(TSubclassOf<UGameplayAbility> abilityClass, const UObject* sourceObject = nullptr) const;

   /// Can be called from the server or locally to get the ability spec handle that matches the ability and optional source object
   FGameplayAbilitySpecHandle FindAbilitySpecHandle(TSubclassOf<UGameplayAbility> abilityClass, const UObject* sourceObject = nullptr) const;

   /// Returns true if this object can initiate an ability activation. Essentially compares the ENetRole of
   /// the items's owner with the NetExecutionPolicy of the ability.
   virtual bool HasAuthorityToActivateAbility(const FGameplayAbilitySpec& spec) const;
   virtual bool HasAuthorityToActivateAbility(TSubclassOf<UGameplayAbility> abilityClass) const;

   /// Attempts to activate an ability given the input mapping index
   virtual bool TryActivateAbilityByInputID(int32 inputID, float holdDuration = 0.0f, bool allowRemoteActivation = false);

   /// Attempts to activate an ability given the enumerated input mapping
   template <typename TEnumType> bool TryActivateAbilityByInputID(TEnumType enumValue, float holdDuration = 0.0f, bool allowRemoteActivation = false)
   {
      return TryActivateAbilityByInputID((int32)enumValue, holdDuration, allowRemoteActivation);
   }

   /// Attempts to activate an ability given he input mapping index and returns if the ability successfully activated.
   /// Should be used in cases where FindAndActivateAbilityByInputID would fail because the triggered ability ends immediately.
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual bool TryActivateAbilityByInputID(EAbilityInputType inputType, bool allowRemoteActivation = false);

   /// Attempts to activate an ability given he input mapping index and returns the ability activated on success.
   /// Note this can return nullptr for an ability successfully activated if the ability immediately ends.
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual UGameplayAbility* FindAndActivateAbilityByInputID(EAbilityInputType inputType, bool allowRemoteActivation = false);

   /// Attempts to activate an ability given the class and returns the ability activated on success.
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual UGameplayAbility* FindAndActivateAbilityByClass(TSubclassOf<UGameplayAbility> abilityToActivate, bool allowRemoteActivation = false);

   /// Attempts to find then cancel an ability given the class and returns true if the ability was found.
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   virtual bool FindAndCancelAbilityByClass(TSubclassOf<UGameplayAbility> abilityToActivate);

   /// Only works on the local client, so it's only useful to local abilities
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   bool IsLocalInputPressed(EAbilityInputType inputCommand) const;
   
   /// Sends an AbilityLocalInputPressed() into the base asc, simulating an input press
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   void SendAbilityLocalInputPressed(EAbilityInputType inputCommand);
   
   /// Sends an AbilityLocalInputReleased() into the base asc, simulating an input release
   UFUNCTION(BlueprintCallable, Category = "Ability|OSE")
   void SendAbilityLocalInputReleased(EAbilityInputType inputCommand);

   /// Version of function in AbilitySystemGlobals that returns the OSE type
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   static UOSEAbilitySystemComponent* GetOSEAbilitySystemComponent(const AActor* actor);

   bool IsGenericConfirmInputReleaseBound(int32 inputID) const { return ((inputID == GenericConfirmInputID) && GenericLocalConfirmReleasedCallbacks.IsBound()); }
   bool IsGenericCancelInputReleaseBound(int32 inputID) const { return ((inputID == GenericCancelInputID) && GenericLocalCancelReleasedCallbacks.IsBound()); }

   /** Generic local callback for generic ConfirmReleasedEvent that any ability can listen to */
   FAbilityConfirmOrCancelReleased   GenericLocalConfirmReleasedCallbacks;

   /** Generic local callback for generic CancelReleasedEvent that any ability can listen to */
   FAbilityConfirmOrCancelReleased   GenericLocalCancelReleasedCallbacks;

   /** Handle confirm/cancel released for target actors */
   virtual void LocalInputConfirmReleased();
   virtual void LocalInputCancelReleased();
   virtual void AbilityLocalInputReleased(int32 inputID) override;

   UPROPERTY(BlueprintAssignable)
   FReceiveAbilityFailedDelegate ReceiveAbilityFailed;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityFailedWithSpecDelegate, const FGameplayAbilitySpec&, abilitySpec, UGameplayAbility*, ability, const FGameplayTagContainer&, failureReason);
   FAbilityFailedWithSpecDelegate ReceiveAbilityFailedWithSpec;

   virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;

   /// Disable or Enable a local user from being able to activate abilities. This should only be used for input/UI etc related inhibition. Do not use for game mechanics.
   /// Now uses a counter internally, so make sure that on/off calls are paired
   virtual void SetUserAbilityActivationInhibited(bool newInhibit) override;

   /// Disable or Enable local user input from activating abilities
   UFUNCTION(BlueprintCallable, Category = "Abilities")
   void SetUserInputInhibited(bool newInhibit);
   UFUNCTION(BlueprintCallable, Category = "Abilities")
   bool IsUserInputInhibited() const;

   // whether there is an ability bound to that event tag. Does not necessarily imply it will fire
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   bool HasAbilityForGameplayEventTag(FGameplayTag eventTag) const;

   // Finds first ability that would actually meaningfully activate for the given gameplay event
   const FGameplayAbilitySpec* FindFirstActivatableAbilityForGameplayEvent(const FGameplayEventData& eventData) const;

   bool HasUpgradeTag(FGameplayTag tag) const { return _upgradeState.HasTag(tag); }
   int32 GetUpgradeValue(FGameplayTag tag, int32 fallback = 0) const { return _upgradeState.GetValue(tag, fallback); }
   void SetUpgradeState(const FUpgradeState& state);
   const FUpgradeState& GetAllUpgradeState() const { return _upgradeState; }

   UFUNCTION(BlueprintPure, Category = "Abilities")
   const TArray<FOSEAbilityInfoRuntime>& GetAbilityInfoRuntime() const { return _abilityInfoRuntime; }

   UFUNCTION(BlueprintPure, Category = "Abilities")
   UEnhancedAbilityInputActionsAsset* GetEnhancedAbilityInputActionsAsset() const { return _GetEnhancedAbilityInputActionsAsset(); }
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityInfoRuntimeAdded, const FOSEAbilityInfoRuntime&, addedInfo);
   UPROPERTY(BlueprintAssignable, Category = "Ability|OSE")
   FOnAbilityInfoRuntimeAdded OnAbilityInfoRuntimeAdded;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityInfoRuntimeRemoved, const FOSEAbilityInfoRuntime&, removedInfo, int, index);
   UPROPERTY(BlueprintAssignable, Category = "Ability|OSE")
   FOnAbilityInfoRuntimeRemoved OnAbilityInfoRuntimeRemoved;

   UFUNCTION(BlueprintPure, Category = "Abilities")
   bool GetTimeSinceTagAdded(const FGameplayTag& tag, float& timeSinceAdded) const;

   UFUNCTION(BlueprintPure, Category = "Abilities")
   bool GetTimeSinceTagRemoved(const FGameplayTag& tag, float& timeSinceRemoved) const;

   // const overload
   FORCEINLINE const FGameplayAbilitySpec* FindAbilitySpecFromHandle(FGameplayAbilitySpecHandle handle) const
   {
      return const_cast<UAbilitySystemComponent*>(static_cast<const UAbilitySystemComponent*>(this))->FindAbilitySpecFromHandle(handle);
   }

protected:
   /** Will be called from GiveAbility or from OnRep. Initializes events (triggers and inputs) with the given ability */
   virtual void OnGiveAbility(FGameplayAbilitySpec& abilitySpec) override;

   /** Will be called from RemoveAbility or from OnRep. Unbinds inputs with the given ability */
   virtual void OnRemoveAbility(FGameplayAbilitySpec& abilitySpec) override;

   /// Called when tags change on the asc
   virtual void OnTagUpdated(const FGameplayTag& tag, bool tagExists) override;

   UFUNCTION()
   void OnAbilityFailed(const UGameplayAbility* ability , const FGameplayTagContainer& tags);

   // Not replicated for now if we can get away without non-owners/server needing this
   UPROPERTY(Transient)
   FUpgradeState _upgradeState;

   //---------------------------------------------------------------------------------------
   // Enhanced Input
   //---------------------------------------------------------------------------------------

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
   UEnhancedAbilityInputActionsAsset* EnhancedAbilityInputActionsAsset = nullptr;
   UEnhancedAbilityInputActionsAsset* _GetEnhancedAbilityInputActionsAsset() const;

   void _OnInputTriggered(const FInputActionInstance& actionInstance);
   void _OnInputCompleted(const FInputActionInstance& actionInstance);
   void _OnConfirmInputTriggered(const FInputActionInstance& actionInstance);
   void _OnConfirmInputCompleted(const FInputActionInstance& actionInstance);
   void _OnCancelInputTriggered(const FInputActionInstance& actionInstance);
   void _OnCancelInputCompleted(const FInputActionInstance& actionInstance);
   void _SendAbilityLocalInputPressed(EAbilityInputType inputCommand, bool allowMultiPress);
   void _SendAbilityLocalInputReleased(EAbilityInputType inputCommand);

private:
   UGameplayAbility* _TryGetActiveAbilityInstanceFromSpec(const FGameplayAbilitySpec& spec);
   
private:
   TArray<FOSEAbilityInfoRuntime> _abilityInfoRuntime;
   uint32 _userAbilityInhibitionCounter = 0;
   uint32 _userInputInihibitionCounter = 0;

   // the asc assumes just one input per button press but enhanced input "Triggered" states will send it repeatedly
   // depending on configuration, so let's just prevent multiple calls into the asc base class.
   TMap<EAbilityInputType, bool> _isAbilityInputTriggered;
   bool _isConfirmTriggered = false;
   bool _isCancelTriggered = false;

   // map of tag : world time the tag was applied/removed, so we can query how long we've had a tag applied to us 
   // for.  useful for AI considerations changing state shortly after tags are gained but not immediately.
   TMap<FGameplayTag, float> _tagAddedWorldTime;
   TMap<FGameplayTag, float> _tagRemovedWorldTime;
};
