// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"

// ose
#include "OSEConditionContext.h"

// ue5
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "OSECondition.generated.h"

enum class EOSEComparisonMethod : uint8;

UCLASS(EditInlineNew, Abstract)
class OSECORE_API UOSECondition : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, Category = "Condition")
    bool Inverted = false;

    bool Evaluate(const FOSEConditionContext& context) const;
protected:
   virtual bool _EvaluateInternal(const FOSEConditionContext& context) const { return false; };
};

USTRUCT()
struct OSECORE_API FOSEConditionSet
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly, Instanced)
   TArray<UOSECondition*> Conditions;

   bool Satisfied(const FOSEConditionContext& context) const;
};

UCLASS(meta = (DisplayName = "Has All Tags"))
class OSECORE_API UOSEHasAllTagsCondition : public UOSECondition
{
   GENERATED_BODY()
public:

   UPROPERTY(EditDefaultsOnly, Category = "Condition")
   FGameplayTagContainer Tags;

protected:
   virtual bool _EvaluateInternal(const FOSEConditionContext& context) const override;
};

UCLASS(meta = (DisplayName = "Has Any Tags"))
class OSECORE_API UOSEHasAnyTagsCondition : public UOSECondition
{
   GENERATED_BODY()
public:

   UPROPERTY(EditDefaultsOnly, Category="Condition")
   FGameplayTagContainer Tags;

protected:
   virtual bool _EvaluateInternal(const FOSEConditionContext& context) const override;
};

UCLASS(meta=(DisplayName="Attribute Percentage"))
class OSECORE_API UOSEAttributeCondition : public UOSECondition
{
   GENERATED_BODY()
public:

   UPROPERTY(EditDefaultsOnly, Category = "Condition")
   FGameplayAttribute Attribute;

   UPROPERTY(EditDefaultsOnly, Category = "Condition")
   FGameplayAttribute MaxAttribute;

   UPROPERTY(EditDefaultsOnly, Category = "Condition")
   EOSEComparisonMethod Comparison;

   UPROPERTY(EditDefaultsOnly, Category = "Condition", meta=(UIMin=0, UIMax=100, ClampMin=0, ClampMax=100))
   float Percentage;

protected:
   virtual bool _EvaluateInternal(const FOSEConditionContext& context) const override;
};

