// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Navigation/TATPathFollowingComponent.h"

// tat
#include "AI/Navigation/TATNavLinkOwnerInterface.h"
#include "Interactables/SwingingDoorNavLinkComponent.h"

// ose
#include "OSESchedulerWorldSubsystem.h"


// ue5
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPathFollowingComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATPathFollowingComponent, Log, All);

namespace TATPathFollowingComponentCVars
{
   static int32 DebugDoors = 0;
   FAutoConsoleVariableRef CVarDebugDoors(
      TEXT("tat.AI.DebugDoors"),
      DebugDoors,
      TEXT("Should we show door debugging"),
      ECVF_Default);
   static int32 DebugGoalOffset = 0;
   FAutoConsoleVariableRef CVarDebugGoalOffset(
      TEXT("tat.AI.DebugGoalOffset"),
      DebugGoalOffset,
      TEXT("Should we show goal offset for the target"),
      ECVF_Default);
}

namespace TATPathFollowingComponent
{
   static FName SchedulerWorkloadGroupName(TEXT("TATPathFollowingComponent"));
}


void UTATPathFollowingComponent::BeginPlay()
{
   Super::BeginPlay();
   AActor* owner = GetOwner();
   if (owner != nullptr)
   {
      owner->OnEndPlay.AddDynamic(this, &ThisClass::OnOwnerEndPlay);
   }
}

void UTATPathFollowingComponent::OnOwnerEndPlay(AActor* actor, EEndPlayReason::Type arg)
{
   UWorld* world = GetWorld();
   if(world == nullptr)
   {
      UE_LOG(LogTemp, Error, TEXT("No world?"));   
   }
   else
   {
      UOSESchedulerWorldSubsystem* scheduler = world->GetSubsystem<UOSESchedulerWorldSubsystem>();
    //  scheduler->RemoveScheduledTask(_scheduledTaskHandle);
   }
}

void UTATPathFollowingComponent::BeginDestroy()
{
   if (HasAnyFlags(RF_BeginDestroyed) == false)
   {
      AActor* Owner = GetOwner();
      if (Owner != nullptr)
      {
         Owner->OnEndPlay.RemoveDynamic(this, &ThisClass::OnOwnerEndPlay);
      }
   }
  
   Super::BeginDestroy();
}


FVector UTATPathFollowingComponent::GetMoveFocus(bool bAllowStrafe) const
{
   if (bAllowStrafe && DestinationActor.IsValid())
   {
      return Super::GetMoveFocus(bAllowStrafe);
   }
   const FVector CurrentMoveDirection = GetCurrentDirection();
   FVector moveFocus = *CurrentDestination + (CurrentMoveDirection * MoveFocusLookForwardAmount);
   if(IsValid(OwnerPawn) && bShouldFocusAtEyeHeightWhenMoving)
   {
      FVector eyePosition;
      FRotator eyeRotation;
      OwnerPawn->GetActorEyesViewPoint(eyePosition, eyeRotation);
      moveFocus.Z = eyePosition.Z;
   }
   return moveFocus;
}

void UTATPathFollowingComponent::UpdateCachedComponents()
{
   Super::UpdateCachedComponents();
   OwnerPawn = Cast<APawn>(GetOwner());
   if (OwnerPawn == nullptr)
   {
      if (const AController* controller = Cast<AController>(GetOwner()))
      {
         OwnerPawn = controller->GetPawn();
      }
   }
}

bool UTATPathFollowingComponent::HasReachedCurrentTarget(const FVector& currentLocation) const
{
   const bool returnVal = Super::HasReachedCurrentTarget(currentLocation);
   if(UTATNavLinkCustomComponent* tatNavLinkComponent = Cast<UTATNavLinkCustomComponent>(GetCurrentCustomLinkOb()))
   {
      if (returnVal)
      {
         if(tatNavLinkComponent->TriggerNavLinkReachedEndPoint(this))
         {
            UE_CVLOG(OwnerPawn, OwnerPawn, LogTemp, Log, TEXT("HasReachedCurrentTarget for %s - triggering end point"), *GetNameSafe(OwnerPawn));
         }
      }
      if(tatNavLinkComponent->CanAgentUpdateNavSegment(this) == false)
      {
         return false;
      }
   }
   return returnVal;
}

void UTATPathFollowingComponent::UpdatePathSegment()
{
   if(const UTATNavLinkCustomComponent* tatNavLinkComponent = Cast<UTATNavLinkCustomComponent>(GetCurrentCustomLinkOb()))
   {
      if(tatNavLinkComponent->CanAgentUpdateNavSegment(this) == false)
      {
         return;
      }
   }
   Super::UpdatePathSegment();
}
void UTATPathFollowingComponent::UnReserveNavLink()
{
   if(IsValid(_futureNavLinkOwner))
   {
      _futureNavLinkOwner->UnReserveForNavAgent(this);
      _futureNavLinkOwner = nullptr;
   }
   _futureNavLinkSegmentIndex = 0;
}
void UTATPathFollowingComponent::OnPathUpdated()
{
   Super::OnPathUpdated();
   UE_CVLOG(OwnerPawn, OwnerPawn, LogTemp, Log, TEXT("OnPathUpdated for %s"), *GetNameSafe(OwnerPawn));
   UnReserveNavLink();
}

