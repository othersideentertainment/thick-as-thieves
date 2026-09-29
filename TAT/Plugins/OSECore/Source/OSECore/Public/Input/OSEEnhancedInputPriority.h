// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEEnhancedInputPriority.generated.h"

class UInputMappingContext;

UENUM(BlueprintType)
enum class EOSEEnhancedInputContextPriority : uint8
{
   // Used by OSECharacterBase to setup default inputs
   Default = 0,

   // Items are one input priority level higher so they override the defaults
   Items = 1,

   // Blueprints can enter custom input contexts as the highest priority.
   // Multiple values are used here in case blueprints need to stack input contexts on their own

   Blueprint_0 = 90,
   Blueprint_1 = 91,
   Blueprint_2 = 92,
   Blueprint_3 = 93,
   Blueprint_4 = 94,

   MAX,
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEInputContextPriority
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
   TSoftObjectPtr<UInputMappingContext> Context;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
   EOSEEnhancedInputContextPriority Priority = EOSEEnhancedInputContextPriority::Default;
};
