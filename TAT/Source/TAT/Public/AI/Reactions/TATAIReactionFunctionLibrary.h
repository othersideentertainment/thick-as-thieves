// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Reactions/TATAIReactionTarget.h"

// ue
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATAIReactionFunctionLibrary.generated.h"

class ATATCharacterAIBase;
struct FUtilityStateTarget;

UCLASS()
class TAT_API UTATAIReactionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "Reaction Event Utils")
   static bool TryRegisterForUtilityTargetReactionRole(const FUtilityStateTarget& utilityTarget,
      ATATCharacterAIBase* reactingAI, FGameplayTag& outRoleTag);

   UFUNCTION(BlueprintCallable, Category = "Reaction Event Utils")
   static bool TryUnregisterForUtilityTargetReactionRole(const FUtilityStateTarget& utilityTarget,
      ATATCharacterAIBase* reactingAI);

   UFUNCTION(BlueprintCallable, Category = "Reaction Event Utils")
   static bool IsRegisteredForUtilityTargetReactionRole(const FUtilityStateTarget& utilityTarget,
      ATATCharacterAIBase* reactingAI, const FGameplayTag& roleTag);
	
};
