// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "NetJobs/NetJobTypes.h"

// ue4
#include "Interfaces/IHttpResponse.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NetJobTypes)

DEFINE_LOG_CATEGORY_STATIC(LogJsonStringHelpers, Log, All)

///////////////////////////////////////////////////////////////////
// JsonStringHelpers
///////////////////////////////////////////////////////////////////

// Taken from UE4's JsonObjectConverter implementation
template<class CharType, class PrintPolicy>
bool JsonObjectToStringInternal(const TSharedRef<FJsonObject>& jsonObject, FString& outJsonString)
{
   TSharedRef<TJsonWriter<CharType, PrintPolicy>> jsonWriter = TJsonWriterFactory<CharType, PrintPolicy>::Create(&outJsonString);
   bool bSuccess = FJsonSerializer::Serialize(jsonObject, jsonWriter);
   jsonWriter->Close();
   return bSuccess;
}

FString JsonStringHelpers::CreateStringFromJsonObject(TSharedRef<FJsonObject> json, JsonStringHelpers::EStringifyMode mode, int maxLength)
{
   FString outString;
   bool couldSerialize;
   if (mode == EStringifyMode::PrettyPrint)
   {
      couldSerialize = JsonObjectToStringInternal<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>(json, outString);
   }
   else
   {
      couldSerialize = JsonObjectToStringInternal<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>(json, outString);
   }
   if (!couldSerialize)
   {
      UE_LOG(LogJsonStringHelpers, Warning, TEXT("Failed to serialize JSON object into a string!"));
   }

   if (maxLength > 0 && outString.Len() > maxLength)
      outString = outString.Left(maxLength) + TEXT("...");

   return outString;
}

TSharedPtr<FJsonObject> JsonStringHelpers::CreateJsonObjectFromString(const FString& jsonString)
{
   TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonString);
   TSharedPtr<FJsonObject> tempObjPtr;
   const bool couldDeserialize = FJsonSerializer::Deserialize(reader, tempObjPtr);
   if (!couldDeserialize)
   {
      UE_LOG(LogJsonStringHelpers, Warning, TEXT("Failed to deserialize string into a JSON object: '%s'"), *jsonString);
      return nullptr;
   }
   return tempObjPtr;
}

///////////////////////////////////////////////////////////////////
// HttpResponseHelpers
///////////////////////////////////////////////////////////////////

bool HttpResponseHelpers::IsResponseValid(FHttpResponsePtr response, bool wasSuccessful)
{
   if (!wasSuccessful || !response.IsValid())
      return false;
   if (EHttpResponseCodes::IsOk(response->GetResponseCode()))
      return true;
   else
      return false;
}

