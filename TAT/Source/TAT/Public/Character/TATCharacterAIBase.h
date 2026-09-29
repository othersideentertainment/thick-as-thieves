// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "Animation/TATCharacterAnimationInterface.h"
#include "Animation/TATHitReactAnimInterface.h"
#include "AI/LivingWorld/TATLivingWorldAgentInterface.h"
#include "Character/TATTeams.h"
#include "Character/TATCharacterBase.h"
#include "Combat/TATCombatDamageBonusTargetInterface.h"
#include "Items/TATInventoryTypes.h"
#include "Items/TATItemInventorySystemInterface.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"
#include "Loot/TATLootInterface.h"
#include "AI/Escalation/TATEscalationState.h"
#include "Disguise/TATDisguiseTargetInterface.h"
#include "AI/Patrol/TATAIPatrolAccessorInterface.h"
#include "AI/TATAsyncRequestComponent.h"

// ose
#include "OSEIndividualAttitudeInterface.h"
#include "OSEIndividualKnowledgeInterface.h"
#include "OSEVoiceLineKnowledgeInterface.h"
#include "AI/Perception/OSEAISightInterface.h"
#include "AI/Perception/OSEStimDatabaseInterface.h"
#include "AI/Perception/OSEAIVisibilityTargetInterface.h"
#include "Detection/OSEDetectionComponentInterface.h"
#include "AI/Alertness/OSEAlertnessInterface.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "AI/Perception/OSEAITargetSightInterface.h"

// UE
#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "TATCharacterAIBase.generated.h"

struct FUtilityStateEvaluatorInstance;
struct FUtilityStateTarget;
class UTATAlertnessComponent;
class UTATPrivateSpaceCharacterComponent;
DECLARE_LOG_CATEGORY_EXTERN(LogTATCharacterAIBase, Log, All);

class UOSEStimDatabase;
class UAttributeSet;
class UTATAlertnessComponent;
class UTATAttributeSet;
class UTATItemInfo;
class UTATItemInventoryComponent;
class UTATKnowledgeComponent;
class UTATNPCClueComponent;
class UTATNPCClueSpawnerComponent;
class UTATPickpocketableComponent;
class UMotionWarpingComponent;
class UTATCharacterAnimationMappingAsset;
struct FTATInventorySlot;

UENUM(BlueprintType)
enum class ETATCharacterUnconsciousState : uint8
{
   // The character is not currently unconscious
   Conscious,

   // The character is currently unconscious, with a set expiration
   DurationUnconscious,

   // The character is currently unconscious, with no set expiration
   IndefiniteUnconscious
};

USTRUCT(BlueprintType)
struct TAT_API FTATCharacterUnconsciousEffectInfo
{
   GENERATED_BODY()
public:

   UPROPERTY(BlueprintReadOnly)
   ETATCharacterUnconsciousState UnconsciousState = ETATCharacterUnconsciousState::Conscious;

   /// For duration-based unconscious, what is the time the server expects the unconscious effect to end
   UPROPERTY(BlueprintReadOnly)
   float ExpectedServerEndTime = 0.0f;

   /// What is the total duration of the unconscious effect that will last longest
   UPROPERTY(BlueprintReadOnly)
   float TotalEffectDuration = 0.0f;
};

USTRUCT()
struct TAT_API FTATCharacterAIIdleAnimations
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   TArray<TSoftObjectPtr<UAnimMontage>> Animations;
};

UENUM(BlueprintType)
enum class ETATCharacterAISearchAnimType : uint8
{
   SameLevel = 0,
   Above = 1,
   Below = 2
};

USTRUCT(BlueprintType)
struct TAT_API FTATCharacterAISearchAnimations
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   TArray<TSoftObjectPtr<UAnimMontage>> Animations;
};

USTRUCT()
struct TAT_API FTATCharacterAISearchAnimationSet
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   TMap<ETATCharacterAISearchAnimType, FTATCharacterAISearchAnimations> AnimationMap;
};


UCLASS()
class TAT_API ATATCharacterAIBase
   : public ATATCharacterBase
   , public ITATItemInventorySystemInterface
   , public ITATLootInventoryInterface
   , public ITATCombatDamageBonusTargetInterface
   , public ITATHitReactAnimInterface
   , public IOSEStimDatabaseInterface
   , public IOSEAISightInterface
   , public IOSEAIVisibilityTargetInterface
   , public ITATCharacterAnimationInterface
   , public ITATDisguiseTargetInterface
   , public ITATLivingWorldAgentInterface
   , public ITATPrivateSpaceCharacterInterface
   , public IOSEDetectionComponentInterface
   , public IOSEAlertnessInterface
   , public IOSEIndividualAttitudeInterface
   , public IOSEVoiceLineKnowledgeInterface
   , public IOSEIndividualKnowledgeInterface
   , public ITATAIPatrolAccessorInterface
   , public IOSEAITargetSightInterface
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEscalationStateChanged, ETATEscalationState, escalationState);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickpocketableActorChanged, const UTATItemInfo*, item);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorViewingChanged, AActor*, viewerActor, bool, isViewing);

public:

   ATATCharacterAIBase(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

protected:
   // from AActor
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // from AOSECharacterBase
   virtual void InitializeAbilities(UOSEAbilitySystemComponent* inComponent, UAttributeBaseSet* inAttributeSet) override;

   void _OnLocalPlayerDetectionValueChanged(float oldDetectionValue, float newDetectionValue) const;

   // from Pawn
   virtual void PossessedBy(AController* newController) override;
   virtual void UnPossessed() override;

public:
   // from IOSEDetectionComponentInterface
   virtual UOSEDetectionComponent* GetDetectionComponent() const override { return _detectionComponent; }
   // from ITATItemInventorySystemInterface
   virtual UTATItemInventoryComponent* GetTATItemInventory() const override { return _tatItemInventory; }

   // from ITATLivingWorldAgentInterface
   UFUNCTION(BlueprintCallable)
   virtual UTATLivingWorldAgentComponent* GetLivingWorldAgentComponent() const override { return _tatLivingWorldAgentComponent; }

   // from ITATAIPatrolAccessorInterface
   virtual APatrolPath* GetPatrolPath_Implementation() const override
   {
      // Overriden in BP as its only set in the BP for now.
      return nullptr;
   }
   
   //---------------------------------------------------------------------------------------
   // IOSEIndividualAttitudeInterface
   //---------------------------------------------------------------------------------------
   virtual UOSEIndividualAttitudeComponent* GetAttitudeComponent() const override;

   //---------------------------------------------------------------------------------------
   // IOSETeamInterface
   //---------------------------------------------------------------------------------------

   UPROPERTY(EditDefaultsOnly, Category = "Teams")
   ETATTeamCharacterType TeamCharacter = ETATTeamCharacterType::Guard;

   virtual uint8 GetTeam() const override;
   virtual uint8 GetOriginalTeam() const override final;
   
   // from IOSEAITargetSightInterface
   virtual EAISightBucket GetBucketForTarget_Implementation() const override { return EAISightBucket::Medium; }
   // end IOSEAITargetSightInterface
   
   //---------------------------------------------------------------------------------------
   // IOSEStimDatabaseInterface
   //---------------------------------------------------------------------------------------

   virtual UOSEStimDatabase* AuthorityGetStimDatabase() const override;
   
   //---------------------------------------------------------------------------------------
   // IOSEIndividualKnowledgeInterface
   //---------------------------------------------------------------------------------------

   virtual UOSEIndividualKnowledgeComponent* GetIndividualKnowledgeComponent() const override;
   
   //---------------------------------------------------------------------------------------
   // IOSEVoiceLineKnowledgeInterface
   //---------------------------------------------------------------------------------------

   virtual UOSEVoiceLineKnowledgeComponent* GetVoiceLineKnowledgeComponent() const override;

   UFUNCTION(BlueprintNativeEvent, Category = "AI")
   void OnDebugHUDShowingChanged(bool isShowing);
   void OnDebugHUDShowingChanged_Implementation(bool isShowing) {}

   //---------------------------------------------------------------------------------------
   // ITATCombatDamageBonusTargetInterface
   //---------------------------------------------------------------------------------------

   virtual bool CanBeSneakAttackedByActor(AActor* actor) const override;
   virtual void OnSneakAttackedByActor(AActor* actor) override;
   virtual bool CanBeCounterAttackedByActor(AActor* actor) const override;
   virtual void OnCounterAttackedByActor(AActor* actor) override;

   //---------------------------------------------------------------------------------------
   // ITATHitReactAnimInterface
   //---------------------------------------------------------------------------------------

   virtual ETATHitReactAnimDirection GetHitReactAnimDirection() const { return _hitReactAnimDirection; }
   virtual void SetHitReactAnimDirection(ETATHitReactAnimDirection direction) { _hitReactAnimDirection = direction; }
   
   //---------------------------------------------------------------------------------------
   // IOSEAISightInterface
   //---------------------------------------------------------------------------------------

   virtual bool IsAllowedToSeeActor(const AActor* actor) const override;
   virtual void ModifySightRangeForSpecificActor(const AActor* actor, float& outSightRadius) const override;
   virtual FString DescribeSightRangeModificationForSpecificActor(const AActor* actor) const override;

   //---------------------------------------------------------------------------------------
   // Detection
   //---------------------------------------------------------------------------------------

   // Event raised when the player is first detected by an AI charcter
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLocalTATCharacterFirstDetected);
   UPROPERTY(BlueprintAssignable)
   FOnLocalTATCharacterFirstDetected OnLocalTATCharacterFirstDetected;

   //---------------------------------------------------------------------------------------
   // Escalation Debug
   //---------------------------------------------------------------------------------------
   UFUNCTION()
   void _AuthorityHandleEscalationStateChanged(ETATEscalationState newEscalationState);

   
   UPROPERTY(BlueprintAssignable, Category = "AI|Debug")
   FOnEscalationStateChanged OnEscalationStateNameChanged;

   ETATEscalationState GetCurrentEscalationState() const { return _escalationState; }

   UFUNCTION(BlueprintPure, Category = "AI|Status")
   void GetUnconsciousEffectInfo(FTATCharacterUnconsciousEffectInfo& effectInfo) const { effectInfo = _unconsciousEffectInfo; }
   
   /// from IOSEAlertnessInterface
   virtual UOSEAlertnessComponent* GetAlertnessComponent() const override;
   virtual EAlertnessLevel GetAlertnessLevel() const override;
   
   //---------------------------------------------------------------------------------------
   // IOSEAIVisibilityTargetInterface
   //---------------------------------------------------------------------------------------
   virtual void AuthorityOnEnterVisibleByActor(AActor* viewingActor) override;
   virtual void AuthorityOnExitVisibleByActor(AActor* viewingActor) override;

   // from ITATLootInventoryInterface
   virtual UTATLootInventoryComponent* GetLootInventoryComponent() const override { return _tatLootInventory; }

   // from ITATCharacterAnimationInterface
   virtual UAnimMontage* GetCharacterMontage(const FGameplayTag& animationTag) const override;

   // from ITATDisguiseTargetInterface
   virtual USkeletalMesh* GetDisguiseTargetMesh(ETATDisguiseMeshType meshType, TArray<UMaterialInterface*>& outOverrideMaterials) const override;
   virtual TSubclassOf<UAnimInstance> GetDisguiseTargetThirdPersonAnimClass() const override;
   virtual TSubclassOf<UAnimInstance> GetDisguiseTargetAnimSetLayer() const override;
   virtual UTATCharacterAnimationMappingAsset* GetDisguiseTargetCharacterAnimationMapping() const override { return _characterAnimationMapping; }
   virtual FTATDisguiseMovementParams GetDisguiseTargetMovementParams() const override;
   virtual ETATDisguiseTargetType GetDisguiseTargetType() const override;

   UFUNCTION(BlueprintCallable)
   UMotionWarpingComponent* GetMotionWarpingComponent() const { return _motionWarpingComponent; }

   /// BP-overridable version of sneak attack calculations that is called by CanBeSneakAttackedByActor()
   UFUNCTION(BlueprintNativeEvent)
   bool BP_CanBeSneakAttackedByActor(AActor* actor) const;
   virtual bool BP_CanBeSneakAttackedByActor_Implementation(AActor* actor) const;

   // from ITATPrivateSpaceCharacterInterface
   UFUNCTION(BlueprintCallable)
   virtual UTATPrivateSpaceCharacterComponent* GetPrivateSpaceCharacterComponent() override { return _privateSpaceCharacterComponent; }
   virtual bool CanBecomeSuspiciousOrIntruder() const override { return _canBecomeSuspiciousOrIntruder; }
   virtual bool CanEverBeAllowedInPrivateArea() const override { return true; }

   UFUNCTION(BlueprintCallable)
   TSoftObjectPtr<UAnimMontage> GetIdleAnimMontage() const;
   
   UFUNCTION(BlueprintCallable)
   TSoftObjectPtr<UAnimMontage> GetSearchAnimMontage() const;

   UFUNCTION(BlueprintCallable)
   TSoftObjectPtr<UAnimMontage> GetSearchAnimMontageFromType(ETATCharacterAISearchAnimType animType) const;


   //---------------------------------------------------------------------------------------
   // Pickpocketing
   //---------------------------------------------------------------------------------------

   UPROPERTY(BlueprintAssignable)
   FOnPickpocketableActorChanged OnPickpocketableItemAddedEvent;

   UPROPERTY(BlueprintAssignable)
   FOnPickpocketableActorChanged OnPickpocketableItemRemovedEvent;

   virtual void SprintRequest() override;
   virtual void SprintCancel() override;

   //---------------------------------------------------------------------------------------
   // IGameplayTagAssetInterface
   //---------------------------------------------------------------------------------------
   virtual void GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const override;

   void CreateNPCClueComponent(const UTATNPCClueSpawnerComponent* spawner);

   UFUNCTION(BlueprintCallable)
   FORCEINLINE UTATNPCClueComponent* GetNPCClueComponent() const { return _clueComponent; }

   UFUNCTION(BlueprintCallable)
   FORCEINLINE UTATAsyncRequestComponent* GetAsyncRequestComponent() const { return _asyncRequestComponent; }

   bool CanJoinLockdown() const { return _canJoinLockdown; } 
   
   bool IsCurrentlyOverlappingStaticGeometry() const { return _IsCurrentlyOverlappingWorldStaticGeometry; }
protected:
   UPROPERTY(EditDefaultsOnly, Category = "AI|TAT")
   bool CanBeFrozen = true;

protected:
   // from OSECharacterBase
   virtual void OnHealthChanged_Implementation(float newValue, float oldValue) override;
   virtual void _OnDamageChanged(const FOnAttributeChangeData& data) override;
   virtual void _OnHealthChanged(const FOnAttributeChangeData& data) override;
   UFUNCTION(BlueprintPure)
   bool IsDebugHUDShowing() const;

   UFUNCTION()
   void _OnRep_EscalationState();
   void _BroadcastEscalationStateChanged();

   UFUNCTION()
   void _OnIsGameFrozenChanged(bool isFrozen);

   UFUNCTION()
   void _OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel);

   virtual void OnLyingDownChanged_Implementation(bool isLyingDown) override;
   virtual void OnUnconsciousChanged_Implementation(bool isUnconscious) override;

   virtual void InitializeItemInventory(UItemInventoryComponent* itemInventoryComponent) override;

   void _OnGameplayTagChanged(FGameplayTag gameplayTag, int32 count);
   void _AuthorityOnAbilitiesInitialized();
   void _AuthorityAddDefaultItems();
   void _AuthorityTryAddPickpocketableComp();

   UFUNCTION(BlueprintNativeEvent)
   void OnPickpocketableItemAdded(const UTATItemInfo* item);
   void OnPickpocketableItemAdded_Implementation(const UTATItemInfo* item);

   UFUNCTION(BlueprintNativeEvent)
   void OnPickpocketableItemRemoved(const UTATItemInfo* item);
   void OnPickpocketableItemRemoved_Implementation(const UTATItemInfo* item);

   UFUNCTION()
   void _OnRep_PickpocketableComponent();

   UFUNCTION(BlueprintCallable)
   void GetPickpocketableItems(TArray<UTATItemInfo*>& items) const;

   /// The number of item slots in the backpack.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
   FTATInventorySize InventorySize;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
   TArray<FTATInventorySlot> DefaultItems;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
   TSubclassOf<UTATPickpocketableComponent> PickpocketableComponentClass;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sneak Attack")
   float SneakAttackDurationSeconds = 1.0f;

   // Effect to apply when health hits zero
   UPROPERTY(EditDefaultsOnly, Category = "Ability")
   TSubclassOf<UGameplayEffect> _InfiniteUnconsciousEffect;

   // This effect will be applied along with the infinite effect, however this one will expire. When it does, it'll apply
   // a "Self Revive" gameplay effect that will trigger a gameplay ability to handle the actual self revival.
   UPROPERTY(EditDefaultsOnly, Category = "Ability")
   FOSEEffectWithSetByCallerTag _FiniteUnconsciousEffect;

   UPROPERTY(EditDefaultsOnly, Category = "Ability")
   FScalableFloat _FiniteUnconsciousEffectDuration = 60.0f;
   
   // The "Hauntstable" NPC should block detection degradation so long as they are overlapping static geometry.
   UPROPERTY(EditDefaultsOnly, Category = "AI|TAT|Static Geometry Overlap")
   bool _ShouldCheckIfOverlappingWorldStaticGeometry { false };
   
