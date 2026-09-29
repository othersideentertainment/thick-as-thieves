// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Interactables/InteractableInterface.h"

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Components/ActorComponent.h"

#include "TATBreakableComponent.generated.h"

class UAbilitySystemComponent;
class UAkAudioEvent;
class UAkComponent;
class UOSEGameplayEffectSet;
class UTATPropHealthAttributeSet;
struct FGameplayEffectSpec;
struct FOnAttributeChangeData;
struct FGameplayCueParameters;

struct FTATAuthorityBreakContext
{
   explicit FTATAuthorityBreakContext(const FGameplayEffectSpec* spec) : Spec(spec) {}
   
   FVector OptionalOrigin = FVector::ZeroVector;

   // Effect spec responsible for the damage event causing the break
   const FGameplayEffectSpec* Spec = nullptr;

   AActor* GetInstigator() const;
};

// A component that represents the health and breakability of environmental
// actors, backed by an ability system component.
//
// Not meant for trivial responses to damage on actors that do not already have it.
// 
// NOTE(2023-09-19): Still in flux, and subject to change, especially w.r.t. replication
UCLASS(Blueprintable, hideCategories = (ComponentTick,ComponentReplication,Replication,Activation, Collision,Sockets,Tags,Cooking))
class TAT_API UTATBreakableComponent : public UActorComponent
   , public IInteractableInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTATBreakableComponent();

   UFUNCTION(BlueprintCallable, Category = "Breakables")
   static UTATBreakableComponent* GetBreakableComponentFromActor(AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "Breakable|Internal")
   static void DispatchBreakableDamageFromGameplayCue(AActor* actor, const FGameplayCueParameters& params);

   virtual void InitializeComponent() override;
   virtual void PostLoad() override;
   virtual ELifetimeCondition GetReplicationCondition() const override { return _everDamaged ? COND_None : COND_OwnerOnly; }

   // Only meant to be called in constructor
   void SetIsBreakableByDefault(bool breakable);

   // For cheats
   void ForceRepair();

   UFUNCTION(BlueprintPure, Category="Breakables")
   bool IsBroken() const;

   UFUNCTION(BlueprintPure, Category = "Breakable")
   bool IsBreakable() const { return _isBreakable; }

   UFUNCTION(BlueprintPure, Category = "Breakable")
   float GetHealth() const;

   UFUNCTION(BlueprintPure, Category = "Breakable")
   float GetMaxHealth() const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageTaken, AActor*, instigator, const FVector&, origin, float, damageAmount);
   UPROPERTY(BlueprintAssignable, Category = "Breakable")
   FOnDamageTaken AuthorityOnDamageTaken;

   // Called when damage taken, but not always guaranteed to be called on clients
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTakenDamageUnreliable, AActor*, instigator, const FVector&, origin, float, damageAmount);
   UPROPERTY(BlueprintAssignable, Category = "Breakable")
   FOnTakenDamageUnreliable UnreliableOnDamageTaken;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, previousHealth, float, newHealth);
   UPROPERTY(BlueprintAssignable, Category = "Breakable")
   FOnHealthChanged OnHealthChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBrokenChanged, bool, isBroken);
   UPROPERTY(BlueprintAssignable, Category = "Breakable")
   FOnBrokenChanged OnBrokenChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecentlyBroken, const FVector &, optionalOrigin);
   UPROPERTY(BlueprintAssignable, Category = "Breakable")
   FOnRecentlyBroken OnRecentlyBroken;

   DECLARE_MULTICAST_DELEGATE_OneParam(FOnBrokenAuthority, const FTATAuthorityBreakContext&);
   FOnBrokenAuthority OnBrokenAuthority;

   // Interactable Interface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;

   void AppendDebugString(FString& outResult) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

   void _AuthorityGrantInitialEffects();
   void _UpdateBrokenTag();

   UFUNCTION()
   void _OnRep_Health(float previousHealth);

   void _OnHealthChanged(float previousHealth);

   void _AuthorityOnHealthAttributeChanged(const FOnAttributeChangeData& data);
   void _AuthorityOnDamageAttributeChanged(const FOnAttributeChangeData& data);

   void _MarkEverDamaged();

   void _AuthorityOnOutOfHealth(AActor* instigator, const FGameplayEffectSpec* spec);
   void _AuthorityEmitBrokenStim(const FVector& origin);

   void _CompleteRepair();

   void _HandleDamageCue(const FGameplayCueParameters& params);

   UFUNCTION(NetMulticast, Unreliable)
   void _MulticastOnRecentlyBroken(FVector_NetQuantize origin);

   UAkComponent* _GetAudioComponent();

protected:
   UPROPERTY(EditDefaultsOnly, Category = Breakable)
   bool _isBreakable = true;

   UPROPERTY(EditDefaultsOnly, Category = Breakable, AdvancedDisplay, meta = (EditCondition = "_isBreakable", EditConditionHides))
   bool _interactableWhenBroken = false;

   // Skips sending the recently broken RPC, if it isn't needed, or sent other ways
   UPROPERTY(EditDefaultsOnly, Category = Breakable, AdvancedDisplay, meta = (EditCondition = "_isBreakable", EditConditionHides))
   bool _skipRecentlyBrokenRpc = false;

   bool _everDamaged = false;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_Health)
   float _health;

   UPROPERTY(EditDefaultsOnly, Category = Breakable, meta = (EditCondition = "_isBreakable", EditConditionHides))
   FScalableFloat _maxHealth = 20;

   UPROPERTY(EditDefaultsOnly, Category = Repair)
   bool _isRepairable;

   UPROPERTY(EditDefaultsOnly, Category = Repair, meta=(EditCondition = "_isRepairable", EditConditionHides))
   FScalableFloat _repairDuration = 5;

   UPROPERTY(EditDefaultsOnly, Category = Repair, AdvancedDisplay, meta = (EditCondition = "_isRepairable", EditConditionHides))
   FName _highlightTagForRepair;

   // If set, overrides the default hearing stim in project settings
   UPROPERTY(EditDefaultsOnly, Category = Stims)
   FGameplayTag _breakStimOverride;

   UPROPERTY(EditDefaultsOnly, Category = Breakable)
   FGameplayTagContainer _intrinsicTags;

   // The sound played when taking damage
   // (not the sound played when broken)
   UPROPERTY(EditDefaultsOnly, Category = Audio)
   TObjectPtr<UAkAudioEvent> _damageSound;

   // Effects that are applied for the purpose of resistances/vulnerabilities
   // NOTE: It is important not to have any effects that change a replicated attribute or grant tags
   UPROPERTY(EditDefaultsOnly, Category = Breakable, meta = (EditCondition = "_isBreakable", EditConditionHides))
   TArray<TObjectPtr<UOSEGameplayEffectSet>> _intrinsicEffects;

private:
   UPROPERTY(Transient)
   UAbilitySystemComponent* _abilitySystemComponent;

   UPROPERTY()
   UTATPropHealthAttributeSet* _healthAttributes;

   TWeakObjectPtr<UAkComponent> _cachedAudioComponent;
};
