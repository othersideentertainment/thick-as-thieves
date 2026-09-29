// (c) 202-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATCombatDamageBonusTargetInterface.generated.h"

UINTERFACE(BlueprintType, MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTATCombatDamageBonusTargetInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATCombatDamageBonusTargetInterface
{
   GENERATED_BODY()

public:
   // can this target be sneak attacked by this actor?
   virtual bool CanBeSneakAttackedByActor(AActor* actor) const = 0;

   // this target has been sneak attacked by this actor
   virtual void OnSneakAttackedByActor(AActor* actor) = 0;

   // can this target be counter attacked by this actor?
   virtual bool CanBeCounterAttackedByActor(AActor* actor) const = 0;

   // this target has been counter attacked by this actor
   virtual void OnCounterAttackedByActor(AActor* actor) = 0;
};
