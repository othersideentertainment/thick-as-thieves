// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverEditorUtilities.h"

// ose
#include "OSEProjectSettings.h"
#include "VoiceOver/OSEVoiceOverLine.h"

// ose editor
#include "VoiceOver/OSEVoiceOverAssetFactories.h"

//wwise
#include "WwiseUnrealHelper.h"

//ue4
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "HAL/PlatformFileManager.h"
#include "GameplayTagsManager.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "Platforms/AkUEPlatform.h"

DEFINE_LOG_CATEGORY_STATIC(LogOSEVoiceOverEditorUtils, Log, All);

namespace VoiceOverEditorUtils {
   static FGameplayTagContainer GetGameplayTagLeafChildren(FGameplayTag baseTag)
   {
      FGameplayTagContainer allChildren = UGameplayTagsManager::Get().RequestGameplayTagChildren(baseTag);

      FGameplayTagContainer result = allChildren;

      for (int32 idx = 0; idx < allChildren.Num(); ++idx)
      {
         FGameplayTag tag = allChildren.GetByIndex(idx);
         FGameplayTag parent = tag.RequestDirectParent();

         if (allChildren.HasTagExact(parent))
         {
            result.RemoveTag(parent);
         }
      }

      return result;
   }

   FString GetVoiceVerbAssetName(const FGameplayTag& verb)
   {
      const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

      check(verb.ToString().StartsWith(settings.VoiceVerbBaseTag.ToString()));
      FString verbTagTail = verb.ToString().RightChop(settings.VoiceVerbBaseTag.ToString().Len());
      TArray<FString> verbTagParts;
      verbTagTail.ParseIntoArray(verbTagParts, TEXT("."));

      return ObjectTools::SanitizeObjectName(TEXT("VOL_") + FString::Join(verbTagParts, TEXT("_")));
   }

   FString GetVoiceVerbPath(const FGameplayTag& verb)
   {
      const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

      check(verb.ToString().StartsWith(settings.VoiceVerbBaseTag.ToString()));
      FString verbTagTail = verb.ToString().RightChop(settings.VoiceVerbBaseTag.ToString().Len());
      TArray<FString> verbTagParts;
      verbTagTail.ParseIntoArray(verbTagParts, TEXT("."));

      return UPackageTools::SanitizePackageName(TEXT("/Game/Audio/Voice/VoiceOverLines/") + verbTagParts[0]);
   }
}


bool FOSEVoiceOverEditorUtilities::PopulateVoiceLine(UOSEVoiceOverLine* line)
{
   if (!IsValid(line))
      return false;

   if (line->VoiceVerb == FGameplayTag::EmptyTag)
   {
      UE_LOG(LogOSEVoiceOverEditorUtils, Error, TEXT("Voice Line is Missing a Voice Verb"));
      return false;
   }

   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

   UDataTable* voiceIdentityInfoTable = settings.VoiceIdentityInfo.LoadSynchronous();

   if (!settings.VoiceIdentityInfo.IsValid())
   {
      UE_LOG(LogOSEVoiceOverEditorUtils, Error, TEXT("Project is Missing Voice Identity Info DataTable"));
      return false;
   }

   UDataTable* voiceVerbToCategory = settings.VoiceVerbToClass.LoadSynchronous();

   if (!settings.VoiceVerbToClass.IsValid())
   {
      UE_LOG(LogOSEVoiceOverEditorUtils, Error, TEXT("Project is Missing Voice Verb To Class DataTable"));
      return false;
   }


   //Fetch the voice verb category.
   FGameplayTagContainer voiceVerbParents = line->VoiceVerb.GetGameplayTagParents();
   if (voiceVerbParents.Num() < 2)
      return false;

   FGameplayTag voiceVerbCategory;
   //Find the tag right before the base tag in the chain and that's our category.
   for (int32 i = 0; i < voiceVerbParents.Num(); ++i)
   {
      if (voiceVerbParents.GetByIndex(i) == settings.VoiceVerbBaseTag && i - 1 >= 0)
      {
         voiceVerbCategory = voiceVerbParents.GetByIndex(i - 1);
      }
   }

   if (voiceVerbCategory == FGameplayTag::EmptyTag)
   {
      UE_LOG(LogOSEVoiceOverEditorUtils, Error, TEXT("Unable to find Voice Verb Base Tag %s in tag %s"), *settings.VoiceVerbBaseTag.ToString(), *line->VoiceVerb.ToString());
      return false;
   }

   //Look up the category in the voiceVerbToCategoryTable
   TArray<FOSEVoiceVerbToCategoryTableRow*> voiceVerbToCategoryRows;
   voiceVerbToCategory->GetAllRows("Auto Populate Voice Over Line", voiceVerbToCategoryRows);

   FGameplayTag voiceIdentityBase;
   for (FOSEVoiceVerbToCategoryTableRow* row : voiceVerbToCategoryRows)
   {
      if (row->VoiceVerb == voiceVerbCategory)
      {
         voiceIdentityBase = row->VoiceIdentity;
         break;
      }
   }

   if (voiceIdentityBase == FGameplayTag::EmptyTag)
   {
      UE_LOG(LogOSEVoiceOverEditorUtils, Error, TEXT("Project is Missing voice identity tag for category: %s"), *voiceVerbCategory.ToString());
      return false;
   }

   UE_LOG(LogOSEVoiceOverEditorUtils, Log, TEXT("VerbCategory: %s -> IdentityBase: %s"), *voiceVerbCategory.ToString(), *voiceIdentityBase.ToString());


   //Get all the voice identities below the identity base, aka the leafs on the gameplay tag graph.
   FGameplayTagContainer identities = VoiceOverEditorUtils::GetGameplayTagLeafChildren(voiceIdentityBase);

   //Load all the voice identity data from the table.
   TArray<FOSEVoiceIdentityInfoTableRow*> voiceIdentityInfoRows;
   voiceIdentityInfoTable->GetAllRows("Auto Populate Voice Over Line", voiceIdentityInfoRows);

   bool changed = false;

   const int32 voiceIdentityBaseTagLength = settings.VoiceIdentityBaseTag.GetGameplayTagParents().Num();

   for (FGameplayTag tag : identities)
   {
      UE_LOG(LogOSEVoiceOverEditorUtils, Log, TEXT("Found Identity: %s"), *tag.ToString());

      FOSEVoiceIdentityInfoTableRow** identityRowPtr = voiceIdentityInfoRows.FindByPredicate([tag](const FOSEVoiceIdentityInfoTableRow* row) {
         return row->VoiceIdentity == tag;
      });

      //If there's no identity data skip this
      if (!identityRowPtr)
      {
         UE_LOG(LogOSEVoiceOverEditorUtils, Error, TEXT("Unable to find Voice Identity Info in Table for %s"), *tag.ToString());
         continue;
      }

      FOSEVoiceIdentityInfoTableRow* identityRow = *identityRowPtr;

      //Construct the file base name based on the identity tag.
      check(tag.ToString().StartsWith(settings.VoiceIdentityBaseTag.ToString()));
      FString identityTagTail = tag.ToString().RightChop(settings.VoiceIdentityBaseTag.ToString().Len());
      TArray<FString> identityTagParts;
      identityTagTail.ParseIntoArray(identityTagParts, TEXT("."));

      check(line->VoiceVerb.ToString().StartsWith(voiceVerbCategory.ToString()));
      FString verbTagTail = line->VoiceVerb.ToString().RightChop(voiceVerbCategory.ToString().Len());
      TArray<FString> verbTagParts;
      verbTagTail.ParseIntoArray(verbTagParts, TEXT("."));

      FString fileBaseName = TEXT("VO_") + FString::Join(identityTagParts, TEXT("_")) + TEXT("_") + FString::Join(verbTagParts, TEXT("_")) + TEXT("_");

      if (!line->Identities.Contains(tag))
      {
         line->Identities.Add(tag);
         changed = true;
      }
   }

   if (changed)
   {
      line->Modify();
   }

   return changed;
}

