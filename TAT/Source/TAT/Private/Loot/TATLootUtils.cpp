// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATLootUtils.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Damage/TATDamageTypes.h"
#include "Engine/AssetManager.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Loot/TATLootActor.h"
#include "Settings/TATMatchSettings.h"
#include "UI/OSERadialPaintLibrary.h"

// ue
#include "Algo/Count.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootUtils)

// static
bool UTATLootUtils::IsLootIdentifierValid(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier)
{
   return lootIdentifier.IsValid() && UTATLootSettings::Get().FindLootInfo(worldContextObject, lootIdentifier) != nullptr;
}

// static
bool UTATLootUtils::IsLootInstanceValid(const UObject* worldContextObject, const FTATLootInstance& lootInstance)
{
   return IsLootIdentifierValid(worldContextObject, lootInstance.Identifier);
}

// static
bool UTATLootUtils::FindLootMetadata(const UObject* worldContextObject, const FTATLootIdentifier lootIdentifier, FTATLootMetadataBP& lootMetadata)
{
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(worldContextObject, lootIdentifier))
   {
      lootMetadata = FTATLootMetadataBP(*lootInfo);
      lootMetadata.LootValue = UTATLootSettings::Get().GetLootValue(worldContextObject, lootIdentifier);
      return true;
   }

   lootMetadata = FTATLootMetadataBP();
   return false;
}

// static
FTATLootInfo UTATLootUtils::FindLootInfo(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier)
{
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(worldContextObject, lootIdentifier);
   return (lootInfo != nullptr) ? *lootInfo : FTATLootInfo();
}

// static
ETATLootType UTATLootUtils::GetLootType(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier)
{
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(worldContextObject, lootIdentifier);
   return (lootInfo != nullptr) ? lootInfo->LootType : ETATLootType::None;
}

int UTATLootUtils::CountLootOfType(const UObject* worldContextObject, TConstArrayView<FTATLootIdentifier> lootIdentifiers, ETATLootType lootType)
{
   return Algo::CountIf(lootIdentifiers, [worldContextObject, lootType](const FTATLootIdentifier& lootId) { return GetLootType(worldContextObject, lootId) == lootType; });
}

int UTATLootUtils::CountValueLootOfType(const UObject* worldContextObject, const TConstArrayView<FTATLootIdentifier> lootIdentifiers, const ETATLootType lootType)
{
   int totalValue = 0;
   for (const FTATLootIdentifier& lootIdentifier : lootIdentifiers)
   {
      if (GetLootType(worldContextObject, lootIdentifier) == lootType)
      {
         totalValue += UTATLootUtils::GetLootBaseValue(worldContextObject, lootIdentifier);
      }
   }
   return totalValue;
}

// static
bool UTATLootUtils::GetLootStaticMeshDataAsyncNative(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier, const FOnLoadedLootStaticMeshDataFn& onLoadComplete)
{
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(worldContextObject, lootIdentifier))
   {
      // Collect soft references to async load
      TArray<FSoftObjectPath> paths;
      if (!lootInfo->LootStaticMesh.StaticMesh.IsNull())
      {
         paths.Add(lootInfo->LootStaticMesh.StaticMesh.ToSoftObjectPath());
      }
      for (const TSoftObjectPtr<UMaterialInterface>& materialOverride : lootInfo->LootStaticMesh.MaterialOverrides)
      {
         if (!materialOverride.IsNull())
         {
            paths.Add(materialOverride.ToSoftObjectPath());
         }
      }

      // Async load the assets and execute the delegate when done
      if (paths.Num() > 0)
      {
         UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(paths), [onLoadComplete, lootIdentifier, lootMeshData = lootInfo->LootStaticMesh]()
         {
            TArray<UMaterialInterface*> materials;
            materials.SetNum(lootMeshData.MaterialOverrides.Num());
            for (int32 i = 0; i < lootMeshData.MaterialOverrides.Num(); i++)
            {
               materials[i] = lootMeshData.MaterialOverrides[i].Get();
            }
            onLoadComplete(lootIdentifier, lootMeshData.StaticMesh.Get(), materials, lootMeshData.RelativeTransform);
         });
      }
      else
      {
         // All soft references were null - just fire the callback immediately
         onLoadComplete(lootIdentifier, nullptr, {}, lootInfo->LootStaticMesh.RelativeTransform);
      }

      return true;
   }

   return false;
}

