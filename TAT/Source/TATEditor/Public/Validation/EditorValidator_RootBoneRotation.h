// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "EditorValidatorBase.h"
#include "EditorValidator_RootBoneRotation.generated.h"

UCLASS(Config=Editor)
class TATEDITOR_API UEditorValidator_RootBoneRotation : public UEditorValidatorBase
{
   GENERATED_BODY()

public:
   UEditorValidator_RootBoneRotation();

protected:
   // from UEditorValidatorBase
   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

protected:
   UPROPERTY(Config)
   TArray<FString> PathsToExcludeRootMotionRotationValidation;
};