TArray<UOSEVoiceOverLine*> FOSEVoiceOverEditorUtilities::GenerateVoiceLines()
{
   const UOSEProjectSettings& settings = UOSEProjectSettings::Get();

   FGameplayTagContainer verbs = VoiceOverEditorUtils::GetGameplayTagLeafChildren(settings.VoiceVerbBaseTag);

   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

   TArray<FAssetData> assetDataList;
   assetRegistryModule.Get().GetAssetsByClass(UOSEVoiceOverLine::StaticClass()->GetClassPathName(), assetDataList);

   TArray<UOSEVoiceOverLine*> changedLines;
   for (const FGameplayTag& verb : verbs)
   {
      FAssetData* asset = assetDataList.FindByPredicate([&](const FAssetData& assetData) { return CastChecked<UOSEVoiceOverLine>(assetData.GetAsset())->VoiceVerb == verb; });

      if (!asset)
      {
         //Fall back to matching an existing asset with the appropriate name
         asset = assetDataList.FindByPredicate([&](const FAssetData& assetData) {
            return assetData.AssetName.ToString() == VoiceOverEditorUtils::GetVoiceVerbAssetName(verb);
         });
      }

      UOSEVoiceOverLine* line = nullptr;

      if (!asset)
      {
         //There's no existing asset for this we need to create one.
         FString assetName = VoiceOverEditorUtils::GetVoiceVerbAssetName(verb);
         FString assetPath = VoiceOverEditorUtils::GetVoiceVerbPath(verb);


         FString packageName = ObjectTools::SanitizeInvalidChars(FPaths::Combine(assetPath, assetName), INVALID_LONGPACKAGE_CHARACTERS);
         UPackage* package = CreatePackage(*packageName);
         UOSEVoiceOverLineFactory* lineFactory = NewObject<UOSEVoiceOverLineFactory>();
         UObject* lineObj = lineFactory->FactoryCreateNew(UOSEVoiceOverLine::StaticClass(), package, FName(assetName), RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn);
         if (!IsValid(lineObj))
         {
            UE_LOG(LogOSEVoiceOverEditorUtils, Error, TEXT("Unable to create %s at %s"), *assetName, *assetPath);
            continue;
         }

         assetRegistryModule.Get().AssetCreated(lineObj);
         GEditor->BroadcastObjectReimported(lineObj);

         line = CastChecked<UOSEVoiceOverLine>(lineObj);
         line->VoiceVerb = verb;
         PopulateVoiceLine(line);
         changedLines.Add(line);
      }
      else
      {
         line = CastChecked<UOSEVoiceOverLine>(asset->GetAsset());

         bool changed = false;

         if (line->VoiceVerb != verb && line->VoiceVerb == FGameplayTag::EmptyTag)
         {
            line->VoiceVerb = verb;
            changed = true;
         }

         changed |= PopulateVoiceLine(line);

         if (changed)
         {
            changedLines.Add(line);
         }
      }
   }

   return changedLines;
}
