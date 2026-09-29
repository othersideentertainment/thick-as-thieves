// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "NetJobs/NetJob.h"

// ue4
#include "CoreMinimal.h"

#include "NetJobTypes.generated.h"

typedef uint64_t NetJobId;
static const NetJobId kInvalidJobId = INDEX_NONE;

USTRUCT()
struct FNetJobCompleteInfo
{
   GENERATED_BODY()

   TSharedPtr<NetJob> Job;
   bool Success = false;
   NetJobId JobId = kInvalidJobId;

   template<class T>
   T* GetJobAs() const
   {
      return Job.Get()->GetAs<T>();
   }
};

namespace JsonStringHelpers
{
   static const int kContentLimitForLogging = 512;

   enum class EStringifyMode : uint8
   {
      Compact,
      PrettyPrint
   };
   FString OSENET_API CreateStringFromJsonObject(TSharedRef<FJsonObject> json, EStringifyMode = EStringifyMode::PrettyPrint, int maxLength = -1);
   TSharedPtr<FJsonObject> CreateJsonObjectFromString(const FString& jsonString);
}

namespace HttpResponseHelpers
{
   bool IsResponseValid(FHttpResponsePtr response, bool wasSuccessful);
}
