// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "GameplayEffectTypes.h"
#include "OSEAbilityFailedContext.generated.h"

USTRUCT(BlueprintType)
struct FOSEAbilityFailedContext : public FGameplayEffectContext
{
   GENERATED_BODY()

   /** The tags associated with the reason for failure */
   UPROPERTY(BlueprintReadWrite, Category = "OSE|Ability")
   FGameplayTagContainer FailureTags;

   virtual UScriptStruct* GetScriptStruct() const override
   {
      return FOSEAbilityFailedContext::StaticStruct();
   }

   virtual bool NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess) override;
};
