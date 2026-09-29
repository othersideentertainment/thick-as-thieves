// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "EditorValidatorBase.h"

#include "EditorValidator_MixedTransparencyMeshes.generated.h"

// Checks for meshes that have a mix of transparent and opaque materials, since that can prevent enabling nanite
UCLASS(Config = Editor)
class TATEDITOR_API UEditorValidator_MixedTransparencyMeshes : public UEditorValidatorBase
{
   GENERATED_BODY()

public:

   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

   UPROPERTY(Config)
   int32 MinTrianglesToWarnAboutTransparency = 50;

   UPROPERTY(Config)
   TArray<FString> PathsToValidate;

   UPROPERTY(Config)
   TArray<FString> PathsToSkipValidation;
};
