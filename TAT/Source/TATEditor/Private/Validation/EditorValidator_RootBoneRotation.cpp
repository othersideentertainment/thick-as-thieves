// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Validation/EditorValidator_RootBoneRotation.h"

// ue4
#include "Animation/AnimSequence.h"
#include "AnimationBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_RootBoneRotation)

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace RootBoneRotationValidationHelpers
{
   static const FName kRootBoneName = TEXT("root");
}

UEditorValidator_RootBoneRotation::UEditorValidator_RootBoneRotation()
   : Super()
{
   // NOTE: This was accidentally off because of a bug, but if actually enabled,
   //       basically everything fails. So leaving it disabled
   bIsEnabled = false;
}

bool UEditorValidator_RootBoneRotation::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   if (!asset->IsA<UAnimSequence>())
      return false;

   const UAnimSequence& animSeq = *CastChecked<UAnimSequence>(asset);

   if (!animSeq.bEnableRootMotion)
      return false;

   FString pathName = asset->GetPathName(nullptr);
   return !PathsToExcludeRootMotionRotationValidation.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); });
}

EDataValidationResult UEditorValidator_RootBoneRotation::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   const UAnimSequence& animSeq = *CastChecked<UAnimSequence>(asset);
   const IAnimationDataModel* dataModel = animSeq.GetDataModel();
   check(dataModel);

   const int numFrames = dataModel->GetNumberOfKeys();
   TArray<FTransform> boneTransforms;
   dataModel->GetBoneTrackTransforms(RootBoneRotationValidationHelpers::kRootBoneName, boneTransforms);

   for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx)
   {
      FRotator rotator = boneTransforms[frameIdx].Rotator();

      // allow yaw, but not pitch/roll rotations
      if (rotator.Pitch != 0.0f || rotator.Roll != 0.0f)
      {
         FNumberFormattingOptions numFmtOpts;
         numFmtOpts.AlwaysSign = true;
         numFmtOpts.MinimumFractionalDigits = 5;

         AssetFails(asset, FText::Format(LOCTEXT("RootBoneRotationValidator_RotatingRootBone", "Root motion animation sequence rotates the root bone pitch ({0}) or roll ({1}) (found at frame {2})"), FText::AsNumber(rotator.Pitch, &numFmtOpts), FText::AsNumber(rotator.Roll, &numFmtOpts), FText::AsNumber(frameIdx)));
         return EDataValidationResult::Invalid;
      }
   }
   
   AssetPasses(asset);
   return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE

