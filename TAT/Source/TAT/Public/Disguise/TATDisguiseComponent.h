// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Items/Disguise/TATDisguiseTool.h"
#include "Disguise/TATDisguiseDegradationData.h"
#include "Disguise/TATDisguiseTargetInterface.h"
#include "Developer/TATDevToolInterface.h"
#include "Player/TATCharacter.h"

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"

// ue5
#include "Engine/DataTable.h"
#include "GameplayEffectTypes.h"

#include "TATDisguiseComponent.generated.h"

struct FGameplayEventData;
class UGameplayEffect;
class UTATCharacterAnimationMappingAsset;
class UTATToolComponent;
class UTATDisguiseToolComponent;
class UNiagaraComponent;

USTRUCT(BlueprintType)
struct TAT_API FTATDisguiseMeshData
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Data")
   TObjectPtr<USkeletalMesh> Mesh;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Data")
   TArray<UMaterialInterface*> Materials;

   bool IsValid() const { return Mesh != nullptr; }

   void Reset()
   {
      Mesh = nullptr;
      Materials.Reset();
   }
};

USTRUCT(BlueprintType)
struct TAT_API FTATDisguiseMeshParams
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   FTATDisguiseMeshData SkeletonMesh;
   
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   FTATDisguiseMeshData ThirdPersonBody;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   FTATDisguiseMeshData ThirdPersonHead;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   FTATDisguiseMeshData FirstPersonLowerBody;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   FTATDisguiseMeshData FirstPersonUpperBody;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   TSubclassOf<UAnimInstance> AnimInstanceClass;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   TSubclassOf<UAnimInstance> AnimLayer;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Mesh Params")
   bool IsDisguise = false;

   void Reset()
   {
      SkeletonMesh.Reset();
      ThirdPersonBody.Reset();
      ThirdPersonHead.Reset();
      FirstPersonLowerBody.Reset();
      FirstPersonUpperBody.Reset();
      AnimInstanceClass = nullptr;
      AnimLayer = nullptr;
      IsDisguise = false;
   }

   bool IsValid() const { return SkeletonMesh.IsValid(); }

   void CopyFrom(ACharacter* character, ACharacter* fallbackCharacter, bool isDisguise);
   void Apply(ACharacter* character) const;
   void Unapply(ACharacter* character) const;
};

USTRUCT(BlueprintType)
struct FTATDisguiseState
{
   GENERATED_BODY()
public:
   
   UPROPERTY(Transient)
   FDisguiseSnapshot CurrentDisguiseData;

   UPROPERTY(Transient)
   bool IsDisguiseActive = false;

   UPROPERTY(Transient)
   uint8 OriginalTeam = 0;
   
   UPROPERTY(Transient)
   float ServerStartTimeSeconds = 0.0f;

   UPROPERTY(Transient)
   float MaxDurationSeconds = 0.0f;
};

/// Stores local player VFX state for the active disguise
/// This is effectively a private struct for UTATDisguiseComponent
USTRUCT()
struct FTATDisguiseLocalClientActiveEffectContainer
{
   GENERATED_BODY()
   
   UPROPERTY()
   TObjectPtr<UNiagaraComponent> RadiusEffectParticles;
   
   // Any other local client VFX state can be added here when needed
   
   /// Destroy all effects in this container (eg. because the disguise is no longer active)
   /// NB. If you add any new effect to this struct, make sure to destroy them in this method!
   void DestroyAllEffects();
};

