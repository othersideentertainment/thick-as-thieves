// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/TATItemActor.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Developer/TATPlayerInventorySettings.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Items/TATItemInfo.h"
#include "Items/TATItemInventoryComponent.h"
#include "Player/TATPlayerState.h"

// ose
#include "OSECommon.h"
#include "Character/OSECharacterBase.h"

// ue4
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATItemActor)


#define LOCTEXT_NAMESPACE "TATItemActor"

ATATItemActor::ATATItemActor()
   : Super()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
}

void ATATItemActor::PostLoad()
{
   Super::PostLoad();

   // Make sure to initialize _stackCount when loading from level or package.
   _stackCount = DefaultStackCount;
}

void ATATItemActor::PostActorCreated()
{
   Super::PostActorCreated();

   // Make sure to initialize _stackCount when creating in Editor or on Spawn.
   _stackCount = DefaultStackCount;
}

void ATATItemActor::PostInitializeComponents()
{
   Super::PostInitializeComponents();
}

void ATATItemActor::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   _TickLootTakingAnimation(deltaTime);
}

void ATATItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATItemActor, _stackCount);
   DOREPLIFETIME(ATATItemActor, _takingCharacter);
}

void ATATItemActor::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if (HasRoomForItem(interactingCharacter))
   {
      prompt.PressAction = _GetCachedTakePrompt();
   }
   else
   {
      prompt.ErrorMessage = UTATPlayerInventorySettings::Get().PromptInventoryFull;
   }
}

bool ATATItemActor::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   // So long as a character isn't trying to take us, we can be taken.
   if (_takingCharacter || _pendingTaker)
   {
      return false;
   }

   if (!UTATItemFunctionLibrary::CanCharacterPickUpThings(interactingCharacter))
   {
      return false;
   }

   return true;
}

FInteractStartResult ATATItemActor::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   // TODO: Verify whether we are taking the whole stack, or just part of it.
   if (!HasRoomForItem(interactingCharacter))
   {
      return FInteractStartResult();
   }

   // Possibly delay completing the take to allow partial animation to play
   // Hopefully relatively few interactables need a similar delay, or it
   // may require something more systemic.
   _pendingTaker = interactingCharacter;
   if (TakeDelay > 0)
   {
      FTimerHandle handle;
      GetWorld()->GetTimerManager().SetTimer(handle, this, &ATATItemActor::_StartPendingTake, TakeDelay);
   }
   else
   {
      _StartPendingTake();
   }
   
   FInteractStartResult result;
   result.InstantAnimationTag = TakeInteractAnimationTag;
   return result;
}

void ATATItemActor::ShowHighlight_Implementation(bool showHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, showHighlight);
}

void ATATItemActor::AuthoritySetStackCount(int32 newStackCount)
{
   check(HasAuthority());

   if (newStackCount <= 0)
   {
      return;
   }

   _stackCount = newStackCount;
   _takePromptCache = FText();
   OnStackCountChanged();
}

