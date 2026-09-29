// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"

#include "TATAuraVisibilityTypes.generated.h"

class UTATAuraVisibilityPerceiverComponent;
class UTATAuraVisibilityTargetComponent;

USTRUCT()
struct TAT_API FTATAuraVisibilityTargetCachedData
{
   GENERATED_BODY()

   FTATAuraVisibilityTargetCachedData() {}
   FTATAuraVisibilityTargetCachedData(TWeakObjectPtr<const UTATAuraVisibilityTargetComponent> auraTarget, const bool perceiverHasLineOfSight);

   UPROPERTY(Transient)
   TWeakObjectPtr<const UTATAuraVisibilityTargetComponent> AuraTarget = nullptr;

   UPROPERTY(Transient)
   bool PerceiverHasLineOfSight = false;
};

UENUM()
enum class ETATAuraVisibilityType : uint8 
{
   None,
   HalfVisible,
   FullVisible
};

// -------------------------------------------------------
// TATAuraVisibilityPerceverSense
// 
// A definition of rules driving conditions under which a perceiver should see a target's aura, as well as 
// the aura's visibility for both the sense-owning perceiver and their teammates.
// -------------------------------------------------------
USTRUCT()
struct FTATAuraVisibilityPerceiverSense
{
   GENERATED_BODY()

public:
   // Returns true if this sense propogates auras to teammates (i.e. requires evaluation on server)
   bool IsSharedSense() const { return SharedAuraVisibility != ETATAuraVisibilityType::None; };

   // Returns true if this sense does not propogate auras to teammates (i.e. can be evaluated on owning client's machine)
   bool IsLocalSense() const { return SharedAuraVisibility == ETATAuraVisibilityType::None; }

public:
   // Serves as an identifier for this sense
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag AuraSenseTag;

   // Aura visibility for the perceiving player
   UPROPERTY(EditDefaultsOnly)
   ETATAuraVisibilityType PerceivingPlayerAuraVisibility = ETATAuraVisibilityType::None;

   // Aura visibility propagated to the perceiving player's teammates. Set to None if you want the aura to only be visible to the perceiving player.
   UPROPERTY(EditDefaultsOnly)
   ETATAuraVisibilityType SharedAuraVisibility = ETATAuraVisibilityType::None;

   // If true, this sense can only perceive auras within the perceiving player's line-of-sight. 
   UPROPERTY(EditDefaultsOnly)
   bool RequiresLineOfSight = true;

   // If true, the perceiving player must be standing still for auras to become visible.
   UPROPERTY(EditDefaultsOnly)
   bool RequiresStationaryPerceiver = false;

   // If true, the owner must be possessed for this sense to perceive target auras. Will generally be true except in edge cases (eg. giving a Tulpa the Second Sight sense).
   UPROPERTY(EditDefaultsOnly)
   bool RequiresOwnerPossession = true;

   // How far from player can a target be before its aura is hidden?
   UPROPERTY(EditDefaultsOnly)
   float EffectiveRange = 6000.f;

   // How long should this aura's visual effects persist after it no longer meets this sense's perception requirements?
   UPROPERTY(EditDefaultsOnly)
   float AuraDurationAfterLastPerceived = 0.f;

   // Float between 0 <-> 1 used in dot product to exclude targets outside line-of-sight (if RequiresLineOfSight = true). 
   // Smaller => wider aura FOV, larger => more narrow.
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "RequiresLineOfSight"))
   float LineOfSightDotProductThreshold = .8f;

   // Perception requires the perceiving player to pass this tag query
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery PerceiverRequiredTagQuery;
};

USTRUCT()
struct FTATAuraVisibilityState
{
   GENERATED_BODY()

public:
   FTATAuraVisibilityState() {}
   FTATAuraVisibilityState(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag, bool isPerceived);

   // Returns true if value changed
   bool SetPerceived(UTATAuraVisibilityTargetComponent* perceivedTarget, bool isPerceived);

   // Returns true if the target meets the aura sense requirements of the perceiver, or is otherwise visible for the sense's aura persistence duration
   bool IsPerceived() const;

   void ClearPersistenceTimer(UTATAuraVisibilityTargetComponent* perceivedTarget);

   UPROPERTY()
   TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> PerceivingPlayer = nullptr;

   UPROPERTY()
   FGameplayTag AuraSenseTag;

private:
   // Timer handle used for aura persistence
   FTimerHandle _timerHandle;

   // True if the target meets the aura sense requirements of the perceiver
   bool _isPerceived = false;
};

USTRUCT()
struct FTATAuraVisibilityStateArray
{
   GENERATED_BODY()

public:
   UPROPERTY()
   TArray<FTATAuraVisibilityState> Items;

   // Updates (or creates) aura sense entry for the given perceiving player / aura sense tag. 
   // Returns true if entry was created/changed, false if already present/unchanged.
   bool UpdateAuraPerceiverSenseEntry(UTATAuraVisibilityTargetComponent* perceivedTarget, TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag, bool isBeingPerceived);

   void ClearAuraSensePersistenceTimer(UTATAuraVisibilityTargetComponent* perceivedTarget, TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag);

   void RemoveAuraEntriesForPerceiver(UTATAuraVisibilityTargetComponent* perceivedTarget, TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer);

   ETATAuraVisibilityType ResolveLocalAuraVisibility() const;

private:
   FTATAuraVisibilityState* _GetAuraVisibilityState(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag);
};