struct FTATDisguiseReductionEventDebugInfo
{
#if TAT_ENABLE_DEV_TOOLS
   FString Source;
   FString Desc;
   float Value = 0.0f;
   bool IsPerFrameEvent = false;
   double RemoveAtWorldTime = 0.0;
#endif
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class UTATDisguiseComponent : public UActorComponent, public ITATDevToolInterface
{
   GENERATED_BODY()

public:
   UTATDisguiseComponent();

   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // From ITATDevToolInterface
   virtual void DrawDevToolObjectEditor(float deltaSeconds) override;

   /// Activate the given disguise with max integrity
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TAT|Disguise")
   bool AuthorityActivateDisguise(const FDisguiseSnapshot& disguiseSnapshot, AActor* caster);

   void AuthorityReduceDisguiseIntegrity(float integrityReduction);
   void AuthoritySetDisguiseIntegrity(float integrity);
   
   UFUNCTION(BlueprintPure, Category = "TAT|Disguise")
   bool IsDisguiseActive() const;

   UFUNCTION(BlueprintPure, Category = "TAT|Disguise")
   ETATDisguiseTargetType GetDisguiseTargetType() const;

   UFUNCTION(BlueprintPure, Category = "TAT|Disguise")
   const FDisguiseSnapshot& GetDisguiseSnapshot() const;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TAT|Disguise")
   void AuthorityEndDisguise();

   float GetRemainingDisguiseIntegrity() const;

   /// Get the remaining integrity of the disguise, scaled to the interval (0, 1)
   /// where 1 is the supplied starting integrity from when the disguise was created
   UFUNCTION(BlueprintPure, Category = "TAT|Disguise")
   float GetNormalizedRemainingDisguiseIntegrity() const;

   UFUNCTION(BlueprintPure, Category = "TAT|Disguise")
   uint8 GetTeamForDisguise() const;

   /// Allows the disguise to override the montage that a character plays
   UAnimMontage* GetDisguisedCharacterMontage(const FGameplayTag& animationTag) const;

   /// When disguised, tool components will call this so we can override how tool anim class layers are linked
   bool HandleToolLinkAnimClassLayers(UTATToolComponent* toolComponent, const TSubclassOf<UAnimInstance>& animClassLayer);

   /// When disguised, tool components will call this so we can override how tool anim class layers are unlinked
   bool HandleToolUnlinkAnimClassLayers(UTATToolComponent* toolComponent, const TSubclassOf<UAnimInstance>& animClassLayer);

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> IsDisguisedEffect;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> IsSelfDisguisedEffect;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> IsGrantingDisguiseEffect;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> EffectAfterDisguiseOver;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> EffectOnObserversAfterDisguiseOver;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisguiseBegin);
   UPROPERTY(BlueprintAssignable)
   FOnDisguiseBegin OnDisguiseBegin;

   DECLARE_MULTICAST_DELEGATE(FOnDisguiseEvent);

   FOnDisguiseEvent OnDisguiseNativeBegin;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisguiseEnd);
   UPROPERTY(BlueprintAssignable)
   FOnDisguiseEnd OnDisguiseEnd;

   FOnDisguiseEvent OnDisguiseNativeEnd;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDisguiseIntegrityChange, float, newIntegrity, float, newNormalizedIntegrity);
   UPROPERTY(BlueprintAssignable)
   FOnDisguiseIntegrityChange OnDisguiseIntegrityChange;
   
private:
   UPROPERTY(Transient)
   TObjectPtr<USceneComponent> _disguisedPlayerCoopIndicatorComponent;

   /// Checks if we are a local client and this disguise component is owned by a remote player who is allied with us
   bool _IsCoopPartner() const;

   void _CoopPartnerTickComponent(float deltaTime);

private:
   bool _IsLocalClient() const;
   
   /// Configures disguise VFX based on the disguise tool config. Only runs on the local client.
   void _LocalClientInit();
   
   /// End play handler for the local client. Useful for disguise visualizations.
   void _LocalClientEndPlay();
   
   /// Tick function that only runs on the local client. Useful for disguise visualizations
   void _LocalClientTickComponent(float deltaTime);
   
   /// Called when disguise is deactivated to remove all VFX spawned during disguise
   void _RemoveAllLocalClientEffects();
   
   bool _hasRunLocalClientInit = false;
   
   struct FDisguiseVFXInfo_Character
   {
      ETATTeamCharacterType CharacterType = ETATTeamCharacterType::Guard;
      float Radius = 0.0f;
      TOptional<float> ViewConeAngleDeg;
   };
   TArray<FDisguiseVFXInfo_Character> _characterProximityVFXInfos;

   UPROPERTY(Transient)
   TArray<TObjectPtr<ACharacter>> _localPlayerNearbyCharacters;

   UPROPERTY(Transient)
   TMap<TObjectPtr<ACharacter>, FTATDisguiseLocalClientActiveEffectContainer> _nearbyCharacterEffects;
   
