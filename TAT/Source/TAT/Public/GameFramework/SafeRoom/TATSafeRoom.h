// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"

// ue5
#include "GameFramework/Actor.h"

#include "TATSafeRoom.generated.h"

class ATATCharacter;
class ATATPlayerController;
class ATATPlayerState;
class ATATSafeRoomPlayerStart;
class UGameplayEffect;

UENUM(BlueprintType)
enum class ETATSafeRoomOwnerType : uint8
{
   None,
   LocalPlayer,
   RemotePlayer
};


/// Tracks which pawns are inside the safe room
USTRUCT()
struct FPawnInSafeRoom
{
   GENERATED_BODY()
public:

   UPROPERTY()
   APawn* Pawn = nullptr;

   /// If we have multiple overlap volumes, we need to track how many the pawn is in,
   /// if it's >0 then they're in the safe room
   UPROPERTY()
   int32 NumOverlaps = 0;
};

/// An actor that represents a safe-room, and keeps track of the player
/// that owns it.
///
/// NOTE: Safe Rooms are soft DEPRECATED pending removal, and should not be used going forward
UCLASS()
class TAT_API ATATSafeRoom : public AActor
{
   GENERATED_BODY()
   
public:

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChanged, ATATPlayerState*, playerState);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOwnerTypeChanged, ETATSafeRoomOwnerType, safeRoomOwnerType);
   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPawnChanged, APawn*, APawn*);


   ATATSafeRoom();

   UPROPERTY(EditInstanceOnly, Category = "SafeRoom")
   TObjectPtr<ATATSafeRoomPlayerStart> SafeRoomPlayerStart;

   /// If true, this safe room is available for players to start the round in
   /// If false, the safe room is available for claiming, but cannot be one of the starting safe rooms
   UPROPERTY(EditInstanceOnly, Category = "SafeRoom")
   bool CanBeSpawnedInInitially = true;

   // numeric identifier of the safe room for use in limiting which spawns are active
   UPROPERTY(EditInstanceOnly, Category = "SafeRoom")
   int32 SafeRoomIndex = INDEX_NONE;

protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

public:
   // Should only be used by the game mode when players log in
   // See CanBeClaimedBy to know if a player can take over a safe room after the game starts
   bool IsAvailableForInitialAssignment() const { return CanBeSpawnedInInitially && _owningPlayer == nullptr; }

   bool CanBeClaimedBy(AActor* claimer) const;

   void AuthoritySetOwningPlayer(ATATPlayerState* playerState);

   bool AuthorityTryClaim(ATATPlayerState* playerState);

   UFUNCTION(BlueprintPure)
   ATATPlayerState* GetOwningPlayer() const { return _owningPlayer; }
   APawn* GetOwningPawn() const { return _owningPawn; }
   ETATSafeRoomOwnerType GetOwnerType() const { return _ownerType; }

   void AuthorityActorEnteredSafeRoomVolume(AActor* actor);
   void AuthorityActorExitedSafeRoomVolume(AActor* actor);

private:
   UFUNCTION()
   void _OnRep_OwningPlayer(ATATPlayerState* previousOwningPlayer);

   void _OnOwningPlayerSet(ATATPlayerState* previousOwningPlayer);
   void _SetOwningPawn(APawn* pawn);

   void _AuthorityRelinquishClaim();

   UFUNCTION()
   void _OnOwningPawnSet(APlayerState* player, APawn* newPawn, APawn* oldPawn);

   void _OnLocalPlayerLoadedIntoMap(ATATPlayerState* playerState, ATATPlayerController* playerController, ATATCharacter* playerCharacter);

   void _RefreshOwnerType();

   FPawnInSafeRoom* _GetPawnInSafeRoom(AActor* actor);

   void _AuthorityApplyOwnerInSafeRoomEffectsToActor(AActor* actor);
   void _AuthorityRemoveOwnerInSafeRoomEffectsFromActor(AActor* actor);

   bool _ShouldIgnorePawnForCheckingClaimability(const APawn* pawn) const;

public:

   UPROPERTY(EditDefaultsOnly)
   TArray<TSubclassOf<UGameplayEffect>> EffectsToApplyToOwnerInSafeRoom;

   FOnPawnChanged OnOwningPawnChanged;

   UPROPERTY(BlueprintAssignable, Category="TAT|SafeRoom")
   FOnPlayerStateChanged OnOwningPlayerChanged;

   UPROPERTY(BlueprintAssignable, Category = "TAT|SafeRoom")
   FOnOwnerTypeChanged OnOwnerTypeChanged;

protected:

private:
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_OwningPlayer, BlueprintGetter=GetOwningPlayer)
   TObjectPtr<ATATPlayerState> _owningPlayer;

   UPROPERTY(Transient)
   TObjectPtr<APawn> _owningPawn;

   // keeping track of if owned by local player for cosmetic reasons
   bool _hasLocalPlayerLoaded = false;
   ETATSafeRoomOwnerType _ownerType = ETATSafeRoomOwnerType::None;

   UPROPERTY(Transient, Replicated)
   TArray<FPawnInSafeRoom> _pawnsInSafeRoom;

   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _actorsWithOwnerInSafeRoomEffectApplied;
};
