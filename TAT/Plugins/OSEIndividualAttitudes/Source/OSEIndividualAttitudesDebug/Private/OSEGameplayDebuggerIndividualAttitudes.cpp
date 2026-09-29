// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSEGameplayDebuggerIndividualAttitudes.h"
#include "OSEIndividualAttitudeComponent.h"
#include "OSEIndividualAttitudeInterface.h"

#if WITH_GAMEPLAY_DEBUGGER

FOSEGameplayDebuggerIndividualAttitudes::FOSEGameplayDebuggerIndividualAttitudes()
{
}

void FOSEGameplayDebuggerIndividualAttitudes::CollectData(APlayerController* ownerPC, AActor* debugActor)
{
   const APawn* pawn = Cast<APawn>(debugActor);
   if(pawn == nullptr)
      return;

   const IOSEIndividualAttitudeInterface* attitudeInterface = Cast<IOSEIndividualAttitudeInterface>(pawn->GetController());
   if(attitudeInterface == nullptr)
      return;

   UOSEIndividualAttitudeComponent* component = attitudeInterface->GetAttitudeComponent();
   if(component == nullptr)
      return;

   component->DescribeSelfToGameplayDebugger(this);
}

TSharedRef<FGameplayDebuggerCategory> FOSEGameplayDebuggerIndividualAttitudes::MakeInstance()
{
   return MakeShareable(new FOSEGameplayDebuggerIndividualAttitudes());
}
#endif
