// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "NetJobs/TATNetJobAnalytics.h"

// tat
#include "NetJobs/TATNetJobSettings.h"
#include "Common/TATVersionEdition.h"

TATNetJobAnalytics::TATNetJobAnalytics(const TSharedPtr<FJsonObject>& json)
   : NetJobJsonPayload(json)
{
}

const FString& TATNetJobAnalytics::GetBaseURL() const
{
   // TODO: Support multiple analytics environments
   const UTATNetJobSettings& settings = UTATNetJobSettings::Get();
   return settings.AnalyticsURL;
}
