// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootActorBase.h"
#include "Loot/TATLootTypes.h"
#include "Variation/Clues/TATClueSourceInterface.h"
#include "Variation/Clues/TATLootClue.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "OSEVoiceLineTraitInterface.h"

// ue
#include "GameplayTagAssetInterface.h"
#include "Engine/DataTable.h"
#include "Templates/SubclassOf.h"
#include "NativeGameplayTags.h"


#include "TATLootActor.generated.h"

class ACharacter;
class UTATMapActorComponent;

USTRUCT()
struct FTATLootActorTakeState
{
   GENERATED_BODY()

   UPROPERTY()
   TObjectPtr<ACharacter> TakingCharacter = nullptr;

   UPROPERTY()
   bool Started = false;

   bool IsSet() const { return TakingCharacter != nullptr; }

   bool operator==(const FTATLootActorTakeState& other) const
   {
      return TakingCharacter == other.TakingCharacter && Started == other.Started;
   }
   bool operator!=(const FTATLootActorTakeState& other) const
   {
      return !(*this == other);
   }
};


UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Object_Loot_HighValue)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Object_Loot_LowValue)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Object_Loot_Dropped)

UCLASS()
class TAT_API ATATLootActor : public ATATLootActorBase
   , public IOSEVoiceLineTraitInterface
   , public ITATClueSourceInterface
   , public IGameplayTagAssetInterface
{
   GENERATED_BODY()

   ATATLootActor();

   // From UObject
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;
   virtual void Tick(float deltaTime) override;
   virtual void GatherCurrentMovement() override;

public:
   /// Called by inventory components to set up a dropped loot item. Must be called before the actor has finished spawning!
   void AuthoritySetupDroppedLootBeforeFinishSpawning(const FTATLootItemVariant& lootItem);

   /// IInteractableInterface
   bool IsInteractable_Implementation(ACharacter* interactingCharacter) const;
   void ShowHighlight_Implementation(bool bShowHighlight);
   FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter);
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt);

   // from ITATClueSourceInterface
   virtual TOptional<FClueSourceParams> GetClueParameters() const override;

   UFUNCTION(BlueprintPure, Category = "Loot")
   FTATLootIdentifier GetLootIdentifier() const;

   /// Checks if this loot item has instance data that must be transferred when picking it up
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool HasLootInstanceData() const;

   UFUNCTION(BlueprintPure, Category = "Loot")
   ETATLootType GetLootType() const;

   UFUNCTION(BlueprintPure, Category = "Loot")
   ETATLootPlacementState GetLootPlacementState() const { return PlacementState; }

   // from IOSEVoiceLineTraitInterface
   virtual void GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const override;

   // from IGameplayTagAssetInterface
   virtual bool HasMatchingGameplayTag(FGameplayTag tagToCheck) const override { return _TagContainer.HasTag(tagToCheck); }
   virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override { return _TagContainer.HasAll(tagContainer); }
   virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override { return _TagContainer.HasAny(tagContainer); }
   virtual void GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const override { tagContainer.AppendTags(_TagContainer); }
   
protected:
   FGameplayTagContainer _TagContainer;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> GameplayEffectToApplyWhenLooting;

   UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
   void _OnLootSuccessfullyPickedUp(ACharacter* pickedupByCharacter);

   UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
   void _OnDropped();

   UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
   void _OnDroppedAuthority();

   /// Event fired for quest-related loot when the local player is on a quest for it (to turn off/on VFX for it)
   /// Not called on dedicated server
   UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
   void _OnRelevantForLocalQuest(bool isQuestRelevant);

