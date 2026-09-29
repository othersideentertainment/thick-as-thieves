// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Types/ISlateMetaData.h"

class UTATScreenWidget;

class TAT_API FTATScreenMetadata : public ISlateMetaData
{
public:
   SLATE_METADATA_TYPE(FTATScreenMetadata, ISlateMetaData)

   FTATScreenMetadata(TWeakObjectPtr<UTATScreenWidget> inScreenObject)
      : ScreenObject(inScreenObject) {}

   // UTATScreenWidget instance this SWidget is associated with
   TWeakObjectPtr<UTATScreenWidget> ScreenObject;
};
