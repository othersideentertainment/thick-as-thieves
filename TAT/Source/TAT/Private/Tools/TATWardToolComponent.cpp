// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATWardToolComponent.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_Ward.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWardToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATWardToolComponent, Log, All)

#if WITH_EDITOR
EDataValidationResult UTATWardToolComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!GetClass()->HasAnyClassFlags(CLASS_Abstract))
   {
      if (!DeployedWardEffect)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Ward Tool '%s' does not define a valid DeployedWardEffect"), *GetName())));
         result = EDataValidationResult::Invalid;
      }
   }

   return result;
}
#endif

bool UTATWardToolComponent::OnRemoveFromToolSet_Implementation()
{
   if (GetOwner() && GetOwner()->HasAuthority())
   {
      AuthorityRemoveAllSpawnedWards();
   }

   return Super::OnRemoveFromToolSet_Implementation();
}

// static
bool UTATWardToolComponent::IsValidWardableTarget(AActor* target)
{
   if (target == nullptr)
   {
      return false;
   }

   if (!target->Implements<UTATWardableInterface>())
   {
      return false;
   }

   return ITATWardableInterface::Execute_CanActivateWard(target);
}

ATATToolWorldActor_Ward* UTATWardToolComponent::AuthoritySpawnWardActor(AActor* targetActor, TSubclassOf<ATATToolWorldActor_Ward> wardClass)
{
   check(GetOwner() && GetOwner()->HasAuthority());
   if (targetActor != nullptr && targetActor->Implements<UTATWardableInterface>())
   {
      const FTATWardPlacementInfo placementInfo = ITATWardableInterface::Execute_GetWardPlacementInfo(targetActor);
      if (placementInfo.IsValid())
      {
         return AuthoritySpawnWardActorWithPlacementInfo(targetActor, wardClass, placementInfo);
      }
   }
   return nullptr;
}

ATATToolWorldActor_Ward* UTATWardToolComponent::AuthoritySpawnWardActorWithPlacementInfo(AActor* targetActor, TSubclassOf<ATATToolWorldActor_Ward> wardClass, const FTATWardPlacementInfo& placementInfo)
{
   check(GetOwner() && GetOwner()->HasAuthority());

   if (targetActor == nullptr || !wardClass)
   {
      return nullptr;
   }

   const FTransform transform{ placementInfo.Rotation.Quaternion(), placementInfo.WorldLocation };
   APawn* instigator = Cast<APawn>(GetOwner());
   ATATToolWorldActor_Ward* ward = GetWorld()->SpawnActorDeferred<ATATToolWorldActor_Ward>(
      wardClass,
      transform,
      GetOwner(),
      instigator,
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
      ESpawnActorScaleMethod::OverrideRootScale);

   ACharacter* owningCharacter = Cast<ACharacter>(GetOwner());
   ward->AuthoritySetupWardBeforeFinishSpawning(targetActor, placementInfo.BoxExtent, (owningCharacter != nullptr) ? owningCharacter->GetPlayerState() : nullptr);

   FTATGearWorldActorParameters params{};
   params.WorldActorClass = wardClass;
   params.ParentToolClass = GetClass();
   ITATGearWorldActorInterface::Execute_AuthorityDeploy(ward, params);

   ward->FinishSpawning(transform);

   return ward;
}

TArray<ATATToolWorldActor_Ward*> UTATWardToolComponent::GetAllSpawnedWards() const
{
   TArray<ATATToolWorldActor_Ward*> result;
   // DeployedWardEffect is checked in IsDataValid
   if (ensure(DeployedWardEffect))
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         TArray<FActiveGameplayEffectHandle> gameplayEffectHandles = UOSEAbilityFunctionLibrary::GetActiveEffectsByClass(asc, DeployedWardEffect);
         result.Reserve(gameplayEffectHandles.Num());
         for (const FActiveGameplayEffectHandle& handle : gameplayEffectHandles)
         {
            if (ATATToolWorldActor_Ward* ward = _GetWardActorFromEffectHandle(asc, handle))
            {
               result.Add(ward);
            }
         }
      }
   }
   return result;
}

