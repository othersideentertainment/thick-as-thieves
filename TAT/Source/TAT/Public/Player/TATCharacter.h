// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Animation/TATCharacterAnimationInterface.h"
#include "Animation/TATHitReactAnimInterface.h"
#include "Combat/TATCombatDamageBonusTargetInterface.h"
#include "Disguise/TATDisguisableCharacterInterface.h"
#include "Interactables/TATLockPicker.h"
#include "Items/TATItemInventorySystemInterface.h"
#include "Loot/TATLootInterface.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"
#include "Disguise/TATDisguiseTargetInterface.h"
#include "Character/TATCharacterBase.h"
#include "AI/StateTrees/Interfaces/TATStateTreeCombatTargetInterface.h"
#include "WorldMap/TATMapActorComponent.h"
#include "AI/UnifiedStealthSystem/TATStealthScoreComponent.h"
#include "AI/UnifiedStealthSystem/TATStealthScoreInterface.h"
#include "AI/Perception/TATHearingStimSourceReactor.h"
#include "Environment/TATAreaVolume.h"

// ose
#include "OSELightDetectionComponent.h"
#include "OSELightDetectionInterface.h"
#include "AI/Alertness/DetectionEnums.h"
#include "AI/Target/DetectionTargetInterface.h"
#include "AI/Perception/OSEAIVisibilityTargetInterface.h"
#include "Player/OSEPlayerCharacter1P.h"
#include "AI/Utility/UtilityAIBehaviorTargetInterface.h"
#include "AI/Utility/UtilityAITokenOwner.h"
#include "AI/Utility/UtilityAITokenRequester.h"
#include "AI/Perception/OSEAITargetSightInterface.h"


// ue
#include "ScalableFloat.h"
#include "Perception/AISightTargetInterface.h"

#include "TATCharacter.generated.h"

enum class ETATInventoryUpdateEventType : uint8;
class ATATAIController;
class ATATPlayerState;
class UTATStealthScoreComponent;
class UTATFirstPersonViewModifier;
class UUtilityAIStateBase;
class UAkRtpc;
class UAkSwitchValue;
class UGameplayEffect;
class UToolComponent;
class UTATCombatComponent;
class UTATItemInventoryComponent;
class UTATLootInventoryComponent;
class UAnimInstance;
class UTATInteractionTargeterComponent;
class UTATDisguiseComponent;
class UTATCharacterAnimationMappingAsset;
class UTATPrivateSpaceCharacterComponent;
class UTATXrayComponent;
struct FTATCharacterLoadoutEntry;

enum EPhysicalSurface : int;

USTRUCT()
struct TAT_API FTATCharacterVisibilityTraceLocation
{
   GENERATED_BODY()
public:

   /// Height of the trace location
   /// 0 = center, 1 = top of character capsule, -1 = bottom of character capsule
   UPROPERTY(EditAnywhere)
   float VerticalTraceDisplacement = 0.0f;

   /// Horizontal location of the trace location
   /// 0 = center, -1 = left of capsule, 1 = right of capsule
   UPROPERTY(EditAnywhere)
   float HorizontalTraceDisplacement = 0.0f;

   /// How important this trace is relative to others
   /// The exact number is not important, only how big it is compared to the importance of the other traces
   UPROPERTY(EditAnywhere)
   float RelativeImportance = 1.0f;
};

