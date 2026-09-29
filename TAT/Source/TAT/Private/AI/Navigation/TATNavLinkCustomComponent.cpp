// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Navigation/TATNavLinkCustomComponent.h"

// tat
#include "AI/TATAIController.h"
#include "AI/Navigation/TATNavLinkOwnerInterface.h"

// ue5
#include "AI/Navigation/NavigationRelevantData.h"
#include "Navigation/PathFollowingComponent.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNavLinkCustomComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATNavLinkCustomComponent, Log, All);

void UTATNavLinkCustomComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->GetClass()->ImplementsInterface(UTATNavLinkOwnerInterface::StaticClass()))
   {
      _ownerComponent = ITATNavLinkOwnerInterface::Execute_GetNavLinkOwnerComponent(GetOwner());
   }

   if (_ownerComponent == nullptr)
   {
      UE_LOG(LogTATNavLinkCustomComponent, Error, TEXT("[%s] TATNavLinkOwnerComponent not found on %s!"),
         *GetName(), *GetOwner()->GetName());
   }
}

bool UTATNavLinkCustomComponent::CanAgentMoveTowardsNavLinkEnd(const UObject* pathComp) const
{
   const UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
   check(agentStateData != nullptr);
   return agentStateData->CanAgentMove;
}

void UTATNavLinkCustomComponent::SetCanAgentMoveTowardsNavLinkEnd(const UObject* pathComp, const bool canAgentMove)
{
   UE_CVLOG(pathComp, pathComp, LogTemp, Log, TEXT("SetCanAgentMoveTowardsNavLinkEnd for %s"), *GetNameSafe(pathComp));
   UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
   check(agentStateData != nullptr);
   agentStateData->CanAgentMove = canAgentMove;
}

bool UTATNavLinkCustomComponent::CheckIfReachedEndOfNavLink(const UObject* pathComp) const
{
   const UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
   check(agentStateData != nullptr);
   return agentStateData->AgentReachedEndOfNavLink;
}

void UTATNavLinkCustomComponent::SetAgentCanUpdateNavSegment(const UObject* pathComp, const bool canAgentUpdateNavSegment)
{
   UE_CVLOG(pathComp, pathComp, LogTemp, Log, TEXT("SetAgentCanUpdateNavSegment for %s"), *GetNameSafe(pathComp));
   UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
   check(agentStateData != nullptr);
   agentStateData->CanAgentUpdateNavSegment = canAgentUpdateNavSegment;
}

bool UTATNavLinkCustomComponent::CanAgentUpdateNavSegment(const UObject* pathComp) const
{
   const UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
   check(agentStateData != nullptr);
   return agentStateData->CanAgentUpdateNavSegment;
}

bool UTATNavLinkCustomComponent::TriggerNavLinkReachedEndPoint(const UObject* pathComp)
{
   UE_CVLOG(pathComp, pathComp, LogTemp, Log, TEXT("TriggerNavLinkReachedEndPoint for %s"), *GetNameSafe(pathComp));
   UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
   check(agentStateData != nullptr);
   if(agentStateData->AgentReachedEndOfNavLink)
      return false;
   agentStateData->AgentReachedEndOfNavLink = true;
   OnTATNavLinkReachedEndPoint.Broadcast(pathComp);
   return true;
}

bool UTATNavLinkCustomComponent::OnLinkMoveStarted(UObject* pathComp, const FVector& destPoint)
{
   UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
   if(agentStateData == nullptr)
   {
      agentStateData = &_agentStateData.Emplace(pathComp, {});
   }

   agentStateData->MoveStarted();
   _numAgentsWaitingToTraverse++;
   const bool returnValue = Super::OnLinkMoveStarted(pathComp, destPoint);
   if(IsValid(pathComp))
   {
      if(ATATAIController* aiController = Cast<ATATAIController>(pathComp->GetOuter()))
      {
         // NOTE : do not update '_numAgentsWaitingToTraverse' when an AI starts to traverse
         // because we initially assume they are able to move until we are told otherwise.

         aiController->StartNavLinkBehavior(NavLinkBehavior, this);
         UE_CVLOG(aiController, aiController, LogTemp, Log, TEXT("StartNavLinkBehavior for %s"), *GetNameSafe(aiController));
         return true;
      }
   }
   return returnValue;
}

void UTATNavLinkCustomComponent::OnLinkMoveFinished(UObject* pathComp)
{
   if(IsValid(pathComp))
   {
      UTATNavLinkCustomComponent::FNavAgentStateData* agentStateData = GetStateDataForAgent(pathComp);
      check(agentStateData != nullptr);

      agentStateData->MoveEnded();

      _numAgentsWaitingToTraverse--;

      if(ATATAIController* aiController = Cast<ATATAIController>(pathComp->GetOuter()))
      {
         UE_VLOG(aiController, LogTemp, Log, TEXT("FinishNavLinkBehavior for %s"), *GetNameSafe(aiController));
         aiController->FinishNavLinkBehavior();
      }
   }
   Super::OnLinkMoveFinished(pathComp);
}

void UTATNavLinkCustomComponent::GetNavigationData(FNavigationRelevantData& data) const
{
   // Do not call super, we're doing that logic in here.
   // Super::GetNavigationData(data);

   TArray<FNavigationLink> navLinks;
   FNavigationLink linkMod = GetLinkModifier();
   linkMod.MaxFallDownLength = 0.f;
   linkMod.LeftProjectHeight = 0.f;
   linkMod.SnapHeight = SnapHeight;
   linkMod.bUseSnapHeight = SnapHeight > 0;
   linkMod.SnapRadius = SnapRadius; 
   navLinks.Add(linkMod);
   NavigationHelper::ProcessNavLinkAndAppend(&data.Modifiers, GetOwner(), navLinks);

   if (bCreateBoxObstacle)
   {
      data.Modifiers.Add(FAreaNavModifier(FBox::BuildAABB(ObstacleOffset, ObstacleExtent), GetOwner()->GetTransform(), ObstacleAreaClass));
   }
}

bool UTATNavLinkCustomComponent::HasReachedLinkStart(const UObject* pathComp, const FVector& currentLocation, const FNavPathPoint& linkStart, const FNavPathPoint& linkEnd) const
{
   const UPathFollowingComponent* pathFollowingComponent = CastChecked<UPathFollowingComponent>(pathComp);
   const float acceptanceRadius = _GetAcceptanceRadiusForNavAgent(pathFollowingComponent);
   return pathFollowingComponent->HasReached(linkStart, EPathFollowingReachMode::ExactLocation, acceptanceRadius);
}

UTATNavLinkCustomComponent::FNavAgentStateData* UTATNavLinkCustomComponent::GetStateDataForAgent(const UObject* agent)
{
   return _agentStateData.Find(agent);
}

const UTATNavLinkCustomComponent::FNavAgentStateData* UTATNavLinkCustomComponent::GetStateDataForAgent(const UObject* agent) const
{
   return _agentStateData.Find(agent);
}

float UTATNavLinkCustomComponent::_GetAcceptanceRadiusForNavAgent(const UPathFollowingComponent* agent)
{
   check(agent != nullptr);
   const float radius = agent->GetDefaultAcceptanceRadius();
   return radius;
}
