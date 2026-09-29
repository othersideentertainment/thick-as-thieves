// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/StateTrees/Interfaces/TATStateTreeCombatTargetInterface.h"
#include "Environment/TATAreaMarkupInterface.h"
#include "Animation/TATAnimSetOverride.h"
#include "AI/Combat/TATCombatPositioningInterface.h"

// ose
#include "OSEVoiceLineTraitInterface.h"
#include "Interactables/InteractableInterface.h"
#include "Interactables/InteractMontageSourceInterface.h"
#include "Player/OSEPlayerCharacter1P.h"
#include "OSEIndividualAttitudeReceiverInterface.h"

// ue
#include "Significance/SignificanceSettings.h"
#include "CoreMinimal.h"
#include "Interactables/OSEInteractionHelpers.h"

#include "TATCharacterBase.generated.h"

class UTATCombatPositioningComponent;
class UTATAnimSetTagTriggerSet;
class UTATAnimSetMapping;
struct FIndividualAttitude;
class UItemInventoryComponent;
class UTATCharacterAnimationMappingAsset;

// Separate tick function just to handle switching meshes to/from leader pose based on LOD of the main mesh
// Separate tick so that it can have tick dependencies without changing that of the main actor tick
USTRUCT()
struct FTATCharacterLeaderMeshTickFunction : public FTickFunction
{
	GENERATED_BODY()

	// Actor that is the target of the tick
	class ATATCharacterBase* Target = nullptr;


	virtual void ExecuteTick(float deltaTime, enum ELevelTick tickType, ENamedThreads::Type currentThread, const FGraphEventRef& myCompletionGraphEvent) override;
	virtual FString DiagnosticMessage() override;
	virtual FName DiagnosticContext(bool bDetailed) override;
};

template<>
struct TStructOpsTypeTraits<FTATCharacterLeaderMeshTickFunction> : public TStructOpsTypeTraitsBase2<FTATCharacterLeaderMeshTickFunction>
{
	enum
	{
		WithCopy = false
	};
};

UCLASS()
class TAT_API ATATCharacterBase
   : public AOSEPlayerCharacter1P,
     public IOSEVoiceLineTraitInterface,
     public ITATAreaMarkupInterface,
     public ITATAnimSetOverrideInterface,
     public IInteractableInterface,
     public IInteractMontageSourceInterface,
     public IOSEIndividualAttitudeReceiverInterface,
     public ITATStateTreeCombatTargetInterface,
     public ITATCombatPositioningInterface

