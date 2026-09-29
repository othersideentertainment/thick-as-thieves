// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "RespawnAreas/TATRespawnMarkerLocationInterface.h"

// ue4
#include "GameFramework/PlayerStart.h"

#include "TATPlayerStart.generated.h"

UCLASS(BlueprintType, HideCategories = (Object, Rendering, Replication, Input, LOD, Cooking))
class TAT_API ATATPlayerStart : public APlayerStart, public ITATRespawnMarkerLocationInterface
{
   GENERATED_BODY()

public:
   ATATPlayerStart(const FObjectInitializer& objectInitializer);

   // from AActor
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;

   int GetAreaIndex() const { return AreaIndex; }

   virtual FVector GetRespawnMarkerLocation() const override;

private:
   UPROPERTY(EditInstanceOnly, Category = "TAT Player Start")
   int AreaIndex = 0;

   // Optional actor that whose location will be used as the position for the respawn marker if a player starts here
   UPROPERTY(EditInstanceOnly, Category = "TAT Player Start")
   TObjectPtr<AActor> RespawnMarkerLocationProxy;
};
