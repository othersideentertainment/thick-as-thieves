// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "ToolInput.generated.h"

class UInputAction;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct OSECORE_API FOSEToolInputInfo
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UInputAction* InputAction = nullptr;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FText InputText;

   // Unique identifier for this input prompt. Allows the HUD to query for its widget representation for stuff like press-and-hold animations.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag PromptTag;

   // this input action should be shown when this query passes against tags that currently exist on the character.  a blank query means to always show it.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTagQuery AbilityTagQuery;

   // This input action should be represented as "disabled" (eg. greyed out) when this query passes against the character's ASC. 
   // A blank query means to never represent this input as "disabled".
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTagQuery DisabledTagQuery;
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEToolInput
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UInputMappingContext* InputContext = nullptr;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TArray<FOSEToolInputInfo> InputInfo;
};
