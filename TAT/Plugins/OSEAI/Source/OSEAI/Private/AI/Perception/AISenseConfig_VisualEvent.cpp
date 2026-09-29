// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/AISenseConfig_VisualEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AISenseConfig_VisualEvent)

UAISenseConfig_VisualEvent::UAISenseConfig_VisualEvent()
{
   DebugColor = FColor::Emerald;
}

TSubclassOf<UAISense> UAISenseConfig_VisualEvent::GetSenseImplementation() const
{
   return Implementation;
}

#if WITH_GAMEPLAY_DEBUGGER
void UAISenseConfig_VisualEvent::DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const
{
   Super::DescribeSelfToGameplayDebugger(perceptionComponent, debuggerCategory);
}
#endif // WITH_GAMEPLAY_DEBUGGER

