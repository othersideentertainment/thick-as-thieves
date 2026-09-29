// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Significance/TATSignificanceWorldSubsystem.h"

// ue
#if WITH_EDITOR
#include "LevelEditorViewport.h"
#endif
#include "SignificanceManager.h"
#include "GameFramework/PlayerController.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSignificanceWorldSubsystem)

void UTATSignificanceWorldSubsystem::Tick(const float deltaTime)
{
   Super::Tick(deltaTime);
   
   const UWorld* world = GetWorld();
   if(world != nullptr)
   {
      USignificanceManager* significanceManager = USignificanceManager::Get(world);
      TArray<FTransform> viewpoints;
      if(significanceManager)
      {
         for(FConstPlayerControllerIterator it = world->GetPlayerControllerIterator(); it; ++it)
         {
            const TWeakObjectPtr<APlayerController> playerController = *it;
            FVector viewLocation;
            FRotator viewRotation;
            playerController->GetPlayerViewPoint(viewLocation, viewRotation);
            viewpoints.Emplace(viewRotation, viewLocation, FVector::OneVector);
         }
#if WITH_EDITOR
         // Just enough to make simulate mode use significance w.r.t. the view position
         if (GEditor && GEditor->bIsSimulatingInEditor && GCurrentLevelEditingViewportClient && GCurrentLevelEditingViewportClient->IsSimulateInEditorViewport())
         {
            viewpoints.Reset();
            viewpoints.Emplace(GCurrentLevelEditingViewportClient->GetViewRotation(), GCurrentLevelEditingViewportClient->GetViewLocation(), FVector::OneVector);
         }
#endif

         if (viewpoints.Num() > 0)
         {
            significanceManager->Update(viewpoints);
         }
      }
   }
}

TStatId UTATSignificanceWorldSubsystem::GetStatId() const
{
   RETURN_QUICK_DECLARE_CYCLE_STAT(UTATSignificanceWorldSubsystem, STATGROUP_Tickables);
}
