// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATPuddleTypes.h"
#include "Character/TATCharacterLandedOnActorInterface.h"

// ose
#include "Math/OSEMathFunctionLibrary.h"

// ue
#include "GameplayTagContainer.h"
#include "Components/BoxComponent.h"
#include "GameplayEffect.h"
#include "NiagaraComponent.h"

#include "TATPuddleCluster.generated.h"


/// Any other puddle state data. Not replicated, but generated locally on each client.
/// This is effectively a private struct for use by ATATPuddleCluster
USTRUCT()
struct TAT_API FTATPuddleState
{
   GENERATED_BODY()

   UPROPERTY()
   int32 PuddleIndex = INDEX_NONE;

   UPROPERTY()
   TObjectPtr<UBoxComponent> Collision;

   UPROPERTY()
   TObjectPtr<UMaterialInstanceDynamic> Material;

   UPROPERTY()
   TObjectPtr<UDecalComponent> Decal;

   UPROPERTY()
   TObjectPtr<UNiagaraComponent> Particles;

   UPROPERTY()
   TSet<TWeakObjectPtr<AActor>> ActorsOverlappingPuddle;

   /// The client time when the puddle was first spawned
   UPROPERTY()
   float ClientSpawnWorldTime = 0.0f;

   /// The current health value. The value in FTATPuddle is authoritative - this one is interpolated for smooth transitions.
   UPROPERTY()
   float HealthCosmetic = 0.0f;

   /// The health value at the time the puddle (or cluster actor) was removed. Used to lerping the health from its current state to zero.
   UPROPERTY()
   float HealthCosmeticAtRemoveTime = 0.0f;

   /// A random value between 0 and 1 assigned to this puddle at spawn time. Used for adding random variations in the decal material.
   UPROPERTY()
   float RandomValue = 0.0f;

   UPROPERTY()
   bool FiredPuddlePendingRemoveEvent = false;

   /// The client time when the puddle was first marked pending remove
   UPROPERTY()
   float ClientPendingRemoveStartWorldTime = 0.0f;

   /// How long the puddle is expected to be in the pending remove state (to give time to play transition-out effects)
   UPROPERTY()
   float ClientPendingRemoveDuration = 0.0f;

   UPROPERTY()
   ETATPuddleSurfaceAngle SurfaceAngle = ETATPuddleSurfaceAngle::Floor;

   UPROPERTY()
   bool IsOutside = false;

#if TAT_ENABLE_DEV_TOOLS
   bool SelectedInDevTool = false;
#endif

   FORCEINLINE bool IsClientPendingRemove() const { return ClientPendingRemoveStartWorldTime > 0.0f; }
   FORCEINLINE float GetClientRemainingLifeSpan(double clientWorldTimeSeconds) const
   {
      return FMath::Max(0.0f, (ClientPendingRemoveStartWorldTime + ClientPendingRemoveDuration) - clientWorldTimeSeconds);
   }
   FORCEINLINE float GetClientRemainingLifeSpanNormalized(double clientWorldTimeSeconds) const
   {
      return FMath::Clamp(GetClientRemainingLifeSpan(clientWorldTimeSeconds) / FMath::Max(0.01f, ClientPendingRemoveDuration), 0.0f, 1.0f);
   }
};


USTRUCT(BlueprintType)
struct TAT_API FTATPuddleMovementDamageEffect
{
   GENERATED_BODY()

   /// How much puddle damage to apply based on the target's velocity (uses TargetVelocityRange to map velocity to this value)
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   FFloatInterval PuddleDamagePerSecondRange = FFloatInterval(0.0f, 0.0f);

   /// The curve to use when interpolating between the min and max values for PuddleDamagePerSecondRange
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   EOSEInterpMode PuddleDamagePerSecondCurve = EOSEInterpMode::Linear;
};


USTRUCT(BlueprintType)
struct TAT_API FTATPuddleMovementGameplayEffect
{
   GENERATED_BODY()

   /// Gameplay effect that can be applied to targets moving in a puddle
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   TSubclassOf<UGameplayEffect> GameplayEffect;