ATATToolWorldActor_Ward* UTATWardToolComponent::GetFirstSpawnedWard() const
{
   // DeployedWardEffect is checked in IsDataValid
   if (ensure(DeployedWardEffect))
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         // Find the active effect that the ward applied, if it's present
         TArray<FActiveGameplayEffectHandle> gameplayEffectHandles = UOSEAbilityFunctionLibrary::GetActiveEffectsByClass(asc, DeployedWardEffect);
         return !gameplayEffectHandles.IsEmpty() ? _GetWardActorFromEffectHandle(asc, gameplayEffectHandles[0]) : nullptr;
      }
   }
   return nullptr;
}

void UTATWardToolComponent::AuthorityAddSpawnedWard(ATATToolWorldActor_Ward* ward)
{
   check(GetOwner() && GetOwner()->HasAuthority());
   if (ward == nullptr)
   {
      UE_LOG(LogTATWardToolComponent, Error, TEXT("AuthoritySetSpawnedWard got a null ward to add"));
      return;
   }

   // DeployedWardEffect is checked in IsDataValid
   if (ensure(DeployedWardEffect))
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         if (ward)
         {
            FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
            effectContext.AddInstigator(GetOwner(), ward);
            asc->ApplyGameplayEffectToSelf(DeployedWardEffect.GetDefaultObject(), 0.0f, effectContext);
         }
      }
   }
}

void UTATWardToolComponent::AuthorityRemoveSpawnedWard(ATATToolWorldActor_Ward* ward)
{
   check(GetOwner() && GetOwner()->HasAuthority());
   if (ward == nullptr)
   {
      UE_LOG(LogTATWardToolComponent, Error, TEXT("AuthorityRemoveSpawnedWard got a null ward to remove"));
      return;
   }

   // DeployedWardEffect is checked in IsDataValid
   if (ensure(DeployedWardEffect))
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
      {
         TArray<FActiveGameplayEffectHandle> gameplayEffectHandles = UOSEAbilityFunctionLibrary::GetActiveEffectsByClass(asc, DeployedWardEffect);
         for (const FActiveGameplayEffectHandle& handle : gameplayEffectHandles)
         {
            ATATToolWorldActor_Ward* effectWard = _GetWardActorFromEffectHandle(asc, handle);
            if (effectWard == ward)
            {
               constexpr int32 stacksToRemove = 1;
               asc->RemoveActiveGameplayEffect(handle, stacksToRemove);
               return;
            }
         }
         UE_LOG(LogTATWardToolComponent, Warning, TEXT("AuthorityRemoveSpawnedWard failed to find active gameplay effect for ward actor '%s'"), *GetNameSafe(ward));
      }
   }
}

void UTATWardToolComponent::AuthorityRemoveAllSpawnedWards()
{
   check(GetOwner() && GetOwner()->HasAuthority());

   // Remove any world actors we spawned in
   for (ATATToolWorldActor_Ward* ward : GetAllSpawnedWards())
   {
      AuthorityRemoveSpawnedWard(ward);
      ward->Destroy();
   }
}

// static
ATATToolWorldActor_Ward* UTATWardToolComponent::_GetWardActorFromEffectHandle(UAbilitySystemComponent* asc, const FActiveGameplayEffectHandle& handle)
{
   check(asc != nullptr);
   if (const FActiveGameplayEffect* activeEffect = asc->GetActiveGameplayEffect(handle))
   {
      // If we have an effect with the right class, get the effect causer - that will be the reference to the ward actor we placed in the world earlier
      AActor* effectCauser = activeEffect->Spec.GetEffectContext().GetEffectCauser();
      if (ATATToolWorldActor_Ward* ward = Cast<ATATToolWorldActor_Ward>(effectCauser))
      {
         return ward;
      }
      else
      {
         UE_LOG(LogTATWardToolComponent, Error, TEXT("_GetWardActorFromEffectHandle: found deployed ward gameplay effect, but it did not have a valid effect causer ('%s')"),
            *GetNameSafe(effectCauser));
      }
   }
   return nullptr;
}
