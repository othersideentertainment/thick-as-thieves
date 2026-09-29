// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Audio/TATCCAudioSubsystem.h"

// tat
#include "GameFramework/TATWorldSettings.h"

// wwise
#include "AkAudioEvent.h"
#include "AkComponent.h"
#include "AkGameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCCAudioSubsystem)

bool UTATCCAudioSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   const UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_DedicatedServer);
}

void UTATCCAudioSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

   // Annoyingly, OnWorldBeginPlay() is called via UEngine::LoadMap() -> UWorld::BeginPlay(), and isn't gated by ATATGameState::OnRep_ReplicatedHasBegunPlay().
   // So clients may have to wait for ATATWorldSettings to broadcast the "actual" replicated beginplay event
   if (GetWorld()->HasBegunPlay())
   {
      _ScheduleRefresh();
   }
   else
   {
      ATATWorldSettings* tatWorldSettings = CastChecked<ATATWorldSettings>(GetWorld()->GetWorldSettings());
      tatWorldSettings->OnWorldBeginPlay.AddUObject(this, &UTATCCAudioSubsystem::_Refresh);
   }
}

void UTATCCAudioSubsystem::AddSoundAtTransform(UAkAudioEvent* audioEvent, const FTransform& transform)
{
   _bucketByEvent.FindOrAdd(audioEvent).Transforms.Add(transform);
   _bucketsToRefresh.AddUnique(audioEvent);
   
   if (GetWorld()->HasBegunPlay())
   {
      _ScheduleRefresh();
   }
}

void UTATCCAudioSubsystem::RemoveSoundAtTransform(UAkAudioEvent* audioEvent, const FTransform& transform)
{
   if (FAudioBucket* bucket = _bucketByEvent.Find(audioEvent))
   {
      // TODO: maybe use a handle later so removal is O(1)
      const int32 index = bucket->Transforms.IndexOfByPredicate([&transform](const FTransform& entry) { return entry.Equals(transform); });
      if (index >= 0)
      {
         bucket->Transforms.RemoveAtSwap(index);
      }
      _bucketsToRefresh.AddUnique(audioEvent);
   
      if (GetWorld()->HasBegunPlay())
      {
         _ScheduleRefresh();
      }
   }
}

bool UTATCCAudioSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

void UTATCCAudioSubsystem::_ScheduleRefresh()
{
   if (!_refreshTimerHandle.IsValid())
   {
      _refreshTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::_Refresh);
   }
}

void UTATCCAudioSubsystem::_Refresh()
{
   // AkComponent construction must be delayed until world has begun play, to ensure our position data assigned to a spawned AkComponent isn't stomped by UAkComponent::BeginPlay() calling UpdateGameObjectPosition()
   check(GetWorld()->HasBegunPlay());
   _refreshTimerHandle.Invalidate();

   for (UAkAudioEvent* event : _bucketsToRefresh)
   {
      _RefreshBucket(event);
   }

   _bucketsToRefresh.Reset();
}

void UTATCCAudioSubsystem::_RefreshBucket(UAkAudioEvent* audioEvent)
{
   FAudioBucket* bucket = _bucketByEvent.Find(audioEvent);
   if (!ensure(bucket))
   {
      return;
   }

   UAkComponent* audioComponent = bucket->AudioComponent.Get();

   if (bucket->Transforms.IsEmpty())
   {
      if (audioComponent)
      {
         // Ak events are not automatically stopped by default when the component is destroyed,
         // so doing that explicitly here. That seems simpler than `StopWhenOwnerDestroyed`, and
         // more reliable given that setting explicitly checks if its owning actor is destroyed.
         audioComponent->Stop();
         audioComponent->DestroyComponent();
      }
      _bucketByEvent.Remove(audioEvent);
      return;
   }

   if (audioComponent == nullptr)
   {
      constexpr bool autoPost = true;
      constexpr bool autoDestroy = false;
      audioComponent = UAkGameplayStatics::SpawnAkComponentAtLocation(this, audioEvent, FVector::Zero(), FRotator(), autoPost, autoDestroy);
      bucket->AudioComponent = audioComponent;
   }

   // TODO: inline this, as it duplicates the positions array twice
   UAkGameplayStatics::SetMultiplePositions(audioComponent, bucket->Transforms);
}
