// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Validation/EditorValidator_MeshCollisionAOE.h"

// ue4
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_MeshCollisionAOE)


#define LOCTEXT_NAMESPACE "AssetValidation"

UEditorValidator_MeshCollisionAOE::UEditorValidator_MeshCollisionAOE()
   : Super()
{
   bIsEnabled = true;
}

bool UEditorValidator_MeshCollisionAOE::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   if (!asset->IsA<UStaticMesh>())
   {
      return false;
   }

   FString pathName = asset->GetPathName(nullptr);
   return PathsToExcludeBlockingAOE.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern);});
}

EDataValidationResult UEditorValidator_MeshCollisionAOE::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   UStaticMesh* meshAsset = CastChecked<UStaticMesh>(asset);
   if (const UBodySetup* bodySetup = meshAsset->GetBodySetup())
   {
      FName defaultCollisionProfile = bodySetup->DefaultInstance.GetCollisionProfileName();
      if (defaultCollisionProfile == BlockAOEProfile)
      {
         AssetFails(asset, FText::Format(LOCTEXT("CollisionValidator_ShouldNotBlock", "Static mesh in a folder for non-wall-like things uses the {0} collision profile. Please change it to {1} (or other, if appropriate)"), FText::FromName(BlockAOEProfile), FText::FromName(AllowAOEProfile)));
         return EDataValidationResult::Invalid;
      }
   }

   AssetPasses(asset);
   return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE

