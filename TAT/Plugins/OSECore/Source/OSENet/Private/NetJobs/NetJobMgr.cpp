// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "NetJobs/NetJobMgr.h"

// ose

// engine
#include "Json.h"
#include "JsonUtilities.h"
#include "PlatformFeatures.h"
#include "Engine/Engine.h"
#include "Templates/SharedPointer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NetJobMgr)

namespace
{
   // hardcoded defines
   // TODO: Could move to .ini for configuration...?

   // Capped to something reasonable?
   const int kMaxSimultaneousRunningJobs = 8;

   // Used in place of an http code when the net job mgr itself is the thing that fails a job
   const int kNetJobMgrFailedJobErrCode = INDEX_NONE;
}

/* static */
UNetJobMgr& UNetJobMgr::Get(const UObject& contextObj)
{
   UGameInstance* gameInstance = contextObj.GetWorld()->GetGameInstance();
   check(gameInstance); // should have one, it lasts the whole app lifetime...?
   UNetJobMgr* netJobMgr = gameInstance->GetSubsystem<UNetJobMgr>();
   check(netJobMgr); // should have one, it's a game instance subsystem...
   return *netJobMgr;
}

UNetJobMgr::UNetJobMgr()
   : Super()
{
   
}

UNetJobMgr::~UNetJobMgr()
{

}

void UNetJobMgr::Initialize(FSubsystemCollectionBase& collection)
{
   _httpModule = &FHttpModule::Get();
   check(_httpModule);
}

void UNetJobMgr::Deinitialize()
{
   // kick any jobs off in an attempt to complete anything in the pipe before the app is closed
   // TODO: This is only going to kick off one job
   // TODO: This might not be viable on consoles so we probably need to pay close attention to how we handle "end session" type net
   //       calls, and instrument reasonable timeouts on our backend(s) instead of assuming they're always going to come in.
   //       Example: On Xbox you can suspend your title, then close the application in the OS, and your app just never gets a Shutdown() call, it's just killed during the suspend.
   _TickStartJobIfNeeded();

   //Stop ticking before we are destroyed
   SetTickableTickType(ETickableTickType::Never);
 }

void UNetJobMgr::_ReleaseHttpRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req)
{
   check(_allHttpRequests.Contains(req));
   req->OnProcessRequestComplete().Unbind();
   _allHttpRequests.Remove(req);
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UNetJobMgr::_AllocateNewHttpRequest()
{
   // This should be the only place that calls FHttpModule::CreateRequest().
   TSharedRef<IHttpRequest, ESPMode::ThreadSafe> newReq = _httpModule->CreateRequest();
   _allHttpRequests.Add(newReq);
   return newReq;
}

UWorld* UNetJobMgr::GetWorld() const
{
   if (UGameInstance* gameInstance = GetGameInstance())
   {
      return gameInstance->GetWorld();
   }
   return nullptr;
}

void UNetJobMgr::Tick(float deltaTime)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_NetJobMgr_Tick);

   _TickStartJobIfNeeded();
}

FQueuedNetJob UNetJobMgr::_QueueNetJobInternal(TSharedRef<NetJob> job, int jobId)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_NetJobMgr_QueueNetJob);

   // can't start a job more than once.
   check(!job->HasBeenStarted());

   UE_LOG(LogNetJob, Verbose, TEXT("Starting a job aimed at %s%s..."), *job->GetBaseURL(), *job->GetEndpoint());

   // kick it
   job->Start();

   // cache it
   FInternalQueuedNetJob& newlyAddedJob = _queuedJobs.Add_GetRef(FInternalQueuedNetJob());
   newlyAddedJob.Job = job;
   newlyAddedJob.JobId = jobId;

   // return data about it
   return FQueuedNetJob(newlyAddedJob.CallbackDelegate, newlyAddedJob.JobId);
}

