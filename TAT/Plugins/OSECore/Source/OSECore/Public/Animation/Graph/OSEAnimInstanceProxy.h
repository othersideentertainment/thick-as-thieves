// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Animation/AnimInstanceProxy.h"

// OSE
#include "Animation/Graph/OSEAnimData.h"
#include "Animation/Graph/OSEAnimActorInfo.h"
#include "OSEAnimInstanceProxy.generated.h"


//--------------------------------------------------------------------------------------------------
/// Animation instance proxy (for efficient multithreaded updates).
/// Allocated from the UOSEAnimInstance.
/// Can be used to perform heavy calculations in a multi-threaded fashion.
//--------------------------------------------------------------------------------------------------

USTRUCT()
struct OSECORE_API FOSEAnimInstanceProxy : public FAnimInstanceProxy
{
   GENERATED_BODY()

public:

   FOSEAnimInstanceProxy();
   FOSEAnimInstanceProxy(UAnimInstance* inAnimInstance);

protected:

   /// Overriding so we can copy data from the instance. Called on the game thread.
   virtual void PreUpdate(UAnimInstance* inAnimInstance, float deltaSeconds) override;

   /// Overriding so we can update the animation data. Called on the task thread.
   virtual void Update(float deltaSeconds) override;

   /// Overriding so we can copy data back to the instance. Called on the game thread.
   virtual void PostUpdate(UAnimInstance* inAnimInstance) const override;

private:

   FOSEAnimActorInfo _actorInfo;
};
