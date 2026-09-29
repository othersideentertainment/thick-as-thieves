// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

//ue4
#include "AssetTypeActions_Base.h"
#include "CoreMinimal.h"


class FAssetTypeActions_OSEVoiceOverBucket : public FAssetTypeActions_Base
{
public:
   virtual FText GetName() const override { return FText::FromString(TEXT("Voice Over Bucket")); }
   virtual FColor GetTypeColor() const override { return FColor(99, 192, 255); }
   virtual UClass* GetSupportedClass() const override;
   virtual uint32 GetCategories() override;

   virtual void OpenAssetEditor(const TArray<UObject*>& objects, TSharedPtr<class IToolkitHost> editWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
};
