// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "OSEVoiceOverPriority.generated.h"


USTRUCT(BlueprintType)
struct OSECORE_API FOSEVoiceOverPriorityRow : public FTableRowBase
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "VoiceOver.Priority"))
   FGameplayTag PriorityTag;

   UPROPERTY(EditDefaultsOnly)
   int32 Priority = 0;
};