void UNetJobMgr::_TickStartJobIfNeeded()
{
   // we start one job per frame so that HttpModule alloc/start spikes don't hurt us

   if (_queuedJobs.Num() == 0)
      return;

   if (_runningJobs.Num() >= kMaxSimultaneousRunningJobs)
      return;

   QUICK_SCOPE_CYCLE_COUNTER(STAT_NetJobMgr_StartNetJob);

   // Grab the next thing out of the queue
   FInternalQueuedNetJob nextJobStruct = _queuedJobs[0]; // copy! 
   _queuedJobs.RemoveAt(0); // remove from front!
   TSharedPtr<NetJob> nextJob = nextJobStruct.Job;
   check(nextJob.IsValid());
   check(nextJob->HasBeenStarted());
   TSharedRef<NetJob> nextJobRef = nextJob.ToSharedRef();

   const FString& baseUrl = nextJobRef->GetBaseURL();
   TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req = _GenerateHttpRequest(baseUrl);
   nextJobRef->GenerateRequest(req);

   {
      QUICK_SCOPE_CYCLE_COUNTER(STAT_NetJobMgr_StartNetJob_SendRequest);

      _AddRunningJobData(nextJobRef, req, nextJobStruct.CallbackDelegate, nextJobStruct.JobId);
      _SendRequest(req); // actually fire off the job at the network layer
   }
}

FQueuedNetJob UNetJobMgr::QueueNetJob(TSharedRef<NetJob> job)
{
   // job ids are just ever-incrementing
   static NetJobId sJobId = 0;
   return _QueueNetJobInternal(job, sJobId++);
}

bool UNetJobMgr::CancelNetJob(NetJobId jobId)
{
   // check for queued jobs, and if it's in the queue we can just remove it now
   for (int idx = 0; idx < _queuedJobs.Num(); ++idx)
   {
      const FInternalQueuedNetJob& queuedJob = _queuedJobs[idx];
      if (queuedJob.JobId == jobId)
      {
         // found and removed from the queue
         _queuedJobs.RemoveAt(idx);
         UE_LOG(LogNetJob, Log, TEXT("Cancelled queued job %d!"), jobId);
         return true;
      }
   }

   // check for running jobs, and then mark them for cancellation
   for (auto it = _runningJobs.CreateIterator(); it; ++it)
   {
      RunningNetJob& runningNetJob = it.Value();
      if (runningNetJob.JobId == jobId)
      {
         // found and marked a running net job as cancelled
         runningNetJob.WasCancelled = true;
         UE_LOG(LogNetJob, Log, TEXT("Cancelled running job %d!"), jobId);
         return true;
      }
   }

   UE_LOG(LogNetJob, Error, TEXT("Tried to cancel a net call but could not find it in the queued or running jobs!"));
   return false;
}

void UNetJobMgr::_OnHttpRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response /* note: can be null! */, bool wasSuccessful)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_NetJobMgr_OnJobComplete);

   check(request.IsValid());

   const RunningNetJob* jobData = _FindRunningJobData(request);
   check(jobData); // we better have this in our set of data!

   float elapsedTime = request->GetElapsedTime();
   _HandleJobCompletion(response, wasSuccessful, elapsedTime, *jobData);
   _RemoveRunningJobData(request);
}

