// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATCCAudioSubsystem.generated.h"

class UAkComponent;
class UAkAudioEvent;

// A subsystem to manage "carbon copies" of audio playing in different locations in the map
// by using a single audio component per sound, and setting multiple locations.
//
// * The CC naming is mirroring the original BP
UCLASS(BlueprintType)
class TAT_API UTATCCAudioSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()
   
public:
   // From UWorldSubsystem
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;

   UFUNCTION(BlueprintCallable)
   void AddSoundAtTransform(UAkAudioEvent* audioEvent, const FTransform& transform);

   // TODO: use some sort of handle, but matching initial BP impl to start with
   UFUNCTION(BlueprintCallable)
   void RemoveSoundAtTransform(UAkAudioEvent* audioEvent, const FTransform& transform);

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

private:
   void _ScheduleRefresh();
   void _Refresh();
   void _RefreshBucket(UAkAudioEvent* audioEvent);

   struct FAudioBucket
   {
      TWeakObjectPtr<UAkComponent> AudioComponent = nullptr;
      TArray<FTransform> Transforms;
   };

   TMap<TWeakObjectPtr<UAkAudioEvent>, FAudioBucket> _bucketByEvent;
   UPROPERTY()
   TArray<TObjectPtr<UAkAudioEvent>> _bucketsToRefresh;
   FTimerHandle _refreshTimerHandle;
};
