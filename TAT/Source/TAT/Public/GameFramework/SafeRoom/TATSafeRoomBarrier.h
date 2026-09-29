// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoomConnectedActor.h"

// ue5
#include "GameplayTagContainer.h"

#include "TATSafeRoomBarrier.generated.h"

class ATATSafeRoom;
class APlayerState;
class ATATPlayerState;

enum class ETATSafeRoomOwnerType : uint8;

// An actor that can only be moved through by the owner of the safe room
///
/// NOTE: Safe Rooms are soft DEPRECATED pending removal, and should not be used going forward
UCLASS()
class TAT_API ATATSafeRoomBarrier : public ATATSafeRoomConnectedActor
{
   GENERATED_BODY()
   
public:
   ATATSafeRoomBarrier();

protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   virtual void NotifyActorEndOverlap(AActor* otherActor) override;

   // Only to be used for visuals
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnOwnerTypeChanged(ETATSafeRoomOwnerType ownerType);

private:
   void _OnOwnerPawnChanged(APawn* newPawn, APawn* oldPawn);
   void _SetShouldIgnoreMovementWithPawn(APawn* pawn, bool shouldIgnore);

   UFUNCTION()
   void _OnOwningPlayerChanged(ATATPlayerState* ownerPlayer);

   UFUNCTION()
   void _OnOwnerTypeChanged(ETATSafeRoomOwnerType ownerType);

private:

   /// Tag on abilities that will be cancelled if the play passes through the barrier
   UPROPERTY(EditInstanceOnly, Category = SafeRoom)
   FGameplayTag _abilityTagToCancelOnTraversal;

   // Track which pawns we've set to ignore our collision during movement
   // If the owning player changes, revert the outstanding ignores
   TArray<TWeakObjectPtr<APawn>> _ignoredPawns;
   TWeakObjectPtr<APlayerState> _playerStateForIgnoredPawns;
};
