// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Interactables/Electrical/TATPowerNetworkInterface.h"

// ose
#include "Damage/TATSimpleDamageableInterface.h"

#include "TATPowerJunction.generated.h"

class UTATPowerNetworkComponent;

/// Power junction box that connects power lines together.
UCLASS(Blueprintable)
class TAT_API ATATPowerJunction
   : public AActor
   , public ITATPowerNetworkInterface
   , public ITATSimpleDamageableInterface
{
   GENERATED_BODY()

public:
   ATATPowerJunction();

protected:
   virtual void OnConstruction(const FTransform& transform) override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;

public:
   // ITATPowerNetworkInterface
   virtual UTATPowerNetworkComponent* GetPowerNetworkComponent() const override { return PowerNetworkComponent; }
   virtual TOptional<FVector> GetPowerNetworkWorldLocationForConnectionIndex(int32 index, bool forDebugVis) const override;

   // ITATSimpleDamageableInterface
   virtual void AuthorityHandleDamage_Implementation(const FTATDamageWithType& damage, const FTATSimpleDamageSource& damageSource) override;

   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Power Junction")
   TObjectPtr<UTATPowerNetworkComponent> PowerNetworkComponent;

   /// Power network connection points that other actors can use to determine where to physically connect to this one
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Junction", Meta = (MakeEditWidget = true))
   TArray<FVector> PowerNetworkConnectorOffsets;

   /// Send a power surge through the power network on any damage event
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Junction")
   bool TriggerPowerSurgeOnDamage = false;

   /// How long should the power surge last?
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Junction", Meta = (EditCondition = "TriggerPowerSurgeOnDamage", ForceUnits = "s", UIMin = "0.01", ClampMin = "0.01"))
   float PowerSurgeDuration = 3.0f;

   /// Disable the power network component when broken. This prevents power from flowing through it to other parts of the power network.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Junction")
   bool DisablePowerNetworkComponentWhenBroken = true;

   /// Gets a power network connector offset transformed into world space
   UFUNCTION(BlueprintPure, Category = "Power Junction")
   bool GetConnectorWorldLocation(int32 connectorIndex, FVector& worldLocation) const;

   /// Called on clients when this junction initiated a power surge to display VFX and SFX
   UFUNCTION(BlueprintImplementableEvent, Category = "Power Junction")
   void OnTriggeredPowerSurgeCosmetic(AActor* powerSurgeInstigator, const FVector& origin);

private:
   UFUNCTION(NetMulticast, Unreliable)
   void _MulticastPowerSurgeStarted(AActor* powerSurgeInstigator, const FVector& origin);
};
