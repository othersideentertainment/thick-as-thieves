// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATTeleportationPortal.generated.h"

UCLASS()
class TAT_API ATATTeleportationPortal : public AActor
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATTeleportationPortal();

   // From AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Teleport|TAT")
   void SetTeleportDestination(FVector teleportDestination)
   {
      _teleportDestination = teleportDestination;
   }

   UFUNCTION(BlueprintPure, Category = "Teleport|TAT")
   FVector GetTeleportDestination()
   {
      return _teleportDestination;
   }

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "Teleport|TAT", meta = (Categories = "Ability"))
   FGameplayTag TeleportEventTag;

private:
   UPROPERTY(Replicated, Transient)
   FVector _teleportDestination;
};


