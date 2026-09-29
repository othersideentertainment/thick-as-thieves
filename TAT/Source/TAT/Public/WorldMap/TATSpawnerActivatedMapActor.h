// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "TATSpawnerActivatedMapActor.generated.h"

class UTATMapActorComponent;

// Actor with a TATMapActorComponent manually activated by calling into AuthoritySetMapTrackingEnabled(). 
// Designed for use with our authority-only ATATActorSpawner classes (as they don't exist on client), but 
// can be driven by other actors too.
UCLASS(Blueprintable)
class TAT_API ATATSpawnerActivatedMapActor : public AActor
{
   GENERATED_BODY()

   ATATSpawnerActivatedMapActor();

   // from UActorComponent
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
   UFUNCTION(BlueprintCallable, Category = "Map")
   void AuthoritySetMapTrackingEnabled(bool enabled);

private:
   UFUNCTION()
   void _OnRep_ShouldRegisterWithMap();

   void _RefreshMapRegistration();

private:
   // Replicated member assigned to true if actor is successfully spawned
   UPROPERTY(ReplicatedUsing=_OnRep_ShouldRegisterWithMap)
   bool _shouldRegisterWithMap = false;

   UPROPERTY(EditAnywhere, Category = "TAT|Map")
   UTATMapActorComponent* _mapActorComponent = nullptr;
};
