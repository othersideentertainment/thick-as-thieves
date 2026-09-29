// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEBugReporterModule.h"

#include "OSEBugSubmitter.h"


#include "HAL/IConsoleManager.h"


#define LOCTEXT_NAMESPACE "FOSEBugReporterModule"

void FOSEBugReporterModule::StartupModule()
{

   IConsoleCommand* Command = IConsoleManager::Get().RegisterConsoleCommand(
      TEXT("OSE.ReportBug"),
      TEXT("Report a bug"),
      FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(FOSEBugSubmitter::SubmitBug),
      ECVF_Cheat
   );
}

void FOSEBugReporterModule::ShutdownModule()
{
   IConsoleManager::Get().UnregisterConsoleObject(TEXT("OSE.ReportBug"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOSEBugReporterModule, OSEBugReporter)
