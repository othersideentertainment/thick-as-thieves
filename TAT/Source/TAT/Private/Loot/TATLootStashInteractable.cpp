// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootStashInteractable.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Loot/TATLootInventory.h"
#include "Loot/TATLootInterface.h"
#include "Loot/TATLootTypes.h"
#include "Loot/TATStashedLootSubsystem.h"
#include "Variation/TATSpawnerComponent.h"
#include "Player/TATPlayerState.h"
#include "WorldMap/TATMapActorComponent.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue
#include "AbilitySystemBlueprintLibrary.h"
#include "Developer/TATLootSettings.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootStashInteractable)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootStash, Log, All);


ATATLootStashInteractable::ATATLootStashInteractable()
{
   bReplicates = true;
   NetDormancy = ENetDormancy::DORM_Initial;

   // We'd like for players to always receive updates about their own stashes, so they can know when they're breached
   // for now this is the most straightforward way of accomplishing this
   bAlwaysRelevant = true;

   _mapActorComponent = CreateDefaultSubobject<UTATMapActorComponent>(TEXT("MapActorComponent"));
   _mapActorComponent->bAutoActivate = false;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootCompnent"));

   _spawnerComponent = CreateDefaultSubobject<UTATSpawnerComponent>(TEXT("SpawnerComponent"));
}

void ATATLootStashInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _interactingCharacter, params);
}

void ATATLootStashInteractable::BeginPlay()
{
   Super::BeginPlay();
}

void ATATLootStashInteractable::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void ATATLootStashInteractable::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   if (HasAuthority())
   {
      _spawnerComponent->AuthorityOnNotSpawn.AddDynamic(this, &ATATLootStashInteractable::_AuthorityOnNotSpawn);
   }

   _mapActorComponent->SetActive(_isMapTracked);
}

void ATATLootStashInteractable::TornOff()
{
   Super::TornOff();
   _StartDisappearing();
}

#if WITH_EDITOR
EDataValidationResult ATATLootStashInteractable::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   auto validateMapTag = [&](FGameplayTag tag, const FString& memberName)
   {
      // Validate tag
      if (!tag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Map tracked loot stash has unassigned %s!"), *memberName)));
      }
      // Make sure tag corresponds to valid sprite entry
      const UTATProjectSettings& tatProjectSettings = UTATProjectSettings::Get();
      if (const UTATMapSpriteDataAsset* mapSpriteDataAsset = tatProjectSettings.MapSpriteData.LoadSynchronous())
      {
         if(!mapSpriteDataAsset->SpriteTable.FindByKey(tag))
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has %s tag that doesn't correspond to an entry in UTATProjectSettings's MapSpriteData!"), *GetName(), *memberName)));
         }
      }
   };

   validateMapTag(_stashPendingMapTag, GET_MEMBER_NAME_STRING_CHECKED(ThisClass, _stashPendingMapTag));
   validateMapTag(_stashOpenMapTag, GET_MEMBER_NAME_STRING_CHECKED(ThisClass, _stashOpenMapTag));
   validateMapTag(_stashClosedMapTag, GET_MEMBER_NAME_STRING_CHECKED(ThisClass, _stashClosedMapTag));

   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR

bool ATATLootStashInteractable::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   check(IsValid(interactingCharacter));

   if (_isDisappearing || _interactingCharacter.IsValid())
   {
      return false;
   }
   
   return interactingCharacter->Implements<UTATLootInventoryInterface>();
}

FInteractStartResult ATATLootStashInteractable::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   check(IsValid(interactingCharacter));

   FInteractStartResult result;
   
   if (UTATLootInventoryComponent* lootInventoryComponent = UTATLootInventoryComponent::GetForActor(interactingCharacter))
   {
      if (lootInventoryComponent->HasStashableLoot())
      {
         FGameplayEventData eventData;
         eventData.OptionalObject = this;
         UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(interactingCharacter, _interactGameplayEvent, eventData);
         result.InstantAnimationTag = _interactInstantAnimation;
      }
   }
   return result;
}

