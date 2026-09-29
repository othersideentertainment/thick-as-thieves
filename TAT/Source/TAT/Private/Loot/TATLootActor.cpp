// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootActor.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Loot/TATLootInstanceIDSubsystem.h"
#include "Loot/TATLootInterface.h"
#include "Loot/TATLootInventory.h"
#include "Loot/TATLootUtils.h"
#include "Player/TATLocalPlayerStateWorldSubsystem.h"
#include "Player/TATPlayerState.h"
#include "WorldMap/TATMapActorComponent.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "AI/Perception/TATAISense_Hearing.h"

#include "Analytics/TATAnalyticsManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootActor)

DEFINE_LOG_CATEGORY_STATIC(LogTATLootActor, Log, All);

UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Object_Loot_HighValue, "AI.Object.Loot.HighValue")
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Object_Loot_LowValue, "AI.Object.Loot.LowValue")
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Object_Loot_Dropped, "AI.Object.Loot.Dropped")

ATATLootActor::ATATLootActor()
{
   NetDormancy = ENetDormancy::DORM_Initial;

   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
}

void ATATLootActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // WasDropped is assigned on spawn, so only needs to rep once
   DOREPLIFETIME_CONDITION(ATATLootActor, PlacementState, COND_InitialOnly);
   DOREPLIFETIME(ATATLootActor, _takeState);
   DOREPLIFETIME(ATATLootActor, _interactingCharacter);
}

void ATATLootActor::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   if (UseInitialPlacementState)
   {
      PlacementState = InitialPlacementState;
   }
}

void ATATLootActor::BeginPlay()
{
   Super::BeginPlay();

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();

   // Get the loot info metadata for this loot actor
   const FTATLootInfo* lootInfo = nullptr;
   if (!LootRowHandle.IsNull())
   {
      lootInfo = lootSettings.GetLootInfo(LootRowHandle);
      if (lootInfo != nullptr)
      {
         if (LootInstance.IsValid())
         {
            // If we were supplied a LootInstance and a LootRowHandle, that's fine as long as the identifiers match
            ensureMsgf(LootInstance.Identifier == lootInfo->LootIdentifier,
               TEXT("LootActor spawned with two different loot types: LootRowHandle=%s, LootInstance=%s"),
               *lootInfo->LootIdentifier.LootTag.ToString(),
               *LootInstance.ToString());
         }
         else
         {
            // Create a new instance (for loot that needs full instance data, do this on the server to get the correct ID and replicate it back)
            if (HasAuthority() && lootInfo->RequiresInstanceStorage())
            {
               LootInstance = lootInfo->CreateDefaultInstance(this);
            }
            else
            {
               // We still expect the loot identifier to be filled out for non-instance data
               LootInstance.Identifier = lootInfo->LootIdentifier;
            }
         }
      }
   }
   else if (LootInstance.Identifier.IsValid())
   {
      if (HasAuthority())
      {
         if (!LootInstance.Id.IsValid())
         {
            UTATLootInstanceIDSubsystem* idSubsystem = UTATLootInstanceIDSubsystem::Get(this);

            if (ensure(idSubsystem))
            {
               FlushNetDormancy();
               LootInstance.Id = FTATLootInstanceId(idSubsystem->AuthorityGetNextLootID());
            }
         }
      }

      lootInfo = lootSettings.FindLootInfo(this, LootInstance.Identifier);
   }
   else
   {
      UE_LOG(LogTATLootActor, Error, TEXT("ATATLootActor spawned without a valid LootRowHandle or LootInstance!"));
      return;
   }

   if (lootInfo == nullptr)
   {
      UE_LOG(LogTATLootActor, Error, TEXT("ATATLootActor was spawned without a valid loot identifier!"));
      return;
   }

   if (PlacementState == ETATLootPlacementState::Dropped)
   {
      _InitDroppedState();
   }

   // TODO: For quest-related loot, wait/fire events for local-player-has-quest-item so it can maybe show/hide sparkles?
   //       (possibly with a subsystem as clearinghouse to avoid each actor listening for chain-o-things)

   if (lootInfo->IsQuestRelated && !IsNetMode(NM_DedicatedServer))
   {
      if (auto* localPlayerStateSubsystem = GetWorld()->GetSubsystem<UTATLocalPlayerStateWorldSubsystem>())
      {
         _OnLocalPlayerQuestLootChange(localPlayerStateSubsystem->GetQuestRelatedLoot());
         localPlayerStateSubsystem->OnLocalQuestRelatedLootChanged.AddUObject(this, &ThisClass::_OnLocalPlayerQuestLootChange);
         // Just explicitly call it if not local (is this preferable than just defaulting to off?)
         if (!_isForLocalPlayerQuest)
         {
            _OnRelevantForLocalQuest(false);
         }
      }
   }
}