bool UNetJobMgr::_HandleJobCompletion(FHttpResponsePtr response, bool wasSuccessful, float elapsedTime, const RunningNetJob& jobData)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_NetJobMgr_HandleJobCompletion);

   NetJob& job = jobData.Job.Get();

   if (jobData.WasCancelled)
   {
      UE_LOG(LogNetJob, Log, TEXT("Job for endpoint %s was cancelled in flight, not sending out completion and/or failure callbacks!"), *job.GetEndpoint());
      return false;
   }

   if (!response.IsValid())
   {
      UE_LOG(LogNetJob, Warning, TEXT("Got no HTTP response for job.  (Network interface may be down?)"));
      job.FailJob(kNetJobMgrFailedJobErrCode);
      jobData.CallCompletedCallbackForFailure();
      // TODO:  We could retry here, too, but this looks more like a code error in this class... we should always get a FHttpResponsePtr yeah?
      return false;
   }

   UE_LOG(LogNetJob, Verbose, TEXT("Response arrived from '%s%s', code: %d, successful: %d, duration: %.02f seconds"), *job.GetBaseURL(), *job.GetEndpoint(), response->GetResponseCode(), wasSuccessful, elapsedTime);

   const bool success = job.HandleResponse(response, wasSuccessful);
   if (success)
   {
      jobData.CallCompletedCallbackForSuccess();
   }
   else
   {
      // fail the job now, or retry?
      bool shouldCompleteJobNow = true;

      // increment our attempt counter if the job requires it, then we'll retry later in higher-logic code
      if (job.GetNumRetryAttempts() != INDEX_NONE)
      {
         job.IncrementAttempt();

         // should we retry this job?
         int numRetryAttempts = jobData.Job->GetNumRetryAttempts();
         int numCompletedAttempts = jobData.Job->GetNumCompletedAttempts();
         if (numRetryAttempts != INDEX_NONE &&
            numCompletedAttempts <= numRetryAttempts)
         {
            UE_LOG(LogNetJob, Warning, TEXT("Response from '%s' was not successful (code: %d, successful: %d).  Attempted %d times, retrying up to a max of %d times..."),
               *job.GetEndpoint(), response->GetResponseCode(), wasSuccessful, numCompletedAttempts, numRetryAttempts);
            shouldCompleteJobNow = false;
            job.Reset();
            FQueuedNetJob newJob = _QueueNetJobInternal(jobData.Job, jobData.JobId);
            newJob.Callback = jobData.CallbackDelegate;
         }
      }

      if (shouldCompleteJobNow)
      {
         UE_LOG(LogNetJob, Warning, TEXT("Response from '%s' was not successful (code: %d, successful: %d)."),
            *job.GetEndpoint(), response->GetResponseCode(), wasSuccessful);
         job.FailJob(response->GetResponseCode());
         jobData.CallCompletedCallbackForFailure();
      }
   }

   return success;
}

void UNetJobMgr::_AddRunningJobData(TSharedRef<NetJob> job, TSharedRef<IHttpRequest, ESPMode::ThreadSafe> httpReq, FNetJobMgrJobFinishedEventDelegate callback, NetJobId jobId)
{
   RunningNetJob jobToAdd(job, httpReq, callback, jobId);
   check(!_runningJobs.Contains(httpReq));
   _runningJobs.Add(httpReq, jobToAdd);
}

UNetJobMgr::RunningNetJob* UNetJobMgr::_FindRunningJobData(FHttpRequestPtr httpReq)
{
   return _runningJobs.Find(httpReq);
}

void UNetJobMgr::_RemoveRunningJobData(FHttpRequestPtr httpReq)
{
   RunningNetJob* callData = _FindRunningJobData(httpReq);
   check(callData);
   _ReleaseHttpRequest(callData->HttpReq);
   _runningJobs.Remove(httpReq);
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UNetJobMgr::_GenerateHttpRequest(const FString& url)
{
   check(_httpModule);
   
   // TODO: We could try and pool and re-use these http requests, but, my last attempt to do that caused a bunch of bugs depending on the platform http impl. -Chooch
   TSharedRef<IHttpRequest, ESPMode::ThreadSafe> request = _AllocateNewHttpRequest();
   request->SetURL(url);
   _SetRequestHeaders(request);
   return request;
}

void UNetJobMgr::_SetRequestHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> request)
{
   request->SetHeader(TEXT("Content-Type"),      TEXT("application/json"));
   request->SetHeader(TEXT("Accept-Language"),   FInternationalization::Get().GetCurrentLocale()->GetName());
   //request->SetHeader(TEXT("User-Agent"),      TEXT("X-UnrealEngine-Agent"));
   //request->SetHeader(TEXT("Accept"),          TEXT("application/json; version=") + serverAPIVersion);
   //request->SetHeader(PlatformHeader,          platformString);
   //request->SetHeader(TEXT("Changelist"),      FString::Printf(TEXT("%d"), gameChangelist));
}

void UNetJobMgr::_SendRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> request)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_NetJobMgr_SendRequest);

   UE_LOG(LogNetJob, Verbose, TEXT("Sending '%s' request to '%s'..."), *request->GetVerb(), *request->GetURL());

   UE_SUPPRESS(LogNetJob, Verbose,
   {
      UE_LOG(LogNetJob, Verbose, TEXT("HTTP headers:"));
      const auto& headers = request->GetAllHeaders();
      for (const auto& headerStr : headers)
         UE_LOG(LogNetJob, Verbose, TEXT("   '%s'"), *headerStr);
   });

   request->OnProcessRequestComplete().BindUObject(this, &UNetJobMgr::_OnHttpRequestComplete);
   request->ProcessRequest();
}

