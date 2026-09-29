// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Breakables/TATBreakableActorFwd.h"
#include "TATBreakableActorInfoInterface.h"

// ue5
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATBreakableBase.generated.h"

// _An_ example actor that includes the breakable component
//
// Not intended to be the sole user of it, but may be a convenience
// base class in cases where there is not already a native base class
UCLASS(Blueprintable)
class TAT_API ATATBreakableBase : public AActor
   , public IAbilitySystemInterface
   , public IGameplayTagAssetInterface //< breakable
   , public ITATBreakableActorInfoInterface
{
   GENERATED_BODY()
   
public:	
   // Sets default values for this actor's properties
   ATATBreakableBase();

   BREAKABLE_ACTOR_DECLARATIONS()
protected:
   // Breakable boilerplate
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UAbilitySystemComponent> _abilitySystemComponent = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TObjectPtr<UTATBreakableComponent> _breakableComponent = nullptr;
};