   /// How frequently we try to apply the effect
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ForceUnits = "s"))
   float ApplicationInterval = 0.0f;

   /// How likely are we to apply the effect based on the target's velocity per second while they're in any puddle in the cluster
   /// (uses TargetVelocityRange to map velocity to this value)
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   FFloatInterval ChanceToApplyPerSecondRange = FFloatInterval(0.0f, 1.0f);

   /// How likely are we to apply the effect based on the target's velocity when they first enter the cluster
   /// (uses TargetVelocityRange to map velocity to this value)
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   FFloatInterval ChanceToApplyOnEnter = FFloatInterval(0.0f, 1.0f);

   /// The curve to use when interpolating between the min and max values for ChanceToApplyPerSecondRange
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   EOSEInterpMode ChanceToApplyCurve = EOSEInterpMode::Linear;
};


USTRUCT(BlueprintType)
struct TAT_API FTATPuddleMovementEffect
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   ETATPuddleTargetFilter TargetFilter = ETATPuddleTargetFilter::AllCharacters;

   /// The velocity range over which to apply movement effects
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (ForceUnits = "cm/s"))
   FFloatInterval TargetVelocityRange = FFloatInterval(0.0f, 500.0f);

   /// Damage puddles that actors are moving around in (based on velocity)
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   FTATPuddleMovementDamageEffect PuddleDamage;

   /// Random chance to apply a gameplay effect (based on velocity)
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   FTATPuddleMovementGameplayEffect GameplayEffect;

   /// If the target is moving slower than the minimum velocity, never apply the effect (otherwise use RandomChanceToApplyEffect.Min)
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   bool DisableIfVelocityLessThanMinimum = true;

   FORCEINLINE bool IsPuddleDamageEnabled() const { return PuddleDamage.PuddleDamagePerSecondRange != FFloatInterval(0.0f, 0.0f); }
   FORCEINLINE bool IsGameplayEffectEnabled() const { return GameplayEffect.GameplayEffect && GameplayEffect.ChanceToApplyPerSecondRange != FFloatInterval(0.0f, 0.0f); }

   float GetPuddleDamagePerSecond(const FVector& velocity) const;
   float GetChancePerSecondToApplyGameplayEffect(const FVector& velocity) const;
   float GetChanceOnEnterToApplyGameplayEffect(const FVector& velocity) const;
};


