// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Validation/EditorValidator_TATAudioComponents.h"

// tat
#include "Audio/TATAudioPortalComponent.h"
#include "Audio/TATAudioRoomComponent.h"

// ose
#include "Utl/OSEBlueprintComponentScrape.h"

// ue5
#include "GameFramework/Actor.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_TATAudioComponents)

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UEditorValidator_TATAudioComponents::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   const UBlueprint* blueprint = Cast<UBlueprint>(asset);
   if (blueprint == nullptr || !blueprint->ParentClass->IsChildOf<AActor>())
   {
      return false;
   }

   // Check that the path isn't one that we want to skip
   const FString pathName = asset->GetPathName(nullptr);
   if (PathsToSkipValidation.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); }))
   {
      return false;
   }

   return true;
}
EDataValidationResult UEditorValidator_TATAudioComponents::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* originalAsset, FDataValidationContext& context)
{
   const UBlueprint* blueprint = CastChecked<UBlueprint>(originalAsset);
   const UClass* generatedClass = blueprint->GeneratedClass;

   const int32 startingNumValidationErorrs = context.GetIssues().Num();

   auto validatePortalComponent = [&, this](const UAkPortalComponent* component)
   {
      if (!component->IsA<UTATAudioPortalComponent>())
      {
         AssetFails(originalAsset, FText::Format(
            LOCTEXT("Validator_TATAudioComponents", "Component {0} is using AkPortalComponent: it needs to use the TAT-level override, TATAudioPortalComponent, instead"),
            FText::FromString(component->GetReadableName())));
      }
   };

   auto validateRoomComponent = [&, this](const UAkRoomComponent* component)
   {
      if (!component->IsA<UTATAudioRoomComponent>())
      {
         AssetFails(originalAsset, FText::Format(
            LOCTEXT("Validator_TATAudioComponents", "Component {0} is using AkRoomComponent: it needs to use the TAT-level override, TATAudioRoomComponent, instead"),
            FText::FromString(component->GetReadableName())));
      }
   };

   BlueprintComponentScrape::FindBlueprintClassComponents<UAkPortalComponent>(generatedClass, validatePortalComponent);
   BlueprintComponentScrape::FindBlueprintClassComponents<UAkRoomComponent>(generatedClass, validateRoomComponent);

   if (const AActor* cdo = generatedClass->GetDefaultObject<AActor>())
   {
      cdo->ForEachComponent<UAkPortalComponent>(false, validatePortalComponent);
      cdo->ForEachComponent<UAkRoomComponent>(false, validateRoomComponent);
   }

   if (context.GetIssues().Num() == startingNumValidationErorrs)
   {
      AssetPasses(originalAsset);
      return EDataValidationResult::Valid;
   }
   else
   {
      return EDataValidationResult::Invalid;
   }
}

#undef LOCTEXT_NAMESPACE

