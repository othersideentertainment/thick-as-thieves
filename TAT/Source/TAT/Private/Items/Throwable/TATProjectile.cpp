// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/Throwable/TATProjectile.h"

// tat
#include "Character/TATTeams.h"
#include "Abilities/TATGameplayTags.h"

// ue
#include "GameFramework/ProjectileMovementComponent.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATProjectile)

void ATATProjectile::BeginPlay()
{
   Super::BeginPlay();

   GetProjectileMovement()->ProjectileGravityScale = InitialReplicatedGravityScale;
}

void ATATProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATProjectile, InitialReplicatedGravityScale, COND_InitialOnly);
}

uint8 ATATProjectile::GetTeam() const
{
   if(const IOSETeamInterface* ownerTeamInterface = Cast<IOSETeamInterface>(GetInstigator()))
   {
      // Ignore disguises, the projectiles should use the original team.
      return UTATTeamAttitudeSolver::GetOriginalTeam(ownerTeamInterface->GetTeam());
   }
   return IOSETeamInterface::GetTeam();
}

FGameplayTag ATATProjectile::GetUtilityAITargetingGroup() const
{
   return _ProjectileTargetingGroup;
}

