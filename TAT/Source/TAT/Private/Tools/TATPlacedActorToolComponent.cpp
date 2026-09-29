// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATPlacedActorToolComponent.h"

// ue5
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlacedActorToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATPlacedActorToolComponent, Log, All)

bool UTATPlacedActorToolComponent::AuthorityGetParametersForWorldActor(FGameplayTag usageTag, FTATGearWorldActorParameters& worldActorParams) const
{
   // TODO: Reconcile the duplication with UTATProjectileToolComponent
   if (!usageTag.IsValid())
   {
      UE_LOG(LogTATPlacedActorToolComponent, Error,
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
      UE_LOG(LogTATPlacedActorToolComponent, Error,
         TEXT("Could not find world actor associated with usage tag '%s' on tool '%s': check WorldActorClasses"),
         *usageTag.ToString(),
         *GetName());
      return false;
   }
}


bool UTATPlacedActorToolComponent::OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool)
{
   bool superRet = Super::OnEquip_Implementation(prevTool);

   // Kick off an async load for any world actors we may spawn, so that we don't have a delay
   for (const TPair<FGameplayTag, TSoftClassPtr<AActor>>& worldActorAndUsage : WorldActorClasses)
   {
      UAssetManager::GetStreamableManager().RequestAsyncLoad(worldActorAndUsage.Value.ToSoftObjectPath());
   }

   return superRet;
}


