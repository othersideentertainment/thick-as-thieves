// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "TATInteractionGateInterface.generated.h"

struct FInteractPrompt;

// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UTATInteractionGateInterface : public UInterface
{
   GENERATED_BODY()
};

// An interface for components that act as bolt-on gates that block interaction
//
// Requires interactables to opt-in to using them
class TAT_API ITATInteractionGateInterface
{
   GENERATED_BODY()
public:

   // Whether this is configured N/A
   // Assumed to not change at runtime
   virtual bool CanEverBlockInteraction() const { return true; }

   // Whether this blocks the interactable to be used
   virtual bool IsInteractionBlocked() const = 0;
   // Assumed to not change at runtime
   virtual bool ShowMessageWhenBlocked() const { return false; }
   // Adds message when blocked
   virtual void AddToPrompt(FInteractPrompt& prompt) const {}
};


// A wrapper struct around a zero or more of interaction gates
USTRUCT()
struct FTATInteractionGateCollection
{
   GENERATED_BODY()

   void Initialize(const AActor* actor);

   bool IsInteractionBlocked() const;
   bool IsInteractionBlockedWithoutMessage() const;
   bool TryAddToPrompt(FInteractPrompt& prompt) const;

private:
   UPROPERTY(Transient)
   TArray<TScriptInterface<ITATInteractionGateInterface>> _interactionGates;
};