void ATATLootActor::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   _TickTakingLerp(deltaTime);
}

void ATATLootActor::GatherCurrentMovement()
{
   if (!_takeState.IsSet())
   {
      // Skip replicating attachment if already started taking, since that can cause hiccups on clients
      Super::GatherCurrentMovement();
   }
}

void ATATLootActor::AuthoritySetupDroppedLootBeforeFinishSpawning(const FTATLootItemVariant& lootItem)
{
   check(HasAuthority());
   PlacementState = ETATLootPlacementState::Dropped;
   if (lootItem.IsInstance())
   {
      LootInstance = lootItem.GetInstanceRef();
   }
   else
   {
      LootInstance.Invalidate();
      LootInstance.Identifier = lootItem.GetIdentifier();
   }
}

#if WITH_EDITOR
EDataValidationResult ATATLootActor::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   // Skip validation for abstract classes (allows for a BP base class without a dummy handle)
   if (!GetClass()->HasAnyClassFlags(CLASS_Abstract))
   {
      // Log unassigned/invalid LootRowHandle
      FTATLootInfo* lootInfo = LootRowHandle.GetRow<FTATLootInfo>(TEXT("GetLootInfoFromRow"));
      if (!lootInfo)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("%s | LootRowHandle does not reference a valid FTATLootInfo data table entry!"), *GetName())));
         result = EDataValidationResult::Invalid;
      }
      else
      {
         // Log unassigned FTATLootInfo ActorClass
         if (lootInfo->ActorClass.IsNull())
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("%s | Associated FTATLootInfo data table entry has null ActorClass! Update entry to reference this actor's class")
               , *GetName())));
            result = EDataValidationResult::Invalid;
         }
         // Log mismatched FTATLootInfo ActorClass
         else if (lootInfo->ActorClass != GetClass())
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("%s | Associated FTATLootInfo data table entry has different ActorClass (%s)! Update entry to reference this actor's class")
               , *GetName()
               , *lootInfo->ActorClass.ToString())));
            result = EDataValidationResult::Invalid;
         }
         // Log unassigned LootTag
         if (!lootInfo->LootIdentifier.LootTag.IsValid())
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("%s | Associated FTATLootInfo data table entry has unassigned LootIdentifier::LootTag! Update entry with a valid LootTag")
               , *GetName())));
            result = EDataValidationResult::Invalid;
         }
         // Log unassigned ToolMesh (large carry loot only)
         if (lootInfo->CarriedLootMeshData.ToolMesh.IsNull() && lootInfo->IsLargeCarry)
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("%s | Associated FTATLootInfo data table entry has unassigned ToolMesh! Update entry with a valid static mesh")
               , *GetName())));
            result = EDataValidationResult::Invalid;
         }

         if(_hasPickupClue)
         {
            _pickupClue.Validate([&context, &result](const FText& error)
            {
               context.AddError(FText::FormatOrdered(INVTEXT("PickupClue: {0}"), error));
               result = EDataValidationResult::Invalid;
            });
         }
      }
   }

   return result;
}
#endif // WITH_EDITOR

bool ATATLootActor::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if (!Super::IsInteractable_Implementation(interactingCharacter))
   {
      return false;
   }

   check(IsValid(interactingCharacter));

   if (_takeState.IsSet())
   {
      return false;
   }

   const UTATLootSettings& tatLootSettings = UTATLootSettings::Get();

   // disallow interaction with quest loot for characters not on a quest for that loot
   const FTATLootInfo* lootInfo = tatLootSettings.FindLootInfo(this, LootInstance.Identifier);
   if (lootInfo && lootInfo->IsQuestRelated)
   {
      // Is it worth an interface here?
      const auto* playerState = interactingCharacter->GetPlayerState<ATATPlayerState>();
      if (playerState == nullptr || !playerState->HasQuestRelatedLoot(LootInstance.Identifier))
      {
         return false;
      }
   }

   return _interactingCharacter.IsValid() == false || interactingCharacter == _interactingCharacter;
}