#if WITH_EDITOR
EDataValidationResult ATATItemActor::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // only check if present to tolerate abstract classes, etc
   if (const UTATItemInfo* itemInfo = Info.GetDefaultObject())
   {
      // if there are situations where we want this to vary, can add an opt out later
      // Gold items can share an ItemInfo, so don't expect ItemActor to match in that case.
      const bool canEnterInventory = itemInfo->InventoryDestination != ETATInventoryDestination::Gold && itemInfo->InventoryDestination != ETATInventoryDestination::UpgradeCurrency;
      if (canEnterInventory && itemInfo->ItemActor != GetClass())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] Item info %s does not have self as ItemActor"), *GetName(), *itemInfo->GetName())));
      }
   }

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void ATATItemActor::_StartPendingTake()
{
   ACharacter* interactingCharacter = _pendingTaker;
   if (_pendingTaker == nullptr || _takingCharacter != nullptr)
   {
      return;
   }

   FlushNetDormancy();
   _takingCharacter = interactingCharacter;
   _StartTakeAnimation();

   if (interactingCharacter && interactingCharacter->IsLocallyControlled())
   {
      OnTakenLocally();
   }

   // if we're the server...
   if (HasAuthority())
   {
      OnTakenAuthority(interactingCharacter);
      _OnTakenAuthority(interactingCharacter);

      // Try to add item to inventory.
      if (auto* inventorySystemInterface = Cast<ITATItemInventorySystemInterface>(interactingCharacter))
      {
         if (UTATItemInventoryComponent* itemInventory = inventorySystemInterface->GetTATItemInventory())
         {
            const int32 stacksAdded = itemInventory->AuthorityAddItemMultiple(Info, _stackCount);
            // TODO: Update _stackCount based on how many were added to the inventory.
         }
      }

      // On the server, keep track of loot pickup stats
      // TODO: Only do this for Loot pickups.
      const UTATProjectSettings& settings = *GetDefault<UTATProjectSettings>();
      UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(interactingCharacter, settings.LootPickedUpStatTag);

      // Destroy the loot after a few seconds delay to give clients some time to animate stuff
      FTimerHandle handle;
      GetWorld()->GetTimerManager().SetTimer(handle, this, &ATATItemActor::_DestroySelf, DestroyTime);
   }
}

FText ATATItemActor::_GetCachedTakePrompt()
{
   if (_takePromptCache.IsEmpty())
   {
      _takePromptCache = _ComputeTakePrompt();
   }
   return _takePromptCache;
}

FText ATATItemActor::_ComputeTakePrompt() const
{
   const UTATPlayerInventorySettings& settings = UTATPlayerInventorySettings::Get();

   FText itemName = Info ? Info.GetDefaultObject()->Name : LOCTEXT("InvalidItem", "NULL ITEM");
   if (_stackCount > 1)
   {
      return FText::FormatNamed(settings.PromptTakeMultiple, TEXT("Item"), itemName, TEXT("Num"), _stackCount);
   }
   else
   {
      return FText::FormatNamed(settings.PromptTakeOne, TEXT("Item"), itemName);
   }
}

void ATATItemActor::_StartTakeAnimation()
{
   OnDisableCollision();
   SetActorTickEnabled(true);
}

void ATATItemActor::_OnDroppedAuthority(const FVector& dropOrigin, const FVector& intendedDropDestination, APawn* droppingPawn)
{
   // just set the position for now
   SetActorLocation(intendedDropDestination);

   BP_OnFixedDropAuthority(dropOrigin, intendedDropDestination);

   Super::_OnDroppedAuthority(dropOrigin, intendedDropDestination, droppingPawn);
}

bool ATATItemActor::HasRoomForItem(ACharacter* interactingCharacter) const
{
   if (UTATItemInventoryComponent* takerInventory = UTATItemInventoryComponent::GetTATItemInventoryFromActor(interactingCharacter))
   {
      return takerInventory->HasRoomForItem(Info, _stackCount);
   }

   return false;
}

void ATATItemActor::_OnRep_StackCount()
{
   OnStackCountChanged();
}

void ATATItemActor::_OnRep_TakingCharacter(ACharacter* oldTakingCharacter)
{
   if (_takingCharacter && oldTakingCharacter != _takingCharacter)
   {
      _StartTakeAnimation();
   }
}

void ATATItemActor::_DestroySelf()
{
   Destroy();
}

void ATATItemActor::_TickLootTakingAnimation(float deltaTime)
{
   if (_takingCharacter)
   {
      const FVector targetLocation = _takingCharacter->GetActorLocation();
      const FVector newLocation = FMath::VInterpTo(GetActorLocation(), targetLocation, deltaTime, TakeSpeed);
      SetActorLocation(newLocation);

      if (FVector::DistSquared(targetLocation, newLocation) < CloseDistance * CloseDistance)
      {
         OnUpdateShowing(false);
         SetActorTickEnabled(false);
      }
   }
}

#undef LOCTEXT_NAMESPACE

