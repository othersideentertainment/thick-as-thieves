// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoomClaimAffectedInterface.h"
#include "Tools/WorldActors/TATToolWorldActor_Base.h"
#include "Traversal/TATTeleportUtilities.h"

#include "TATToolWorldActor_RecallingStone.generated.h"

class UTATRecallingStoneToolComponent;

UCLASS()
class TAT_API ATATToolWorldActor_RecallingStone : public ATATToolWorldActor_Base, public ITATSafeRoomClaimAffectedInterface
{
   GENERATED_BODY()
public:
   ATATToolWorldActor_RecallingStone();

   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   UFUNCTION(BlueprintCallable)
   bool GetTeleportDestination(FVector& outLocation, FRotator& outRotation) const;

   // From ITATSafeRoomClaimAffectedInterface
   virtual void AuthorityOnSurroundingSafeRoomClaimed(ATATSafeRoom* safeRoom) override;

   FTATTeleportQuerySettings TeleportSettings;

private:
   UTATRecallingStoneToolComponent* _GetRecallingStoneTool() const;

   UPROPERTY(Transient, Replicated)
   FVector _teleportFacingDirection;
};
