// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "OSEBugReportManager.h"

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "OSEBugReporterSettings.generated.h"

/**
 * 
 */
UCLASS(defaultconfig, Config = Game,Meta = (DisplayName = "[OSE] BugReporter Settings"))
class OSEBUGREPORTER_API UOSEBugReporterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, config)
   TSubclassOf<UOSEBugReportManager> ReportManager;

   UPROPERTY(EditAnywhere, config)
   FString URL;

   UPROPERTY(EditAnywhere, config)
   FString DefectProjectKey;

   UPROPERTY(EditAnywhere, config)
   FString ChangelistFieldId;

	
};
