// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"

#include "TATClueSetBase.generated.h"

struct FConstStructView;


struct FTATClueSetContext
{
   FGameplayTagContainer ContextTags;
};

// An abstract base class for clue sets
UCLASS(Abstract)
class TAT_API UTATClueSetBase : public UDataAsset
{
   GENERATED_BODY()

public:
   TArray<FConstStructView> FindRelevantClueViews(const FTATClueSetContext& context) const;
   virtual void AddRelevantClueViews(const FTATClueSetContext& context, TArray<FConstStructView>& result) const {}

#if WITH_EDITOR
   // not recursive
   virtual void VisitClueSetDependencies(TFunctionRef<void (const UTATClueSetBase*)> visitor) const {}
#endif

};
