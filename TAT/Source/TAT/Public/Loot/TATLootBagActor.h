// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootActorBase.h"
#include "Loot/TATLootTypes.h"

#include "TATLootBagActor.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UTATLootInventoryComponent;

UCLASS()
class TAT_API ATATLootBagActor : public ATATLootActorBase
{
   GENERATED_BODY()

   ATATLootBagActor();

   // From UActorComponent
   void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   /// IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;

public:
   void AuthorityPopulateLootItems(TConstArrayView<FTATLootIdentifier> lootIdentifierEntries, TConstArrayView<FTATLootInstance> lootInstanceEntries);

   void AuthorityAddLootItem(const FTATLootIdentifier& lootIdentifier);
   void AuthorityAddLootItem(const FTATLootInstance& lootInstance);
   void AuthorityAddLootItem(const FTATLootItemVariant& lootItem);

   const FTATLootContainer& GetDepositedLoot() const { return _lootContainer; }

   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBagBreak, const FTATLootContainer& loot, ACharacter* interactingCharacter);
   FOnBagBreak AuthorityOnLootBagBreak;

private:
   void _TryCompletePinata(ACharacter* interactingCharacter);

   // Returns appropriate interaction prompt, caching the formatted string after first being built by _ComputeTakePrompt()
   FText _GetCachedTakePrompt(const int roomForLoot);

   void _PinataDropAllItems(const FVector& dropOrigin);

   void _ExecutePinataCue(UAbilitySystemComponent* takingAsc) const;

   void _SetInteractingCharacter(const ACharacter* character);

private:
   UPROPERTY(Replicated, Transient)
   FTATLootContainer _lootContainer;

   // Tracks the currently-interacting character (should only be assigned on authority)
   UPROPERTY(Replicated)
   TWeakObjectPtr<const ACharacter> _interactingCharacter;

   // Radius that items will be spread out around the drop location. Uses a random value between min and max.
   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (ClampMin = "0", UIMin = "1"))
   FFloatInterval _pinataRadiusRange{ 30.0f, 60.0f };

   // The min and max item counts that are mapped to _interactDurationItemCount to determine how long the interact duration is
   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (ClampMin = "0", UIMin = "0"))
   FInt32Interval _interactDurationItemCount{ 0, 16 };

   // The min and max interact duration (in seconds) for this loot bag.
   // The min value will be used if there are less than or equal to _interactDurationItemCount.Min
   // The max value will be used if there are greater than or equal to _interactDurationItemCount.Max
   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (ClampMin = "0.01", UIMin = "0.01"))
   FFloatInterval _interactDurationRange{ 2.0f, 5.0f };

   // Minimum duration of the press-and-hold interaction. Interaction takes longer the more items are inside the bag.
   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (ClampMin = "0.01", UIMin = "0.01"))
   float _minInteractDuration = 2.f;

   // Maximum duration of the press-and-hold interaction. Interaction takes longer the more items are inside the bag.
   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (ClampMin = "0.01", UIMin = "0.01"))
   float _maxInteractDuration = 5.f;

   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (Categories = "InteractAnimation"))
   FGameplayTag _interactHoldAnimation;

   UPROPERTY(EditDefaultsOnly, Category = "Interaction")
   FOSEHeldActionCues _interactHoldActionCues;

   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (Categories = "GameplayCue"))
   FGameplayTag _takeGameplayCue;

   // Cached interaction prompt (formatted text string with loot name spliced in)
   FText _takePromptCache;

   // Cached number of loot items in the bag. Used to refresh _takePromptCache if the room for loot has changed.
   int32 _numItemsInContainerCache = -1;
};