#if WITH_EDITORONLY_DATA
   UPROPERTY(EditAnywhere, Category = "AI|TAT|Static Geometry Overlap")
   bool _DebugCheckOverlapWorldStaticGeometry { false };
#endif
   
   UPROPERTY(EditDefaultsOnly, Category = "AI|TAT|Static Geometry Overlap", meta=(EditCondition="_ShouldCheckIfOverlappingWorldStaticGeometry"))
   float _TimeBetweenOverlappingWorldStaticGeometryChecks { 0.5f };
   
   FTimerHandle _CheckOverlappingWorldStaticGeometryHandle;
   // This will only be updated if _ShouldCheckIfOverlappingWorldStaticGeometry == true
   bool _IsCurrentlyOverlappingWorldStaticGeometry { false };
   
   UPROPERTY(EditAnywhere, Category = "AI|TAT|Static Geometry Overlap", meta=(EditCondition="_ShouldCheckIfOverlappingWorldStaticGeometry"))
   float _CheckOverlappingWorldStaticHeightMultiplier { 0.6f };
   UPROPERTY(EditAnywhere, Category = "AI|TAT|Static Geometry Overlap", meta=(EditCondition="_ShouldCheckIfOverlappingWorldStaticGeometry"))
   float _CheckOverlappingWorldStaticHeightRadius { 1.f };
   
   void _HandleCheckOverlappingWorldStaticGeometry();
   
   UPROPERTY(EditAnywhere)
   UTATLootInventoryComponent* _tatLootInventory = nullptr;

   // Transient glyph actor class spawned upon KO
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT", Meta = (Categories = "Indicator"))
   FGameplayTag KnockedOutGlyphIndicatorType;

   UPROPERTY(EditDefaultsOnly, Category = "Character|AI|Wealth Class", meta=(Categories="AI.Character.WealthClass"))
   FGameplayTag WealthClass;

   // Delay before the knocked out glyph is spawned
   UPROPERTY(EditDefaultsOnly, Category = "Character|TAT", meta = (EditCondition = "KnockedOutGlyphIndicatorType.IsValid()", ClampMin = "0.0", UIMin = "0.0", Units = "seconds"))
   float KnockedOutGlyphSpawnDelaySeconds = 0.0f;

   UPROPERTY(EditDefaultsOnly, Category="AI|Light Detection")
   UCurveFloat* _LightIntensityToRangeMultiplierCurve { nullptr };

   UPROPERTY(EditDefaultsOnly, Category = "Animation")
   UTATCharacterAnimationMappingAsset* _characterAnimationMapping = nullptr;

   UPROPERTY(EditDefaultsOnly, Category="AI|Living World")
   UTATLivingWorldAgentComponent* _tatLivingWorldAgentComponent = nullptr;

   /// What first person upper body mesh should a character disguised as this one use?
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation (Disguise)")
   TObjectPtr<USkeletalMesh> _firstPersonUpperBodyMeshForDisguise;

   /// What first person lower body mesh should a character disguised as this one use?
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation (Disguise)")
   TObjectPtr<USkeletalMesh> _firstPersonLowerBodyMeshForDisguise;

   /// What type of NPC is this?  Guard/Civilian?  Helps with Disguise emotes
   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Disguise")
   ETATDisguiseTargetType DisguiseTargetType;

   
   UPROPERTY(EditDefaultsOnly, Category="AI|Idles")
   TMap<ETATEscalationState, FTATCharacterAIIdleAnimations> _idleMontages;
   UPROPERTY(EditDefaultsOnly, Category="AI|Search")
   TMap<ETATEscalationState, FTATCharacterAISearchAnimationSet> _searchMontages;