void ATATLootActor::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

FInteractStartResult ATATLootActor::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   if (_interactingCharacter.IsValid())
   {
      UE_LOG(LogTATLootActor, Warning, TEXT("Player %s attempted to interact with loot, but player %s is already interacting with it")
         , *interactingCharacter->GetName()
         , *_interactingCharacter.Get()->GetName());

      return FInteractStartResult();
   }
   if (ITATLootInventoryInterface* lootInterface = Cast<ITATLootInventoryInterface>(interactingCharacter))
   {
      // Get character loot inventory
      UTATLootInventoryComponent* lootInventoryComponent = lootInterface->GetLootInventoryComponent();
      check(lootInventoryComponent);

      const UTATLootSettings& tatLootSettings = UTATLootSettings::Get();

      // Find associated loot data table entry
      const FTATLootInfo* lootInfo = tatLootSettings.FindLootInfo(this, LootInstance.Identifier);
      if (!lootInfo)
      {
         UE_LOG(LogTATLootActor, Error, TEXT("StartInteract() | %s could not find FTATLootInfo associated with %s!"), *GetName(), *LootInstance.ToString());
         return FInteractStartResult();
      }

      if (!lootInventoryComponent->HasRoomForLoot(LootInstance.Identifier))
      {
         return FInteractStartResult();
      }
      
      FInteractStartResult result;
      result.Delay = lootInfo->TimeTakenToSteal;
      result.bWaitForDelay = true;
      result.HoldAnimationTag = tatLootSettings.TakeInteractAnimationTag;
      result.HoldSourceEffect = GameplayEffectToApplyWhenLooting;
      
      if (HasAuthority())
      {
         FlushNetDormancy();
         _interactingCharacter = interactingCharacter;
      }
      
      return result;
   }

   return FInteractStartResult();
}

bool ATATLootActor::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      if (interactingCharacter != _interactingCharacter)
      {
         UE_LOG(LogTATLootActor, Verbose, TEXT("EndInteract() | Ignoring interaction from player %s (currently locked to %s)")
            , *interactingCharacter->GetName()
            , *_interactingCharacter.Get()->GetName());

         return false;
      }
      if (const ITATLootInventoryInterface* lootInterface = Cast<ITATLootInventoryInterface>(interactingCharacter))
      {
         // Get character loot inventory
         UTATLootInventoryComponent* lootInventoryComponent = lootInterface->GetLootInventoryComponent();

         // Component lives on TATPlayerState, which might not be replicated
         if (!IsValid(lootInventoryComponent))
         {
            return false;
         }
         const UTATLootSettings& tatLootSettings = UTATLootSettings::Get();

         // Find associated loot data table entry
         const FTATLootInfo* lootInfo = tatLootSettings.FindLootInfo(this, LootInstance.Identifier);
         if (!lootInfo)
         {
            UE_LOG(LogTATLootActor, Error, TEXT("StartInteract() | %s could not find FTATLootInfo associated with %s!"), *GetName(), *LootInstance.ToString());
            return false;
         }

         const UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(interactingCharacter, false);
         FPredictionKey predictionKey = asc ? asc->GetPredictionKeyForNewAction() : FPredictionKey();

         if (lootInventoryComponent->HandlePickupLootItem(lootInfo->LootIdentifier, this, predictionKey))
         {
            if(PlacementState != ETATLootPlacementState::Dropped)
            {
               // On the server, keep track of loot pickup stats (not including dropped ones)
               UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(interactingCharacter, UTATProjectSettings::Get().LootPickedUpStatTag);
            }

            _ExecutePickupGameplayCue(*lootInfo, interactingCharacter);

            if (HasAuthority())
            {
               _AuthorityExecutePickupGameplayEvent(*lootInfo, interactingCharacter);
            }
            
            if (UTATAnalyticsManager* analyticsManager = GetGameInstance()->GetSubsystem<UTATAnalyticsManager>())
            {
               FTATAnalyticsCustomFields fields;
               fields.Set(TEXT("Location"), GetActorLocation().ToCompactString());
               fields.Set(TEXT("LootID"), lootInfo->LootIdentifier.ToString());
               analyticsManager->OnDesignEventWithCustomFields(TEXT("LootCollected"), fields);
            }

            _takeState.TakingCharacter = interactingCharacter;
            _takeState.Started = true;
            _StartTakeAnimation();

            // Show clue if there is one, and the loot has never been picked up by anyone
            // (may want to reconsider if used in PvP, but this is the current design)
            if(_hasPickupClue && PlacementState == ETATLootPlacementState::InitialSpawn)
            {
               _pickupClue.OnLootTaken(interactingCharacter);
            }

            if (HasAuthority())
            {
               // Spawn transient glyph upon pickup
               UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
               if (thiefVisionSubsystem && _pickupTransientGlyphIndicatorType.IsValid())
               {
                  const FTransform spawnTransform = interactingCharacter->GetTransform();
                  thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(_pickupTransientGlyphIndicatorType, spawnTransform);
               }

               const FGameplayTag stimTag = PlacementState == ETATLootPlacementState::InitialSpawn && _firstTimePickedUpStim.IsValid() ? _firstTimePickedUpStim : _pickedUpStim;
               if (stimTag.IsValid())
               {
                  // Might as well report on interacting character, so they get credit?
                  UTATAISense_Hearing::ReportNoiseEvent(this, stimTag, GetActorLocation(), interactingCharacter);
               }

               // Notify listeners of pickup event
               AuthorityOnPickedUp.Broadcast(this, interactingCharacter);

               // Destroy self after lerping to player
               SetLifeSpan(tatLootSettings.GetLootActorLifetimeAfterPickup());
            }
         }
      }
   }

   if (HasAuthority() && _interactingCharacter == interactingCharacter)
   {
      FlushNetDormancy();
      _interactingCharacter = nullptr;
   }
   return true;
}

