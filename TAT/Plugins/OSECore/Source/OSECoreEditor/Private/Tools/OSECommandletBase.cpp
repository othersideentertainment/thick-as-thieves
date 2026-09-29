// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSECommandletBase.h"

// ose

// ue4
#include "Misc/FeedbackContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECommandletBase)

DEFINE_LOG_CATEGORY(LogOSECommandlet);

UOSECommandletBase::UOSECommandletBase(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   LogToConsole = true;
   ShowErrorCount = true;
}

// Commandlet for validating data
int32 UOSECommandletBase::Main(const FString& fullCommandLine)
{
   int retCode = 0;
   
   // logging
   UE_LOG(LogOSECommandlet, Display, TEXT("--------------------------------------------------------------------------------------------"));
   UE_LOG(LogOSECommandlet, Display, TEXT("Running %s"), _GetOSECommandletName());

   // subclasses should run it's logic here
   retCode += _RunOSECommandlet(fullCommandLine);

   // we return an error code based on how many errors + warnings we've seen while running
   TArray<FString> allErrors;
   TArray<FString> allWarnings;
   GWarn->GetErrors(allErrors);
   GWarn->GetWarnings(allWarnings);
   const int numErrors = allErrors.Num();
   const int numWarnings = allWarnings.Num();
   
   UE_LOG(LogOSECommandlet, Display, TEXT("Finished running %s with %d errors and %d warnings"), _GetOSECommandletName(), numErrors, numWarnings);
   UE_LOG(LogOSECommandlet, Display, TEXT("--------------------------------------------------------------------------------------------"));

   // err code is # of issues seen
   retCode += (numErrors + numWarnings);
   return retCode;
}