private:
   struct FDisguiseEventInfo
   {
      FTATDisguiseIntegrityResult Result;
      FFloatRange StealthScoreRange;
      ETATDisguiseVisibilityRequirement VisibilityRequirement = ETATDisguiseVisibilityRequirement::Always;
   };
   
   bool _ShouldHandleNearbyCharacter(ACharacter* nearbyCharacter, ETATTeamCharacterType matchCharacterType) const;

   UTATDisguiseToolComponent* _GetDisguiseToolComponent() const;

   void _OnDamageChanged(const FOnAttributeChangeData& onAttributeChangeData);

   void _OnDisguiseBegin();
   void _OnDisguiseBroken();
   void _OnReductionGameplayTagNewOrRemoved(FGameplayTag tag, int32 newCount);
   void _OnReductionGameplayEvent(const FGameplayEventData* payload);
   void _OnReductionAttributeChange(const FOnAttributeChangeData& data);

   /// The same as _OnReductionEventOccur, but operates on a float instead of modifying state.
   /// Useful for doing a series of integrity changes in sequence and only modifying state once at the end.
   void _HandleIntegrityReductionEventInternal(
      FTATDisguiseReductionEventDebugInfo&& debugInfo,
      float& inOutIntegrityValue,
      const FTATDisguiseIntegrityResult& integrityResult,
      const FDisguiseEventInfo& eventInfo,
      TArrayView<const AActor*> relatedActors = {}) const;

   void _OnReductionEventOccur(
      FTATDisguiseReductionEventDebugInfo&& debugInfo,
      const FTATDisguiseIntegrityResult& integrityResult,
      const FDisguiseEventInfo& eventInfo,
      TArrayView<const AActor*> relatedActors = {});

   bool _AuthorityMeetsStealthScoreRequirements(const FFloatRange& stealthScoreRange) const;
   bool _AuthorityMeetsVisibilityRequirements(ETATDisguiseVisibilityRequirement visRequirement, TArrayView<const AActor*> relatedActors = {}) const;

   void _AuthorityApplyDisguiseEffects(AActor* caster);
   void _AuthorityRemoveDisguiseEffects();

   void _OnAbilityActivated(UGameplayAbility* gameplayAbility);
   void _AuthorityBindToGameplayEvents();
   void _AuthorityUnbindToGameplayEvents();

   UPROPERTY(Transient)
   TObjectPtr<ATATCharacter> _character { nullptr };

   void _SetDisguise(bool isDisguiseVisible);

   bool _ShouldDisguiseBeVisible() const;
   void _RecomputeDisguiseVisibility();

   UFUNCTION()
   void _OnRep_CurrentDisguiseState(const FTATDisguiseState& oldState);

   UFUNCTION()
   void _OnRep_CurrentDisguiseIntegrity();

   void _UpdateNearbyCharacters(TArray<TObjectPtr<ACharacter>>& outCharacters, float searchRadius, bool requireConscious = true) const;

   UPROPERTY(Transient)
   TArray<TObjectPtr<ACharacter>> _authorityNearbyCharacters;

   UPROPERTY(Transient)
   UTATCharacterAnimationMappingAsset* _disguisedCharacterAnimationMapping = nullptr;

   TWeakObjectPtr<UTATToolComponent> _lastAnimLinkedToolComponent;
   TSubclassOf<UAnimInstance> _lastAnimLinkedToolComponentAnimClassLayer;

   UPROPERTY(Transient)
   FTATDisguiseMeshParams _origMeshParams;
   UPROPERTY(Transient)
   FTATDisguiseMeshParams _disguisedMeshParams;

   UPROPERTY(Transient, ReplicatedUsing= _OnRep_CurrentDisguiseState)
   FTATDisguiseState _currentDisguiseState;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_CurrentDisguiseIntegrity)
   float _currentDisguiseIntegrity = 0.0f;

   /// This is owned by the disguise tool component, but we cache it here at BeginPlay for fast access
   float _maxDisguiseIntegrity = 100.0f;

   bool _authorityHasBoundToGameplayEvents = false;

   bool _isDisguiseCurrentlyVisible = false;
   
   /// "Events" that always apply over time (assuming general requirements are met)
   TArray<FDisguiseEventInfo> _alwaysReduceOverTimeEvents;

   FDisguiseEventInfo _MakeDisguiseEventInfo(const FTATDisguiseIntegrityReductionDataRow& dataRow) const;
   struct FDisguiseEventInfo_DelegateHandle
   {
      FDisguiseEventInfo EventInfo;
      FDelegateHandle Handle;
   };

   struct FDisguiseEventInfo_GameplayTag
   {
      FDisguiseEventInfo EventInfo;
      FGameplayTag Tag;
   };

   float _EvalUpgradeCurve(const FScalableFloat& upgradeCurve, FGameplayTag upgradeTag) const;

   /// Tags that cause integrity to reduce over time
   TArray<FDisguiseEventInfo_GameplayTag> _gameplayTagReduceOverTimeEventInfos;

   struct FDisguiseGameplayTagChangeInfo
   {
      FDisguiseEventInfo EventInfo;
      FDelegateHandle Handle;
      bool TriggerOnTagAdded = true;
   };
   /// Tags that when added or removed cause integrity changes
   TMap<FGameplayTag, FDisguiseGameplayTagChangeInfo> _gameplayTagChangeEventInfos;

   /// Gameplay events that cause integrity changes
   TMap<FGameplayTag, FDisguiseEventInfo_DelegateHandle> _gameplayEventInfos;

   struct FDisguiseEventInfo_Character
   {
      ETATTeamCharacterType CharacterType = ETATTeamCharacterType::Guard;
      FDisguiseEventInfo EventInfo;
      float Radius = 0.0f;
      TOptional<float> ViewConeAngleDegrees;
      bool UseAISightToLimitMaxViewConeDistance = false;
   };
   TArray<FDisguiseEventInfo_Character> _characterProximityEventInfos;
   TArray<FDisguiseEventInfo_Character> _characterViewConeEventInfos;

   struct FDisguiseAttributeChangeInfo
   {
      FDisguiseEventInfo EventInfo;
      FDelegateHandle Handle;
      float AttributeDeltaMultiplier = 0.0f;
      float MaxExtraReductionFromAttributeDelta = 0.0f;
   };
   /// Gameplay attributes that cause integrity changes when changed
   TMap<FGameplayAttribute, FDisguiseAttributeChangeInfo> _gameplayAttributeDecreaseEventInfos;

   /// Abilities with ability tags that match a tag query that cause integrity changes
   struct FDisguiseAbilityInfo
   {
      FDisguiseEventInfo EventInfo;
      FTATDisguiseIntegrityEventConfig_Ability Config;
   };
   TArray<FDisguiseAbilityInfo> _abilityEventInfos;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery _damageQueryToBreakDisguise;
   // UPROPERTY(EditDefaultsOnly)
   // float _stealthScoreThreshold { 0.5f };
   UPROPERTY(EditDefaultsOnly)
   float _disguiseMaxDuration { 30.f };
   // UPROPERTY(EditDefaultsOnly)
   // float _disguiseIntegrityDegradeMultiplierAboveThreshold { 1.f };
   // UPROPERTY(EditDefaultsOnly)
   // float _disguiseIntegrityDegradeMultiplierBelowThreshold { 2.f };
   
   /// Event handle for ability events
   FDelegateHandle _abilityActivatedDelegate;

   /// Tag to check in an abilities ActivationBlockedTags to determine if it's considered an "activated" ability or not.
   FGameplayTag _disableAbilitiesTag;

   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _appliedDisguiseEffects;

   FActiveGameplayEffectHandle _disguiseEffectHandle;

   TWeakObjectPtr<AActor> _authorityDisguiseCaster;

#if TAT_ENABLE_DEV_TOOLS
   struct FDisguiseDevToolUI
   {
      bool DebugDraw = false;
      bool ShowStealthScore = false;
      bool ShowAllDataTableRows = false;
      TArray<FTATDisguiseReductionEventDebugInfo> IntegrityReductionEvents;
   };
   FDisguiseDevToolUI _devToolState;
#endif
};