private:

   UTATKnowledgeComponent* _GetTATKnowledgeComponent() const;

   void _AuthoritySpawnKOGlyph();

   void _AuthorityOnGameplayEffectWithDurationAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle);
   void _AuthorityOnAnyGameplayEffectAdded(UAbilitySystemComponent* abilitySystemComponent, const FGameplayEffectSpec& gameplayEffectSpec, FActiveGameplayEffectHandle activeGameplayEffectHandle);

   void _AuthorityOnGameplayEffectRemoved(const FActiveGameplayEffect& activeEffect);

   void _AuthorityRecomputeUnconsciousEffectState();

   float _defaultTimeDilation = float(INDEX_NONE);
   ETATHitReactAnimDirection _hitReactAnimDirection = ETATHitReactAnimDirection::None;

   UPROPERTY(EditDefaultsOnly)
   UMotionWarpingComponent* _motionWarpingComponent = nullptr;

   UPROPERTY()
   UTATItemInventoryComponent* _tatItemInventory = nullptr;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_PickpocketableComponent)
   UTATPickpocketableComponent* _pickpocketableComponent = nullptr;

   UPROPERTY(Transient)
   AController* _possessedController = nullptr;

   UPROPERTY()
   UTATAttributeSet* _tatAttributes = nullptr;

   // used to determine how long it's been since we left the neutral alertness state
   float _worldTimeLeavingNeutralState = 0.0f;

   UPROPERTY(Transient, Replicated)
   FTATCharacterUnconsciousEffectInfo _unconsciousEffectInfo;

   UPROPERTY(Transient, ReplicatedUsing = "_OnRep_EscalationState")
   ETATEscalationState _escalationState = static_cast<ETATEscalationState>(0);

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Private Areas", meta = (AllowPrivateAccess="true"))
   UTATPrivateSpaceCharacterComponent* _privateSpaceCharacterComponent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category="Private Areas")
   bool _canJoinLockdown { true };
   
   UPROPERTY(EditDefaultsOnly, Category="Private Areas")
   bool _canBecomeSuspiciousOrIntruder { true };
   
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Alertness", meta = (AllowPrivateAccess="true"))
   UTATAlertnessComponent* _alertnessComponent { nullptr };
   
   UPROPERTY(EditDefaultsOnly)
   UOSEDetectionComponent* _detectionComponent { nullptr };
   
   UPROPERTY(Transient, BlueprintReadOnly, Category="Attitude", meta = (AllowPrivateAccess="true"))
   UOSEIndividualAttitudeComponent* _individualAttitudeComponent { nullptr };

   UPROPERTY(EditDefaultsOnly, Category="AI|Escalation")
   FGameplayTagContainer _GameplayTagsThatTriggerEscalationToVigilant;

   int _DefaultMovementGroupUID { 0 };
   FDelegateHandle _onBamboozledDelegateHandle;

   UPROPERTY(EditDefaultsOnly, Category = "AI|Clues")
   TSoftClassPtr<UTATNPCClueComponent> _clueComponentClass = nullptr;
   
   UPROPERTY(EditDefaultsOnly, Category = "Tick", DisplayName="Significance Tick Config")
   TObjectPtr<class UTATSignificanceBasedTickConfig> _movementSignificanceTickConfig = nullptr;

   UPROPERTY(Transient)
   TObjectPtr<UTATNPCClueComponent> _clueComponent = nullptr;

   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATAsyncRequestComponent> _asyncRequestComponent = nullptr;
};
