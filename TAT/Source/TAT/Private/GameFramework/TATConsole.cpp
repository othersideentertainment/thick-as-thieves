// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATConsole.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATCheatManager.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"

// ue5
#include "GameplayTagsSettings.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATConsole)

namespace ConsoleHelpers
{
   static void AddForceVariantAutoComplete(TArray<FAutoCompleteCommand>& list)
   {
      FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
      FARFilter filter;
      filter.ClassPaths.Add(UTATSceneVariantConfig::StaticClass()->GetClassPathName());

      assetRegistryModule.Get().EnumerateAssets(filter, [&list](const FAssetData& assetData) {
         FAutoCompleteCommand& command = list.Emplace_GetRef();
         command.Command = FString::Format(TEXT("ForceVariant {0}"), {assetData.AssetName.ToString() });
         command.Desc = FString::Format(TEXT("Adds {0} to the list of variant overrides"), { assetData.AssetName.ToString() });
         return true;
         });
   }

   static void AddUnlockAutoComplete(TArray<FAutoCompleteCommand>& list)
   {
      const UGameplayTagsSettings* tagSettings = GetDefault<UGameplayTagsSettings>();
      const FGameplayTagCategoryRemap* remap = tagSettings->CategoryRemapping.FindByPredicate([](const FGameplayTagCategoryRemap& remap) {return remap.BaseCategory == TEXTVIEW("UnlockableCategory");});
      if(remap)
      {
         for(const FString& prefix : remap->RemapCategories)
         {
            const FGameplayTag prefixTag = FGameplayTag::RequestGameplayTag(FName(prefix), false);
            if(!prefixTag.IsValid())
            {
               continue;
            }
            FGameplayTagContainer container = UGameplayTagsManager::Get().RequestGameplayTagChildren(prefixTag);
            for(const FGameplayTag& unlockableTag : container)
            {
               list.Emplace_GetRef() .Command = FString::Format(TEXT("UnlockAdd {0}"), { unlockableTag.ToString() });
               list.Emplace_GetRef() .Command = FString::Format(TEXT("UnlockRemove {0}"), { unlockableTag.ToString() });
            }
         }
      }
   }
}

void UTATConsole::AugmentRuntimeAutoCompleteList(TArray<FAutoCompleteCommand>& list)
{
   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   UClass* cheatClass = settings.ProjectCheatManagerSubclass.LoadSynchronous();
   check(cheatClass);

   for (TFieldIterator<const UField> funcIt(cheatClass); funcIt; ++funcIt)
   {
      if (const UFunction* func = Cast<const UFunction>(*funcIt))
      {
         if (func->HasAnyFunctionFlags(FUNC_Exec) && cheatClass->IsChildOf(func->GetOuterUClassUnchecked()))
         {
            FAutoCompleteCommand cmd = _GenerateFuncAutoComplete(func, false);
            FAutoCompleteCommand serverCmd = _GenerateFuncAutoComplete(func, true);
            list.Add(cmd);
            list.Add(serverCmd);
         }
      }
   }

   ConsoleHelpers::AddForceVariantAutoComplete(list);
   ConsoleHelpers::AddUnlockAutoComplete(list);
}

FAutoCompleteCommand UTATConsole::_GenerateFuncAutoComplete(const UFunction* func, bool isServerExec)
{
   FAutoCompleteCommand cmd;
   cmd.Color = isServerExec ? FColor::Cyan : FColor::Green;
   cmd.Command = FString::Printf(TEXT("%s%s%s"),
      isServerExec ? TEXT("ServerExec ") : TEXT(""),
      *UTATCheatManager::kTATCheatPrefix,
      *func->GetName());

   // build a help string
   // append each property (and it's type) to the help string
   FString desc;
   for (TFieldIterator<FProperty> propIt(func); propIt && (propIt->PropertyFlags & CPF_Parm); ++propIt)
   {
      FProperty* prop = *propIt;
      desc += FString::Printf(TEXT("%s[%s] "), *prop->GetName(), *prop->GetCPPType());
   }
   cmd.Desc = desc;
   return cmd;
}

