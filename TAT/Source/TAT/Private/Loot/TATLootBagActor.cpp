// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootBagActor.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootInterface.h"
#include "Loot/TATLootInventory.h"
#include "Loot/TATLootUtils.h"

// ue
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h" 
#include "AbilitySystemGlobals.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootBagActor)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootBagActor, Log, All);

ATATLootBagActor::ATATLootBagActor()
{
   // These should only be spawned dynamically, not placed in map
   NetDormancy = ENetDormancy::DORM_DormantAll;
}

void ATATLootBagActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATLootBagActor, _lootContainer);
   DOREPLIFETIME(ATATLootBagActor, _interactingCharacter);
}

bool ATATLootBagActor::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if (!Super::IsInteractable_Implementation(interactingCharacter))
   {
      return false;
   }

   // Only one character can interact at a time (return true for _interactingCharacter so prompt doesn't disappear)
   return !_interactingCharacter.IsValid() || interactingCharacter == _interactingCharacter;
}

FInteractStartResult ATATLootBagActor::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   check(IsValid(interactingCharacter));
   
   FInteractStartResult result;

   if (const ITATLootInventoryInterface* lootInterface = Cast<ITATLootInventoryInterface>(interactingCharacter))
   {
      // Get character loot inventory
      const UTATLootInventoryComponent* lootInventoryComponent = lootInterface->GetLootInventoryComponent();
      check(lootInventoryComponent);

      const int32 numItemsInContainer = _lootContainer.Num();
      if (numItemsInContainer == 0)
      {
         UE_LOG(LogTATLootBagActor, Error, TEXT("Loot bag is empty (it should already be destroyed!)"));
         return result;
      }

      // Abort interaction if another character is already interacting
      if (_interactingCharacter.IsValid())
      {
         UE_LOG(LogTATLootBagActor, Warning, TEXT("Player %s attempted to interact with loot bag, but player %s is already interacting with it")
            , *interactingCharacter->GetName()
            , *_interactingCharacter.Get()->GetName());

         return result;
      }
      else if (HasAuthority())
      {
         // Otherwise lock interaction to this player
         _SetInteractingCharacter(interactingCharacter);
      }

      // Interact duration increases from min -> max with the number of items being taken
      const float interactDuration = FMath::GetMappedRangeValueClamped<float>(
         FVector2f(static_cast<float>(_interactDurationItemCount.Min), static_cast<float>(_interactDurationItemCount.Max)),
         FVector2f(_interactDurationRange.Min, _interactDurationRange.Max),
         static_cast<float>(numItemsInContainer));

      result.bWaitForDelay = true;
      result.Delay = interactDuration;
      result.HoldAnimationTag = _interactHoldAnimation;
      result.HoldActionCues = _interactHoldActionCues;

      UE_LOG(LogTATLootBagActor, Verbose, TEXT("Player %s attempting to pinata %d items (%f seconds)...")
         , *interactingCharacter->GetName()
         , numItemsInContainer
         , interactDuration);
   }

   return result;
}

bool ATATLootBagActor::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      // Limit interactions to a single character at a time
      if (interactingCharacter != _interactingCharacter)
      {
         UE_LOG(LogTATLootBagActor, Verbose, TEXT("EndInteract() | Ignoring interaction from player %s (currently locked to %s)")
            , *interactingCharacter->GetName()
            , *_interactingCharacter.Get()->GetName());

         return false;
      }

      _TryCompletePinata(interactingCharacter);
   }

   if (HasAuthority() && _interactingCharacter == interactingCharacter)
   {
      // Clear interacting character ref, allowing others to interact
      UE_LOG(LogTATLootBagActor, Verbose, TEXT("EndInteract() | freeing interaction to other characters"));
      _SetInteractingCharacter(nullptr);
   }
   
   return false;
}

void ATATLootBagActor::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   outPrompt.HoldAction = _GetCachedTakePrompt(_lootContainer.Num());
}

void ATATLootBagActor::AuthorityPopulateLootItems(TConstArrayView<FTATLootIdentifier> lootIdentifierEntries, TConstArrayView<FTATLootInstance> lootInstanceEntries)
{
   check(HasAuthority());

   if (lootIdentifierEntries.Num() == 0 && lootInstanceEntries.Num() == 0)
   {
      return;
   }

   // Add each item (with proper checks)
   for (const FTATLootIdentifier& lootIdentifier : lootIdentifierEntries)
   {
      AuthorityAddLootItem(lootIdentifier);
   }

   for (const FTATLootInstance& lootInstance : lootInstanceEntries)
   {
      AuthorityAddLootItem(lootInstance);
   }
}

