// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OSEAISightInterface.generated.h"

UINTERFACE()
class OSEAI_API UOSEAISightInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IOSEAISightInterface
{
   GENERATED_BODY()

public:
   // Can this actor (the listener in the sight sense) see this other actor?
   virtual bool IsAllowedToSeeActor(const AActor* actor) const { return true; }
   // If we want to modify the sight range for a certain actor, override this in your project.
   virtual void ModifySightRangeForSpecificActor(const AActor* actor, float& outSightRadius) const {}

   virtual bool ShouldCheckBoundsForVision() const { return false; }

   // Optional information describing the process by which the sight range of a given actor is affected by modifiers.
   // Useful for debugging.
   virtual FString DescribeSightRangeModificationForSpecificActor(const AActor* actor) const { return FString(); }
};
