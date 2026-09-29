// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class OSEGENERICGRAPHEDITOR_API FOSEGenericGraphAssetTypeActions : public FAssetTypeActions_Base
{
public:
   FOSEGenericGraphAssetTypeActions(EAssetTypeCategories::Type InAssetCategory);

   virtual FText GetName() const override;
   virtual FColor GetTypeColor() const override;
   virtual UClass* GetSupportedClass() const override;
   virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
   virtual uint32 GetCategories() override;

private:
   EAssetTypeCategories::Type MyAssetCategory;
};
