// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/SafeRoom/TATSafeRoom.h"

// tat
#include "Character/TATAstralProjectionCharacter.h"
#include "GameFramework/TATTravelMgr.h"
#include "GameFramework/SafeRoom/TATSafeRoomPlayerStart.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "TATGameInstance.h"
#include "UI/TATToastBroadcaster.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSafeRoom)

// Sets default values
ATATSafeRoom::ATATSafeRoom()
{
   PrimaryActorTick.bCanEverTick = false;
   bReplicates = true;
   // Make safe rooms always relevant, since we want to update map icons, etc. as they're claimed
   bAlwaysRelevant = true;
}

// Called when the game starts or when spawned
void ATATSafeRoom::BeginPlay()
{
   Super::BeginPlay();

   if (!IsNetMode(NM_DedicatedServer))
   {
      // There may be earlier hooks for the local player state being fully replicated (since this cares about other things, but this should be sufficient)
      UTATGameInstance::Get(this).GetTravelMgr().OnLocalPlayerLoadedIntoMap.AddUObject(this, &ATATSafeRoom::_OnLocalPlayerLoadedIntoMap);
   }
}

void ATATSafeRoom::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (IsValid(_owningPlayer))
   {
      _owningPlayer->OnPawnSet.RemoveAll(this);
   }

   UTATGameInstance::Get(this).GetTravelMgr().OnLocalPlayerLoadedIntoMap.RemoveAll(this);

   Super::EndPlay(endPlayReason);
}

#if WITH_EDITOR
void ATATSafeRoom::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (!SafeRoomPlayerStart)
      {
         FFormatNamedArguments arguments;
         arguments.Add(TEXT("ActorName"), FText::FromString(GetActorNameOrLabel()));
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} : SafeRoom has not SafeRoomPlayerStart")), arguments)));
      }
      else if (SafeRoomPlayerStart->SafeRoom != this)
      {
         FFormatNamedArguments arguments;
         arguments.Add(TEXT("ActorName"), FText::FromString(GetActorNameOrLabel()));
         arguments.Add(TEXT("OtherSafeRoomName"), FText::FromString(GetNameSafe(SafeRoomPlayerStart->SafeRoom)));
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} : SafeRoom's SafeRoomPlayerStart is set to {OtherSafeRoomName}, should be set to the safe room")), arguments)));
      }
   }
}
#endif

void ATATSafeRoom::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATSafeRoom, _owningPlayer);
   DOREPLIFETIME(ATATSafeRoom, _pawnsInSafeRoom);
}

bool ATATSafeRoom::CanBeClaimedBy(AActor* claimer) const
{
   // If we already have an owner, we can't be claimed
   if (_owningPlayer != nullptr)
   {
      return false;
   }

   // If any pawns besides the claimer are in the safe room, it can't be claimed
   for (const FPawnInSafeRoom& pawnInSafeRoom : _pawnsInSafeRoom)
   {
      if (IsValid(pawnInSafeRoom.Pawn) && pawnInSafeRoom.Pawn != claimer && pawnInSafeRoom.NumOverlaps > 0)
      {
         // In some cases, e.g. downed players or astral projection pawns, we want to ignore the presence of a pawn,
         // and still allow the safe room to be claimed
         if (!_ShouldIgnorePawnForCheckingClaimability(pawnInSafeRoom.Pawn))
         {
            return false;
         }
      }
   }

   return true;
}

void ATATSafeRoom::AuthoritySetOwningPlayer(ATATPlayerState* playerState)
{
   check(HasAuthority());
   check(IsValid(playerState));
   // Player must not have an existing safe room: give up the old one first if they do
   check(_owningPlayer == nullptr);

   SetOwner(playerState);
   _owningPlayer = playerState;
   _OnOwningPlayerSet(/*previousOwningPlayer*/nullptr);

   // Set the player start as the respawn point as temp glue to make claiming sorta work
   if (ATATPlayerController* controller = Cast<ATATPlayerController>(playerState->GetPlayerController()))
   {
      controller->AuthoritySetRespawnPoint(SafeRoomPlayerStart);
   }
}

bool ATATSafeRoom::AuthorityTryClaim(ATATPlayerState* playerState)
{
   check(HasAuthority());
   check(IsValid(playerState));

   APawn* pawn = playerState->GetPawn();
   if (pawn == nullptr)
   {
      return false;
   }

   if (!CanBeClaimedBy(pawn))
   {
      return false;
   }

   // Not bothering to clear claim on preview safe room, as safe rooms are semi-defunct
#if 0
   if (ATATSafeRoom* existingSafeRoom = playerState->GetChosenSafeRoom())
   {
      existingSafeRoom->_AuthorityRelinquishClaim();
   }
#endif

   // Send a toast to all clients that the safe room has been claimed
   if (ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(this))
   {
      toastBroadcaster->ClientToastBroadcast_PlayerClaimedSafeRoom(playerState, this);
   }

   AuthoritySetOwningPlayer(playerState);

   return true;
}

void ATATSafeRoom::_AuthorityRelinquishClaim()
{
   check(HasAuthority());
   check(_owningPlayer != nullptr);

   SetOwner(nullptr);

   ATATPlayerState* previousOwningPlayer = _owningPlayer;
   _owningPlayer = nullptr;
   _OnOwningPlayerSet(previousOwningPlayer);
}

