// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tutorial/TATTutorialScript.h"

// tat
#include "Tutorial/TATTutorialAction.h"
#include "Tutorial/TATTutorialCondition.h"
#include "Tutorial/TATTutorialValidationParams.h"

// ue
#include "Misc/DataValidation.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTutorialScript)

#if WITH_EDITOR
EDataValidationResult UTATTutorialScript::IsDataValid(class FDataValidationContext& context) const
{
   ValidateSteps(nullptr, [&context](const FText& message)
   {
      context.AddError(message);
   });

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UTATTutorialScript::ValidateSteps(const UWorld* optionalWorld, TFunctionRef<void(const FText&)> reportError) const
{
   using FReportErrorRef = FTATTutorialValidationParams::FReportErrorRef;
   for (int i = 0; i < Steps.Num(); i++)
   {
      const FTATTutorialStep& step = Steps[i];

      auto reportStep = [&reportError, &step, i](const FText& message)
      {
         reportError(FText::FormatOrdered(INVTEXT("Step[{0}-{1}]{2}"), i, FText::FromString(step.StepName), message));
      };

      auto reportCondition = [&reportStep](const TCHAR* label)
      {
         return [&reportStep, label](const FText& message)
         {
            reportStep(FText::FormatOrdered(INVTEXT("[{0}] {1}"), FText::FromString(label), message));
         };
      };
      if (step.EnterCondition)
      {
         step.EnterCondition->Validate({.World = optionalWorld, .ErrorReporter = FReportErrorRef(reportCondition(TEXT("EnterCondition")))});
      }
      if (step.ExitCondition)
      {
         step.ExitCondition->Validate({.World = optionalWorld, .ErrorReporter = FReportErrorRef(reportCondition(TEXT("ExitCondition")))});
      }
      if (step.SkipCondition)
      {
         step.SkipCondition->Validate({ .World = optionalWorld, .ErrorReporter = FReportErrorRef(reportCondition(TEXT("SkipCondition"))) });
      }

      for (int actionIndex = 0; actionIndex < step.Actions.Num(); actionIndex++)
      {
         const UTATTutorialAction* action = step.Actions[actionIndex];
         if (action)
         {
            action->Validate({.World = optionalWorld, .ErrorReporter = FReportErrorRef([&](const FText& message)
            {
               reportStep(FText::FormatOrdered(INVTEXT("[Action {0}] {1}"), actionIndex, message));
            })});
         }
      }
         
   }
}
#endif
