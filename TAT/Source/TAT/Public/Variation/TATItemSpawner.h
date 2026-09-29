// (c) 2021-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "TATActorSpawner.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue4
#include "CoreMinimal.h"

#include "TATItemSpawner.generated.h"

class UBoxComponent;

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATItemSpawner : public ATATActorSpawner
{
   GENERATED_BODY()

public:
   ATATItemSpawner();

protected:
   
   // Check to make sure the navigation is at a valid point.
   // Detect when path building is going to move a pathnode around
   // This may be undesirable for LDs (ie b/c cover links define slots by offsets)
   virtual void _ValidateCollision() override;
   virtual UShapeComponent* _GetShapeComponent() const override;

   virtual void AuthorityOnActorSpawnFinished_Implementation(const FTATVariationSpawnContext& spawnContext, AActor* actor, const FRandomStream& randomStream) override;

private:
   UPROPERTY(Transient)
   UBoxComponent* _boxComponent = nullptr;

   // If true, picking up spawned loot will trigger the endgame timer
   // CONSIDER: should this be a spawn modifier instead?
   UPROPERTY(EditInstanceOnly, Category="Pickup")
   bool _triggerEndgameOnPickup = false;

   UPROPERTY(EditInstanceOnly, Category="Pickup", meta = (EditCondition = "_triggerEndgameOnPickup", EditConditionHides))
   FTATSceneRequirement _triggerEndgameSceneRequirement;
};
