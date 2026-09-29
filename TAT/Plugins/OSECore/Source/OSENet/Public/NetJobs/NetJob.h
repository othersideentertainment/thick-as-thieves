// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "JsonObjectConverter.h"

#include "NetJob.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNetJob, Log, All)

class IHttpRequest;
class NetJob;
class UNetJobMgr;

DECLARE_MULTICAST_DELEGATE_OneParam(FOSENetJobDelegate, NetJob*)

UENUM()
enum class ENetJobRequestType : uint8
{
   Get,
   Post,
   Put,
   Delete
};

class OSENET_API NetJob
{
   friend class UNetJobMgr;

public:
   NetJob() {}
   virtual ~NetJob() {}

   // Returns the type of request.
   virtual ENetJobRequestType GetRequestType() const = 0;

   // Returns the base url string like "http://undertow-development.com"
   virtual const FString& GetBaseURL() const = 0;

   // Returns the endpoint string like "/matchmaking/start/"
   virtual FString GetEndpoint() const = 0;

   // If this job failed, what was the http response code?  INDEX_NONE if it wasn't set
   int GetHttpErrorResponseCode() const { return _httpErrorResponseCode; }

   // Has this job started or is it still in the queue?
   bool HasBeenStarted() const { return _hasStarted; }

   // Has this job been completed?
   bool HasCompleted() const { return _isComplete; }

   // Was this job a success?
   bool WasSuccessful() const { return _isSuccess; }

   // Is this job important enough to attempt multiple times?  If so, how many times should we attempt it?  INDEX_NONE = only attempt once
   virtual int GetNumRetryAttempts() const { return INDEX_NONE; }

   // Get a specific job subclass
   template<class T>
   T* GetAs()
   {
      // TODO: Is there any way to get some runtime checking in !shipping builds
      // in here?  Can't d-cast with RTTI off and this is not a UObject subclass.
      return static_cast<T*>(this);
   }

protected:
   // subclasses should call to set whether the job was successful.
   void SetWasSuccessful(bool success);

   // base url + endpoint for logging only -- expensive so let's not call this outside of logging?
   FString _GenerateLoggingName() const;

   // Called when we're generating an http request for this job, add anything to the request that's needed
   virtual void OnGenerateRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req) { };

   // Called when we hear a response back after the request is complete.  Return true if this job is successful
   virtual bool OnHandleResponse(FHttpResponsePtr response) { return true; }

   // Called from Start(), immediately before the job is handed to the NetJobMgr.
   virtual void OnStart() {}

   // Called if the job failed to execute, or if the server indicated that the request could not be completed.
   virtual void OnFailure(int32 httpStatus) { };

   // Called to Reset a job if we're going to re-kick it for another attempt.  Should reset any runtime state but keep around any payload for additional attempts.
   // Most net jobs do not need to implement this as only failed jobs (without any kind of response yet) are reset/re-run.
   virtual void OnReset() { };

private:

   // These should only be called from NetJobMgr. If subclasses need to handle anything they'll call OnFunc() style protected functions
   void Start(); // Tells the job it's about to be run.  Eventually either FailJob() or CompleteJob() will be called.
   void FailJob(int32 httpStatus);
   int GetNumCompletedAttempts() const { return _attemptNum; }
   void IncrementAttempt() { ++_attemptNum; }
   void GenerateRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req);
   bool HandleResponse(FHttpResponsePtr response, bool wasSuccessful);
   void Reset();

   // Called when we fail, or we receive a response, and marks the job as completed
   void _MarkJobCompleted();

private:
   int _httpErrorResponseCode = INDEX_NONE;
   bool _hasStarted = false;
   bool _isComplete = false;
   bool _isSuccess = false;
   int _attemptNum = 0;
};

// This class is the abstract base class of all json-based jobs, subclasses need to figure out how to store the json payload
class OSENET_API NetJobJson : public NetJob
{
public:
   NetJobJson();

protected:
   // called on us from our parent NetJob
   virtual void OnGenerateRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req) override;
   virtual bool OnHandleResponse(FHttpResponsePtr response) override;

   // for our subclasses
   virtual TSharedPtr<FJsonObject> GetJsonPayload() const = 0;
   virtual void OnComplete(TSharedRef<FJsonObject> payload) = 0;

private:
   // complete the json job with this payload
   virtual void CompleteJob(TSharedRef<FJsonObject> payload);

protected:
   // hold onto our request json obj so its lifetime matches ours
   TSharedPtr<FJsonObject> _jsonRequest;
};

// This class makes it super-easy to use the built-in JSON<->USTRUCT conversion for our net jobs
//
// To use this:
//
// 1) define default-constructible USTRUCT() types that can be converted to and from JSON
//    via FJsonObjectConverter.  The request is RequestStruct and the response is ResponseStruct.
// 2) pass those USTRUCT types as the RequestStruct and ResponseStruct to this template class and extend it.
//
// ...that should be it, the subclass should transparently pack/unpack data to/from JSON as needed.
//
template<typename RequestStruct, typename ResponseStruct>
class NetJobRequestAndResponse : public NetJobJson
{
public:
   NetJobRequestAndResponse()
      : NetJobJson()
   {
   }

   NetJobRequestAndResponse(const RequestStruct& inRequest)
      : NetJobJson()
      , _request(inRequest)
   {
   }

   NetJobRequestAndResponse(RequestStruct&& inRequest)
      : NetJobJson()
      , _request(MoveTemp(inRequest))
   {
   }