void ATATLootActor::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   // Get character loot inventory, check if has room
   if (const ITATLootInventoryInterface* lootInterface = Cast<ITATLootInventoryInterface>(interactingCharacter))
   {
      // Get character loot inventory
      const UTATLootInventoryComponent* lootInventoryComponent = lootInterface->GetLootInventoryComponent();

      // Component lives on TATPlayerState, which might not be replicated
      if (!IsValid(lootInventoryComponent))
      {
         return;
      }

      bool promptIsError = false;
      const bool playerHasRoom = lootInventoryComponent->HasRoomForLoot(LootInstance.Identifier);
      const bool playerInventoryFull = lootInventoryComponent->IsInventoryFull();
      const FText takePrompt = _GetCachedTakePrompt(promptIsError, playerHasRoom, playerInventoryFull);
      if (promptIsError)
      {
         outPrompt.ErrorMessage = takePrompt;
      }
      else
      {
         outPrompt.HoldAction = takePrompt;
      }
      outPrompt.InteractStatusTag = InteractionStatusTag;
   }
}

TOptional<ITATClueSourceInterface::FClueSourceParams> ATATLootActor::GetClueParameters() const
{
   const FTATLootInfo* lootInfo = LootRowHandle.GetRow<FTATLootInfo>(TEXT("GetLootInfoByRowHandle"));
   if(lootInfo == nullptr)
   {
      return NullOpt;
   }

   // TODO: Need some heuristic for supporting clues once the fallback isn't in the loot data
   //       For now, use major loot-ness, but this may change if there are eventual non-major
   //       Loot spawned in spawners that may also want loot that is non-clue-worthy (and thus
   //       should not unconditionally look for it, or use a fallback).
   if(lootInfo->LootType != ETATLootType::MajorLoot)
   {
      return NullOpt;
   }

   return FClueSourceParams { lootInfo->LootIdentifier.LootTag, lootInfo->ClueSet  };
}

FTATLootIdentifier ATATLootActor::GetLootIdentifier() const
{
   return LootInstance.Identifier;
}

bool ATATLootActor::HasLootInstanceData() const
{
   if (LootInstance.IsValid())
   {
      if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(this, LootInstance.Identifier))
      {
         return lootInfo->RequiresInstanceStorage();
      }
   }
   return false;
}

