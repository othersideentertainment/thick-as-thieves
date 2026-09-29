// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "InteractableInterface.h"

// ue
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

#include "OSEInteractableToggleSwitchComponent.generated.h"

class ACharacter;
class AOSESyncedToggle;

USTRUCT()
struct OSEINTERACTION_API FOSEInteractableToggleData
{
   GENERATED_BODY()

   UPROPERTY(Transient)
   float ServerTimeSeconds = 0.f;

   UPROPERTY(Transient)
   TWeakObjectPtr<ACharacter> Character;

   FORCEINLINE bool operator>=(const FOSEInteractableToggleData& other) const { return ServerTimeSeconds >= other.ServerTimeSeconds; }

   /// Returns true if FOSEInteractableToggleData occurred within the provided replicated-duplicate threshold. 
   /// Should only call on clients!
   FORCEINLINE bool IsReplicatedDuplicate(const FOSEInteractableToggleData& replicatedInteractToggleData, float clientLocalPredictReplicatedDuplicateThreshold) const 
   { 
      return FMath::IsNearlyEqual(ServerTimeSeconds, replicatedInteractToggleData.ServerTimeSeconds, clientLocalPredictReplicatedDuplicateThreshold);
   }

   FString ToString() const;
};

UENUM()
enum class EOSEInteractableToggleSwitchUsageBehavior : uint8
{
   // Standard switch behavior
   UsageTogglesSyncedActor,

   // OnToggled / OnRecentlyToggled called without toggling synced actor
   UsageCosmeticOnly,

   // No interaction allowed
   UsageDisabled
};

/// A simple interactable component that toggles a AOSESyncedToggle when used. 
/// Ideal for "switches" that shouldn't maintain their own on/off state, but control that of another actor.
///
/// TODO: Switch to use IOSEToggleInterface. Also maybe move to game-level?
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class OSEINTERACTION_API UOSEInteractableToggleSwitchComponent : public UStaticMeshComponent, public IInteractableInterface
{
   GENERATED_BODY()

public:
   UOSEInteractableToggleSwitchComponent();

   // From UActorComponent
   void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void InitializeComponent() override;

   // From IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;

protected:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEOnSwitchToggled, ACharacter*, character);
   
   /// Implement for ephemeral one-shot effects (i.e. SFX, animation). 
   /// Skipped on clients if a lag spike delays the replicated-interact-time beyond _recentInteractThreshold
   UPROPERTY(BlueprintAssignable)
   FOSEOnSwitchToggled OnRecentlyToggled;

   /// May not be recent
   UPROPERTY(BlueprintAssignable)
   FOSEOnSwitchToggled OnToggled;

   /// Actor to toggle when used. If unassigned, the owning actor (if AOSESyncedToggle) is used
   UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Interactable)
   AOSESyncedToggle* _syncedToggle = nullptr;

private:
   // Must be called on both server/client with the same value!
   UFUNCTION(BlueprintCallable)
   void _SetUsageBehavior(EOSEInteractableToggleSwitchUsageBehavior usageBehavior);

   void _AuthorityToggleSyncedToggle();
   
   UFUNCTION()
   void _OnToggleDelayElapsed();

   UFUNCTION()
   void _OnRep_ServerLastInteractData(const FOSEInteractableToggleData& oldInteractData);

private:
   // (9/19/24) - TAT NOTE
   // Right now (in BP_InteractableToggleSwitch_Boobytrappable) this is being separately set by server/client to the same value, via resolved FTATSceneRequirement.
   // If a future use case arises where client/server do not share the information/authority to jointly assign a value, this field should be updated to replicate.
   //UPROPERTY(Replicated)
   EOSEInteractableToggleSwitchUsageBehavior _usageBehavior = EOSEInteractableToggleSwitchUsageBehavior::UsageTogglesSyncedActor;

   /// If true, this switch can be pulled (to no effect) when the associated synced-toggle actor is in its permanent resting state
   UPROPERTY(EditAnywhere, Category = Interactable)
   bool _allowCosmeticToggleInPermanentSwitchState = true;

   /// Interact animation tag for an instant press
   UPROPERTY(EditDefaultsOnly, Category = Interactable, meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag _interactInstantAnimationTag;

   UPROPERTY(EditAnywhere, Category = Interactable)
   FText _togglePrompt;

   /// Threshold for whether a replicated interaction is considered "old"
   UPROPERTY(EditAnywhere, Category = Interactable, meta = (Units = "seconds", UIMin = 0, ClampMin = 0))
   float _recentInteractThresholdSeconds = 0.75f;

   /// Threshold for a client disambiguating between a replicated server-interact-time duplicating a local-predict interaction, and a uniquely replicated interaction
   UPROPERTY(EditAnywhere, Category = Interactable, meta = (Units = "seconds", UIMin = 0, ClampMin = 0))
   float _clientLocalPredictDedupeThresholdSeconds = 0.5f;

   /// Optional delay between interaction and toggling the target actor
   UPROPERTY(EditAnywhere, Category = Interactable, meta = (Units = "seconds", UIMin = 0, ClampMin = 0))
   float _toggleDelaySeconds = 0.f;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_ServerLastInteractData)
   FOSEInteractableToggleData _lastInteractData;

   FTimerHandle _toggleDelayTimer;
};
