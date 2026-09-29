// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Traps/Old/TrapEmitterInterface.h"

// ue4
#include "Components/StaticMeshComponent.h"
#include "SimpleTrapEmitterComponent.generated.h"


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API USimpleTrapEmitterComponent_Old : public UStaticMeshComponent, public ITrapEmitterInterface_Old
{
   GENERATED_BODY()

public:   
   // Sets default values for this component's properties
   USimpleTrapEmitterComponent_Old();

public:
   virtual void OnTriggered_Implementation() override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

protected:
   // Perform any server-authoritative or durable effects of the trigger
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta = (BlueprintProtected))
   void AuthorityOnTriggered();

   UFUNCTION(NetMulticast, Unreliable)
   void ClientOnTriggered();

   // Perform any ephemeral effects associated with the effect. If this needs to be persistent/durable, do something else
   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, DisplayName="LocalOnTriggered", meta=(BlueprintProtected, ScriptName= "LocalOnTriggered"))
   void K2_LocalOnTriggered();
};