ETATLootType ATATLootActor::GetLootType() const
{
   return UTATLootUtils::GetLootType(this, LootInstance.Identifier);
}

static FGameplayTag GetTraitGameplayTagForLootType(const ETATLootType lootType)
{
   switch (lootType)
   {
      case ETATLootType::MinorLoot: return TAG_AI_Object_Loot_LowValue;
      case ETATLootType::MajorLoot: return TAG_AI_Object_Loot_HighValue;
      default: return FGameplayTag::EmptyTag;
   }
}

void ATATLootActor::GetActorTraitsForVoiceLines(FGameplayTagContainer& tagContainer) const
{
   const FGameplayTag tagToAdd = GetTraitGameplayTagForLootType(GetLootType());
   if(tagToAdd.IsValid())
   {
      tagContainer.AddTag(tagToAdd);
   }
}

void ATATLootActor::_ExecutePickupGameplayCue(const FTATLootInfo& lootInfo, const ACharacter* interactingCharacter)
{
   check(IsValid(interactingCharacter));

   const UTATLootSettings& tatLootSettings = UTATLootSettings::Get();
   const FGameplayTag pickupCueTag = tatLootSettings.GetLootPickupGameplayCue(lootInfo);
   if (pickupCueTag.IsValid())
   {
      UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(interactingCharacter);
      check(asc);
      asc->ExecuteGameplayCue(pickupCueTag, FGameplayCueParameters());
   }
   else
   {
      UE_LOG(LogTATLootActor, Warning, TEXT("GetLootPickupGameplayCue() returned invalid gameplay tag for loot info %s! Could not execute gameplay cue for pickup"), *lootInfo.DisplayName.ToString());
   }
}

void ATATLootActor::_AuthorityExecutePickupGameplayEvent(const FTATLootInfo& lootInfo, const ACharacter* interactingCharacter)
{
   check(HasAuthority());
   check(IsValid(interactingCharacter));

   const UTATLootSettings& tatLootSettings = UTATLootSettings::Get();
   const FGameplayTag pickupEventTag = tatLootSettings.GetLootPickupGameplayEventTag(lootInfo);
   if (pickupEventTag.IsValid())
   {
      FGameplayEventData eventData;
      eventData.EventTag = pickupEventTag;
      eventData.OptionalObject = this;

      UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(interactingCharacter);
      check(asc);
      asc->HandleGameplayEvent(pickupEventTag, &eventData);
   }
   else
   {
      UE_LOG(LogTATLootActor, Warning, TEXT("GetLootPickupGameplayEventTag() returned invalid gameplay tag for loot info %s! Could not execute gameplay event for pickup"), *lootInfo.DisplayName.ToString());
   }
}

FText ATATLootActor::_GetCachedTakePrompt(bool& promptIsError, bool playerHasRoom, bool playerInventoryFull)
{
   if (_takePromptCache.IsEmpty())
   {
      _takePromptCache = _ComputeTakePrompt();
   }

   if (playerHasRoom)
   {
      return _takePromptCache;
   }

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();

   // Determine what error message to show
   promptIsError = true;
   if (playerInventoryFull)
   {
      return lootSettings.PromptInventoryFull;
   }
   else if (!playerHasRoom)
   {
      return lootSettings.PromptItemDoesNotFitInInventory;
   }

   ensureMsgf(false, TEXT("Unknown error determining ATATLootActor prompt text"));
   return FText::GetEmpty();
}

FText ATATLootActor::_ComputeTakePrompt() const
{
   // Retrieve loot name from data table entry
   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   const FTATLootInfo* lootInfo = lootSettings.FindLootInfo(this, LootInstance.Identifier);
   if (!lootInfo)
   {
      UE_LOG(LogTATLootActor, Warning, TEXT("_GetCachedTakePrompt() failed due to UTATLootSettings::FindLootInfo() returning nullptr!"));
      return FText();
   }

   return FText::FormatNamed(lootSettings.PromptTakeOne, TEXT("Item"), lootInfo->DisplayName);
}

