// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Tokens/TATTokenEffect_LootDecoy.h"

// tat
#include "Items/Tokens/TATTokenInventoryComponent.h"

// ue
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTokenEffect_LootDecoy)


bool FTATTokenEffect_LootDecoy::TryDropDecoy(const AActor* owningActor, const FTATLootIdentifier& lootType, const FTransform& transform)
{
   UTATTokenInventoryComponent* inventory = UTATTokenInventoryComponent::FromActor(owningActor);
   if(inventory == nullptr)
   {
      return false;
   }

   TTATTokenEffectFindResult<FTATTokenEffect_LootDecoy> found = inventory->FindTokenWithEffect<FTATTokenEffect_LootDecoy>(
      [lootType](const FTATTokenEffect_LootDecoy& effect) { return effect.Loot == lootType; });

   if(!found.IsValid())
   {
      return false;
   }

   TSoftClassPtr<AActor> decoyClass = found.Effect->DecoyActorClass;
   UAssetManager::GetStreamableManager().RequestAsyncLoad(decoyClass.ToSoftObjectPath(),
      [weakWorld = MakeWeakObjectPtr(owningActor->GetWorld()), decoyClass, transform]
      {
         if (UWorld* world = weakWorld.Get())
         {
            world->SpawnActor(decoyClass.Get(), &transform);
         }
      });

   // remove token after spawn/load started
   inventory->AuthorityRemoveTokenAt(found.Index);

   return true;
}

#if WITH_EDITOR
void FTATTokenEffect_LootDecoy::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(!Loot.IsValid())
   {
      reportError(INVTEXT("No Loot Specified"));
   }

   if(DecoyActorClass.IsNull())
   {
      reportError(INVTEXT("No DecoyActorClass"));
   }
}
#endif
