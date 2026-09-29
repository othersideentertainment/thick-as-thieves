// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/TATPickpocketableComponent.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Items/TATItemInfo.h"
#include "Items/TATItemInventoryComponent.h"
#include "Items/TATItemInventorySystemInterface.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Interactables/OSEInteractionHelpers.h"
#include "Player/OSEPlayerStats.h"
#include "OSECoreCollision.h"

// ue4
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPickpocketableComponent)

#define LOCTEXT_NAMESPACE "TATPickpocketing"

DEFINE_LOG_CATEGORY_STATIC(LogPickpocket, Log, All)

FPickpocketableVisuals::FPickpocketableVisuals()
   : RelativeLocation(FVector::ZeroVector)
   , RelativeRotation(FRotator::ZeroRotator)
   , RelativeUniformScale(1.0f)
   , Perspective(EMeshPerspective::ThirdPerson)
   , MeshAsset(nullptr)
{
}


UStaticMeshComponent* FPickpocketableVisuals::CreateAndRegisterMeshComponent(AActor* owner, USceneComponent* parent) const
{
   UStaticMesh* meshAsset = MeshAsset.Get();
   if (meshAsset == nullptr)
   {
      return nullptr;
   }

   UStaticMeshComponent* meshComponent = NewObject<UStaticMeshComponent>(owner, NAME_None, RF_Transactional | RF_Transient);
   meshComponent->SetRelativeLocation(RelativeLocation);
   meshComponent->SetRelativeRotation(RelativeRotation);
   meshComponent->SetupAttachment(parent);
   meshComponent->SetStaticMesh(meshAsset);
   meshComponent->bUseAttachParentBound = true;

   meshComponent->SetCollisionObjectType(ECC_WorldDynamic);
   meshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   meshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

   switch (Perspective)
   {
   case EMeshPerspective::ThirdPerson:
   {
      meshComponent->SetOwnerNoSee(true);
      meshComponent->CastShadow = true;
      meshComponent->bCastHiddenShadow = true;
   }
   break;

   case EMeshPerspective::FirstPerson:
   {
      meshComponent->SetOnlyOwnerSee(true);
      meshComponent->bCastDynamicShadow = false;
      meshComponent->bReceivesDecals = false;
      meshComponent->CastShadow = false;
   }
   break;
   }

   USceneComponent* parentToUse = FToolVisuals::FindToolRoot(owner, parent, Perspective);

   FTransform relativeXfm(RelativeRotation, RelativeLocation, FVector(RelativeUniformScale, RelativeUniformScale, RelativeUniformScale));
   meshComponent->SetRelativeTransform(relativeXfm);
   meshComponent->AttachToComponent(parentToUse, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);

   if (!meshComponent->IsRegistered())
      meshComponent->RegisterComponent();

   return meshComponent;
}

// Sets default values for this component's properties
UTATPickpocketableComponent::UTATPickpocketableComponent()
{
   // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
   // off to improve performance if you don't need them.
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   SetIsReplicatedByDefault(true);
   SetAutoActivate(true);

   // ...

   TakeSpeed = 8;
   CloseDistance = 10;
   DelayBeforeClearTaker = 1.f;
   DelayBeforeRemoveItems = 0.3f;

}

void UTATPickpocketableComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _takingCharacter, params);
}

void UTATPickpocketableComponent::OnComponentDestroyed(bool destroyingHierarchy)
{
   _DestroyInteractCapsule();

   for (const FPickpocketVisualsEntry& entry : _takeVisuals)
   {
      if (UPrimitiveComponent* mesh = entry.Mesh.Get())
      {
         mesh->DestroyComponent();
      }
   }

   Super::OnComponentDestroyed(destroyingHierarchy);
}

bool UTATPickpocketableComponent::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   // TODO: check if space in inventory
   return _takingCharacter == nullptr && _itemsToTake.Num() > 0;
}

void UTATPickpocketableComponent::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   IInteractableInterface::Execute_GetInteractPrompt(GetOwner(), interactingCharacter, prompt);

   if (_HasRoomForItem(interactingCharacter))
   {
      prompt.PressAction = LOCTEXT("PickpocketPrompt", "Pick Pocket");
   }
   else
   {
      prompt.PressAction = FText::GetEmpty();
      prompt.ErrorMessage = InventoryFullPrompt;
   }
}

FInteractStartResult UTATPickpocketableComponent::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   // proxy interaction to the character if it has an active hold, so you can still do both
   if (_DoesOwnerHaveHoldInteraction(interactingCharacter))
   {
      return IInteractableInterface::Execute_StartInteract(GetOwner(), interactingCharacter);
   }

   if (_HasRoomForItem(interactingCharacter))
   {
      _StartTake(interactingCharacter);
   }

   return FInteractStartResult();
}