   virtual ~NetJobRequestAndResponse() { }

   const RequestStruct& GetRequest() const { return _request; }
   const ResponseStruct& GetResponse() const { return _response; }

protected:
   RequestStruct _request;
   ResponseStruct _response;

protected:
   // Like OnComplete(), but unpacks the JSON into a USTRUCT() for you.
   // If the first parameter is false, then we failed to build the specified type of
   // USTRUCT from the JSON payload that was returned from the server.
   virtual void OnCompleteStruct(bool parsedStruct, ResponseStruct& payload)
   {
      // default behavior if not defined:  successful as long as we parsed the struct
      NetJobJson::SetWasSuccessful(parsedStruct);
   }

private:
   virtual TSharedPtr<FJsonObject> GetJsonPayload() const override;

   virtual void OnComplete(TSharedRef<FJsonObject> payload) override;
};

template<typename RequestStruct, typename ResponseStruct>
inline TSharedPtr<FJsonObject> NetJobRequestAndResponse<RequestStruct, ResponseStruct>::GetJsonPayload() const
{
   return FJsonObjectConverter::UStructToJsonObject(_request, 0, 0, nullptr);
}

template<typename RequestStruct, typename ResponseStruct>
inline void NetJobRequestAndResponse<RequestStruct, ResponseStruct>::OnComplete(TSharedRef<FJsonObject> payload)
{
   const bool parsed = FJsonObjectConverter::JsonObjectToUStruct(payload, &_response, 0, 0);
   OnCompleteStruct(parsed, _response);
}

// These are helper subclasses for if you don't need either a request or response struct.
// For example, the endpoint that uploads a bit of data doesn't have any response beyond "yes, that worked" or "no, that didn't work",
// so the job doesn't really need a USTRUCT() defined for the response type.
USTRUCT()
struct FDummyNetJobRequest
{
   GENERATED_BODY()

   UPROPERTY()
   bool Dummy = false;
};

USTRUCT()
struct FDummyNetJobResponse
{
   GENERATED_BODY()

   UPROPERTY()
   bool Dummy = false;
};

template<typename ResponseType>
class NetJobNoRequest : public NetJobRequestAndResponse<FDummyNetJobRequest, ResponseType>
{
public:
   NetJobNoRequest()
      : NetJobRequestAndResponse<FDummyNetJobRequest, ResponseType>()
   {
   }
   virtual ~NetJobNoRequest() { }

protected:
   // Stub, override to do something interesting, we consider this job a success if we parsed the struct at all
   virtual void OnCompleteStruct(bool parsedStruct, ResponseType& payload) override
   {
      NetJob::SetWasSuccessful(parsedStruct);
   }

private:
   // Should never actually get called!
   virtual TSharedPtr<FJsonObject> GetJsonPayload() const override final
   {
      unimplemented();
      return MakeShareable(new FJsonObject());
   }
};

template<typename RequestType>
class NetJobNoResponse : public NetJobRequestAndResponse<RequestType, FDummyNetJobResponse>
{
public:
   NetJobNoResponse()
      : NetJobRequestAndResponse<RequestType, FDummyNetJobResponse>()
   {
   }
   NetJobNoResponse(const RequestType& inRequest)
      : NetJobRequestAndResponse<RequestType, FDummyNetJobResponse>(inRequest)
   {
   }

   NetJobNoResponse(RequestType&& inRequest)
      : NetJobRequestAndResponse<RequestType, FDummyNetJobResponse>(MoveTemp(inRequest))
   {
   }

   virtual ~NetJobNoResponse() {}

protected:
   // will be called if the job completed successfully, can override if you want to take some action there
   virtual void OnCompleteNoResponse(TSharedRef<FJsonObject> payload) {};

private:
   // Can't fail at this point, since the response from the server is just the success/failure in the meta field.
   virtual void OnComplete(TSharedRef<FJsonObject> payload) override final
   {
      OnCompleteNoResponse(payload);
      NetJob::SetWasSuccessful(true);
   }

   // Should never actually get called!
   virtual void OnCompleteStruct(bool parsedStruct, FDummyNetJobResponse& payload) override final
   {
      unimplemented();
   }
};

// Net job that doesn't have params *or* a response... we use this for the 'ping' endpoint,
// but I guess you could use it for other things?  Maybe?

class NetJobNoResponseOrRequest : public NetJobRequestAndResponse<FDummyNetJobRequest, FDummyNetJobResponse>
{
public:
   NetJobNoResponseOrRequest()
      : NetJobRequestAndResponse<FDummyNetJobRequest, FDummyNetJobResponse>()
   {
   }
   virtual ~NetJobNoResponseOrRequest() { }

protected:
   // will be called if the job completed successfully, can override if you want to take some action there
   virtual void OnCompleteNoResponse(TSharedRef<FJsonObject> payload) {};

private:
   // Should never actually get called!
   virtual TSharedPtr<FJsonObject> GetJsonPayload() const override final
   {
      unimplemented();
      return MakeShareable(new FJsonObject());
   }

   // Can't fail at this point, since the response from the server is just the success/failure in the meta field.
   virtual void OnComplete(TSharedRef<FJsonObject> payload) override final
   {
      OnCompleteNoResponse(payload);
      NetJobJson::SetWasSuccessful(true);
   }

   // Should never actually get called!
   virtual void OnCompleteStruct(bool parsedStruct, FDummyNetJobResponse& payload) override final
   {
      unimplemented();
   }
};