void ATATSafeRoom::AuthorityActorEnteredSafeRoomVolume(AActor* actor)
{
   check(HasAuthority());
   
   // Ignore non-pawns
   if (APawn* pawn = Cast<APawn>(actor))
   {
      if (FPawnInSafeRoom* pawnInSafeRoom = _GetPawnInSafeRoom(actor))
      {
         pawnInSafeRoom->NumOverlaps++;
      }
      else
      {
         FPawnInSafeRoom newPawnInSafeRoom;
         newPawnInSafeRoom.Pawn = pawn;
         newPawnInSafeRoom.NumOverlaps = 1;
         _pawnsInSafeRoom.Add(newPawnInSafeRoom);
      }
   }

   if (actor == _owningPawn)
   {
      _AuthorityApplyOwnerInSafeRoomEffectsToActor(actor);
   }
}

void ATATSafeRoom::AuthorityActorExitedSafeRoomVolume(AActor* actor)
{
   check(HasAuthority());
   
   // If our owning pawn is entering the safe room, apply the expected effects
   if (actor == _owningPawn)
   {
      _AuthorityRemoveOwnerInSafeRoomEffectsFromActor(actor);
   }

   if (FPawnInSafeRoom* pawnInSafeRoom = _GetPawnInSafeRoom(actor))
   {
      pawnInSafeRoom->NumOverlaps--;
   }

   _pawnsInSafeRoom.RemoveAllSwap([](const FPawnInSafeRoom& pawnInSafeRoom)
   {
      return !IsValid(pawnInSafeRoom.Pawn) || pawnInSafeRoom.NumOverlaps == 0;
   });
}

void ATATSafeRoom::_OnRep_OwningPlayer(ATATPlayerState* previousOwningPlayer)
{
   _OnOwningPlayerSet(previousOwningPlayer);
}

void ATATSafeRoom::_OnOwningPlayerSet(ATATPlayerState* previousOwningPlayer)
{
   // Remove the pawn set callback from the previous owning player
   if (IsValid(previousOwningPlayer))
   {
      previousOwningPlayer->OnPawnSet.RemoveAll(this);
   }

   if (_owningPlayer != nullptr)
   {
      _SetOwningPawn(_owningPlayer->GetPawn());
      _owningPlayer->OnPawnSet.AddUniqueDynamic(this, &ATATSafeRoom::_OnOwningPawnSet);
   }
   else
   {
      _SetOwningPawn(nullptr);
   }

   _RefreshOwnerType();

   OnOwningPlayerChanged.Broadcast(_owningPlayer);
}

void ATATSafeRoom::_SetOwningPawn(APawn* pawn)
{
   if (pawn != _owningPawn)
   {
      APawn* oldPawn = _owningPawn;
      _owningPawn = pawn;

      if (HasAuthority())
      {
         // Remove the effect from the old pawn, it'll no-op if it doesn't exist
         _AuthorityRemoveOwnerInSafeRoomEffectsFromActor(oldPawn);

         // If we're switching ownership to a pawn that's already in the safe room,
         // add the expected effects. They'll be removed when they exit
         if (_GetPawnInSafeRoom(pawn) != nullptr)
         {
            _AuthorityApplyOwnerInSafeRoomEffectsToActor(pawn);
         }
      }

      OnOwningPawnChanged.Broadcast(pawn, oldPawn);
   }
}

void ATATSafeRoom::_OnOwningPawnSet(APlayerState* player, APawn* newPawn, APawn* oldPawn)
{
   _SetOwningPawn(newPawn);
}

void ATATSafeRoom::_OnLocalPlayerLoadedIntoMap(ATATPlayerState* playerState, ATATPlayerController* playerController, ATATCharacter* playerCharacter)
{
   _hasLocalPlayerLoaded = true;
   _RefreshOwnerType();
}

void ATATSafeRoom::_RefreshOwnerType()
{
   if (_hasLocalPlayerLoaded)
   {
      ETATSafeRoomOwnerType newType = ETATSafeRoomOwnerType::None;
      if (_owningPlayer)
      {
         newType = _owningPlayer->IsLocalPlayerState() ? ETATSafeRoomOwnerType::LocalPlayer : ETATSafeRoomOwnerType::RemotePlayer;
      }

      if (newType != _ownerType)
      {
         _ownerType = newType;
         OnOwnerTypeChanged.Broadcast(_ownerType);
      }
   }
}

FPawnInSafeRoom* ATATSafeRoom::_GetPawnInSafeRoom(AActor* actor)
{
   return _pawnsInSafeRoom.FindByPredicate([&](const FPawnInSafeRoom& pawnInSafeRoom)
   {
      return pawnInSafeRoom.Pawn == actor;
   });
}

void ATATSafeRoom::_AuthorityApplyOwnerInSafeRoomEffectsToActor(AActor* actor)
{
   check(HasAuthority());

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      for (TSubclassOf<UGameplayEffect> effectToApply : EffectsToApplyToOwnerInSafeRoom)
      {
         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         effectContext.AddInstigator(this, this);
         FActiveGameplayEffectHandle effectHandle = asc->ApplyGameplayEffectToSelf(effectToApply.GetDefaultObject(), 0.0f, effectContext);
         _actorsWithOwnerInSafeRoomEffectApplied.Add(actor, effectHandle);
      }
   }
}

void ATATSafeRoom::_AuthorityRemoveOwnerInSafeRoomEffectsFromActor(AActor* actor)
{
   check(HasAuthority());

   _actorsWithOwnerInSafeRoomEffectApplied.CancelByActor(actor);
}

bool ATATSafeRoom::_ShouldIgnorePawnForCheckingClaimability(const APawn* pawn) const
{
   // Ignore astral projection pawns: they cannot respawn in place, so they will not be inside a safe room after another player claimed it
   // Also, there's currently no way for a claiming player to force them out
   if (pawn->IsA<ATATAstralProjectionCharacter>())
   {
      return true;
   }

   // Ignore unconscious player pawns for similar reasons to above
   if (const ATATCharacter* tatCharacter = Cast<const ATATCharacter>(pawn))
   {
      return tatCharacter->IsUnconscious();
   }

   return false;
}