bool UTATPickpocketableComponent::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      IInteractableInterface::Execute_EndInteract(GetOwner(), interactingCharacter, context);
   }
   else if(context.IsProbablyInstant() && _HasRoomForItem(interactingCharacter))
   {
      _StartTake(interactingCharacter);
   }

   return true;
}

void UTATPickpocketableComponent::ShowHighlight_Implementation(bool showHighlight)
{
   // TODO: proxy to parent if relevant (will need interacting character)

   for (const FPickpocketVisualsEntry& entry : _takeVisuals)
   {
      if (UPrimitiveComponent* mesh = entry.Mesh.Get())
      {
         mesh->SetRenderCustomDepth(showHighlight);
      }
   }
}

// Called every frame
void UTATPickpocketableComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (_takingCharacter)
   {
      for (const FPickpocketVisualsEntry& entry : _takeVisuals)
      {
         if (USceneComponent* visuals = entry.Mesh.Get())
         {
            const FVector newLocation = FMath::VInterpTo(visuals->GetRelativeLocation(), FVector::ZeroVector, deltaTime, TakeSpeed);
            visuals->SetRelativeLocation(newLocation);

            if (newLocation.SizeSquared() < CloseDistance * CloseDistance)
            {
               visuals->SetVisibility(false);
            }
         }
      }
   }
}

void UTATPickpocketableComponent::BeginPlay()
{
   Super::BeginPlay();

   if (AOSECharacterBase* character = GetCharacterOwner())
   {
      character->OnLyingDown.AddUObject(this, &UTATPickpocketableComponent::_OnLyingDownChanged);
      character->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATPickpocketableComponent::_InitInventory));
   }
}

void UTATPickpocketableComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (AOSECharacterBase* character = GetCharacterOwner())
   {
      character->OnLyingDown.RemoveAll(this);
   }

   Super::EndPlay(endPlayReason);
}

void UTATPickpocketableComponent::GetAttachedItems(TArray<UTATItemInfo*>& items) const
{
   items.Reserve(_takeVisuals.Num());
   for (const FPickpocketVisualsEntry& entry : _takeVisuals)
   {
      if (UTATItemInfo* item = entry.Item.Get())
      {
         items.Add(item);
      }
   }
}

void UTATPickpocketableComponent::_StartTake(ACharacter* takingCharacter)
{
   _takingCharacter = takingCharacter;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _takingCharacter, this);

   _StartTakeAnimation();
   _PlayTakingCue(takingCharacter);
   
   K2_OnStartTake(takingCharacter);

   if (GetOwnerRole() == ROLE_Authority)
   {
      if (UTATItemInventoryComponent* takerInventory = UTATItemInventoryComponent::GetTATItemInventoryFromActor(takingCharacter))
      {
         FTATInventoryNotifyScope inventoryBatch(takerInventory);
         for (const FTATInventorySlot& slot : _itemsToTake)
         {
            takerInventory->AuthorityAddItemMultiple(slot.ItemInfo, slot.StackCount);
         }
      }

      UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(takingCharacter, UTATProjectSettings::Get().PocketsPickedStatTag);

      GetWorld()->GetTimerManager().SetTimer(_clearTakerTimerHandler, this, &UTATPickpocketableComponent::_OnClearTakerTimer, DelayBeforeClearTaker);
      GetWorld()->GetTimerManager().SetTimer(_removeItemsTimerHandler, this, &UTATPickpocketableComponent::_OnRemoveItemsTimer, DelayBeforeRemoveItems);
   }
}

void UTATPickpocketableComponent::_StartTakeAnimation()
{
   SetComponentTickEnabled(true);
   for (const FPickpocketVisualsEntry& entry : _takeVisuals)
   {
      if (USceneComponent* visuals = entry.Mesh.Get())
      {
         visuals->AttachToComponent(_takingCharacter->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
      }
   }
}

void UTATPickpocketableComponent::_PlayTakingCue(ACharacter* takingCharacter)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(takingCharacter))
   {
      asc->ExecuteGameplayCue(PickpocketCueTag);
   }

   if (takingCharacter->IsLocallyControlled())
   {
      if (_itemsToTake.Num() > 0)
      {
         K2_OnTakenLocally(_itemsToTake[0].ItemInfo.GetDefaultObject());
      }
   }
}

