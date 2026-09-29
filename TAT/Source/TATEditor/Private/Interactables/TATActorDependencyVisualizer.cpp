// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATActorDependencyVisualizer.h"

// tat
#include "Developer/TATActorDependencyVisComponent.h"
#include "Developer/TATEditorActorDependencySubsystem.h"


void FTATActorDependencyVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if(const auto* dependency = Cast<UTATActorDependencyVisComponent>(component))
   {
      if(const auto* dependencySubsystem = dependency->GetWorld()->GetSubsystem<UTATEditorActorDependencySubsystem>())
      {
         TConstArrayView<TWeakObjectPtr<AActor>> targets = dependency->IsReversed ?
            dependencySubsystem->GetReverseDependencies(dependency->GroupKey, dependency->GetOwner()) :
            dependencySubsystem->GetDependencies(dependency->GroupKey, dependency->GetOwner());

         FVector origin = dependency->GetOwner()->GetActorLocation();
         FColor color = dependency->Color;
         ESceneDepthPriorityGroup priorityGroup = dependency->DrawForeground ? SDPG_Foreground : SDPG_World;
         for(TWeakObjectPtr<AActor> weakTarget : targets)
         {
            if(const AActor* target = weakTarget.Get())
            {
               pdi->DrawLine(origin, target->GetActorLocation(), color, priorityGroup);
            }
         }
      }
   }
}
