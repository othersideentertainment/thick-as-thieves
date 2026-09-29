// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "WorldConditions/SmartObjectWorldConditionBase.h"
#include "GameplayTagContainer.h"

#include "TATWorldCondition_SmartObjectOwnerTagQuery.generated.h"

USTRUCT()
struct TAT_API FTATWorldCondition_SmartObjectOwnerTagQueryState
{
   GENERATED_BODY()

   FDelegateHandle DelegateHandle;
};

/// World condition to match tags of the Smart Object's owning Actor (which must implement ITATSmartObjectTagInterface).
///
/// A shim to the existing ITATSmartObjectTagInterface
/// TODO: evaluate whether this structure makes sense now there is much more flexibility in the conditions that it can have
USTRUCT(meta = (DisplayName = "[TAT] Match Gameplay tags on SmartObject owner"))
struct TAT_API FTATWorldCondition_SmartObjectOwnerTagQuery : public FSmartObjectWorldConditionBase
{
   GENERATED_BODY()

   using FStateType = FTATWorldCondition_SmartObjectOwnerTagQueryState;

protected:
#if WITH_EDITOR
   virtual FText GetDescription() const override;
#endif

   virtual TObjectPtr<const UStruct>* GetRuntimeStateType() const override { 

      static TObjectPtr<const UStruct> ptr{ FStateType::StaticStruct() };
      return &ptr;
   }
   virtual bool Initialize(const UWorldConditionSchema& schema) override;
   virtual bool Activate(const FWorldConditionContext& context) const override;
   virtual FWorldConditionResult IsTrue(const FWorldConditionContext& context) const override;
   virtual void Deactivate(const FWorldConditionContext& context) const override;

   // Smart Object's owning actor for which the tags must match the query. The Actor must implement ITATSmartObjectTagInterface.
   FWorldConditionContextDataRef SmartObjectActorRef;

public:
   // Tags on the Smart Object's owning actor that need to match this query for the condition to evaluate true.
   UPROPERTY(EditAnywhere, Category = "Default")
   FGameplayTagQuery TagQuery;
};
