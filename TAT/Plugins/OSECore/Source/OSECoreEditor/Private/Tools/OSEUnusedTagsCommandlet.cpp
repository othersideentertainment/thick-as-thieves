// (c) 2023-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEUnusedTagsCommandlet.h"

// ue5
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "Editor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUnusedTagsCommandlet)


UOSEUnusedTagsCommandlet::UOSEUnusedTagsCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

int UOSEUnusedTagsCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   TArray<FString> tokens;
   TArray<FString> switches;
   TMap<FString, FString> params;
   ParseCommandLine(*fullCommandLine, tokens, switches, params);

   int32 targetReferenceCount = 0;
   if(FString* refCountString = params.Find(TEXT("Count")))
   {
      targetReferenceCount = FCString::Atoi(**refCountString);
   }

   _FindUnusedTags(targetReferenceCount);

   return 0;
}

void UOSEUnusedTagsCommandlet::_FindUnusedTags(int32 referenceCount)
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
   IAssetRegistry& assetRegistry = assetRegistryModule.Get();

   FGameplayTagContainer allTags;
   UGameplayTagsManager::Get().RequestAllGameplayTags(allTags, true);

   int32 unusedTagCount = 0;
   TArray<FAssetIdentifier> foundReferencers;

   for (const FGameplayTag& tag : allTags)
   {
      foundReferencers.Reset();
      assetRegistry.GetReferencers(FAssetIdentifier(FGameplayTag::StaticStruct(), tag.GetTagName()), foundReferencers);

      if (foundReferencers.Num() == referenceCount)
      {
         UE_LOG(LogOSECommandlet, Display, TEXT("Tag with %d asset references: %s"), foundReferencers.Num(), *tag.ToString());
         unusedTagCount++;
      }
   }

   UE_LOG(LogOSECommandlet, Display, TEXT("Found %d tags with %d asset references"), unusedTagCount, referenceCount);
   UE_LOG(LogOSECommandlet, Display, TEXT("(reminder this only counts references from other assets)"));
}
