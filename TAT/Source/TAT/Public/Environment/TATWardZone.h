// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/TATMapVariationMgrComponent.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"

#include "TATWardZone.generated.h"

/// Different visual states that wards can appear in for local players
UENUM(BlueprintType)
enum class ETATWardZoneVisualState : uint8
{
   Nearby_ZoneVisible  UMETA(DisplayName = "Nearby (Zone Visible)", Tooltip = "The player is nearby and the ward zone should be clearly visible to the player"),
   Nearby_ZoneHidden   UMETA(DisplayName = "Nearby (Zone Hidden)", Tooltip = "The player is nearby, but the ward zone should be hidden or visually subtle to the player"),
   FarAway             UMETA(Tooltip = "The player's distance is greater than VisualStateMaxDistance"),
};

/// How to determine if a nearby player sees a ward zone in the ZoneVisible or ZoneHidden state
UENUM(BlueprintType)
enum class ETATWardZoneVisibleCriteria : uint8
{
   AlwaysVisible                  UMETA(Tooltip = "For nearby players, the zone is always in the ZoneVisible state"),
   AlwaysHidden                   UMETA(Tooltip = "For nearby players, the zone is always in the ZoneHidden state"),
   WardApplicationCriteria        UMETA(Tooltip = "The zone should be visible if the ward's effect would apply to the player"),
   WardApplicationCriteriaInverse UMETA(Tooltip = "The zone should be visible if the ward's effect would NOT apply to the player"),
   CustomTagQuery                 UMETA(Tooltip = "Use a custom tag query to determine zone visibility"),
};

USTRUCT(BlueprintType)
struct TAT_API FTATWardZoneState
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone State")
   double LastChangeTime = 0.0;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone State")
   bool IsActive = false;
};

struct TAT_API FTATWardZonePlayerVisibilityState
{
   ETATWardZoneVisualState State = ETATWardZoneVisualState::FarAway;
   bool ZoneActive = false;

   bool operator==(const FTATWardZonePlayerVisibilityState& rhs) const
   {
      return State == rhs.State && ZoneActive == rhs.ZoneActive;
   }
};

USTRUCT(BlueprintType)
struct TAT_API FTATWardZoneInteractConfig
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone Interact Config")
   FText Prompt;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone Interact Config")
   FGameplayTag ActionTag;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone Interact Config")
   FGameplayTag StatusTag;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone Interact Config")
   bool IsHold = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone Interact Config", Meta = (EditCondition = "IsHold", UIMin = 0, ClampMin = 0, ForceUnits = "s"))
   float HoldDuration = 1.0f;
};

USTRUCT(BlueprintType)
struct TAT_API FTATWardZoneGameplayEffectParam
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone Gameplay Effect Parameter", Meta = (Categories = "SetByCaller"))
   FGameplayTag Tag;

   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone Gameplay Effect Parameter")
   float Value = 0.0f;
};

USTRUCT()
struct FTATWardZoneGameplayEffect
{
   GENERATED_BODY()

   /// Gameplay effect to apply to a character triggering the ward
   UPROPERTY(EditDefaultsOnly, Category = "Ward Zone|Ward Gameplay Effect")
   TSubclassOf<UGameplayEffect> WardGameplayEffect;

   /// Gameplay effect to apply to a character triggering the ward
   UPROPERTY(EditDefaultsOnly, Category = "Ward Zone|Ward Gameplay Effect")
   float EffectLevel = -1.0f;

   /// SetByCaller parameters to pass to the WardGameplayEffect
   UPROPERTY(EditDefaultsOnly, Category = "Ward Zone|Ward Gameplay Effect", meta = (TitleProperty="{Tag} -> {Value}"))
   TArray<FTATWardZoneGameplayEffectParam> SetByCallerParams;
};