void ATATLootStashInteractable::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   if (const UTATLootInventoryComponent* lootInventoryComponent = UTATLootInventoryComponent::GetForActor(interactingCharacter))
   {
      const UTATLootSettings& lootSettings = UTATLootSettings::Get();
      if (!lootInventoryComponent->HasStashableLoot())
      {
         outPrompt.ErrorMessage = lootSettings.PromptStashNoLootToDeposit;
      }
      else
      {
         outPrompt.PressAction = lootSettings.PromptStashUse;
      }
   }
}

void ATATLootStashInteractable::AuthorityDepositLoot(ACharacter* depositingCharacter)
{
   if (UTATLootInventoryComponent* lootInventoryComponent = UTATLootInventoryComponent::GetForActor(depositingCharacter))
   {
      _AuthorityDepositLoot(lootInventoryComponent, depositingCharacter);
   }
}

void ATATLootStashInteractable::_OnRep_InteractingCharacter()
{
   _NotifyStashInUseChanged();
}

void ATATLootStashInteractable::_NotifyStashInUseChanged()
{
   if(!_isDisappearing)
   {
      BP_OnStashInUseChanged(_interactingCharacter.IsValid());
   }
}

void ATATLootStashInteractable::_AuthoritySetInteractingCharacter(AActor* interactingCharacter)
{
   if(_interactingCharacter != interactingCharacter)
   {
      FlushNetDormancy();
      _interactingCharacter = interactingCharacter;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _interactingCharacter, this);
      _NotifyStashInUseChanged();
   }
}

bool ATATLootStashInteractable::AuthorityTrySetInteractingCharacter(AActor* interactingCharacter)
{
   if(_interactingCharacter != nullptr && _interactingCharacter != interactingCharacter)
   {
      return false;
   }

   _AuthoritySetInteractingCharacter(interactingCharacter);
   return true;
}

void ATATLootStashInteractable::AuthorityClearInteractingCharacter(const AActor* interactingCharacter)
{
   if(_interactingCharacter == interactingCharacter)
   {
      _AuthoritySetInteractingCharacter(nullptr);
   }
}

void ATATLootStashInteractable::_AuthorityDepositLoot(UTATLootInventoryComponent* lootInventoryComponent, ACharacter* depositingCharacter)
{
   check(HasAuthority());
   check(IsValid(lootInventoryComponent));

   // check if the inventory has authority. If this interactable actor is torn off, it will think it has authority, but nothing else will
   // I have also added better authority checks in the calls to this, but might as well have redundancy
   if (!lootInventoryComponent->GetOwner()->HasAuthority())
   {
      return;
   }
   
   ATATPlayerState* depositingPlayerState = depositingCharacter->GetPlayerState<ATATPlayerState>();
   check(IsValid(depositingPlayerState));
   
   FlushNetDormancy();

   FTATLootInventoryNotifyScope notifyScope(lootInventoryComponent);

   TArray<FTATLootIdentifier> lootToDeposit;
   const int32 itemsDeposited = lootInventoryComponent->AuthorityMoveLootToStash([&lootToDeposit](const FTATLootItemVariant& lootItem)
      {
         lootToDeposit.Add(lootItem.GetIdentifier());
      });

   if (itemsDeposited == 0)
   {
      UE_LOG(LogTATLootStash, Error, TEXT("_AuthorityMoveLootToStash() called for player %s without any loot to deposit!"), *lootInventoryComponent->GetOwner()->GetName());
      return;
   }

   if (UTATStashedLootSubsystem* stashSubsystem = GetWorld()->GetSubsystem<UTATStashedLootSubsystem>())
   {
      stashSubsystem->AuthorityAddStashedLoot(depositingPlayerState->GetTeam(), lootToDeposit);
   }

   // tear off as a sentinel for disappearing
   TearOff();
   _StartDisappearing();
}

void ATATLootStashInteractable::_AuthorityOnNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   check(HasAuthority());

   UE_LOG(LogTATLootStash, Verbose, TEXT("[%s] _AuthorityOnNotSpawn() | destroying actor..."), *GetName());

   Destroy();
}

void ATATLootStashInteractable::_StartDisappearing()
{
   _isDisappearing = true;
   BP_OnStartDisappearing();
}