private:

   void _ExecutePickupGameplayCue(const FTATLootInfo& lootInfo, const ACharacter* interactingCharacter);
   void _AuthorityExecutePickupGameplayEvent(const FTATLootInfo& lootInfo, const ACharacter* interactingCharacter);

   // Returns appropriate interaction prompt, caching the formatted string after first being built by _ComputeTakePrompt()
   FText _GetCachedTakePrompt(bool& promptIsError, bool playerHasRoom, bool playerInventoryFull);

   // Builds the pickup interaction prompt using the loot item name
   FText _ComputeTakePrompt() const;

   void _InitDroppedState();
   
   void _AddMapActorComponent(TSubclassOf<UTATMapActorComponent> mapActorComponentClass);

   void _StartTakeAnimation();

   UFUNCTION()
   void _OnRep_TakeState(const FTATLootActorTakeState& oldState);
   void _TickTakingLerp(float deltaTime);

   void _OnLocalPlayerQuestLootChange(TConstArrayView<FTATLootIdentifier> questLootIds);
public:
   // Should point to a FTATLootInfo data table entry containing metadata
   UPROPERTY(EditDefaultsOnly, Meta = (RowType = "/Script/TAT.TATLootInfo"))
   FDataTableRowHandle LootRowHandle;

   UPROPERTY(EditAnywhere, Category = "TAT|Interaction")
   FGameplayTag InteractionStatusTag;
   
   // Loot instance data, if this loot actor has any. If used, this is expected to be assigned during actor spawn.
   UPROPERTY(Transient)
   FTATLootInstance LootInstance;

   UFUNCTION(BlueprintCallable, BlueprintPure)
   const FTATLootInstance& GetLootInstance() const { return LootInstance; }

   bool GetCanPlaceCallingCard() const { return _canPlaceCallingCard; }
   void SetCanPlaceCallingCard(bool canPlaceCallingCard) { _canPlaceCallingCard = canPlaceCallingCard; }

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot", Meta = (InlineEditConditionToggle))
   bool UseInitialPlacementState = false;

   /// The placement state that this loot actor will be initialized to at spawn time
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Loot", Meta = (EditCondition = "UseInitialPlacementState"))
   ETATLootPlacementState InitialPlacementState = ETATLootPlacementState::InitialSpawn;

   // True if this loot actor instance was dropped by a player (see UTATLootInventoryComponent::_AuthoritySpawnLootActor())
   UPROPERTY(Replicated, Transient)
   ETATLootPlacementState PlacementState = ETATLootPlacementState::InitialSpawn;

   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPickedUp, const ATATLootActor* lootActor, ACharacter* interactingCharacter);
   // Called right as the loot actor is picked up, shortly before destruction
   FOnPickedUp AuthorityOnPickedUp;

private:
   // Cached interaction prompt (formatted text string with loot name spliced in)
   FText _takePromptCache;

   bool _isForLocalPlayerQuest = false;

   // Transient glyph indicator type to spawn when this loot item is picked up
   UPROPERTY(EditDefaultsOnly, Category = "Loot", Meta = (Categories = "Indicator"))
   FGameplayTag _pickupTransientGlyphIndicatorType;

   /// Stim to apply when picked up
   UPROPERTY(EditDefaultsOnly, Category = "Loot|Stim", Meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag _firstTimePickedUpStim;

   /// Stim to apply when picked up
   UPROPERTY(EditDefaultsOnly, Category = "Loot|Stim", Meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag _pickedUpStim;

   /// Stim to apply when picked up
   UPROPERTY(EditDefaultsOnly, Category = "Loot|Stim", Meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag _droppedStim;

   UPROPERTY(EditDefaultsOnly, Category = "Clue", Meta = (InlineEditConditionToggle))
   bool _hasPickupClue = false;

   // Clue shown when the loot is picked up for the first time
   UPROPERTY(EditDefaultsOnly, Category = "Clue", Meta = (EditCondition = "_hasPickupClue"))
   FTATLootClue _pickupClue;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_TakeState)
   FTATLootActorTakeState _takeState;

   UPROPERTY(Transient)
   bool _canPlaceCallingCard = true;
   
   UPROPERTY(Replicated)
   TWeakObjectPtr<const ACharacter> _interactingCharacter { nullptr };
};
