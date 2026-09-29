// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Object.h"

#include "TATTutorialCondition.generated.h"

struct FTATTutorialValidationParams;
struct FTATTutorialParams;


UCLASS(Abstract, Const, DefaultToInstanced, EditInlineNew, CollapseCategories)
class TAT_API UTATTutorialCondition : public UObject
{
   GENERATED_BODY()

public:
   virtual bool IsMet(const FTATTutorialParams& params) const { return true; }
   virtual FString GetDebugName() const;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const {}
#endif
};
