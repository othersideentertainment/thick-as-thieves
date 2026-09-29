// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "Interactables/Electrical/TATPowerJunction.h"

// tat
#include "Damage/TATDamageTypes.h"
#include "Interactables/Electrical/TATPowerNetworkComponent.h"
#include "Interactables/Electrical/TATPowerLine.h"
#include "Interactables/Electrical/TATPowerNetworkSubsystem.h"

// ue
#include "Components/StaticMeshComponent.h"
#include "Logging/MessageLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPowerJunction)

ATATPowerJunction::ATATPowerJunction()
{
   bReplicates = true;
   NetDormancy = DORM_Initial;

   PowerNetworkComponent = CreateDefaultSubobject<UTATPowerNetworkComponent>(TEXT("PowerNetworkComponent"));
   PowerNetworkComponent->PowerNetworkRole = ETATPowerNetworkRole::Junction;
}

void ATATPowerJunction::OnConstruction(const FTransform& transform)
{
   Super::OnConstruction(transform);

#if WITH_EDITOR
   // This makes it easier to move junctions by allowing connected power lines to automatically update their spline components to match the new location.
   // It's a little janky in that it requires ATATPowerJunction to be directly aware of ATATPowerLine instead of working exclusively through the PowerNetworkComponent,
   // but it's just here as a small editor-only quality-of-life improvement.
   if (UTATPowerNetworkSubsystem* powerNetworkSubsystem = GetWorld()->GetSubsystem<UTATPowerNetworkSubsystem>())
   {
      for (const TWeakObjectPtr<AActor>& conn : powerNetworkSubsystem->GetReverseConnections(this))
      {
         ATATPowerLine* powerLine = Cast<ATATPowerLine>(conn.Get());
         if (powerLine != nullptr && powerLine->AutoApplySplineCableSimulation)
         {
            powerLine->RunCableSimulationAndApplyToSpline();
         }
      }
   }
#endif
}

void ATATPowerJunction::BeginPlay()
{
   Super::BeginPlay();
}

void ATATPowerJunction::EndPlay(const EEndPlayReason::Type reason)
{
   Super::EndPlay(reason);
}

TOptional<FVector> ATATPowerJunction::GetPowerNetworkWorldLocationForConnectionIndex(int32 index, bool forDebugVis) const
{
   FVector loc = GetActorLocation();
   if (forDebugVis || GetConnectorWorldLocation(index, loc))
   {
      return loc;
   }
   return NullOpt;
}

void ATATPowerJunction::AuthorityHandleDamage_Implementation(const FTATDamageWithType& damage, const FTATSimpleDamageSource& damageSource)
{
   if (TriggerPowerSurgeOnDamage && damage.DamageAmount > 0)
   {
      if (PowerNetworkComponent->AuthorityTriggerPowerSurge(PowerSurgeDuration))
      {
         _MulticastPowerSurgeStarted(damageSource.SourceActor.Get(), damageSource.Origin);
      }
   }
}

bool ATATPowerJunction::GetConnectorWorldLocation(int32 connectorIndex, FVector& worldLocation) const
{
   if (!PowerNetworkConnectorOffsets.IsValidIndex(connectorIndex))
   {
      worldLocation = GetActorLocation();
      return false;
   }
   worldLocation = GetActorTransform().TransformPosition(PowerNetworkConnectorOffsets[connectorIndex]);
   return true;
}

void ATATPowerJunction::_MulticastPowerSurgeStarted_Implementation(AActor* powerSurgeInstigator, const FVector& origin)
{
   if (GetNetMode() != NM_DedicatedServer)
   {
      OnTriggeredPowerSurgeCosmetic(powerSurgeInstigator, origin);
   }
}
