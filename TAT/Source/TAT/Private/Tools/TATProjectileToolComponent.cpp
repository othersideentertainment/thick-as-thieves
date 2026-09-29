// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATProjectileToolComponent.h"

// tat
#include "Developer/TATToolSettings.h"

// ue5
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATProjectileToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATProjectileToolComponent, Log, All)

bool UTATProjectileToolComponent::AuthorityGetParametersForWorldActor(FGameplayTag usageTag, FTATGearWorldActorParameters& worldActorParams) const
{
   // Configure what kind of optional data the world actor wants in the tool BP by implementing this function
   worldActorParams.WorldActorData = GetToolWorldActorData();

   // TODO: Reconcile the duplication with UTATPlacedActorToolComponent
   if (!usageTag.IsValid())
   {
      UE_LOG(LogTATProjectileToolComponent, Error,
         TEXT("AuthorityGetParametersForWorldActor given empty usage tag on tool '%s'"),
         *GetName());
      return false;
   }

   if (const TSoftClassPtr<AActor>* foundWorldActorClass = WorldActorClasses.Find(usageTag))
   {
      worldActorParams.WorldActorClass = *foundWorldActorClass;
      worldActorParams.ParentToolClass = GetClass();

      return true;
   }
   else
   {
      UE_LOG(LogTATProjectileToolComponent, Error,
         TEXT("Could not find world actor associated with usage tag '%s' on tool '%s': check WorldActorClasses"),
         *usageTag.ToString(),
         *GetName());
      return false;
   }
}

FVector UTATProjectileToolComponent::GetSuggestedProjectileVelocity(FVector startPos, FVector endPos)
{
   FVector launchVelocity = FVector::ZeroVector;
   UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, launchVelocity, startPos, endPos, 0, ArcParam);

   return launchVelocity.GetClampedToMaxSize(MaxInitialSpeed);
}


bool UTATProjectileToolComponent::OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool)
{
   bool superRet = Super::OnEquip_Implementation(prevTool);

   // Kick off an async load for the projectile if we have one, so that we don't have a delay when spawning one in
   if (!ProjectileToSpawn.IsNull())
   {
      UAssetManager::GetStreamableManager().RequestAsyncLoad(ProjectileToSpawn.ToSoftObjectPath());
   }

   return superRet;
}