//--------------------------------------------------------------------------------------------------
/// Game-specific base character
//--------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable)
class TAT_API ATATCharacter
   : public ATATCharacterBase
   , public IAISightTargetInterface
   , public ITATItemInventorySystemInterface
   , public ITATLockPicker
   , public ITATLootInventoryInterface
   , public ITATHitReactAnimInterface
   , public IAIDetectionTargetInterface
   , public ITATDisguisableCharacterInterface
   , public IUtilityAIBehaviorTargetInterface
   , public IUtilityAITokenOwnerInterface
   , public IUtilityAITokenRequesterInterface
   , public IOSEAIVisibilityTargetInterface
   , public ITATCombatDamageBonusTargetInterface
   , public ITATCharacterAnimationInterface
   , public ITATDisguiseTargetInterface
   , public ITATPrivateSpaceCharacterInterface
   , public IOSELightDetectionInterface
   , public ITATStealthScoreInterface
   , public ITATHearingStimSourceReactor
   , public IOSEAITargetSightInterface
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecentlyEquippedWeaponChanged, TSubclassOf<UToolComponent>, weaponItemClass);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRecentlyEquippedItemChanged, TSubclassOf<UToolComponent>, itemClass);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectHighlightChanged, bool, isHighlighted);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewingActorsChanged, const TArray<AActor*>&, viewingActors);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAreaInfoChanged, const FTATAreaInfo&, areaInfo);

public:

   /// Sets default values for this character's properties
   ATATCharacter(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // from AActor
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void Tick(float deltaTime) override;
   virtual void BecomeViewTarget(class APlayerController* pc) override;

   // from APawn
   virtual void PawnClientRestart() override;

   // from OSECharacterBase
   virtual void InitializeAbilities(UOSEAbilitySystemComponent* inComponent, UAttributeBaseSet* inAttributeSet) override;
   virtual void ResetAbilities() override;
   virtual float GetMovementMaxSpeedMultiplierToApply() const override;
   
   // start IOSEAITargetSightInterface
   virtual EAISightBucket GetBucketForTarget_Implementation() const override { return EAISightBucket::High; };
   // end IOSEAITargetSightInterface

   virtual void HandleTeamChanged();
   virtual void HandleTeamMemberJoined(ATATCharacter* newTeamMember);
   UFUNCTION(NetMulticast, Reliable)
   void MulticastOnQuestLootChanged(ETATInventoryUpdateEventType eventType);
   void RevealOnMapIfApplicable() const;

   /// area info
   void ServerSetAreaInfo(const FTATAreaInfo& areaInfo);
   void ServerClearAreaInfo();
   UFUNCTION(BlueprintPure, Category = "Area Info")
   const FTATAreaInfo& GetAreaInfo() const { return _areaInfo; }
   UPROPERTY(BlueprintAssignable)
   FOnAreaInfoChanged OnAreaInfoChanged;

protected:
   UFUNCTION()
   void _OnUserSettingApplied(FGameplayTag settingTag);
   void _RefreshCameraFov();
   UFUNCTION()
   void _OnMapActorRegistered(const UTATMapActorComponent* mapActor);
   virtual void _OnHealthChanged(const FOnAttributeChangeData& data) override;
   virtual void _OnDamageChanged(const FOnAttributeChangeData& data) override;

   UFUNCTION()
   void _OnStealthScoreChanged();

   virtual void HandleSetPlayerState() override;
   // Called when the PlayerState is set or replicated (on all clients + server)
   // Can be used as a hook for thing that are based off the player state,
   // such as initializing colors from player state, or major-loot-having-changes
   // (although the latter may still benefit from a native hook if doing something)
   // Deliberately slightly sticky during transient un-possession
   UFUNCTION(BlueprintImplementableEvent, meta=(BlueprintProtected, DisplayName = "OnPlayerStateSet"))
   void BP_OnPlayerStateSet(AOSEPlayerState* previousPlayerState, AOSEPlayerState* currentPlayerState);

   virtual void OnPlayerStateChanged(APlayerState* newPlayerState, APlayerState* oldPlayerState) override;
   
   UPROPERTY(EditDefaultsOnly, Category = "Map")
   UTATMapActorComponent* _mapActorComponent = nullptr;

   FTATAreaInfo _areaInfo;

public:
#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // from ITATLockPicker
   virtual bool CanLockPick() const override;

   // from ITATLootInventoryInterface
   virtual UTATLootInventoryComponent* GetLootInventoryComponent() const override;

   // from IAISightTargetInterface
   virtual bool CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor = nullptr,
      const bool* wasVisible = nullptr, int32* userData = nullptr) const override;

   // from IOSELightDetectionInterface
   virtual bool CanLightRayHitActor(const FVector& fromLocation, const AActor* actorToIgnore, int& outNumberOfLoSChecksPerformed, float& outMinHitDistance) const;
   virtual float GetCurrentLightIntensityPlusMinimumValue() const override;
   virtual float GetActualLightIntensityFromLightSources() const override;
   
   //---------------------------------------------------------------------------------------
   // IOSEAIVisibilityTargetInterface
   //---------------------------------------------------------------------------------------
   virtual void AuthorityOnEnterVisibleByActor(AActor* viewingActor) override;
   virtual void AuthorityOnExitVisibleByActor(AActor* viewingActor) override;

   // from ITATCombatDamageBonusTargetInterface
   virtual bool CanBeSneakAttackedByActor(AActor* actor) const override;
   virtual void OnSneakAttackedByActor(AActor* actor) override;
   virtual bool CanBeCounterAttackedByActor(AActor* actor) const override;
   virtual void OnCounterAttackedByActor(AActor* actor) override;

   // from ITATCharacterAnimationInterface
   virtual UAnimMontage* GetCharacterMontage(const FGameplayTag& animationTag) const override;

   // from ITATDisguiseTargetInterface
   virtual USkeletalMesh* GetDisguiseTargetMesh(ETATDisguiseMeshType meshType, TArray<UMaterialInterface*>& outOverrideMaterials) const override;
   virtual TSubclassOf<UAnimInstance> GetDisguiseTargetThirdPersonAnimClass() const override;
   virtual TSubclassOf<UAnimInstance> GetDisguiseTargetAnimSetLayer() const override;
   virtual UTATCharacterAnimationMappingAsset* GetDisguiseTargetCharacterAnimationMapping() const override { return _characterAnimationMapping; }
   virtual FTATDisguiseMovementParams GetDisguiseTargetMovementParams() const override;
   virtual ETATDisguiseTargetType GetDisguiseTargetType() const override;

   /// Returns the component that should be activated for coop partners whenever this character becomes disguised (and deactivated when the disguise ends)
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character|TAT")
   USceneComponent* GetDisguisedPlayerCoopIndicatorComponent() const;
   virtual USceneComponent* GetDisguisedPlayerCoopIndicatorComponent_Implementation() const { return nullptr; }

   // Returns true if we are currently seen by another actor
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TAT | AI")
   bool AuthorityIsBeingViewed() const;

   UFUNCTION(BlueprintNativeEvent)
   bool BP_CanBeSneakAttackedByActor(AActor* actor) const;
   bool BP_CanBeSneakAttackedByActor_Implementation(AActor* actor) const;

