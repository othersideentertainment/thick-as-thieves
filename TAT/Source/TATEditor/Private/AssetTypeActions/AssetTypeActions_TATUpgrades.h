// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "OSEGenericGraphAssetTypeActions.h"

// ue
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_TATUpgradeType : public FAssetTypeActions_Base
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("TAT Upgrade Type")); }
   virtual FColor GetTypeColor() const override { return FColor(248, 255, 107); }
   virtual UClass* GetSupportedClass() const override;
   virtual uint32 GetCategories() override;
};

class FAssetTypeActions_TATUpgradeGraph : public FOSEGenericGraphAssetTypeActions
{
public:
   FAssetTypeActions_TATUpgradeGraph();
   virtual FText GetName() const override { return FText::FromString(TEXT("TAT Upgrade Graph")); }
   virtual UClass* GetSupportedClass() const override;
};
