// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

//ue4
#include "CoreMinimal.h"
#include "AssetTypeActions/AssetTypeActions_DataAsset.h"

class FAssetTypeActions_TATMapVariationBase : public FAssetTypeActions_DataAsset
{
public:
   virtual uint32 GetCategories() override;
   virtual FText GetDisplayNameFromAssetData(const FAssetData& assetData) const override;
};

class FAssetTypeActions_TATSpawnData : public FAssetTypeActions_TATMapVariationBase
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("Spawn Data")); }
   virtual FColor GetTypeColor() const override { return FColor(99, 43, 48); }
   virtual UClass* GetSupportedClass() const override;
};

class FAssetTypeActions_TATSceneSet : public FAssetTypeActions_TATMapVariationBase
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("TAT Scene Set")); }
   virtual FColor GetTypeColor() const override { return FColor(231, 235, 144); }
   virtual UClass* GetSupportedClass() const override;
};

class FAssetTypeActions_TATSceneAsset : public FAssetTypeActions_TATMapVariationBase
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("TAT Scene")); }
   virtual FColor GetTypeColor() const override { return FColor(250, 223, 99); }
   virtual UClass* GetSupportedClass() const override;
};

class FAssetTypeActions_TATSceneVariant : public FAssetTypeActions_TATMapVariationBase
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("TAT Scene Variant")); }
   virtual FColor GetTypeColor() const override { return FColor(230, 175, 46); }
   virtual UClass* GetSupportedClass() const override;
};

class FAssetTypeActions_TATClueSet : public FAssetTypeActions_TATMapVariationBase
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("Clue Set")); }
   virtual UClass* GetSupportedClass() const override;
};

class FAssetTypeActions_TATCompoundClueSet : public FAssetTypeActions_TATMapVariationBase
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("Compound Clue Set")); }
   virtual UClass* GetSupportedClass() const override;
};

class FAssetTypeActions_TATMatchQuestDescription : public FAssetTypeActions_TATMapVariationBase
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("Match Quest Description")); }
   virtual UClass* GetSupportedClass() const override;
};
