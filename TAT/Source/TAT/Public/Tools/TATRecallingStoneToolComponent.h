// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATPlacedActorToolComponent.h"

#include "TATRecallingStoneToolComponent.generated.h"

class UGameplayEffect;
class ATATToolWorldActor_RecallingStone;

UCLASS()
class TAT_API UTATRecallingStoneToolComponent : public UTATPlacedActorToolComponent
{
   GENERATED_BODY()
public:

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   virtual bool OnRemoveFromToolSet_Implementation() override;

   UFUNCTION(BlueprintPure)
   ATATToolWorldActor_RecallingStone* GetSpawnedTeleportStone() const;

   void AuthoritySetSpawnedTeleportStone(ATATToolWorldActor_RecallingStone* teleportStone);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityRemoveSpawnedTeleportStone();

   /// Gameplay effect that is applied after we've deployed the recalling stone,
   /// which switches us to the secondary fire mode
   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> SecondaryFireAvailableEffect;
};


