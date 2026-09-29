// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "NetJobs/NetJob.h"

// ose
#include "NetJobs/NetJobTypes.h"

// ue4
#include "Interfaces/IHttpResponse.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NetJob)

DEFINE_LOG_CATEGORY(LogNetJob);

///////////////////////////////////////////////////////////////////
// NetJob
///////////////////////////////////////////////////////////////////

void NetJob::Start()
{
   if (_hasStarted)
   {
      UE_LOG(LogNetJob, Error, TEXT("NetJob::Start(): job '%s' was started multiple times!"), *_GenerateLoggingName());
      return;
   }

   UE_LOG(LogNetJob, VeryVerbose, TEXT("NetJob::Start(): job '%s' starting..."), *_GenerateLoggingName());
   OnStart();
   _hasStarted = true;
}

void NetJob::FailJob(int32 httpStatus)
{
   // job is done once we fail
   _MarkJobCompleted();

   if (!_hasStarted)
   {
      UE_LOG(LogNetJob, Error, TEXT("NetJob::FailJob(): job '%s' had FailJob() called without being started!"), *_GenerateLoggingName());
      return;
   }

   if (_isComplete)
   {
      UE_LOG(LogNetJob, Error, TEXT("NetJob::FailJob(): job '%s' had FailJob() called while already completed!"), *_GenerateLoggingName());
      return;
   }

   UE_LOG(LogNetJob, VeryVerbose, TEXT("NetJob::FailJob(): job '%s' FAILED!  HTTP status: %d"), *_GenerateLoggingName(), httpStatus);

   _httpErrorResponseCode = httpStatus;

   OnFailure(httpStatus);
   _isComplete = true;
   _isSuccess = false;
}

void NetJob::_MarkJobCompleted()
{
   if (!_hasStarted)
   {
      UE_LOG(LogNetJob, Error, TEXT("NetJob::CompleteJob(): job '%s' had CompleteJob() called while not started!"), *_GenerateLoggingName());
      return;
   }

   if (_isComplete)
   {
      UE_LOG(LogNetJob, Error, TEXT("NetJob::CompleteJob(): job '%s' had CompleteJob() called while already completed!"), *_GenerateLoggingName());
      return;
   }

   UE_LOG(LogNetJob, VeryVerbose, TEXT("NetJob::CompleteJob(): job '%s' complete."), *_GenerateLoggingName());
   _isComplete = true;
}

void NetJob::GenerateRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req)
{
   switch (GetRequestType())
   {
   case ENetJobRequestType::Get:
      {
         req->SetVerb(TEXT("GET"));
      }
      break;
   case ENetJobRequestType::Post:
      {
         req->SetVerb(TEXT("POST"));
      }
      break;
   case ENetJobRequestType::Put:
      {
         req->SetVerb(TEXT("PUT"));
      }
      break;
   case ENetJobRequestType::Delete:
      {
         req->SetVerb(TEXT("DELETE"));
      }
      break;
   default:
      UE_LOG(LogNetJob, Warning, TEXT("Job %s did not define a valid request type!  Failing the job!"), *_GenerateLoggingName());
      check(false); // this is a case we should never hit
      break;
   }

   // let subclasses add things in!
   OnGenerateRequest(req);
}

bool NetJob::HandleResponse(FHttpResponsePtr response, bool wasSuccessful)
{
   // job is done once we have a response
   _MarkJobCompleted();

   // base class handles generic http failures
   if (!HttpResponseHelpers::IsResponseValid(response, wasSuccessful))
      return false;

   // subclasses can handle specific things like failing to parse json, unexpected responses etc
   return OnHandleResponse(response);
}

void NetJob::SetWasSuccessful(bool success)
{
   UE_LOG(LogNetJob, VeryVerbose, TEXT("NetJob: job '%s' set success to: %s"), *_GenerateLoggingName(), success ? TEXT("true") : TEXT("false"));
   _isSuccess = success;
}

void NetJob::Reset()
{
   UE_LOG(LogNetJob, VeryVerbose, TEXT("NetJob: job '%s' resetting for next attempt"), *_GenerateLoggingName());

   _httpErrorResponseCode = INDEX_NONE;
   _hasStarted = false;
   _isComplete = false;
   _isSuccess = false;

   // Intentionally not resetting this so we can keep the state between attempts...
   //_attemptNum = 0;

   OnReset();
}

FString NetJob::_GenerateLoggingName() const
{
   // default logging name is verb + base url + endpoint
   return FString::Printf(TEXT("%s:%s%s"), *UEnum::GetValueAsString(GetRequestType()), *GetBaseURL(), *GetEndpoint());
}

///////////////////////////////////////////////////////////////////
// NetJobJson
///////////////////////////////////////////////////////////////////

NetJobJson::NetJobJson()
   : NetJob()
{
}

void NetJobJson::OnGenerateRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req)
{
   if (TSharedPtr<FJsonObject> payload = GetJsonPayload())
   {
      if (payload->Values.Num() > 0)
      {
         _jsonRequest = MakeShareable(new FJsonObject(*payload));

         req->SetContentAsString(JsonStringHelpers::CreateStringFromJsonObject(_jsonRequest.ToSharedRef(), JsonStringHelpers::EStringifyMode::Compact));

         // log it with pretty print instead of compact
         UE_SUPPRESS(LogNetJob, Verbose,
         {            
            FString contentsForLogging = JsonStringHelpers::CreateStringFromJsonObject(_jsonRequest.ToSharedRef(), JsonStringHelpers::EStringifyMode::PrettyPrint);
            if (contentsForLogging.Len() > JsonStringHelpers::kContentLimitForLogging)
               contentsForLogging = contentsForLogging.Left(JsonStringHelpers::kContentLimitForLogging) + TEXT("...");
            UE_LOG(LogNetJob, Verbose, TEXT("Generated json string for '%s': '%s'"), *_GenerateLoggingName(), *contentsForLogging);
         });
      }
      else
      {
         req->SetContentAsString(FString());
      }
   }
   else
   {
      UE_LOG(LogNetJob, Error, TEXT("Job %s did not produce a valid JSON payload to put in the HTTP request!"), *_GenerateLoggingName());
   }
}

bool NetJobJson::OnHandleResponse(FHttpResponsePtr response)
{
   UE_SUPPRESS(LogNetJob, Verbose,
   {
      FString contentsForLogging = response->GetContentAsString();
      if (contentsForLogging.Len() > JsonStringHelpers::kContentLimitForLogging)
         contentsForLogging = contentsForLogging.Left(JsonStringHelpers::kContentLimitForLogging) + TEXT("...");
      UE_LOG(LogNetJob, Verbose, TEXT("Response arrived from '%s', JSON: '%s'"), *_GenerateLoggingName(), *contentsForLogging);
   });

   TSharedPtr<FJsonObject> responseJson = JsonStringHelpers::CreateJsonObjectFromString(response->GetContentAsString());
   if (!responseJson.IsValid())
   {
      UE_LOG(LogNetJob, Warning, TEXT("Response from '%s' did not parse into a valid JSON object. Failing the job..."), *_GenerateLoggingName());
      return false;
   }

   // let our subclasses handle the constructed json object
   CompleteJob(responseJson.ToSharedRef());
   return true;
}

void NetJobJson::CompleteJob(TSharedRef<FJsonObject> payload)
{
   SetWasSuccessful(true);
   OnComplete(payload);
}