/// TODO(2025-07-08): A lot of the functionality in the is class (interaction, conditional activation, conditional visibility) is no longer
///                   relevant. If this remains the case post-PAM, then should strip them out.
///                   
///
/// Original comment:
/// A zone placed in levels that applies gameplay effects (wards) to characters entering them if those characters meet specific criteria.
/// Ward zones can alter their visual appearance based on the state of the player viewing them.
UCLASS()
class TAT_API ATATWardZone
   : public AActor
   , public IInteractableInterface
{
   GENERATED_BODY()

   ATATWardZone();

public:
   // From AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void Tick(float deltaTime) override;
   virtual void NotifyActorBeginOverlap(AActor* otherActor) override;
   virtual void NotifyActorEndOverlap(AActor* otherActor) override;

   // From IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

   /// The initial active state of the ward at actor spawn time
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone")
   bool WardInitiallyActive = true;

   /// Can players interact with this ward zone to activate it when it's deactivated?
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Interaction")
   bool AllowInteractToActivate = false;

   /// Can players interact with this ward zone to deactivate it when it's activated?
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Interaction")
   bool AllowInteractToDeactivate = false;

   /// Interact prompt shown to players when the ward zone is inactive
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Interaction", Meta = (EditCondition = "AllowInteractToActivate"))
   FTATWardZoneInteractConfig WardActivatePrompt;

   /// Interact prompt shown to players when the ward zone is active
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Interaction", Meta = (EditCondition = "AllowInteractToDeactivate"))
   FTATWardZoneInteractConfig WardDeactivatePrompt;

   /// Executed gameplay cue responsible for performing one-off toggle feedback (eg. animation, SFX)
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Interaction", Meta = (EditCondition = "AllowInteractToActivate || AllowInteractToDeactivate", Categories = "GameplayCue"))
   FGameplayTag ActivationToggleGameplayCue;

   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Interaction", Meta = (EditCondition = "AllowInteractToActivate || AllowInteractToDeactivate", Categories = "InteractAnimation"))
   FGameplayTag InteractAnimationTag;

   /// Only apply ward effects to targets matching this gameplay tag query.
   /// Targets inside the zone that do not match are polled in case they start matching later.
   ///
   /// For example, if you require targets to NOT be crouching and a crouching player
   /// enters the ward, then that player will be affected by the ward as soon as they stand up.
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Target Criteria")
   FGameplayTagQuery WardApplicationCriteria;

   /// Should the ward trigger for player/human-controlled characters?
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Target Criteria")
   bool ApplyWardToPlayerCharacters = true;

   /// Should the ward trigger for NPCs?
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Target Criteria")
   bool ApplyWardToAICharacters = false;

   /// The amount of time after a ward applies to a character during which the ward can't apply to the character again.
   /// You can set this duration to zero and do all the logic using a gameplay effect instead.
   /// This is only generally needed for simple cases or to avoid cases where characters might be quickly skirting around the edge of a ward.
   ///
   /// If set to zero, the ward will apply exactly once upon entering the ward (exiting and re-entering the ward will cause it to apply again).
   ///
   /// If WardDamage is enabled, this can function as a simple periodic damage effect:
   ///  - Damage will be dealt, the character becomes immune for this duration, then as soon as the immunity duration is up, the damage will apply again.
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Target Criteria", Meta = (UIMin = 0, ClampMin = 0, ForceUnits = "s"))
   float WardImmunityDuration = 0.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Ward Zone|Ward Gameplay Effect", meta = (TitleProperty="{WardGameplayEffect}"))
   TArray<FTATWardZoneGameplayEffect> WardGameplayEffects;

   /// Remove the ward from characters that leave the zone?
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Ward Gameplay Effect")
   bool RemoveWardGameplayEffectOnLeaveZone = false;

   /// Remove wards from all affected characters when the ward zone is deactivated?
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Ward Gameplay Effect")
   bool RemoveWardGameplayEffectOnDeactivate = false;

   /// Optional noise stim to trigger whenever the ward's gameplay effect is applied to a character
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Ward Gameplay Effect", Meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag WardNoiseStim;

   /// Distance threshold for players to be considered "far away".
   /// Far away players will see the ward zone as visually minimal or hidden.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ward Zone|Visual State")
   float VisualStateMaxDistance = 2000.0f;

   /// How to determine zone visibility for nearby players
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Visual State")
   ETATWardZoneVisibleCriteria VisualStateNearbyCriteria = ETATWardZoneVisibleCriteria::AlwaysVisible;

   /// Custom tag query that players must match for the ward zone to be in the "Nearby Visible" state for that player.
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Ward Zone|Visual State", Meta = (EditCondition = "VisualStateNearbyCriteria == ETATWardZoneVisibleCriteria::CustomTagQuery"))
   FGameplayTagQuery VisualStateNearbyVisibleTagQuery;

   /// The size of the zone
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Replicated, Category = "Ward Zone", meta = (MakeEditWidget = true))
   FVector ZoneSize = {30, 200, 200};

   /// Checks if a character is inside the ward zone's collision.
   /// Note that this does NOT check if that character has been affected by the ward.
   UFUNCTION(BlueprintPure, Category = "Ward Zone")
   bool IsCharacterInZone(ACharacter* character) const;

   /// Checks if a character had the ward applied.
   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Ward Zone")
   bool AuthorityIsCharacterWarded(ACharacter* character) const;

   /// Checks if the ward zone is active/enabled or not.
   /// A non-active ward zone will not apply effects to characters.
   UFUNCTION(BlueprintPure, Category = "Ward Zone")
   bool IsWardZoneActive() const { return _wardZoneState.IsActive; }

   /// Sets a ward zone's active state.
   /// A ward zone must be in the active state to apply effects to characters.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ward Zone")
   void SetWardZoneActive(bool newActive);

   /// Called by UTATWardZoneSubsystem for local players only to keep the visual state in sync with the player's location and state
   void TickLocalPlayerVisualState(ACharacter* character, bool firstTick, FTATWardZonePlayerVisibilityState& prevState);

   /// Called when the local player's distance to the ward zone or visual state tag query changes
   UFUNCTION(BlueprintNativeEvent)
   void OnLocalPlayerVisualStateChange(ACharacter* character, ETATWardZoneVisualState state, bool isWardZoneActive);

   /// Sets the ward to be locally interactable or not.
   /// This is intended to be used only for the local player - it is not replicated.
   ///
   /// The primary use-case here is to hide the interact UX when the related VFX is hidden for that player.
   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Ward Zone")
   void SetWardZoneInteractableForLocalPlayer(bool newInteractable);

   /// Checks if the local player is allowed to interact with the ward zone.
   /// This checks if interaction is configured at all as well as if SetWardZoneInteractableForLocalPlayer was used.
   UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = "Ward Zone")
   bool IsWardZoneInteractableForLocalPlayer() const;

