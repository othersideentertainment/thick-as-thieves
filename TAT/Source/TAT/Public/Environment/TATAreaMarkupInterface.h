// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "TATAreaMarkupInterface.generated.h"

class ATATAreaMarkupVolume;
// This class does not need to be modified.
UINTERFACE()
class UTATAreaMarkupInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATAreaMarkupInterface
{
   GENERATED_BODY()

public:
   virtual const FGameplayTagContainer& GetAreaMarkupTags() const = 0;
   virtual void AddArea(TObjectPtr<ATATAreaMarkupVolume> area) = 0;
   virtual void RemoveArea(TObjectPtr<ATATAreaMarkupVolume> area) = 0;
   virtual const TArray<TWeakObjectPtr<ATATAreaMarkupVolume>>& GetAreasCurrentlyContainingActor() const = 0;
};