void UTATPathFollowingComponent::OnPathFinished(const FPathFollowingResult& result)
{
   Super::OnPathFinished(result);
   UE_CVLOG(OwnerPawn, OwnerPawn, LogTemp, Log, TEXT("OnPathFinished for %s"), *GetNameSafe(OwnerPawn));
   UnReserveNavLink();
}

void UTATPathFollowingComponent::SetMoveSegment(const int32 segmentStartIndex)
{
   Super::SetMoveSegment(segmentStartIndex);
   const int32 endSegmentIndex = segmentStartIndex + 1;
   const FNavigationPath* pathInstance = Path.Get();
   if (pathInstance != nullptr && pathInstance->GetPathPoints().IsValidIndex(segmentStartIndex) && pathInstance->GetPathPoints().IsValidIndex(endSegmentIndex))
   {
      // Get the nav path point for the next segment so we can check if it has a custom link
      const FNavPathPoint& nextPathPoint = pathInstance->GetPathPoints()[endSegmentIndex];
      if (nextPathPoint.CustomNavLinkId != FNavLinkId::Invalid)
      {
         const UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
         if(UTATNavLinkCustomComponent* customNavLink = Cast<UTATNavLinkCustomComponent>(navSys->GetCustomLink(nextPathPoint.CustomNavLinkId)))
         {
            if (UTATNavLinkOwnerComponent* navLinkOwnerComponent = customNavLink->GetNavLinkOwner())
            {
               navLinkOwnerComponent->ReserveForNavAgent(this);
               _futureNavLinkOwner = navLinkOwnerComponent;
               _futureNavLinkSegmentIndex = endSegmentIndex;
               UE_CVLOG(OwnerPawn, OwnerPawn, LogTemp, Log, TEXT("SetMoveSegment for %s - reserving"), *GetNameSafe(OwnerPawn));
            }
         }
      }
   }
}

void UTATPathFollowingComponent::OnSegmentFinished()
{
   if(MoveSegmentStartIndex == _futureNavLinkSegmentIndex)
   {
      const auto owner = GetOwner();
      UE_VLOG(owner, LogTemp, Log, TEXT("OnSegmentFinished for %s"), *GetNameSafe(owner));
      UnReserveNavLink();
   }
   Super::OnSegmentFinished();
}

void UTATPathFollowingComponent::FollowPathSegment(const float deltaTime)
{
   INavMovementInterface* movementInterface = NavMovementInterface.Get();
   const float agentRadius = movementInterface->GetNavAgentPropertiesRef().AgentRadius;
   const FVector targetMovementPosition = movementInterface->GetFeetLocation() + CurrentMoveInput;
   
   if(const UTATNavLinkCustomComponent* tatNavLinkComponent = Cast<UTATNavLinkCustomComponent>(GetCurrentCustomLinkOb()))
   {
#if ENABLE_DRAW_DEBUG
      if(TATPathFollowingComponentCVars::DebugDoors)
      {
         DrawDebugLine(GetWorld(), targetMovementPosition, GetCurrentTargetLocation(), FColor::Yellow);
      }
#endif
      if (movementInterface && HasReachedCurrentTarget(movementInterface->GetFeetLocation()))
      {
#if ENABLE_DRAW_DEBUG
         if(TATPathFollowingComponentCVars::DebugDoors)
         {
            DrawDebugSphere(GetWorld(), targetMovementPosition, agentRadius, 8, FColor::Red, false);
         }
#endif
         // We've reached the end, now awaiting the path segment to be updated via CanAgentUpdateNavSegment to return true
         return;
      }
      if(tatNavLinkComponent->CanAgentMoveTowardsNavLinkEnd(this) == false)
      {
#if ENABLE_DRAW_DEBUG
         if(TATPathFollowingComponentCVars::DebugDoors)
         {
            DrawDebugSphere(GetWorld(), targetMovementPosition, agentRadius, 8, FColor::Cyan, false);
         }
#endif
         return;
      }
#if ENABLE_DRAW_DEBUG
      if(TATPathFollowingComponentCVars::DebugDoors)
      {
         DrawDebugSphere(GetWorld(), targetMovementPosition, agentRadius, 8, FColor::Yellow, false);
      }
#endif
   }
   else
   {
#if ENABLE_DRAW_DEBUG
      if(TATPathFollowingComponentCVars::DebugDoors)
      {
         DrawDebugLine(GetWorld(), targetMovementPosition, GetCurrentTargetLocation(), FColor::Red);
      }
#endif
   }
   Super::FollowPathSegment(deltaTime);
}

FVector UTATPathFollowingComponent::TransformGoalLocation(const AActor& goalActor,
                                                          const FVector& goalActorLocation,
                                                          const FVector& offset) const
{
#if ENABLE_DRAW_DEBUG
   if(TATPathFollowingComponentCVars::DebugGoalOffset != 0)
   {
      DrawDebugLine(GetWorld(), goalActorLocation, goalActorLocation + offset, FColor::Green, false, 0.f ,0.f, 3.f);
   }
#endif
   return goalActorLocation + offset;
}

FVector UTATPathFollowingComponent::GetRemainingPathSegmentVector() const
{
   if (const INavMovementInterface* movementInterface = NavMovementInterface.Get())
   {
      return (GetCurrentTargetLocation() - movementInterface->GetFeetLocation());
   }
   else
   {
      UE_LOG(LogTATPathFollowingComponent, Error, TEXT("[%s] GetRemainingPathSegmentVector called while MovementComp was null!"),
         *GetName());

      return FVector::ZeroVector;
   }
}