protected:
   virtual void _AuthorityApplyWardToCharacter(ACharacter* character);
   virtual void _AuthorityRemoveWardFromCharacter(ACharacter* character);
   virtual bool _AuthorityDoesCharacterMatchWardCriteria(ACharacter* character) const;

   /// Called on clients and servers when the ward zone is activated or deactivated.
   /// Useful to change VFX state.
   UFUNCTION(BlueprintNativeEvent)
   void OnWardZoneActiveChanged(bool isActive, bool isRecentChange);

   /// Called on all characters entering the zone (regardless of if the ward was triggered)
   UFUNCTION(BlueprintNativeEvent)
   void OnCharacterEnterZone(ACharacter* character);

   /// Called on all characters leaving the zone (regardless of if the ward was triggered)
   UFUNCTION(BlueprintNativeEvent)
   void OnCharacterLeaveZone(ACharacter* character);

   /// Called on authority when a character that matches the target criteria enters the zone, or when a character
   /// inside the zone that previously did not match the target criteria now does match.
   UFUNCTION(BlueprintImplementableEvent)
   void AuthorityOnCharacterApplyWard(ACharacter* character);

   UFUNCTION(BlueprintImplementableEvent)
   void AuthorityOnCharacterRemoveWard(ACharacter* character);

   /// Called on all clients (but not on dedicated servers) whenever a ward is applied to a character
   UFUNCTION(BlueprintImplementableEvent)
   void OnCharacterApplyWardCosmetic(ACharacter* character);

   UFUNCTION(NetMulticast, Unreliable)
   void _MulticastWardApplied(ACharacter* character);

   UFUNCTION()
   void _HandleMapStateChanged(ETATMapVariationLoadingState currentState);

   void _WaitForWorldBeginPlayOrTrigger();

   void _OnWorldBeginPlay();

   void _HandleInitialOverlaps();

   void _HandleActorOverlapBegin(AActor* actor);
   void _HandleActorOverlapEnd(AActor* actor);

   UFUNCTION()
   void _OnRep_WardZoneState(const FTATWardZoneState& prevState);

   void _FireActivationToggleGameplayCue(ACharacter* character);

   bool _isReadyToProcessActorOverlaps = false;

   /// Local player only - if false, disable interacting with the ward.
   /// This is intended to allow hiding the ward's interactable point when it's not intended to be locally visible.
   bool _localPlayerAllowInteraction = true;

   UPROPERTY(Transient)
   TSet<ACharacter*> _overlappingCharacters;


   struct FTATWardEffectEntry
   {
      TWeakObjectPtr<ACharacter> Character = nullptr;
      FActiveGameplayEffectHandle EffectHandle;

      bool operator==(TWeakObjectPtr<ACharacter> otherCharacter) const { return Character == otherCharacter; }
   };

   /// The set of characters that have had the ward effect applied and the active gameplay effect handle applied with the ward (if any)
   /// May be present multiple times if there are multiple effects
   TArray<FTATWardEffectEntry> _authorityWardedEffects;

   /// Map of the last time the ward triggered (per-character).
   /// This is separate from _authorityWardedCharacters because this needs to persist after the ward is removed.
   TMap<TWeakObjectPtr<ACharacter>, float> _authorityWardTriggerTime;

   UPROPERTY(Transient, Replicated, ReplicatedUsing = _OnRep_WardZoneState)
   FTATWardZoneState _wardZoneState;
};
