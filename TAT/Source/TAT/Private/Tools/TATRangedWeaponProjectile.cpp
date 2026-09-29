// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATRangedWeaponProjectile.h"

// tat
#include "Damage/TATDamageFunctionLibrary.h"
#include "Combat/TATCombatSettings.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

// ue5
#include "GameplayTagAssetInterface.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATRangedWeaponProjectile)

void ATATRangedWeaponProjectile::BeginPlay()
{
   Super::BeginPlay();

   GetProjectileMovement()->OnProjectileStop.AddUniqueDynamic(this, &ThisClass::ProjectileHasHitObject);
}

void ATATRangedWeaponProjectile::ProjectileHasHitObject(const FHitResult& impact)
{
   BP_OnProjectileStop(impact);

   if (HasAuthority())
   {
      AActor* targetActor = impact.GetActor();
      bool shouldDealDamage = true;
      if (IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(targetActor))
      {
         const UTATCombatSettings& combatSettings = UTATCombatSettings::Get();
         shouldDealDamage = !tagInterface->HasMatchingGameplayTag(combatSettings.IsShieldedTag);
      }
      if (shouldDealDamage)
      {
         UTATDamageFunctionLibrary::DealDamage(GetInstigator(), targetActor, Damage, GetActorLocation(), impact);
      }

      if (HitGameplayEffect && targetActor != nullptr)
      {
         if (UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(targetActor))
         {
            constexpr float effectLevel = 1.0f;

            FGameplayEffectContextHandle ctx = asc->MakeEffectContext();
            AActor* owner = GetOwner();
            ctx.AddInstigator(owner, owner);

            FTATGameplayEffectSetByCallerParam::ApplyGameplayEffectWithParams(asc, HitGameplayEffect, HitGameplayEffectSetByCallerParams, effectLevel, ctx);
         }
      }

      Destroy();
   }
}