bool UTATPickpocketableComponent::_HasRoomForItem(ACharacter* interactingCharacter) const
{
   if (_itemsToTake.Num() == 0) return false;

   if (UTATItemInventoryComponent* takerInventory = UTATItemInventoryComponent::GetTATItemInventoryFromActor(interactingCharacter))
   {
      // Invariant: only the first item can take up inventory slots
      const FTATInventorySlot& slot = _itemsToTake[0];
      if (!AllowTakingDuplicates && takerInventory->HasItemOfClass(slot.ItemInfo))
      {
         return false;
      }

      return takerInventory->HasRoomForItem(slot.ItemInfo, slot.StackCount);
   }

   return false;
}

bool UTATPickpocketableComponent::_DoesOwnerHaveHoldInteraction(ACharacter* interactingCharacter) const
{
   if (ACharacter* characterOwner = Cast<ACharacter>(GetOwner()))
   {
      return UOSEInteractionHelpers::HasHoldCharacterInteraction(interactingCharacter, characterOwner);
   }

   return false;
}

AOSECharacterBase* UTATPickpocketableComponent::GetCharacterOwner() const
{
   return Cast<AOSECharacterBase>(GetOwner());
}

void UTATPickpocketableComponent::_OnRep_TakingCharacter(ACharacter* oldTakingCharacter)
{
   if (_takingCharacter && _takingCharacter != oldTakingCharacter)
   {
      _StartTakeAnimation();
   }
   else if (_takingCharacter == nullptr && oldTakingCharacter != nullptr)
   {
      _OnTakingCharacterCleared();
   }
}

void UTATPickpocketableComponent::_InitInventory()
{
   if (UTATItemInventoryComponent* inventory = UTATItemInventoryComponent::GetTATItemInventoryFromActor(GetOwner()))
   {
      _inventory = inventory;
      inventory->ItemsChanged.AddUniqueDynamic(this, &UTATPickpocketableComponent::_OnInventoryChanged);
      _OnInventoryChanged();
   }
}

void UTATPickpocketableComponent::_OnInventoryChanged()
{
   UTATItemInventoryComponent* inventory = _inventory.Get();
   if (!inventory) return;

   if (_takingCharacter != nullptr) return;

   TArray< FTATInventorySlot, TInlineAllocator<4>> slotsToUse;
   for (const FTATInventorySlot& slot : inventory->GetBackpack())
   {
      if (slot.ItemInfo.GetDefaultObject()->IsPickpocketable)
      {
         if (slotsToUse.Num() == 0)
         {
            slotsToUse.Add(slot);
         }
         else
         {
            UE_LOG(LogPickpocket, Warning, TEXT("Character '%s' has multiple pickpocketable items that take inventory slots: %s and %s"), *GetOwner()->GetName(), *slotsToUse[0].ItemInfo->GetName(), *slot.ItemInfo->GetName());
         }
      }
   }

   for (const FTATInventorySlot& slot : inventory->GetQuestItems())
   {
      if (slot.ItemInfo.GetDefaultObject()->IsPickpocketable)
      {
         slotsToUse.Add(slot);
      }
   }

   _itemsToTake = slotsToUse;

   _DestroyLeftoverVisuals();

   for (const FTATInventorySlot& slot : _itemsToTake)
   {
      UTATItemInfo* defaultObject = slot.ItemInfo->GetDefaultObject<UTATItemInfo>();
      if(_takeVisuals.ContainsByPredicate([defaultObject] (const auto& entry) { return entry.Item == defaultObject; })) continue;

      FPickpocketVisualsEntry entry;
      entry.Item = defaultObject;
      entry.Mesh = defaultObject->PickpocketVisuals.CreateAndRegisterMeshComponent(GetOwner(), this);
      _takeVisuals.Add(entry);

      OnPickpocketableItemAdded.Broadcast(defaultObject);

      if (entry.Mesh.IsExplicitlyNull())
      {
         _LoadVisualsForItem(entry.Item);
      }
   }

   if (_itemsToTake.Num() > 0)
   {
      _CreateInteractCapsule();
   }
   else
   {
      _DestroyInteractCapsule();
   }
}

void UTATPickpocketableComponent::_LoadVisualsForItem(TWeakObjectPtr<UTATItemInfo> itemCdo)
{
   check(itemCdo.IsValid());
   TWeakObjectPtr<UTATPickpocketableComponent> weakThis(this);
   UAssetManager::GetStreamableManager().RequestAsyncLoad(itemCdo->PickpocketVisuals.MeshAsset.ToSoftObjectPath(), [weakThis, itemCdo] {
      if (weakThis.IsValid() && itemCdo.IsValid())
      {
         weakThis->_ApplyLoadedVisuals(itemCdo);
      }
      });
}

