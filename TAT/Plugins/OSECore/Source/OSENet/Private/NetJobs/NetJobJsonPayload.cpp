// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "NetJobs/NetJobJsonPayload.h"
#include "NetJobs/NetJobTypes.h"

NetJobJsonPayload::NetJobJsonPayload(const TSharedPtr<FJsonObject>& jsonRequest)
   : NetJobJson()
{
   _jsonRequest = jsonRequest;
   check(_jsonRequest);
}

void NetJobJsonPayload::OnComplete(TSharedRef<FJsonObject> payload)
{
   _jsonResponse = payload;
}

FString NetJobJsonPayload::CreateJsonResponseString() const
{
   if (_jsonResponse)
   {
      return JsonStringHelpers::CreateStringFromJsonObject(_jsonResponse.ToSharedRef(), JsonStringHelpers::EStringifyMode::Compact, JsonStringHelpers::kContentLimitForLogging);
   }
   return FString();
}