namespace LootBagHelpers
{
   template<typename T>
   void AuthorityAddLootItemToContainer(ATATLootBagActor* self, FTATLootContainer& container, const T& lootItem)
   {
      check(self != nullptr);
      check(self->HasAuthority());

      // Only valid loot should populate a loot bag actor (and invalid loot should never enter an inventory to begin with)
      check(lootItem.IsValid());

      self->FlushNetDormancy();
      container.Add(lootItem);

      UE_LOG(LogTATLootBagActor, Verbose, TEXT("[%s] | Loot item %s deposited (new size = %d)"),
         *self->GetName(),
         *lootItem.ToString(),
         container.Num());
   }
}

void ATATLootBagActor::AuthorityAddLootItem(const FTATLootIdentifier& lootIdentifier)
{
   LootBagHelpers::AuthorityAddLootItemToContainer(this, _lootContainer, lootIdentifier);
}

void ATATLootBagActor::AuthorityAddLootItem(const FTATLootInstance& lootInstance)
{
   LootBagHelpers::AuthorityAddLootItemToContainer(this, _lootContainer, lootInstance);
}

void ATATLootBagActor::AuthorityAddLootItem(const FTATLootItemVariant& lootItem)
{
   LootBagHelpers::AuthorityAddLootItemToContainer(this, _lootContainer, lootItem);
}

void ATATLootBagActor::_TryCompletePinata(ACharacter* interactingCharacter)
{
   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(interactingCharacter, false);
   _ExecutePinataCue(asc);

   if (HasAuthority())
   {
      _PinataDropAllItems(GetActorLocation() + FVector(0, 0, 10));
      AuthorityOnLootBagBreak.Broadcast(_lootContainer, interactingCharacter);
      _lootContainer.Reset();
      Destroy();
   }
}

FText ATATLootBagActor::_GetCachedTakePrompt(const int roomForLoot)
{
   const int32 numItems = _lootContainer.Num();
   if (_takePromptCache.IsEmpty() || _numItemsInContainerCache != numItems)
   {
      _takePromptCache = FText::FormatNamed(UTATLootSettings::Get().PromptTakeLootBagContents, TEXT("Num"), numItems);
      _numItemsInContainerCache = numItems;
   }
   return _takePromptCache;
}

void ATATLootBagActor::_PinataDropAllItems(const FVector& dropOrigin)
{
   const int32 numItemsToDrop = _lootContainer.Num();
   if (numItemsToDrop == 0)
   {
      return;
   }

   auto spawnLootActor = [this](const FTATLootItemVariant& lootItem, const FVector& dropLocation)
   {
      if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, lootItem.GetIdentifier()))
      {
         UTATLootUtils::SpawnLootActorAsync(GetWorld(), this, lootInfo->ActorClass, lootItem, dropLocation);
      }
   };

   // If we're only dropping one item, drop it in the middle rather than using a spread algorithm
   if (numItemsToDrop == 1)
   {
      FTATLootItemVariant droppedItem;
      _lootContainer.Get(0, &droppedItem);
      spawnLootActor(droppedItem, dropOrigin);
      return;
   }

   int32 pinataIndex = 0;
   _lootContainer.ForEach([&](const FTATLootIdentifier& lootId, const FTATLootInstance* lootInstance)
   {
      const FVector dropLocation = UTATLootUtils::CalcLootPinataSpawnLocation(dropOrigin, pinataIndex, numItemsToDrop, _pinataRadiusRange);
      if (lootInstance != nullptr)
      {
         spawnLootActor(FTATLootItemVariant(*lootInstance), dropLocation);
      }
      else
      {
         spawnLootActor(FTATLootItemVariant(lootId), dropLocation);
      }
      ++pinataIndex;
   });
}

void ATATLootBagActor::_ExecutePinataCue(UAbilitySystemComponent* takingAsc) const
{
   if (takingAsc == nullptr || !_takeGameplayCue.IsValid())
   {
      return;
   }

   FGameplayCueParameters params;
   params.Location = GetActorLocation();
   takingAsc->ExecuteGameplayCue(_takeGameplayCue, params);
}

void ATATLootBagActor::_SetInteractingCharacter(const ACharacter* character)
{
   FlushNetDormancy();
   _interactingCharacter = character;
}