void UTATPickpocketableComponent::_ApplyLoadedVisuals(TWeakObjectPtr<UTATItemInfo> itemCdo)
{
   check(itemCdo.IsValid());
   FPickpocketVisualsEntry* found = _takeVisuals.FindByPredicate([itemCdo](const auto& entry) { return entry.Item == itemCdo; });
   if (found && found->Mesh.IsExplicitlyNull())
   {
      found->Mesh = itemCdo->PickpocketVisuals.CreateAndRegisterMeshComponent(GetOwner(), this);
   }
}

void UTATPickpocketableComponent::_OnRemoveItemsTimer()
{
   UTATItemInventoryComponent* inventory = _inventory.Get();
   if (!inventory) return;

   FTATInventoryNotifyScope inventoryBatch(inventory);
   TArray<FTATInventorySlot> itemsToRemove = MoveTemp(_itemsToTake);
   for (const FTATInventorySlot& slot : itemsToRemove)
   {
      inventory->AuthorityRemoveItemStack(slot.StackId);
   }
}

void UTATPickpocketableComponent::_OnClearTakerTimer()
{
   _takingCharacter = nullptr;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _takingCharacter, this);

   _OnTakingCharacterCleared();
}

void UTATPickpocketableComponent::_OnLyingDownChanged(bool isLyingDown)
{
   ACharacter* character = GetCharacterOwner();
   if (!character || !_interactCapsule) return;

   // re-position capsule when lying down
   // This may eventually need to account for the proportions of the character in question
   const FVector newPosition = isLyingDown ?
      CapsuleLyingDownOffset :
      CapsuleLocation;
   _interactCapsule->SetRelativeLocation(newPosition);

   // attach to body when down, as that it less consistent relative to the capsule, and it does not have to avoid the capsule
   if (isLyingDown)
   {
      USceneComponent* attachRoot = FToolVisuals::FindToolRoot(character, character->GetRootComponent(), EMeshPerspective::ThirdPerson);
      AttachToComponent(attachRoot, FAttachmentTransformRules::KeepRelativeTransform, LyingDownAttachPoint);
   }
   else
   {
      AttachToComponent(character->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
   }
}

void UTATPickpocketableComponent::_DestroyLeftoverVisuals()
{
   _takeVisuals.RemoveAllSwap([this](const FPickpocketVisualsEntry& entry) {
      if (!_itemsToTake.ContainsByPredicate([&entry](const auto& slot) { return slot.ItemInfo.GetDefaultObject() == entry.Item.Get(); }))
      {
         if (UStaticMeshComponent* mesh = entry.Mesh.Get())
         {
            mesh->DestroyComponent();
         }
         OnPickpocketableItemRemoved.Broadcast(entry.Item.Get());
         return true;
      }
      return false;
   });
}

void UTATPickpocketableComponent::_OnTakingCharacterCleared()
{
   _InitInventory();
   SetComponentTickEnabled(false);
}

void UTATPickpocketableComponent::_CreateInteractCapsule()
{
   AActor* owner = GetOwner();
   if (_interactCapsule == nullptr && owner)
   {
      _interactCapsule = NewObject<UCapsuleComponent>(owner, NAME_None, RF_Transactional | RF_Transient);
      //_interactCapsule->SetupAttachment(this);
      _interactCapsule->SetRelativeLocation(CapsuleLocation);
      _interactCapsule->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight, false);
      _interactCapsule->SetGenerateOverlapEvents(false);
      _interactCapsule->SetCollisionObjectType(ECC_Pawn);
      _interactCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
      _interactCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
      _interactCapsule->SetCollisionResponseToChannel(COLLISION_INTERACT, ECR_Overlap);
      _interactCapsule->ShapeColor = FColor::Yellow;
      _interactCapsule->ComponentTags.Add(UOSEInteractionHelpers::kInteractNoHighlightTag);
      _interactCapsule->RegisterComponent();

      GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UTATPickpocketableComponent::_AttachInteractCapsule);

      if (AOSECharacterBase* character = GetCharacterOwner())
      {
         _OnLyingDownChanged(character->IsLyingDown());
      }
   }
}

void UTATPickpocketableComponent::_AttachInteractCapsule()
{
   if (_interactCapsule && _interactCapsule->GetAttachParent() != this)
   {
      _interactCapsule->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
   }
}

void UTATPickpocketableComponent::_DestroyInteractCapsule()
{
   if (_interactCapsule)
   {
      _interactCapsule->DestroyComponent();
      _interactCapsule = nullptr;
   }
}

#undef LOCTEXT_NAMESPACE
