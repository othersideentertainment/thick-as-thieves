// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "NetJobs/NetJobJsonPayload.h"

// engine
#include "CoreMinimal.h"

//#include "TATNetJobAnalytics.generated.h"

///////////////////////////////////////////////////////////////////
// TATNetJobAnalytics
///////////////////////////////////////////////////////////////////

class TATNetJobAnalytics : public NetJobJsonPayload
{
public:
   TATNetJobAnalytics(const TSharedPtr<FJsonObject>& json);

   virtual const FString& GetBaseURL() const override final;
   virtual FString GetEndpoint() const override final { return TEXT("/"); }
   virtual ENetJobRequestType GetRequestType() const override final { return ENetJobRequestType::Post; }
};
