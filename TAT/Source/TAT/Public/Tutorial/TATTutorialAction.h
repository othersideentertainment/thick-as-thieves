// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Object.h"

#include "TATTutorialAction.generated.h"

struct FTATTutorialParams;
struct FTATTutorialValidationParams;

USTRUCT()
struct FTATTutorialActionHandle
{
   GENERATED_BODY()

   // An optional object to keep alive
   UPROPERTY()
   TObjectPtr<UObject> OptionalObject = nullptr;

   // optional cancel action
   TFunction<void()> CancelAction;
};

UCLASS(Abstract, Const, DefaultToInstanced, EditInlineNew, CollapseCategories)
class TAT_API UTATTutorialAction : public UObject
{
   GENERATED_BODY()

public:
   // Must always eventually call next
   virtual FTATTutorialActionHandle Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
   {
      next();
      return {};
   }

   virtual FString GetDebugName() const;
#if WITH_EDITOR
   virtual void Validate(const FTATTutorialValidationParams& params) const {}
#endif

protected:
#if WITH_EDITOR
   virtual void PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostLoad() override;
#endif
   
private:
#if WITH_EDITORONLY_DATA
   // Just a dummy property to use as a TitleCondition
   UPROPERTY(Transient, VisibleAnywhere, meta = (editcondition = "false", EditConditionHides))
   FString _debugNameForEditor;
#endif
};
