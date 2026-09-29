// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSEGameplayDebuggerPerception.h"

//OSE
#include "OSEPerceptionComponent.h"

#if WITH_GAMEPLAY_DEBUGGER

FOSEGameplayDebuggerPerception::FOSEGameplayDebuggerPerception()
{

}

//UE


void FOSEGameplayDebuggerPerception::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
   UOSEPerceptionComponent* PerceptionComponent = nullptr;
   APawn* MyPawn = Cast<APawn>(DebugActor);
   if (MyPawn)
   {
      AController* Controller = MyPawn->GetController();

      PerceptionComponent = MyPawn->FindComponentByClass<UOSEPerceptionComponent>();
      // try the controller if the Pawn doesn't have it
      if (PerceptionComponent == nullptr && Controller)
      {
         PerceptionComponent = Controller->FindComponentByClass<UOSEPerceptionComponent>();
      }
   }

   if (PerceptionComponent == nullptr && DebugActor != nullptr)
   {
      PerceptionComponent = DebugActor->FindComponentByClass<UOSEPerceptionComponent>();
   }

   if (PerceptionComponent)
   {
      PerceptionComponent->DescribeSelfToGameplayDebugger(this);
   }
}

TSharedRef<FGameplayDebuggerCategory> FOSEGameplayDebuggerPerception::MakeInstance()
{
   return MakeShareable(new FOSEGameplayDebuggerPerception());
}

#endif //WITH_GAMEPLAY_DEBUGGER
