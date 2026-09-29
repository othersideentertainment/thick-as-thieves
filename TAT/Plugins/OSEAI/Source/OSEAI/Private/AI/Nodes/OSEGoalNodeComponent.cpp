// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Nodes/OSEGoalNodeComponent.h"

// ose
#include "AI/Utility/UtilityAITokenOwnerGameplayTagCount.h"

// ue
#include "Components/SphereComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGoalNodeComponent)

DEFINE_LOG_CATEGORY(LogGoalNode);

void UOSEGoalNodeComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority())
   {
      const TArray<FOSEAITokenInfo> defaultTokens = { GoalNodeToken };
      const TArray<FOSEAITokenInfo> maxTokenDebt;
      _tokenOwner = UUtilityAITokenOwnerGameplayTagCount::AuthorityCreate(this, defaultTokens, maxTokenDebt);

      _guardLocationAssignments.Reserve(GoalNodeToken.Count);
      for (int i = 0; i < GoalNodeToken.Count; i++)
      {
         _guardLocationAssignments.Add(nullptr);
      }
   }
}

void UOSEGoalNodeComponent::OnRegister()
{
#if WITH_EDITOR
   if (GIsEditor && !IsRunningCommandlet())
   {
      // Based off of visualization component in "DisplayClusterCameraComponent.h/.cpp"
      if (DebugSphereComponent == nullptr)
      {
         DebugSphereComponent = NewObject<USphereComponent>(this, NAME_None, RF_Transactional | RF_TextExportTransient);
         if (DebugSphereComponent)
         {
            DebugSphereComponent->SetupAttachment(this);
            DebugSphereComponent->SetIsVisualizationComponent(true);
            DebugSphereComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
            DebugSphereComponent->SetMobility(EComponentMobility::Movable);
            DebugSphereComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            DebugSphereComponent->bHiddenInGame = true;
            DebugSphereComponent->CastShadow = false;
            DebugSphereComponent->CreationMethod = CreationMethod;
            DebugSphereComponent->RegisterComponentWithWorld(GetWorld());
         }
      }
      RefreshDebugRepresentation();
   }
#endif

   Super::OnRegister();
}

FVector UOSEGoalNodeComponent::AuthorityAssignGuardToLocation(AActor* guard)
{
   check(guard);
   check(GetOwner());
   check(GetOwner()->HasAuthority());

   int32 index = -1;
   for (int32 i = 0; i < _guardLocationAssignments.Num(); i++)
   {
      if (_guardLocationAssignments[i] == nullptr)
      {
         index = i;
         break;
      }
   }

   if (index == -1) 
   {
      return FVector::ZeroVector;
      //UE_LOG(LogGoalNode, Warning, TEXT("%s: Unable to assign guard %s -- no location index found."), *GetName(), *guard->GetName());
   }

   _guardLocationAssignments[index] = guard;

   // Create a grid large enough to hold the maximum number of guards
   int32 dim = FMath::CeilToInt(FMath::Sqrt((float)GoalNodeToken.Count));
   float cellSize = (GoalRadius * 2) / dim;

   // Translate index to cell x,y
   int32 y = index / dim;
   int32 x = index % dim;
   UE_LOG(LogGoalNode, Log, TEXT("%s: Assigning guard %s to slot (%d, %d)."), *GetName(), *guard->GetName(), x, y);

   // Start with the location of the actor minus the radius on X and Y (which would be 
   // the (0, 0) of our grid).
   FVector location = GetOwner()->GetActorLocation() - FVector(GoalRadius, GoalRadius, 0);

   // Offset by position of selected grid cell, plus random position within the cell.
   location.X += (x * cellSize) + FMath::RandRange(0.0f, cellSize);
   location.Y += (y * cellSize) + FMath::RandRange(0.0f, cellSize);

   return location;
}

void UOSEGoalNodeComponent::AuthorityReleaseGuardLocation(AActor* guard)
{
   // This seems to happen during EndPlay.
   if (guard == nullptr)
   {
      return;
   }

   check(GetOwner());
   check(GetOwner()->HasAuthority());

   for (int32 i = 0; i < _guardLocationAssignments.Num(); i++)
   {
      if (_guardLocationAssignments[i] == guard)
      {
         _guardLocationAssignments[i] = nullptr;
         return;
      }
   }
   UE_LOG(LogGoalNode, Warning, TEXT("%s: No assignment found for guard %s on release."), *GetName(), *guard->GetName());
}

#if WITH_EDITOR
void UOSEGoalNodeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
   RefreshDebugRepresentation();
}

void UOSEGoalNodeComponent::RefreshDebugRepresentation()
{
   if (DebugSphereComponent)
   {
      DebugSphereComponent->SetVisibility(DrawDebug);
      DebugSphereComponent->SetSphereRadius(GoalRadius);
      // The sprite components don't get updated in real time without forcing render state dirty
      DebugSphereComponent->MarkRenderStateDirty();
   }
}
#endif

