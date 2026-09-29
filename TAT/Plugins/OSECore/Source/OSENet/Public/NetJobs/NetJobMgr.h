// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "NetJobs/NetJob.h"
#include "NetJobs/NetJobTypes.h"

// ue4
#include "CoreMinimal.h"
#include "Tickable.h"
#include "Containers/Queue.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/EngineTypes.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Online/CoreOnline.h"

#include "NetJobMgr.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetJobMgrJobFinishedEventDelegate, const FNetJobCompleteInfo&, netJob);

struct FQueuedNetJob
{
   FQueuedNetJob(FNetJobMgrJobFinishedEventDelegate& inCB, NetJobId inJobId)
      : Callback(inCB)
      , JobId(inJobId)
   {

   }
   FNetJobMgrJobFinishedEventDelegate& Callback;
   NetJobId JobId = kInvalidJobId;
};

UCLASS(ClassGroup = (Custom), BlueprintType)
class OSENET_API UNetJobMgr : public UGameInstanceSubsystem, public FTickableGameObject
{
   GENERATED_BODY()
   
public:
   static UNetJobMgr& Get(const UObject& contextObj);

   UNetJobMgr();
   ~UNetJobMgr();

   // Queue up this net job to run.  Will eventually call either OnComplete() or OnFailure() on the job.
   FQueuedNetJob QueueNetJob(TSharedRef<NetJob> job);
   bool CancelNetJob(NetJobId jobId);

   // Static utility method to create/queue a job
   template<typename JobType, typename... Args>
   static FQueuedNetJob CreateAndQueueNetJob(const UObject& worldContextObject, Args&& ...args)
   {
      UNetJobMgr& netJobMgr = UNetJobMgr::Get(worldContextObject);
      TSharedRef<JobType> job = MakeShareable(new JobType(Forward<Args>(args)...));
      return netJobMgr.QueueNetJob(job);
   }

protected:
   // from USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize();

   // from UObject
   virtual UWorld* GetWorld() const;

   // from FTickableGameObject
   virtual bool IsTickable() const { return !HasAnyFlags(RF_ClassDefaultObject); }
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UNetJobMgr, STATGROUP_Tickables); }
   virtual void Tick(float deltaTime) override;

private:
   FHttpModule* _httpModule = nullptr;

   struct FInternalQueuedNetJob
   {
      TSharedPtr<NetJob> Job;
      FNetJobMgrJobFinishedEventDelegate CallbackDelegate;
      NetJobId JobId = kInvalidJobId;
   };

   struct RunningNetJob
   {
      RunningNetJob(TSharedRef<NetJob> inJob, TSharedRef<IHttpRequest, ESPMode::ThreadSafe> inReq, const FNetJobMgrJobFinishedEventDelegate& inCallback, NetJobId inJobId)
         : Job(inJob)
         , HttpReq(inReq)
         , CallbackDelegate(inCallback)
         , WasCancelled(false)
         , JobId(inJobId)
      {
      }

      void CallCompletedCallback(bool success) const
      {
         if (CallbackDelegate.IsBound())
         {
            FNetJobCompleteInfo completedNetJob;
            completedNetJob.Success = success;
            completedNetJob.Job = Job;
            completedNetJob.JobId = JobId;
            CallbackDelegate.Broadcast(completedNetJob);
         }
      }

      void CallCompletedCallbackForFailure() const
      {
         CallCompletedCallback(false);
      }
      void CallCompletedCallbackForSuccess() const { CallCompletedCallback(true); }

      TSharedRef<NetJob> Job;
      TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpReq;
      FNetJobMgrJobFinishedEventDelegate CallbackDelegate;
      bool WasCancelled = false;
      NetJobId JobId = kInvalidJobId;
   };

   // job queue creation / management
   FQueuedNetJob _QueueNetJobInternal(TSharedRef<NetJob> job, int jobId);
   void _TickStartJobIfNeeded();
   TArray<FInternalQueuedNetJob> _queuedJobs; // jobs that are queued up to run but not executing yet (either waiting to be started, or waiting for auth to complete)

   // running job management
   TMap<FHttpRequestPtr, RunningNetJob> _runningJobs; // jobs that are currently running
   void _AddRunningJobData(TSharedRef<NetJob> job, TSharedRef<IHttpRequest, ESPMode::ThreadSafe> httpReq, FNetJobMgrJobFinishedEventDelegate callback, NetJobId jobId);
   RunningNetJob* _FindRunningJobData(FHttpRequestPtr httpReq);
   void _RemoveRunningJobData(FHttpRequestPtr httpReq);

   TSharedRef<IHttpRequest, ESPMode::ThreadSafe> _GenerateHttpRequest(const FString& url);
   void _SetRequestHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> request);

   void _ReleaseHttpRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> req);
   TSharedRef<IHttpRequest, ESPMode::ThreadSafe> _AllocateNewHttpRequest();
   TArray<TSharedRef<IHttpRequest, ESPMode::ThreadSafe>> _allHttpRequests;

   void _SendRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> request);

   void _OnHttpRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool wasSuccessful);
   bool _HandleJobCompletion(FHttpResponsePtr response, bool wasSuccessful, float elapsedTime, const RunningNetJob& jobData);
};
