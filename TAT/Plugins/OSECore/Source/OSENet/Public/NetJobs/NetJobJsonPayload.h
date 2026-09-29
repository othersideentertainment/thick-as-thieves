// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "NetJobs/NetJob.h"

// engine
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

///////////////////////////////////////////////////////////////////
// NetJobJson
///////////////////////////////////////////////////////////////////

class OSENET_API NetJobJsonPayload : public NetJobJson
{
public:
   NetJobJsonPayload(const TSharedPtr<FJsonObject>& jsonRequest);
   virtual ENetJobRequestType GetRequestType() const override { return ENetJobRequestType::Post; }
   virtual FString GetEndpoint() const override { return FString(); }
   virtual TSharedPtr<FJsonObject> GetJsonPayload() const override { return _jsonRequest; }
   virtual void OnComplete(TSharedRef<FJsonObject> payload) override;

   TSharedPtr<FJsonObject> GetJsonResponse() const { return _jsonResponse; }
   FString CreateJsonResponseString() const;

protected:
   TSharedPtr<FJsonObject> _jsonResponse;
};
