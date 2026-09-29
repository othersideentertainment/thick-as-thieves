// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "EditorValidatorBase.h"
#include "EditorValidator_AssetEngineVersionSet.generated.h"

UCLASS(Config=Editor)
class TATEDITOR_API UEditorValidator_AssetEngineVersionSet : public UEditorValidatorBase
{
   GENERATED_BODY()

public:
   UEditorValidator_AssetEngineVersionSet();

protected:
   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;
};
