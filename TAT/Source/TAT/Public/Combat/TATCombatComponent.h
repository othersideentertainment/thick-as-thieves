// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Combat/CombatComponent.h"

// ue4
#include "GameplayTagContainer.h"

#include "TATCombatComponent.generated.h"


class UAbilitySystemComponent;
class UAnimMontage;
class UTATMeleeWeaponToolComponent;

// ===============================================================================================================
// ================================================ Chain Attacks ================================================
// ===============================================================================================================
//
// Chain attacks work as such:
// * The player swings their weapon
//     * New chain index is selected on client and included in event payload for attack ability
//     * On attack activation, client and server validate the chain index is valid, and set the index on the tool if it is
// * If it hits a target, client and server merely mark that a swing hit something
//     * on the next swing, this is required to for the next chain to be used.
// * A chain attack continues until the chain index reaches the end of SequentialAttackPools, where it is reset to 0
// * Can only be performed by players at the moment (as they rely on TATGameplayAbility_DispatchMeleeAttack passing along
// the Tool.Usage.XXX tag in the InstigatorTags, and this ability isn't used for AI currently)
// 
// Currently, the state is loosely spread out and only the index is replicated, with the rest kept in sync without
// any real enforcement from the server.
// 
// The intention here is to "favor the shooter", placing great trust in the client's timing and the server validating it 
// loosely (with fudge). 
// 
// ===============================================================================================================

// Locally-updated state used to track data necessary to maintain a chain attack
struct FTATChainAttackState
{
   // Time before which the next swing in the chain has to happen, in local time
   float ChainExpirationTime = 0;

   int32 ChainIndexForLandedSwing = 0;

   // False until the swing lands
   bool SwingLanded = false;
};


/// Settings that control how the player can shove a target
USTRUCT()
struct TAT_API FTATShoveSettings
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ShoveAbilityGameplayEvent;

   UPROPERTY(EditDefaultsOnly)
   bool RequireMomentumToActivate = false;

   UPROPERTY(EditDefaultsOnly)
   TArray<TObjectPtr<UAnimMontage>> ShoveMontages;

   UPROPERTY(EditDefaultsOnly)
   float ShoveDispatchCooldownSeconds = 1.f;
};

UCLASS(BlueprintType)
class TAT_API UTATCombatComponent : public UCombatComponent
{
   GENERATED_BODY()

public:
   UTATCombatComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   int32 GetNextChainAttackIndex() const;

   UFUNCTION(BlueprintPure, Category = "TAT|Combat|Chain Attack")
   FORCEINLINE bool IsChainAttackInProgress() const { return GetNextChainAttackIndex() > 0; }

   bool NotifyStartAttack(FGameplayTag usageTag, const UTATMeleeWeaponToolComponent* equippedWeapon, const UAnimMontage* animMontage, int32 chainIndex);

   // Called by locally-predicted ability on/after impact with target (i.e. Combat.Attack.Hit gameplay event)
   // NOTE: No longer called with a valid prediction key (this was just easier, and I didn't need it, but could be shuffled around if there is a need)
   UFUNCTION(BlueprintCallable)
   void HandleAttackConnectWithTarget(const UTATMeleeWeaponToolComponent* meleeWeaponTool, FGameplayTag usageTag, int32 chainIndex, const FHitResult& hitResult);

   // Expected to be called when a heavy attack swing is executed, with a normalized value indicating the charge duration.
   UFUNCTION(BlueprintCallable)
   void SetLastChargeAttackDurationNormalized(float durationNormalized);

public:
   /// Should we cause a stagger if we land on top of another character
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Combat|Stagger")
   bool StaggerOnLandOnCharacter = false;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Combat|Stagger", Meta=(EditCondition="StaggerOnLandOnCharacter", Categories = "Combat.Unarmed.Stagger"))
   FGameplayTag LandOnStaggerEventTag;

protected:
   virtual void _OnCombatHitValidTargetCharacter(AOSECharacterBase& attacker, AOSECharacterBase& defender, const FHitResult& hit) override;
   virtual bool _OnCombatHitNonCharacterActor(AOSECharacterBase& attacker, AActor& defender, const FHitResult& hit) override;
   virtual bool _CanAttackerBeHostileToDefenderTeam(AOSECharacterBase& attacker, AOSECharacterBase& defender) const override;
   bool _IsNonCharacterTargetValid(const AOSECharacterBase& attacker, const AActor& defender, const FHitResult& hit) const;
   void _SendEventsToAttackerAndDefender(const UAbilitySystemComponent& attacker, AActor* defenderActor, const FHitResult& hit);
   virtual FName _GetBucketForHit(const FHitResult& hit) const override;

   UFUNCTION()
   void _OnBumpedInto(AOSECharacterBase* defender, const FHitResult& impact);
   UFUNCTION()
   void _OnOwnerLanded(const FHitResult& impact);

   void _SendStaggerEvent(FGameplayTag eventTag, AActor* defender, const FHitResult& impact);

private:
   void _AuthorityOnChainAttackEnded(bool completed);
   void _AuthoritySpawnChainAttackThiefVisionGlyph();
   float _GetSecundsUntilChainAttackExpiration(bool includeFudge) const;
   int32 _GetAllowedChainAttackIndex(bool includeFudge) const;
   
   void _ResetChainAttackState();

   UFUNCTION()
   void _OnEquippedToolChanged();

   const UAnimMontage* _SelectShoveMontage() const;

private:
   // Tracks timing/impact data for the most recently performed swing
   FTATChainAttackState _chainAttackState;
   
   // Index to be used when selecting the next attack in the chain
   UPROPERTY(Replicated)
   int32 _lastChainAttackIndex = 0;

   FTimerHandle _authorityChainAttackExpirationTimer;

   // Indicator to spawn after a chain attack ends
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Thief Vision", meta = (Categories = "Indicator", DisplayName = "Chain Attack Indicator"))
   FGameplayTag _chainAttackThiefVisionIndicator;

   // Offset (from the last connecting hit) at which to spawn a chain attack indicator
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Thief Vision", meta = (DisplayName = "Indicator Offset"))
   FVector _chainAttackThiefVisionIndicatorSpawnOffset = FVector::ZeroVector;

   // Cached location of last connecting hit with valid target
   FVector _lastConnectingHitLocation = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Combat|Shove")
   FTATShoveSettings _shoveSettings;

   float _authorityLastShoveTimeSeconds = 0.f;

   float _lastChargeAttackDurationNormalized = 0.f;
};