{
   GENERATED_BODY()
public:
   /// Sets default values for this character's properties
   ATATCharacterBase(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   // from ACharacter
   virtual void SetBase(UPrimitiveComponent* newBase, const FName boneName = NAME_None, bool notifyActor = true) override;

   // from OSECharacterBase
   virtual void InitializeAbilities(UOSEAbilitySystemComponent* inComponent, UAttributeBaseSet* inAttributeSet) override;
   virtual void ResetAbilities() override;
   virtual void AuthorityOnKnockedOutByOtherCharacter_Implementation(AOSECharacterBase* otherCharacter) override;
protected:
   virtual bool _InterceptLanded(const FHitResult& hit) override;

public:
   
   /// IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* InteractingCharacter, FInteractPrompt& outPrompt) override;
   //---------------------------------------------------------------------------------------
   // IOSEVoiceLineTraitInterface
   //---------------------------------------------------------------------------------------
   virtual void GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const override;
   
   virtual void GetTraits(FGameplayTagContainer& tagContainer) const;

   // Begin ITATAreaMarkupInterface
   virtual const FGameplayTagContainer& GetAreaMarkupTags() const override { return _areaMarkupTags; };
   virtual void AddArea(TObjectPtr<ATATAreaMarkupVolume> area) override;
   virtual void RemoveArea(TObjectPtr<ATATAreaMarkupVolume> area) override;
   virtual const TArray<TWeakObjectPtr<ATATAreaMarkupVolume>>& GetAreasCurrentlyContainingActor() const override;
   // End ITATAreaMarkupInterface

   // begin IOSEIndividualAttitudeReceiverInterface
   virtual void AttitudeChangedFromActor(AActor* sourceOfChange, EOSEIndividualAttitude attitude) override;
   virtual EOSEIndividualAttitude GetAttitudeFromActor(const AActor* sourceOfChange) const override;
   const FIndividualAttitude* FindAttitude(const AActor* sourceOfChange) const;
   FIndividualAttitude* FindAttitude(const AActor* sourceOfChange);
   // end IOSEIndividualAttitudeReceiverInterface

   // Anim Set Override
   virtual void SuppressAnimSets(bool isSuppressed) override;
   virtual void AddAnimSetRequest(const FTATAnimSetRequest& request) override;
   virtual void RemoveAnimSetRequest(const FTATAnimSetRequest& request) override;
   virtual void RemoveAnimSetBySource(FObjectKey source) override;

   
   FORCEINLINE USkeletalMeshComponent* GetMesh3P_Body() const { return Mesh3P_Body; }

   float GetSignificanceByDistance(float distance) const;

   // begin ITATStateTreeCombatTargetInterface
   virtual void OnEnterTargetedByStateTree(AOSECharacterBase* aiCharacter) override;
   virtual void OnExitTargetedByStateTree(AOSECharacterBase* aiCharacter) override;
   // end ITATStateTreeCombatTargetInterface
   
   // begin ITATCombatPositioningInterface
   virtual UTATCombatPositioningComponent* GetTATCombatPositioningComponent_Implementation() const override { return _combatPositioningComponent; }
   // end ITATCombatPositioningInterface

protected:
   virtual void RegisterActorTickFunctions(bool shouldRegister) override;

   TSubclassOf<UAnimInstance> _GetAnimSetForTag(FGameplayTag tag) const;
   void _ScheduleAnimSetRefresh();
   void _SetAnimSet(TSubclassOf<UAnimInstance> animSetClass);
   void _RefreshAnimSet();
   void _OnAnimSetTriggerChanged(const FGameplayTag tag, int count);

   virtual void _OnHealthChanged(const FOnAttributeChangeData& data) override;

   /// Third person view body mesh
   /// (Note: named without underscore for consistency with head mesh)
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
   TObjectPtr<USkeletalMeshComponent> Mesh3P_Body;
   
   UPROPERTY(Replicated, Transient)
   TArray<FIndividualAttitude> _individualAttitudes;
  
   UPROPERTY(EditDefaultsOnly)
   UTATCombatPositioningComponent* _combatPositioningComponent { nullptr };

   // TODO: Legacy item inventory
   /// Called after the item inventory component has been created. Can be used to grant default items.
   virtual void InitializeItemInventory(UItemInventoryComponent* itemInventoryComponent);
   /// Item inventory component
   UPROPERTY(Replicated, Transient)
   UItemInventoryComponent* ItemInventoryComponent = nullptr;
   /// This will be true if it created the item inventory component in the C++ constructor, false if it was created later and needs replication fixups
   UPROPERTY(BlueprintReadOnly, Category = "Inventory")
   bool bCreatedStaticItemInventoryComponent = false;
   /// If true, the initial item inventory system initialization has happened
   UPROPERTY(Transient, BlueprintReadOnly, Category = "Inventory")
   bool bIsInventoryInitialized = false;

   UPROPERTY(Transient, BlueprintReadOnly, Category="AI.Markup")
   FGameplayTagContainer _areaMarkupTags;
   
   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<ATATAreaMarkupVolume>> _areaMarkupVolumes;
   
   UPROPERTY(EditDefaultsOnly, Category="AI|TAT", meta=(Categories="AI.Trait"))
   FGameplayTagContainer _characterTraits;

   UPROPERTY(EditDefaultsOnly, Category = "AI|TAT", meta=(Categories = "AI.Trait"))
   FGameplayTagContainer _voiceLineTraitsToTrack;

   UPROPERTY(Transient)
   FGameplayTagContainer _voiceLineTraitsApplied;

   UPROPERTY(EditDefaultsOnly)
   FSignificanceSettings _significanceSettings;

   // Mapping of anim sets in different states
   UPROPERTY(EditDefaultsOnly, Category = "Animation")
   TObjectPtr<UTATAnimSetMapping> _animSetMapping;

   // Triggers of AnimSet from gameplay tags on character
   UPROPERTY(EditDefaultsOnly, Category = "Animation")
   TObjectPtr<UTATAnimSetTagTriggerSet> _animSetTagTriggers;
   
   UPROPERTY(Transient)
   TSubclassOf<UAnimInstance> _currentAnimSet = nullptr;
   
   FTATAnimSetOverrides _animSetOverrides;
   FTimerHandle _refreshAnimSetTimer;
   bool _suppressAnimSets = false;

   FTATCharacterLeaderMeshTickFunction _leaderMeshTickFunction;

public:
   // IInteractMontageSourceInterface
   virtual UInteractMontageMappingAsset* GetInteractMontageMapping() const override { return _interactMontageAsset; }

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Interaction")
   TObjectPtr<UInteractMontageMappingAsset> _interactMontageAsset;

   // Multiplied with root motion to influence how far a staggered target travels
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stagger", meta = (UIMin = 0, ClampMin = 0))
   float _staggerRootMotionMultiplier = 1.f;

   // Player stat to increment on the player who knocks them out (if knocked out by a player)
   // Not a stat on self
   UPROPERTY(EditDefaultsOnly, Category = "PlayerStats", meta = (Categories="PlayerStats"))
   FGameplayTag _knockOutStatForAttackingPlayer;
   
   void _AuthorityResetAIKnowledgeOfMyself();

private:
   void _OnVoiceLineAppliedTraitTagsChanged(const FGameplayTag tag, int32 newTagCount);

   virtual void _OnMoveInput(const FInputActionValue& value) override;
   virtual void _OnLookInput(const FInputActionValue& value) override;
   
};
