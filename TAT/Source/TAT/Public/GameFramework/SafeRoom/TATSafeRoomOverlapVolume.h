// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoomConnectedActor.h"

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"

// ue5
#include "GameplayTagContainer.h"

#include "TATSafeRoomOverlapVolume.generated.h"

class ATATSafeRoom;
class UGameplayEffect;
class UOSEShapeCollisionTrackerComponent;

// A volume placed over the insides of a safe room, to detect players who are inside it
///
/// NOTE: Safe Rooms are soft DEPRECATED pending removal, and should not be used going forward
UCLASS()
class TAT_API ATATSafeRoomOverlapVolume : public ATATSafeRoomConnectedActor
{
   GENERATED_BODY()
public:
   ATATSafeRoomOverlapVolume();

   virtual void BeginPlay() override;

   /// Tags to check for whether to apply the effects or not, e.g. Trait.Player
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer RequiredPlayerTags;

   UPROPERTY(EditDefaultsOnly)
   TArray<TSubclassOf<UGameplayEffect>> EffectsToApplyToPlayers;

   UFUNCTION(BlueprintImplementableEvent)
   void BP_AuthorityOnActorEnterVolume(AActor* actor);

   UFUNCTION(BlueprintImplementableEvent)
   void BP_AuthorityOnActorExitVolume(AActor* actor);

private:

   UFUNCTION()
   void _AuthorityOnActorEnterShape(AActor* actor);
   UFUNCTION()
   void _AuthorityOnActorExitShape(AActor* actor);
   UFUNCTION()
   void _AuthorityOnSafeRoomOwningPlayerChanged(ATATPlayerState* owningPlayer);

   UPROPERTY(EditDefaultsOnly)
   UOSEShapeCollisionTrackerComponent* _collisionTrackerComponent = nullptr;

   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _actorsWithEffectApplied;
};
