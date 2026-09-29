// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Damage/TATDamageAssetSubsystem.h"

// tat
#include "Damage/TATDamageSettings.h"
#include "Developer/TATProjectSettings.h"

// ue5
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDamageAssetSubsystem)

void UTATDamageAssetSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   // TODO: This has already broken the rule by adding a second settings class
   //       If there is a third one, instead data-drive the settings classes to
   //       preload from (plus interface or something).
   TArray<FSoftObjectPath> pathsToLoad;
   UTATDamageSettings::Get().AppendEffectsToPreload(pathsToLoad);
   UTATProjectSettings::Get().AppendEffectsToPreload(pathsToLoad);
   _loadingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), FStreamableDelegate());
}

void UTATDamageAssetSubsystem::Deinitialize()
{
   if (_loadingHandle)
   {
      _loadingHandle->ReleaseHandle();
      _loadingHandle.Reset();
   }

   Super::Deinitialize();
}
