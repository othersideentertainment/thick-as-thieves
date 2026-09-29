// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATInteractionGateInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractionGateInterface)


void FTATInteractionGateCollection::Initialize(const AActor* actor)
{
   check(actor);

   for (UActorComponent* component : actor->GetComponents())
   {
      if (ITATInteractionGateInterface* gate = Cast<ITATInteractionGateInterface>(component))
      {
         if (gate->CanEverBlockInteraction())
         {
            _interactionGates.Add(component);
         }
      }
   }
}

bool FTATInteractionGateCollection::IsInteractionBlocked() const
{
   for(TScriptInterface<ITATInteractionGateInterface> gatePtr : _interactionGates)
   {
      const ITATInteractionGateInterface* gate = gatePtr.GetInterface();
      if(gate && gate->IsInteractionBlocked())
      {
         return true;
      }
   }
   return false;
}

bool FTATInteractionGateCollection::IsInteractionBlockedWithoutMessage() const
{
   for(TScriptInterface<ITATInteractionGateInterface> gatePtr : _interactionGates)
   {
      const ITATInteractionGateInterface* gate = gatePtr.GetInterface();
      if(gate && gate->IsInteractionBlocked() && !gate->ShowMessageWhenBlocked())
      {
         return true;
      }
   }
   return false;
}

bool FTATInteractionGateCollection::TryAddToPrompt(FInteractPrompt& prompt) const
{
   for(TScriptInterface<ITATInteractionGateInterface> gatePtr : _interactionGates)
   {
      const ITATInteractionGateInterface* gate = gatePtr.GetInterface();
      if(gate && gate->IsInteractionBlocked() && gate->ShowMessageWhenBlocked())
      {
         gate->AddToPrompt(prompt);
         return true;
      }
   }

   return false;
}
