// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "LockpickableInterface.generated.h"

DECLARE_MULTICAST_DELEGATE(FTATOnRequestCancelLockpicking);

// This class does not need to be modified.
UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class TAT_API ULockpickableInterface : public UInterface
{
   GENERATED_BODY()
};

/**
 *
 */
class TAT_API ILockpickableInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   // Unlocks and resets the jammed state
   UFUNCTION(BlueprintCallable, Category = "Lockpickable")
   virtual void Unlock() = 0;

   // Default relock
   virtual void Lock() {}

   // just trying to prevent overloads in naming
   // optional
   virtual void LockWithKey() {}

   // Called when a lockpick minigame track has been completed, setting the next track index
   virtual void OnLockpickTrackCompleted(int32 trackIndex) {}

   // When opening the lockpicking minigame, what is the first track that still needs completing?
   virtual int32 GetLockpickCurrentTrack() { return 0; }

   // Whether the lockpicking can end in failure rather than just continuing
   UFUNCTION(BlueprintCallable, Category = "Lockpickable")
   virtual bool CanLockpickFail() const { return false; }

   // Called when an attempt to lockpick this object explicitly fails (rather than being cancelled)
   UFUNCTION(BlueprintCallable, Category = "Lockpickable")
   virtual void OnLockpickFailed() {}

   // Called when a lockpick attempt is cancelled, either voluntarily or involuntarily (i.e. player damaged)
   UFUNCTION(BlueprintCallable, Category = "Lockpickable")
   virtual void OnLockpickCancelled() {}

   // The lockpickable actor can provide a delegate that it will be call to request that lockpicking be cancelled
   // (eg. the door being lockpicked was forced open or broken)
   virtual FTATOnRequestCancelLockpicking* GetOnRequestCancelLockpickingDelegate() { return nullptr; }
};
