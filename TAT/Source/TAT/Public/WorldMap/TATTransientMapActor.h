// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATTransientMapActor.generated.h"

class UTATMapActorComponent;

UCLASS()
class TAT_API ATATTransientMapActor : public AActor
{
   GENERATED_BODY()

   ATATTransientMapActor();

   // From AActor
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void BeginPlay() override;

public:
   UPROPERTY(EditDefaultsOnly, Category = "World Map")
   TObjectPtr<UTATMapActorComponent> MapActorComponent = nullptr;

   // Used to lookup map-registration data
   UPROPERTY(Replicated)
   FGameplayTag TransientMapActorIdentifier;

   // Optional map sprite identifier provided to MapActorComponent on activation
   UPROPERTY(Replicated)
   FGameplayTag MapSpriteOverride;
};