UCLASS(BlueprintType)
class TAT_API ATATPuddleCluster
   : public AActor
   , public ITATCharacterLandedOnActorInterface
{
   GENERATED_BODY()

public:
   ATATPuddleCluster();

   // From AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void OnConstruction(const FTransform& transform) override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void Tick(float deltaSeconds) override;

   // From ITATCharacterLandedOnActorInterface
   virtual bool WantsToHandleCharacterLandedEvents(ATATCharacterBase* character) const override;
   virtual void OnCharacterLandedOnThisActor(ATATCharacterBase* character, const FHitResult& landedHit, UPrimitiveComponent* overlappedComponent, bool& outInterceptLandedEvent) override;

   /// The maximum health value of each puddle
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay", Meta = (UIMin = "1.0", ClampMin = "0.00001"))
   float PuddleMaxHealth = 100.0f;

   /// How many times per second to check for puddle health changes (eg. from weather, or from actors moving around in a puddle)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay", Meta = (UIMin = "1.0", UIMax = "30.0"))
   float PuddleHealthUpdatesPerSecond = 4.0f;

   /// Effects to apply when actors (eg. characters) are moving around in a puddle
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay")
   TArray<FTATPuddleMovementEffect> PuddleActorMovementEffects;

   /// How much to reduce puddle health per second when they are outside, for each weather type
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay", Meta = (Categories = "Weather.Type"))
   TMap<FGameplayTag, float> OutsidePuddleHealthLossPerSecondFromWeather;

   /// How much to reduce puddle health per second based on the puddle's rotation
   UPROPERTY(EditDefaultsOnly, Category = "Puddle Cluster - Gameplay", Meta = (ArraySizeEnum = "/Script/TAT.ETATPuddleSurfaceAngle"))
   float PuddleHealthLossPerSecondFromAngle[static_cast<int32>(ETATPuddleSurfaceAngle::MAX)] = { 0.0f };

   /// Gameplay effects to apply to characters in any puddle in the cluster.
   /// They can optionally be auto-removed when the character is no longer in any puddle in the cluster.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay", Meta = (TitleProperty = "GameplayEffect"))
   TArray<FTATPuddleGameplayEffect> GameplayEffects;

   /// How long should a puddle stick around after being removed?
   /// This gives it time to play transition-out effects.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay", Meta = (UIMin = 0, ClampMin = 0, ForceUnits = "s"))
   float PuddleLifetimeAfterRemove = 1.0f;

   /// If non-zero, this actor will be destroyed this number of seconds after its last puddle was removed.
   /// This is mostly intended to allow VFX to complete before the actor goes away.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay", Meta = (UIMin = 0, ClampMin = 0, ForceUnits = "s"))
   float PuddleClusterLifetimeAfterLastPuddleRemoved = 2.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay|Collision")
   FCollisionProfileName PuddleCollisionProfile;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay|Collision")
   TEnumAsByte<ECollisionChannel> PuddleCollisionChannel = ECC_WorldDynamic;

   /// Allows puddles to provide custom logic for when characters land on them.
   /// This enables the OnCharacterLandedOnPuddle blueprint event, and allows suppressing fall damage (if enabled separately)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay|Collision")
   bool EnableCharacterLandedOnPuddleEvents = false;

   /// Prevent all fall damage when a character lands on an active puddle.
   /// Note that you can also provide custom behavior in the OnCharacterLandedOnPuddle event.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puddle Cluster - Gameplay|Collision", Meta = (EditCondition = "EnableCharacterLandedOnPuddleEvents"))
   bool PreventCharacterFallDamageWhenLandingOnPuddle = false;

   /// The base puddle color, passed to the material as the decal color as well as the Niagara particle system.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals")
   FLinearColor PuddleColor = FLinearColor::White;

   /// Base material for each puddle's decal component
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   TObjectPtr<UMaterialInterface> PuddleDecalMaterial;

   /// Added to the puddle's extent on the decal component.
   /// Allows the decal to be slightly larger or smaller than the overlap area for the puddle.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   FVector PuddleDecalExtentMargin = FVector::ZeroVector;

   /// The parameter name for each puddle's "health" value.
   /// The value is passed to the material as a normalized value between 0 and 1, where 1 represents a puddle at full health.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   FName PuddleDecalMaterial_HealthParamName = NAME_None;

   /// A material param that is set to 1.0 if the puddle is taking weather-based damage or 0.0 if it is not (eg. the puddle is outside and it's raining)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   FName PuddleDecalMaterial_TakingWeatherDamageParamName = NAME_None;

   /// A material param that is set to 1.0 if the puddle is taking angle-based damage or 0.0 if it is not (eg. the puddle is on a wall, show the slime dripping down)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   FName PuddleDecalMaterial_TakingAngleDamageParamName = NAME_None;

   /// A material param that will be set to the game time that a puddle was spawned
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   FName PuddleDecalMaterial_SpawnTimeParamName = NAME_None;

   /// The parameter name used for a random value between 0 and 1 assigned to each puddle.
   /// The value will be constant over the lifetime of the puddle.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   FName PuddleDecalMaterial_RandomValueParamName = NAME_None;

   /// How quickly puddle health changes are applied to the decal material
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Decal")
   float PuddleHealthInterpSpeed = 50.0f;

   /// Niagara particle system to spawn with each puddle. Note that a NiagaraComponent will only be spawned and attached if this asset is valid.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Particles")
   TObjectPtr<UNiagaraSystem> PuddleParticleSystem;

   /// Relative rotation to set on the Niagara component when spawned
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Particles")
   FRotator PuddleParticlesRelativeRotation = FRotator::ZeroRotator;

   /// The particle system parameter name for the puddle's color.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Particles")
   FName PuddleParticleSystem_ColorParamName = NAME_None;

   /// The parameter name for each puddle's "health" value.
   /// The value is passed to the particle system as a normalized value between 0 and 1, where 1 represents a puddle at full health.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puddle Cluster - Visuals|Particles")
   FName PuddleParticleSystem_HealthParamName = NAME_None;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Puddle Cluster")
   int32 AuthorityAddPuddle(const FTATPuddleTransform& puddleTransform);

   /// Resets a puddle's health to maximum. Only works if the puddle has not yet reached zero health. Returns true if successful.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Puddle Cluster")
   bool AuthorityResetPuddleHealthToMax(int32 puddleId);

   /// Looks for a puddle with health greater than minHealthToAllowRefresh.
   /// If it finds one, refresh its health to maximum. Otherwise, spawn a new puddle.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Puddle Cluster")
   int32 AuthorityAddOrRefreshPuddle(bool& outIsNewPuddle, const FTATPuddleTransform& puddleTransform, float maxRefreshDistance = 0.0f, float minHealthToAllowRefresh = 1.0f);

private:
   void _AuthorityRemovePuddlesInternal(TConstArrayView<int32> puddleIds);

public:
   /// Instantly removes a puddle.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Puddle Cluster")
   bool AuthorityRemovePuddle(int32 puddleId);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Puddle Cluster")
   void AuthorityApplyDamageToPuddle(int32 puddleId, float damageAmount);

   /// Called just before a puddle is spawned. This is a good place to put spawn VFX/SFX.
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddlePreAdded(const FTATPuddle& puddle);

   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddleAdded(const FTATPuddle& puddle);

   /// Called just after a puddle's health drops to zero, but a few seconds before it's fully removed.
   /// This is a good place to put despawn VFX/SFX.
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddlePendingRemove(const FTATPuddle& puddle, float puddleLifespanRemaining);

   /// Called just before a puddle is removed
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddleRemoved(const FTATPuddle& puddle);

   /// Called when any actor starts overlapping a single puddle
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddleBeginOverlap(int32 puddleId, AActor* actor);

   /// Called when any actor stops overlapping a single puddle
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddleEndOverlap(int32 puddleId, AActor* actor);

   /// Called when a character enters any puddle in this cluster
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddleClusterCharacterEnter(ACharacter* character);

   /// Called when a character is no longer in any puddle in this cluster
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddleClusterCharacterLeave(ACharacter* character);

   /// Called just before this actor is auto-destroyed. Useful for handling transition-out VFX/SFX.
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnPuddleClusterPendingDestroy();

   /// Called when a character goes from a falling state to a walking state on a puddle.
   /// Important: This event will only be fired if the EnableCharacterLandedOnPuddleEvents property is enabled.
   UFUNCTION(BlueprintNativeEvent, Category = "Puddle Cluster")
   void OnCharacterLandedOnPuddle(int32 puddleId, ACharacter* character);

   UFUNCTION(BlueprintPure, Category = "Puddle Cluster")
   bool GetPuddle(int32 puddleId, FTATPuddle& puddleData) const;

   UFUNCTION(BlueprintPure, Category = "Puddle Cluster")
   bool GetPuddleComponents(int32 puddleId, UBoxComponent*& boxComponent, UMaterialInstanceDynamic*& decalMaterial, UDecalComponent*& decalComponent, UNiagaraComponent*& niagaraComponent) const;

   UFUNCTION(BlueprintPure, Category = "Puddle Cluster")
   FORCEINLINE bool IsValidPuddleId(int32 puddleId) const { return puddleId != INDEX_NONE && _puddleStates.Contains(puddleId); }

   UFUNCTION(BlueprintPure, Category = "Puddle Cluster")
   FORCEINLINE int32 NumPuddles() const { return _puddles.Num(); }

   /// Check if new puddles can be added to this cluster (if not, this actor is probably about to be destroyed)
   FORCEINLINE bool IsAvailableForNewPuddles() const { return !_puddleClusterPendingDestroy && GetLifeSpan() <= 0; }

   FVector GetPuddleClusterCentroid() const;

   bool IsSphereOverlappingPuddleCluster(const FVector& origin, float radius) const;
   FORCEINLINE bool IsSphereOverlappingPuddleCluster(const FSphere& sphere) const { return IsSphereOverlappingPuddleCluster(sphere.Center, sphere.W); }

   /// Called by UTATPuddleSubsystem when the puddle dev tool is open
   void DrawPuddleClusterDevTool();

   UFUNCTION()
   void _AuthorityUpdatePuddles();

   struct FPuddlePair
   {
      FTATPuddle* Puddle = nullptr;
      FTATPuddleState* State = nullptr;
      explicit operator bool() const { return Puddle != nullptr && State != nullptr; }
   };
   FPuddlePair _GetPuddleAndStateById(int32 puddleId) const;

   FTATPuddleState _CreatePuddleState(const FTATPuddle& puddle, int32 puddleIndex);
   void _UpdatePuddleState(FTATPuddleState& puddleState, const FTATPuddle& puddle, bool fromTick = false, float tickDeltaSeconds = 0.0f);
   void _MarkPuddlePendingRemove(FTATPuddleState& puddleState, const FTATPuddle& puddle, TOptional<float> overrideRemainingLifeSpan = NullOpt);
   void _DestroyPuddleState(FTATPuddleState& puddleState, int32 puddleId);

   void _SyncPuddleStates();

   void _TryAddCharacterToCluster(ACharacter* character);
   void _TryRemoveCharacterFromCluster(ACharacter* character, bool force = false);

   void _AuthorityOnPuddleClusterEnter(ACharacter* character);
   void _AuthorityOnPuddleClusterLeave(ACharacter* character);

   TOptional<float> _AuthorityGetMovementGameplayEffectApplicationDeltaSeconds(int32 effectIdx);

   void _AuthorityTryApplyMovementGameplayEffect(ACharacter* character, TOptional<float> deltaSeconds = NullOpt, const FTATPuddleMovementEffect* movementEffect = nullptr);

   bool _IsActorOverlappingAnyPuddle(AActor* character, int32 ignorePuddleId = INDEX_NONE) const;

   UFUNCTION()
   void _OnPuddleComponentBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex,
      bool fromSweep, const FHitResult& sweepResult);

   UFUNCTION()
   void _OnPuddleComponentEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

   UFUNCTION()
   void _OnRep_PuddleClusterPendingDestroy();

   UFUNCTION()
   void _OnRep_Puddles(const TArray<FTATPuddle>& oldPuddles);

   /// How much to reduce puddle health per second for outdoor puddles
   UPROPERTY(Transient)
   float _outsidePuddleDamagePerSecond = 0.0f;

   /// The number of puddles taking damage per second based on state that will not change dynamically.
   /// Eg. a puddle outside in the rain (if configured) or a puddle on a ceiling (if configured)
   int32 _numPuddlesTakingStateBasedDamagePerSecond = 0;

   /// Number of puddles that have reached zero health and will be destroyed soon
   int32 _numPuddlesPendingRemove = 0;

   /// Next puddle id to assign when creating a new puddle
   int32 _authorityNextPuddleId = 1;

   UPROPERTY(Transient, Replicated, ReplicatedUsing = "_OnRep_PuddleClusterPendingDestroy")
   bool _puddleClusterPendingDestroy = false;

   UPROPERTY(Transient, Replicated, ReplicatedUsing = "_OnRep_Puddles")
   TArray<FTATPuddle> _puddles;

   /// Puddle id to puddle state data
   UPROPERTY(Transient)
   TMap<int32, FTATPuddleState> _puddleStates;

   /// Mapping of puddle-related components to puddle ids. Intended for fast lookups from overlap events.
   TMap<TWeakObjectPtr<UPrimitiveComponent>, int32> _componentToPuddleIdMap;

   TMap<TPair<TWeakObjectPtr<ACharacter>, int32>, FActiveGameplayEffectHandle> _activeGameplayEffects;

   UPROPERTY(Transient)
   TSet<TObjectPtr<ACharacter>> _charactersInCluster;

   TMap<int32, double> _authorityLastMovementEffectApplicationWorldTime;

   double _authorityLastPuddleDamageUpdateWorldTime = 0;
};
