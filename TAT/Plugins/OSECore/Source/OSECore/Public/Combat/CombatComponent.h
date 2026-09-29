// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Animation/AnimMetadata_Combat.h"
#include "Animation/AnimNotifyState_Combat.h"
#include "Combat/CombatUtl.h"

// ue4
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Engine/CollisionProfile.h"

#include "CombatComponent.generated.h"

class AOSECharacterBase;
class UAnimMetadata_CombatHitboxes;
class UAnimSequenceBase;
class UOSEGameplayAbility_MeleeAttack;

UCLASS(BlueprintType)
class OSECORE_API UCombatComponent 
   : public UActorComponent
{
   GENERATED_BODY()

public:
   UCombatComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   // Gameplay events via combat anim notifies
   virtual void OnCombatAnimationMontageStart(const UAnimSequenceBase& animation, ECombatAttackDirection attackDirection);
   virtual void OnCombatAnimationSequenceStart(const UAnimSequenceBase& animation, const UAnimMetadata_CombatHitboxes& animMetadata);
   virtual void OnCombatAnimationSequenceEnd(const UAnimSequenceBase& animation);
   virtual void OnCombatAnimationMontageEnd(const UAnimSequenceBase& animation);
   virtual void OnCombatAnimationAutoAim(const UAnimSequenceBase& animation, const UAnimMetadata_CombatHitboxes& animMetadata);
   virtual void OnCombatRangedProjectileSpawn(const UAnimSequenceBase& animation);
   virtual void OnCombatRangedReload(const UAnimSequenceBase& animation);
   virtual void EnableHitbox(const UAnimSequenceBase& animation, int hitboxIndex);
   virtual void DisableHitbox(const UAnimSequenceBase& animation, int hitboxIndex);

   // Gameplay events via abilities
   virtual void OnCombatMeleeAttackAbilityStart(const UOSEGameplayAbility_MeleeAttack& ability);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatAnimationMontageStartEvent);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnCombatAnimationMontageStartEvent OnCombatAnimationMontageStartEvent;
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatAnimationMontageEndEvent);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnCombatAnimationMontageEndEvent OnCombatAnimationMontageEndEvent;
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatAnimationCanBeInterruptedEvent);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnCombatAnimationCanBeInterruptedEvent OnCombatAnimationCanBeInterruptedEvent;

   // List of AI that are agressive towards us
   UFUNCTION(BlueprintPure, Category = "Combat")
   const TArray<FCombatActorInfo>& GetAggressiveCharacterList() const { return _aggroCharacters.CharacterInfos; }

   // Broadcast on both the server and client when the list of AI that are aggressive to us changes
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAggressiveCharacterListChanged, const TArray<FCombatActorInfo>&, agressiveCharacterList);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnAggressiveCharacterListChanged OnAggressiveCharacterListChanged;

   // Utl for the anim montage that talks with us
   void SetLastMontagePosition(float montagePosition) { _lastMontagePosition = montagePosition; }
   float GetLastMontagePosition() const { return _lastMontagePosition; }

   // Utl for interacting with combat input abilities
   UFUNCTION(BlueprintPure, Category = "Combat")
   bool CanCombatAnimationBeInterrupted() const;
   bool HasUninterruptibleCombatAnimation() const;

   /// Returns the number of seconds that have passed since the last time an attack was made
   UFUNCTION(BlueprintPure, Category = "Combat")
   float SecondsSinceLastAttackInitiated() const;

   /// Is combat disabled?  Happens when overextended/parried
   UFUNCTION(BlueprintPure, Category = "Combat")
   bool HasCombatDisabledStatus() const;

   /// Returns the number of seconds that have passed since combat was disabled, or -1 if combat is not disabled
   UFUNCTION(BlueprintPure, Category = "Combat")
   float SecondsSinceCombatDisabledStatusGained() const;

   UFUNCTION(BlueprintPure, Category = "Combat")
   ECombatAttackDirection GetCombatAttackDirection() const { return _currentAttackDirection; }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatDisabledStatusChanged, bool, hasCombatDisabledStatus);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnCombatDisabledStatusChanged OnCombatDisabledStatusChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCombatHitValidTargetCharacter, AOSECharacterBase*, attacker, AOSECharacterBase*, defender, const FHitResult&, hit);
   UPROPERTY(BlueprintAssignable, Category = "Combat")
   FOnCombatHitValidTargetCharacter OnCombatHitValidTargetCharacter;

   virtual void AddTargetingCharacter(AOSECharacterBase* character);
   virtual void RemoveTargetingCharacter(AOSECharacterBase* character);