public:
   /// ITraversalInterface (Wall Climb)
   virtual bool CanWallClimb() const override;

   /// from ITraversalInterface (sprinting)
   virtual bool CanSprint() const override;
   virtual void SprintRequest() override;
   virtual void OnStartSprinting_Implementation() override;
   virtual void OnStopSprinting_Implementation() override;
   virtual bool CanScramble() const override;
   virtual void OnStartScrambling_Implementation(const FHitResult& initialClimbImpact);
   virtual void OnStopScrambling_Implementation(const FHitResult& initialClimbImpact);

   // from ITATItemInventorySystemInterface
   virtual UTATItemInventoryComponent* GetTATItemInventory() const override { return _tatItemInventory; }

   // from ACharacter
   virtual void Crouch(bool bClientSimulation = false) override;
   virtual void UnCrouch(bool bClientSimulation = false) override;
   virtual float PlayAnimMontage(UAnimMontage* animMontage, float inPlayRate, FName startSectionName) override;

   // from IOSETeamInterface
   virtual uint8 GetTeam() const override;

   // Force crouch separately from the input-driven value
   UFUNCTION(BlueprintCallable)
   void ForceCrouch(bool bShouldCrouch);

   // begin ITATStealthScoreInterface
   virtual float GetStealthScore() const override { return _stealthScoreComponent->GetStealthDetectionScore(); };
   virtual UTATStealthScoreComponent* GetStealthScoreComponent() const override { return _stealthScoreComponent; }
   // end ITATStealthScoreInterface
   
   // begin ITATHearingStimSourceReactor
   virtual void HandleReactToOwnStim(const FGameplayTag& stimTag, float loudness) override;
   // end ITATHearingStimSourceReactor

   virtual void OnLyingDownChanged_Implementation(bool isLyingDown) override;
   virtual void OnUnconsciousChanged_Implementation(bool isUnconscious) override;

   virtual void PossessedBy(AController* newController) override;
   virtual void NotifyControllerChanged() override;

   /// weapon
   UFUNCTION(BlueprintPure, Category = "Ability|OSE")
   bool IsWeaponEquipped() const;

   /// recently equipped weapon:
   /// - if we currently have a weapon equipped, this is it
   /// - if we are equipping a spell/ability, this is the weapon we'll switch back to
   UFUNCTION(BlueprintPure)
   TSubclassOf<UToolComponent> GetRecentlyEquippedWeaponClass() const { return _recentlyEquippedWeaponClass; }
   UPROPERTY(BlueprintAssignable)
   FOnRecentlyEquippedWeaponChanged OnRecentlyEquippedWeaponChanged;

   /// recently equipped non-weapon tool:
   /// - if we currently have a non-weapon tool equipped, this is it
   /// - if we have a weapon equipped, this is the one we'll switch back to
   UFUNCTION(BlueprintPure)
   TSubclassOf<UToolComponent> GetRecentlyEquippedToolClass() const { return _recentlyEquippedToolClass; }
   UPROPERTY(BlueprintAssignable)
   FOnRecentlyEquippedItemChanged OnRecentlyEquippedToolChanged;

   // "in combat" is able to be inferred via externally replicated state so this suite of "in combat" functionality can be bound/queried on all player characters
   UFUNCTION(BlueprintPure, Category = "Combat")
   bool GetIsInCombat() const { return _isInCombat; }
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsInCombatChanged, bool, isInCombat);
   UPROPERTY(BlueprintAssignable)
   FOnIsInCombatChanged OnIsInCombatChanged;

   // combat things
   UFUNCTION(BlueprintPure, Category = "Combat")
   UTATCombatComponent* GetTATCombatComponent() const;
   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void AuthorityOnSneakAttack(AActor* defender, const FHitResult& hitResult);
   virtual void AuthorityOnSneakAttack_Implementation(AActor* defender, const FHitResult& hitResult) { }
   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void AuthorityOnFlankAttack(AActor* defender, const FHitResult& hitResult);
   virtual void AuthorityOnFlankAttack_Implementation(AActor* defender, const FHitResult& hitResult) { }
   UFUNCTION(BlueprintNativeEvent, Category = "Combat")
   void AuthorityOnCounterAttack(AActor* defender, const FHitResult& hitResult);
   virtual void AuthorityOnCounterAttack_Implementation(AActor* defender, const FHitResult& hitResult) { }

   // from OSECharacterBase
   virtual void AuthorityOnKnockedOutByOtherCharacter_Implementation(AOSECharacterBase* otherCharacter) override;
   virtual void AuthorityOnKnockedOutByNonCharacterSource_Implementation() override;

   UFUNCTION(BlueprintNativeEvent)
   void AuthorityOnEarlyDisconnect();

   // object highlighting, driven on the local player by UTATHighlightStateMgrComponent
   UFUNCTION(BlueprintPure, Category = "Object Highlight")
   bool IsObjectHightEnabled() const { return _objectHighlightCount > 0; }
   void AddObjectHighlight();
   void RemoveObjectHighlight();
   UPROPERTY(BlueprintAssignable, Category = "Object Highlight")
   FOnObjectHighlightChanged OnObjectHighlightChanged;

   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "Character|TAT")
   const TArray<AActor*>& AuthorityGetViewingActors() const { ensure(HasAuthority()); return _authorityViewingActors; }
   
   UPROPERTY(BlueprintAssignable, Category = "Character|TAT")
   FOnViewingActorsChanged OnViewingActorsChanged;

   UFUNCTION(BlueprintPure, Category = "Character|TAT")
   bool IsBehavingSuspiciously(EAlertnessLevel allyAlertnessLevel) const;

   // Gets the Health Bar Block Percent Values that determine the composition of the health bars.  Validated to add up to 100%!
   // The first member of the array will determine the size of the left-most block of health
   UFUNCTION(BlueprintPure, Category = "Character|TAT|Health")
   const TArray<int32>& GetHealthBarBlocks() const
   {
      return HealthBlockPercentages;
   }

   // from ITATHitReactAnimInterface
   virtual ETATHitReactAnimDirection GetHitReactAnimDirection() const { return _hitReactAnimDirection; }
   virtual void SetHitReactAnimDirection(ETATHitReactAnimDirection direction) { _hitReactAnimDirection = direction; }

   // from IDetectionTargetInterface
   virtual void AuthorityOnDetectionStateChanged(AActor* detector, EActorDetectionState previousDetectionState, EActorDetectionState currentDetectionState) override;
   virtual void AuthorityOnDetectionValueUpdateForState(AActor* detector, EActorDetectionState detectionState, float detectionValue) override;

   // from ITATDisguisableCharacterInterface
   virtual UTATDisguiseComponent* GetDisguiseComponent_Implementation() const override
   {
      return _disguiseComponent;
   }

   virtual void OnStartMantling_Implementation() override;

   // from ITATPrivateSpaceCharacterInterface
   UFUNCTION(BlueprintCallable)
   virtual UTATPrivateSpaceCharacterComponent* GetPrivateSpaceCharacterComponent() override { return _privateSpaceCharacterComponent; }
   virtual bool CanBecomeSuspiciousOrIntruder() const override { return true; }
   virtual bool CanEverBeAllowedInPrivateArea() const override { return false; }
