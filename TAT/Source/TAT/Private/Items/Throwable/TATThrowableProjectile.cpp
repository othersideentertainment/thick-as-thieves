// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Throwable/TATThrowableProjectile.h"

// tat
#include "Character/TATTeams.h"
#include "Abilities/TATGameplayTags.h"

// ue4
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThrowableProjectile)

// Sets default values
ATATThrowableProjectile::ATATThrowableProjectile()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = false;
}

void ATATThrowableProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATThrowableProjectile, InitialVelocity, COND_InitialOnly);
}

uint8 ATATThrowableProjectile::GetTeam() const
{
   if(const IOSETeamInterface* ownerTeamInterface = Cast<IOSETeamInterface>(GetInstigator()))
   {
      // Ignore disguises, the projectiles should use the original team.
      return UTATTeamAttitudeSolver::GetOriginalTeam(ownerTeamInterface->GetTeam());
   }
   return IOSETeamInterface::GetTeam();
}

FGameplayTag ATATThrowableProjectile::GetUtilityAITargetingGroup() const
{
   return _ProjectileTargetingGroup;
}

// Called when the game starts or when spawned
void ATATThrowableProjectile::BeginPlay()
{
   Super::BeginPlay();

   ProjectileMovement->Velocity = InitialVelocity;

   // Ignore collision with instigator and vice versa
   if (AActor* instigator = GetInstigator())
   {
      if (auto myPrimitive = Cast<UPrimitiveComponent>(RootComponent))
      {
         myPrimitive->IgnoreActorWhenMoving(instigator, true);
      }
      if (auto theirPrimitive = Cast<UPrimitiveComponent>(instigator->GetRootComponent()))
      {
         theirPrimitive->IgnoreActorWhenMoving(this, true);
      }
   }
   
}