void ATATLootActor::_InitDroppedState()
{
   check(PlacementState == ETATLootPlacementState::Dropped);

   
   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   const FTATLootInfo* lootInfo = lootSettings.FindLootInfo(this, LootInstance.Identifier);
   if (!lootInfo)
   {
      UE_LOG(LogTATLootActor, Error, TEXT("_HandleWasDropped() failed due to UTATLootSettings::FindLootInfo() returning nullptr!"));
      return;
   }

   UE_LOG(LogTATLootActor, Verbose, TEXT("Loot actor %s was dropped"), *lootInfo->DisplayName.ToString());

   if (HasAuthority())
   {
      _OnDroppedAuthority();
      _TagContainer.Reset();
      _TagContainer.AddTag(TAG_AI_Object_Loot_Dropped);
      _TagContainer.AddTag(GetTraitGameplayTagForLootType(lootInfo->LootType));

      if (_droppedStim.IsValid())
      {
         UTATAISense_Hearing::ReportNoiseEvent(this, _droppedStim, GetActorLocation(), this);
      }
   }
   
   // Everything after this is cosmetic only
   if (GetNetMode() == NM_DedicatedServer)
   {
      return;
   }

   TSoftClassPtr<UTATMapActorComponent> mapActorComponentClass;
   // Show dropped major loot on map screen.
   if (lootInfo->IsQuestRelated)
   {
      mapActorComponentClass = lootSettings.DroppedQuestLootMapActorClass;
   }
   else if (lootInfo->LootType == ETATLootType::MajorLoot)
   {
      mapActorComponentClass = lootSettings.DroppedMajorLootMapActorClass;      
   }

   if (!mapActorComponentClass.IsNull())
   {
      TWeakObjectPtr<ATATLootActor> weakThis(this);
      UAssetManager::GetStreamableManager().RequestAsyncLoad(mapActorComponentClass.ToSoftObjectPath(), [weakThis, mapActorComponentClass]
      {
         if (weakThis.IsValid())
         {
            weakThis->_AddMapActorComponent(mapActorComponentClass.Get());
         }
      });
   }
   else
   {
      UE_LOG(LogTATLootActor, Warning, TEXT("Could not track dropped major loot actor %s on the map screen due to invalid DroppedMajorLootMapActorClass in UTATLootSettings!"), *GetName());
   }

   // Notify BP to produce dropped loot visuals
   _OnDropped();
}

void ATATLootActor::_AddMapActorComponent(TSubclassOf<UTATMapActorComponent> mapActorComponentClass)
{
   UTATMapActorComponent* mapActorComponent = NewObject<UTATMapActorComponent>(this, mapActorComponentClass.Get(), TEXT("MapActorComponent"));
   check(mapActorComponent);
   mapActorComponent->RegisterComponent();;
}

void ATATLootActor::_StartTakeAnimation()
{
   check(_takeState.TakingCharacter && _takeState.Started);
   SetActorEnableCollision(false);
   SetActorTickEnabled(true);

   AttachToActor(_takeState.TakingCharacter, FAttachmentTransformRules::KeepWorldTransform);

   _OnLootSuccessfullyPickedUp(_takeState.TakingCharacter);
}

void ATATLootActor::_OnRep_TakeState(const FTATLootActorTakeState& oldState)
{
   if (_takeState.TakingCharacter && _takeState.Started && _takeState != oldState)
   {
      _StartTakeAnimation();
   }
}

void ATATLootActor::_TickTakingLerp(float deltaTime)
{
   if (_takeState.TakingCharacter && _takeState.Started)
   {
      const UTATLootSettings& settings = UTATLootSettings::Get();

      const FVector targetLocation = _takeState.TakingCharacter->GetActorLocation();
      const FVector newLocation = FMath::VInterpTo(GetActorLocation(), targetLocation, deltaTime, settings.TakeSpeed);
      SetActorLocation(newLocation);

      if (FVector::DistSquared(targetLocation, newLocation) < FMath::Square(settings.CloseDistance))
      {
         SetActorHiddenInGame(true);
         SetActorTickEnabled(false);
      }
   }
}

void ATATLootActor::_OnLocalPlayerQuestLootChange(TConstArrayView<FTATLootIdentifier> questLootIds)
{
   bool newIsRelevant = questLootIds.Contains(LootInstance.Identifier);
   if (newIsRelevant != _isForLocalPlayerQuest)
   {
      _isForLocalPlayerQuest = newIsRelevant;
      _OnRelevantForLocalQuest(_isForLocalPlayerQuest);
   }
}
