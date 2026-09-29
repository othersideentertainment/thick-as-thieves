// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/EditorValidator_StaticActorBlueprints.h"

// ose
#include "Utl/OSEBlueprintComponentScrape.h"

// ue5
#include "Components/StaticMeshComponent.h"
#include "Components/LocalLightComponent.h"
#include "Misc/DataValidation.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_StaticActorBlueprints)


#define LOCTEXT_NAMESPACE "AssetValidation"

bool UEditorValidator_StaticActorBlueprints::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   const UBlueprint* blueprint = Cast<UBlueprint>(asset);
   if (blueprint == nullptr || !blueprint->ParentClass->IsChildOf<AActor>())
   {
      return false;
   }

   const FString pathName = asset->GetPathName(nullptr);
   return PathsToValidate.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); }) 
         && !PathsToSkipValidation.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); })
         && !BaseClassesToSkip.ContainsByPredicate([blueprint](const TSoftClassPtr<AActor>& baseClass) { return blueprint->ParentClass->IsChildOf(baseClass.Get()); });
}

EDataValidationResult UEditorValidator_StaticActorBlueprints::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   const UBlueprint* blueprint = CastChecked<UBlueprint>(asset);
   const UClass* generatedClass = blueprint->GeneratedClass;

   auto validateComponent = [&, this](const USceneComponent* component)
   {
      if (!(component->IsA<UStaticMeshComponent>() || component->IsA<ULocalLightComponent>()))
      {
         return;
      }

      if (component->Mobility == EComponentMobility::Movable)
      {
         AssetFails(asset, FText::Format(LOCTEXT("StaticActorBlueprints_MovableComponent", "Component {0} on static actor is movable. Please set mobility to static (or maybe stationary), or exclude from validation"), FText::FromString(component->GetReadableName())));
      }
   };

   BlueprintComponentScrape::FindBlueprintClassComponents<USceneComponent>(generatedClass, validateComponent);

   const AActor* cdo = generatedClass->GetDefaultObject<AActor>();
   if (cdo)
   {
      cdo->ForEachComponent<USceneComponent>(false, validateComponent);
   }

   if (context.GetIssues().Num() == 0)
   {
      AssetPasses(asset);
      return EDataValidationResult::Valid;
   }
   else
   {
      return EDataValidationResult::Invalid;
   }
}

#undef LOCTEXT_NAMESPACE
