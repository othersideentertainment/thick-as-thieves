// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATStringTablePreloadSubsystem.h"

// ue
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/AssetManager.h"
#include "Internationalization/StringTable.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStringTablePreloadSubsystem)

namespace StringTableCVars
{
   static TAutoConsoleVariable<bool> PreloadStringTables(
      TEXT("TAT.StringTables.Preload"),
      true,
      TEXT("Whether to preload all string tables at startup")
      );

   // Not sure if this is needed or not yet
   static TAutoConsoleVariable<bool> PreloadOnServer(
      TEXT("TAT.StringTables.Preload.DedicatedServer"),
      false,
      TEXT("Whether to preload string tables on dedicated servers")
      );

   // Nothing pre-loaded is currently accessed early enough to
   // meaningfully race in a packaged build, but it may be in PIE,
   // since you can load directly into a match. We could sync load
   // on PIE only, but it may not be worth the risk of masking bugs.
   static TAutoConsoleVariable<bool> PreloadAsync(
      TEXT("TAT.StringTables.Preload.Async"),
      false,
      TEXT("Whether to async load the string tables (rather than sync)")
   );
}

bool UTATStringTablePreloadSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   if (!StringTableCVars::PreloadStringTables.GetValueOnGameThread())
   {
      return false;
   }

   const UGameInstance* gameInstance = CastChecked<UGameInstance>(outer);
   if (gameInstance->IsDedicatedServerInstance() && !StringTableCVars::PreloadOnServer.GetValueOnGameThread())
   {
      return false;
   }

   return true;
}

void UTATStringTablePreloadSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   FARFilter filter;
   filter.ClassPaths.Add(UStringTable::StaticClass()->GetClassPathName());
   filter.bIncludeOnlyOnDiskAssets = true;
   
   TArray<FSoftObjectPath> stringTablesToLoad;
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATStringTablePreloadSubsystem::Initialize::FindStringTables)
      const IAssetRegistry& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
      assetRegistry.EnumerateAssets(filter, [&stringTablesToLoad](const FAssetData& assetData)
         {
            stringTablesToLoad.Add(assetData.GetSoftObjectPath());
            return true;
         });
   }

   if (StringTableCVars::PreloadAsync.GetValueOnGameThread())
   {
      _streamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(stringTablesToLoad), FStreamableDelegate(),
         FStreamableManager::DefaultAsyncLoadPriority,
         true);
   }
   else
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATStringTablePreloadSubsystem::LoadSync)
      _streamHandle = UAssetManager::GetStreamableManager().RequestSyncLoad(MoveTemp(stringTablesToLoad), true);
   }
   
}

void UTATStringTablePreloadSubsystem::Deinitialize()
{
   _streamHandle.Reset();
   Super::Deinitialize();
}
