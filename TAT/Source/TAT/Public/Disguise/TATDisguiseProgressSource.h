// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEAbilityInfo.h"
#include "TATDisguiseProgressSource.generated.h"


// A progress source that polls the current disguise integrity
UCLASS()
class TAT_API UTATDisguiseProgressSource : public UOSEAbilityProgressSource
{
   GENERATED_BODY()

public:

   virtual float GetProgress(const AActor* character) const override;
};
