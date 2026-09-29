// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "OSEBugReportManager.h"

#include "CoreMinimal.h"
#include "HttpFwd.h"

#include "OSEJiraBugReportManager.generated.h"



/**
 * 
 */
UCLASS()
class OSEBUGREPORTER_API UOSEJiraBugReportManager : public UOSEBugReportManager
{
	GENERATED_BODY()

   virtual void ReportBug(const FOSEBugData& Data) override;

   void ProcessResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool connectedSuccessfully);

protected:
    TSharedRef<FJsonObject> CreateJsonObject(const FOSEBugData& Data, const FString& projectKey) const;
    FString GetJiraProject() const;
    FString GetRestURI() const;
    FString GetBrowseURI() const;
    FString GetUser() const;
    FString GetPwd() const;
   

	
};
