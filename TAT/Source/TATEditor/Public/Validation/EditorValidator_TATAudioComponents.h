// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "EditorValidatorBase.h"

#include "EditorValidator_TATAudioComponents.generated.h"

// Checks that we use the TAT-level overrides for some Ak Audio components, not the Ak base class
UCLASS(Config = Editor)
class TATEDITOR_API UEditorValidator_TATAudioComponents : public UEditorValidatorBase
{
   GENERATED_BODY()
public:
   virtual bool CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const override;
   virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) override;

   // We may want to allow exceptions for validating this
   UPROPERTY(Config)
   TArray<FString> PathsToSkipValidation;

};