public:
   //---------------------------------------------------------------------------------------
   // IUtilityAIBehaviorTargetInterface
   //---------------------------------------------------------------------------------------

   virtual void AuthorityOnEnterTargetedByBehavior_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) override;
   virtual void AuthorityOnExitTargetedByBehavior_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) override;
   virtual void AuthorityOnEnterTargetedByGoal_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) override;
   virtual void AuthorityOnExitTargetedByGoal_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior) override;

   //---------------------------------------------------------------------------------------
   // IUtilityAITokenOwnerInterface
   //---------------------------------------------------------------------------------------

   virtual UUtilityAITokenOwner* AuthorityGetTokenOwner_Implementation() const override { check(HasAuthority()); return _tokenOwner; }

private:

   UPROPERTY(Transient)
   UUtilityAITokenOwner* _tokenOwner = nullptr;
protected:
   UPROPERTY(EditDefaultsOnly, Category = "AI|Stealth")
   TObjectPtr<UTATStealthScoreComponent> _stealthScoreComponent { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   TArray<FOSEAITokenInfo> DefaultAITokens;

   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   TArray<FOSEAITokenInfo> MaxAITokenDebt;


public:
   //---------------------------------------------------------------------------------------
   // IUtilityAITokenRequesterInterface
   //---------------------------------------------------------------------------------------

   virtual UUtilityAITokenRequester* AuthorityGetTokenRequester() const override { check(HasAuthority()); return _tokenRequester; }
   virtual FVector GetMoveGoalOffset(const AActor* movingActor) const override;
   virtual void GetMoveGoalReachTest(const AActor* movingActor,
                                     const FVector& moveOffset,
                                     FVector& goalOffset,
                                     float& goalRadius,
                                     float& goalHalfHeight) const override;
private:
   UPROPERTY(Transient)
   UUtilityAITokenRequester* _tokenRequester = nullptr;
protected:
   // from OSECharacterBase
   virtual void OnHealthChanged_Implementation(float newValue, float oldValue) override;
   virtual void _OnIsCharacterReadyChanged(bool isReady) override;

   UFUNCTION(BlueprintImplementableEvent, Category = "Game Is Frozen")
   void _OnIsGameFrozenChanged(bool isFrozen);

   float _GetStaminaAttributeValue() const;

   virtual void OnRep_PlayerState() override;

private:

   struct FVisibilityTraceLocationAndImportance
   {
      FVector Location;
      float RelativeImportance;
   };

   using FVisibilityTraceLocationArray = TArray<FVisibilityTraceLocationAndImportance, TInlineAllocator<8>>;

   void _TickIsInCombat(float deltaTime);
   void _TickStaminaRTPC();
   UToolComponent* _GetCurrentTool() const;
   void _SetIsInCombat(bool newIsInCombat);
   void _RestartRecentlyUsedWeaponTimer();

   void _UpdateUnCrouch(bool bClientSimulation);

   // Spawns the KnockedOutGlyphIndicatorType upon a player's death
   void _AuthoritySpawnKOGlyph();

   UFUNCTION()
   void _AuthorityOnPhysicalMaterialChangedWhileClimbing(const FOSEMovementMaterialContext& CurrentContext, const FOSEMovementMaterialContext& PreviousContext);
   
   // Returns surface-specific climb stamina burn override, or default climb stamina burn if no override specified for surface
   float _GetClimbStaminaBurnForSurface(EPhysicalSurface physicalSurface, float effectPeriod) const;

   void _OnLocalDilationChanged(const FOnAttributeChangeData& data);
   void _OnWorldDilationChanged(const FOnAttributeChangeData& data);

   void _OnSuppressInteractionTagChanged(const FGameplayTag tag, int32 newTagCount);

   void _OnHiddenFromViewTagChanged(const FGameplayTag tag, int32 newTagCount);
   void _OnOnlyOverlapCapsuleTagChanged(const FGameplayTag tag, int32 newTagCount);

   void _GetAILOSCheckLocations(FVisibilityTraceLocationArray& locations) const;
   void _DebugDrawAILOSCheckLocations() const;

   void _AuthorityDropAllInventory();
   void _AuthorityRemoveRelevantThiefVisionIndicatorsOnKnockout();
   void _AuthorityPermanentKO();

   void _ForEachToolWithKnockoutHandlerInterface(TFunctionRef<void(UToolComponent*)> callback) const;

   UFUNCTION()
   void _OnGameStateSetEvent(AGameStateBase* gameState);

   UFUNCTION()
   void _OnRep_ViewingActors();
   void _BroadcastViewingActorsChanged();

   UFUNCTION()
   void _OnEquippedToolChanged();

   UFUNCTION()
   void _OnCombatAnimationMontageEnded();

   UFUNCTION(BlueprintCallable)
   void PlayDip(float dipSpeed = 1, float dipStrength = 1);
   
   virtual void _ModifyCameraTransform(float deltaTime, FTransform& transform) override;
   virtual void _ModifyFirstPersonMeshViewTransform(float deltaTime, FTransform& transform) override;

   void _TogglePlayerHasQuestLootOnMap(bool hasQuestLoot) const;

   void _BroadcastAreaInfoChanged();

protected:
   // Downed / Dead

   UPROPERTY(EditDefaultsOnly, Category = "Ability|Unconscious")
   TSubclassOf<UGameplayEffect> DownedTemporaryEffect;

   UPROPERTY(EditDefaultsOnly, Category = "Ability|Unconscious")
   TSubclassOf<UGameplayAbility> FinalKOAbility;

   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT", Meta = (Categories = "Indicator"))
   FGameplayTag KnockedOutGlyphIndicatorType;

   // What's our movement speed reduction when a weapon is held and unstated?
   UPROPERTY(EditDefaultsOnly, Category = "Weapon", DisplayName = "Weapon Equipped Max Speed Multiplier: Walking")
   float WeaponEquippedMaxSpeedMultiplierWalking = 1.0f;
   
   UPROPERTY(EditDefaultsOnly, Category = "Weapon", DisplayName = "Weapon Equipped Max Speed Multiplier: Sprinting")
   float WeaponEquippedMaxSpeedMultiplierSprinting = 1.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Weapon")
   float BlockingMaxSpeedMultiplier = 0.35f;

   // What's our movement speed reduction when carrying a body?
   UPROPERTY(EditDefaultsOnly, Category = "Carrying")
   float CarryingMaxSpeedMultiplier = 0.3f;

   // How long after we've attacked something with our weapon do we put it away?
   UPROPERTY(EditDefaultsOnly, Category = "Weapon")
   float RecentlyUsedWeaponTimeSeconds = 10.0f;

   // How long after we've attacked something with our weapon do we put it away?
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT")
   float ViewedByAggressiveCharacterFalloffTimer = 3.0f;

   // When checking if a guard can see us, what locations should we use for visibility traces?
   UPROPERTY(EditAnywhere, Category = "Character|TAT|AI Sight")
   TArray<FTATCharacterVisibilityTraceLocation> VisibilityTraceLocations;

   // What additional tags will prevent us from sprinting
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT")
   FGameplayTagContainer SprintSuppressionTags;

   // What additional tags will prevent us from wall climbing
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT")
   FGameplayTagContainer WallClimbSuppressionTags;

   UPROPERTY(EditDefaultsOnly, Category = "Animation")
   UTATCharacterAnimationMappingAsset* _characterAnimationMapping = nullptr;

   // Tag which, when applied, will cause the player to show up on the in-game map
   UPROPERTY(EditDefaultsOnly, Category = "UI")
   FGameplayTag ShowOnMapTag;

   // The % of max health each block is composed of.  Validated to add up to 100%!
   // The first member of the array will determine the size of the left-most block of health
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT|Health")
   TArray<int32> HealthBlockPercentages;

   // Inventory

   virtual void InitializeItemInventory(UItemInventoryComponent* itemInventoryComponent) override;

private:
   UPROPERTY(EditDefaultsOnly, Instanced, Category = "Camera")
   TArray<TObjectPtr<UTATFirstPersonViewModifier>> _firstPersonModifiers;
   
   // Gameplay effects applied for the duration of sprinting
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT")
   TSubclassOf<UGameplayEffect> _sprintDurationEffect = nullptr;

   // NOTE: Can be invalid while sprinting (if Status.StaminaConsumptionDisabled tag present on player)
   FActiveGameplayEffectHandle _sprintDurationEffectHandle;

   // Gameplay effect applied for the duration of climbing
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT")
   TSubclassOf<UGameplayEffect> _climbDurationEffect = nullptr;

   // NOTE: Can be invalid while climbing (if Status.StaminaConsumptionDisabled tag present on player)
   FActiveGameplayEffectHandle _climbDurationEffectHandle;

   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT|Traversal|Sprinting")
   float _minimumStaminaToBeginSprinting = 20.f;

   // Tag for set-by-caller magnitude of the climb duration effect
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT|Traversal|Climbing", meta = (EditCondition = "_climbDurationEffect != nullptr", Categories = "SetByCaller"))
   FGameplayTag _climbEffectSetByCallerTag;

   // Stamina burn (per second) to apply for climb duration effect on various physical surface types
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT|Traversal|Climbing", meta = (EditCondition = "_climbDurationEffect != nullptr",ClampMax = "0.0", UIMax = "0.0"))
   TMap<TEnumAsByte<EPhysicalSurface>, FScalableFloat> _climbStaminaBurnSurfaceOverrides;

   // Default stamina burn (per second) to pass into _climbDurationEffect. Used when climbing a surface type unspecified in _climbStaminaBurnSurfaceOverrides
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT|Traversal|Climbing", meta = (EditCondition = "_climbDurationEffect != nullptr", ClampMax = "0.0", UIMax = "0.0"))
   FScalableFloat _defaultClimbStaminaBurnPerSecond = 1.0f;

   // Cue executed when the player attempts to sprint without enough stamina
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT|Traversal|Audio|Sprinting", meta = (Categories = "GameplayCue"))
   FGameplayTag _sprintRequestWithoutStaminaGameplayCue;

   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT|Traversal|Audio")
   UAkRtpc* _staminaRtpc = nullptr;

   /// Tag of abilities to cancel when mantling
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT")
   FGameplayTag _cancelOnMantleAbilityTag;

   // Number of lock tumblers that start unlocked when lockpicking
   // TODO: maybe only implement in subclasses with non-zero/upgradable values?
   UPROPERTY(EditDefaultsOnly, Category = "Lockpick")
   int32 _freeLockpickTumblers;

   // What is our currently equipped weapon?
   UPROPERTY(Transient)
   TSubclassOf<UToolComponent> _recentlyEquippedWeaponClass;
   UPROPERTY(Transient)
   TSubclassOf<UToolComponent> _recentlyEquippedToolClass;

   // how much time left to put away our weapon?
   float _recentlyUsedWeaponTimeRemaining = float(INDEX_NONE);

   // are we in combat?
   bool _isInCombat = false;

   bool _wantsToCrouch;
   bool _forceCrouch;
   int _objectHighlightCount = 0;
   ETATHitReactAnimDirection _hitReactAnimDirection = ETATHitReactAnimDirection::None;


   UPROPERTY(Transient)
   UTATItemInventoryComponent* _tatItemInventory = nullptr;

   UPROPERTY(Transient)
   TArray<AActor*> _authorityViewingActors;

   UPROPERTY(Transient)
   UTATInteractionTargeterComponent* _interactionTargeter;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
   UTATDisguiseComponent* _disguiseComponent = nullptr;

   UPROPERTY(EditDefaultsOnly)
   UOSELightDetectionComponent* _lightDetectionComponent { nullptr };

   UPROPERTY(Replicated)
   uint8 _cachedPlayerStateTeam = IOSETeamInterface::kInvalidTeam;

   // Character-identifying switch used to produce unique voice-driven cues for each character
   UPROPERTY(EditDefaultsOnly, Category="Character|TAT")
   UAkSwitchValue* _characterSwitch = nullptr;

   // From Status.HiddednFromView tag
   bool _isHiddenFromView = false;
   bool _isOnlyOverlapCapsule = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Private Areas", meta = (AllowPrivateAccess="true"))
   UTATPrivateSpaceCharacterComponent* _privateSpaceCharacterComponent = nullptr;

   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATXrayComponent> _xrayComponent = nullptr;

   // Dealing with sprint toggle setting changing
   // TODO: If we refactor sprint into an ability rather than directly bound in OSECharacter, this is likely to change
   void _RefreshSprintToggleSetting();

   UFUNCTION()
   void _OnInputSettingsConfigChanged();

   UFUNCTION()
   void _OnInputHardwareTypeChanged(EOSEInputHardwareType inputHardwareType);

   bool _ShouldShowOnMap() const;

   UFUNCTION()
   void _OnShowOnMapTagChanged(FGameplayTag tag, int32 newTagCount);

   UFUNCTION()
   void _OnCurrentOutfitChanged(ATATPlayerState* ps, const TArray<FTATCharacterLoadoutEntry>& outfitLoadout);

   void _LoadAndApplyCharacterOutfit(const TArray<FTATCharacterLoadoutEntry>& outfitLoadout);
};
