// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/DetectionEnums.h"

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATAIIncorrectObjectStateInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTATAIIncorrectObjectStateInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATAIIncorrectObjectStateInterface
{
   GENERATED_BODY()

public:

   /// Returns whether equipment is locked. If true, tool can't be equipped
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "AI|TAT|Smart Object")
   bool AuthorityIsObjectInCorrectState(bool allowIgnoringOfState = true) const;
   virtual bool AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState = true) const { return true; }
};