// static
bool UTATLootUtils::GetLootStaticMeshDataAsync(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier, FOnLoadedLootStaticMeshData onLoadComplete)
{
   return GetLootStaticMeshDataAsyncNative(worldContextObject, lootIdentifier,
      [onLoadComplete](FTATLootIdentifier lootIdentifier, UStaticMesh* staticMesh, const TArray<UMaterialInterface*>& materialOverrides, const FTransform& relativeTransform)
      {
         onLoadComplete.ExecuteIfBound(lootIdentifier, staticMesh, materialOverrides, relativeTransform);
      });
}

// static
int32 UTATLootUtils::GetLootBaseValue(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier)
{
   return UTATLootSettings::Get().GetLootValue(worldContextObject, lootIdentifier);
}

// static
int32 UTATLootUtils::GetLootInstanceBaseValue(const UObject* worldContextObject, const FTATLootInstance& lootInstance)
{
   return UTATLootSettings::Get().GetLootValue(worldContextObject, lootInstance);
}

// static
FTATLootInstance UTATLootUtils::MakeLootInstance(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier)
{
   //NB. This function is set as the native make function for FTATLootInstance, so changes here affects any blueprint that creates loot instances.
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(worldContextObject, lootIdentifier))
   {
      return lootInfo->CreateDefaultInstance(worldContextObject);
   }
   FTATLootInstance result;
   result.Identifier = lootIdentifier;
   return result;
}

// static
bool UTATLootUtils::RequiresInstanceStorage(const UObject* worldContextObject, FTATLootIdentifier lootIdentifier)
{
   const FTATLootInfo* lootInfo = UTATLootSettings::Get().FindLootInfo(worldContextObject, lootIdentifier);
   return (lootInfo != nullptr) ? lootInfo->RequiresInstanceStorage() : false;
}

// static
FVector UTATLootUtils::CalcLootPinataSpawnLocation(const FVector& pinataLocation, int32 itemIndex, int32 numItems, const FFloatInterval& radiusRange)
{
   const float angleIntervalDegrees = 360.0f / static_cast<float>(numItems);
   const float pinataAngleDegrees = static_cast<float>(itemIndex) * angleIntervalDegrees;
   const float radius = FMath::RandRange(radiusRange.Min, radiusRange.Max);
   return FVector(UOSERadialPaintLibrary::FindPointOnCircle(FVector2D(pinataLocation.X, pinataLocation.Y), radius, pinataAngleDegrees), pinataLocation.Z);
}

// static
ATATLootActor* UTATLootUtils::SpawnLootActor(UWorld* world, const AActor* droppingActor, TSubclassOf<ATATLootActor> lootClass, const FTATLootItemVariant& lootItem, const FVector& suggestedDropLocation)
{
   check(world != nullptr);
   AActor* owner = nullptr;
   APawn* instigator = nullptr;
   FTransform spawnTransform;
   if (ATATLootActor* lootActor = world->SpawnActorDeferred<ATATLootActor>(lootClass, spawnTransform, owner, instigator, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
   {
      lootActor->AuthoritySetupDroppedLootBeforeFinishSpawning(lootItem);
      lootActor->SetCanPlaceCallingCard(false);
      FVector spawnLocation = suggestedDropLocation;
      UTATItemFunctionLibrary::FindDropLocationFromSuggestedStart(lootActor, droppingActor, UTATProjectSettings::Get().ItemDropTraceProfile, suggestedDropLocation, spawnLocation);
      spawnTransform = FTransform(spawnLocation);
      
      lootActor->FinishSpawning(spawnTransform);
      return lootActor;
   }
   return nullptr;
}

void UTATLootUtils::SpawnLootActorAsync(UWorld* world, const AActor* droppingActor, const TSoftClassPtr<ATATLootActor>& lootClass, const FTATLootItemVariant& lootItem, const FVector& suggestedDropLocation)
{
   check(world != nullptr);
   if (lootClass.IsValid())
   {
      SpawnLootActor(world, droppingActor, lootClass.Get(), lootItem, suggestedDropLocation);
      return;
   }

   UAssetManager::GetStreamableManager().RequestAsyncLoad(lootClass.ToSoftObjectPath(),
      [weakWorld = MakeWeakObjectPtr(world), weakDroppingActor = MakeWeakObjectPtr(droppingActor), lootClass, lootItem, suggestedDropLocation]
      {
         // Intentionally not checking weakDroppingActor - that's allowed to be null
         if (weakWorld.IsValid())
         {
            SpawnLootActor(weakWorld.Get(), weakDroppingActor.Get(), lootClass.Get(), lootItem, suggestedDropLocation);
         }
      });
}
