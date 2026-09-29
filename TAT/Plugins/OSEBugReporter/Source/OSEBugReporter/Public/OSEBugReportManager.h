// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OSEBugReportManager.generated.h"


struct FOSEBugData;
/**
 * 
 */
UCLASS()
class OSEBUGREPORTER_API UOSEBugReportManager : public UObject
{
	GENERATED_BODY()
public:
   virtual void ReportBug(const FOSEBugData& Data){};
	
};