protected:
   // for our subclasses to implement their game-specific combat abilities
   virtual bool _IsTargetValid(AOSECharacterBase& attacker, AOSECharacterBase& defender, float forwardDotTolerance = float(INDEX_NONE)) const;
   virtual bool _CanAttackerBeHostileToDefenderTeam(AOSECharacterBase& attacker, AOSECharacterBase& defender) const;
   virtual void _OnCombatHitValidTargetCharacter(AOSECharacterBase& attacker, AOSECharacterBase& defender, const FHitResult& hit);
   virtual void _OnCombatHitInvalidTargetCharacter(AOSECharacterBase& attacker, AOSECharacterBase& defender, const FHitResult& hit) { }
   virtual bool _OnCombatHitNonCharacterActor(AOSECharacterBase& attacker, AActor& defender, const FHitResult& hit) { return true; }
   virtual FName _GetBucketForHit(const FHitResult& hit) const;

   void _SetCombatSwingTraceProfile(FCollisionProfileName profileName) { _combatSwingTraceProfile = profileName; }

private:
   void _ResetCombatAnimState();
   void _TickHitboxes();
   void _TraceHitbox(const FCombatHitboxMetadata& hitboxMetadata, TArray<FHitResult>& outHitResults, bool debugDraw);
   void _TryAutoAimOnSwing(const TMap<int, FCombatHitboxMetadata>& hitBoxes);
   void _SendGenericAttackerEvent(const FGameplayTag& eventTag) const;
   FTransform _GetHitboxWorldXfm(const FCombatHitboxMetadata& hitboxMetadata) const;
   
   void _BindToAbilitySystemComponentInit();
   void _OnAbilitySystemComponentInit();

   UFUNCTION()
   void OnRep_AggroCharacters();
   void _BroadcastAggroCharactersChanged();

   // tag callback(s) from the asc
   void _OnStatusCombatDisabledChanged(const FGameplayTag tag, int32 newTagCount);

   UPROPERTY(EditDefaultsOnly, Category="Combat")
   bool _UseViewRotationWhenCalculatingHitBoxes { true };

protected:
   FCombatAnimationInfo _combatAnimInfo;

   /// 0-1 value where 1 is directly facing the target, the higher it is the more dead-on we need to be facing it.
   UPROPERTY(EditDefaultsOnly, Category = "Combat|Auto Aim", meta = (ClampMin = "0.0", ClampMax = "0.99", UIMin = "0.0", UIMax = "0.99"))
   float AutoAimForwardDotTolerance = 0.75f;

private:
   bool _combatAnimationInProgress = false;
   bool _combatAnimationCanBeInterrupted = false;
   TMap<int, FCombatHitboxMetadata> _hitboxes;
   TArray<int> _enabledHitboxes;

   using FActorAndBucket = TTuple<TWeakObjectPtr<AActor>, FName>;
   TArray<FActorAndBucket> _hitActors;
   float _lastMontagePosition = 0.0f;
   FCollisionProfileName _combatSwingTraceProfile;
   ECombatAttackDirection _currentAttackDirection = ECombatAttackDirection::None;

   // who's attacking us?
   UPROPERTY(ReplicatedUsing = OnRep_AggroCharacters)
   FCombatActorInfoArray _aggroCharacters;

   // System time at which last attack was made
   float _lastAttackInitiatedTime = float(INDEX_NONE);
   // System time at which we last gained a "combat disabled" status
   float _lastCombatDisabledTagGainedTime = float(INDEX_NONE);
};

OSECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogCombatComponent, Log, All);
