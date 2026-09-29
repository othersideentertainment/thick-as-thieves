// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Navigation/TATNavLinkOwnerComponent.h"

// ue5
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNavLinkOwnerComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATNavLinkOwnerComponent, Log, All);

namespace TATNavLinkUtilities
{
   static FString GetNavAgentInfo(const UPathFollowingComponent* pathFollowingComponent)
   {
      return (pathFollowingComponent != nullptr) 
         ? pathFollowingComponent->GetFullName()
         : TEXT("NULL");
   }
}

UTATNavLinkOwnerComponent::UTATNavLinkOwnerComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UTATNavLinkOwnerComponent::RegisterNavLink(UTATNavLinkCustomComponent* navLink)
{
   if (navLink != nullptr)
   {
      _navLinks.Add(navLink);
   }
   else
   {
      UE_LOG(LogTATNavLinkOwnerComponent, Error, TEXT("[%s] RegisterNavLink called with null nav-link!"), *GetName());
   }
}

void UTATNavLinkOwnerComponent::UnregisterNavLink(UTATNavLinkCustomComponent* navLink)
{
   if (navLink != nullptr)
   {
      _navLinks.Remove(navLink);
   }
   else
   {
      UE_LOG(LogTATNavLinkOwnerComponent, Error, TEXT("[%s] UnregisterNavLink called with null nav-link!"), *GetName());
   }
}

void UTATNavLinkOwnerComponent::ReserveForNavAgent(UPathFollowingComponent* pathFollowingComponent)
{
   if (pathFollowingComponent != nullptr)
   {
      bool alreadyExists = false;
      _agentsReservingNavLink.Add(pathFollowingComponent, &alreadyExists);
      UE_CVLOG(pathFollowingComponent->GetOwner(),
         pathFollowingComponent->GetOwner(),
         LogTATNavLinkOwnerComponent, 
         Log, 
         TEXT("ReserveForNavAgent for %s new count %i"),
         *GetNameSafe(pathFollowingComponent->GetOwner()),
         _agentsReservingNavLink.Num());
      AddAgentTraversing(pathFollowingComponent);

      if (!alreadyExists)
      {
         OnNavLinkReservationChanged.Broadcast(GetNumberOfAgentsReservingNavLink());
      }
   }
   else
   {
      UE_LOG(LogTATNavLinkOwnerComponent, Error, TEXT("[%s] ReserveForNavAgent called with null agent!"), *GetName());
   }
}

void UTATNavLinkOwnerComponent::UnReserveForNavAgent(UPathFollowingComponent* pathFollowingComponent)
{
   if (pathFollowingComponent != nullptr)
   {
      RemoveAgentTraversing(pathFollowingComponent);
      const int32 numRemoved = _agentsReservingNavLink.Remove(pathFollowingComponent);
      UE_CVLOG(pathFollowingComponent->GetOwner(),
         pathFollowingComponent->GetOwner(),
         LogTATNavLinkOwnerComponent,
         Log,
         TEXT("UnReserveForNavAgent for %s new count %i"),
         *GetNameSafe(pathFollowingComponent->GetOwner()),
         _agentsReservingNavLink.Num());

      if (numRemoved > 0)
      {
         // AI shouldn't attempt to unreserve it's spot with this nav-link owner before
         // it removes itself as the navigating agent (if applicable) but in case one
         // does, make sure we clear it as the navigating agent.
         const bool isCurrentlyTraversing = IsThisNavAgentTraversingNavLink(pathFollowingComponent);

         ensureMsgf(isCurrentlyTraversing == false, TEXT("[%s] UnReserveForNavAgent called for a nav agent (%s) who was still traversing!"),
            *GetName(), *pathFollowingComponent->GetName());
         
         if (isCurrentlyTraversing == false)
         {
            RemoveNavAgentTraversingNavLink(pathFollowingComponent);
         }

         OnNavLinkReservationChanged.Broadcast(GetNumberOfAgentsReservingNavLink());
      }
   }
   else
   {
      UE_LOG(LogTATNavLinkOwnerComponent, Error, TEXT("[%s] UnReserveForNavAgent called with null agent!"), *GetName());
   }
}

bool UTATNavLinkOwnerComponent::TrySetNavAgentTraversingNavLink(UPathFollowingComponent* pathFollowingComponent)
{
   OnNavLinkAgentTraversalStarted.Broadcast(pathFollowingComponent);
   return true;
}

void UTATNavLinkOwnerComponent::RemoveNavAgentTraversingNavLink(UPathFollowingComponent* pathFollowingComponent)
{
   OnNavLinkAgentTraversalFinished.Broadcast(pathFollowingComponent);
}

bool UTATNavLinkOwnerComponent::IsThisNavAgentTraversingNavLink(UPathFollowingComponent* pathFollowingComponent) const
{
   return _agentsTraversing.Contains(pathFollowingComponent);
}

int UTATNavLinkOwnerComponent::GetNumberOfOtherAgentsReservingNavLink(
   UPathFollowingComponent* queryingPathFollowingComponent) const
{
   int count = GetNumberOfAgentsReservingNavLink();
   const int actualCount = count;
   if(_agentsReservingNavLink.Contains(queryingPathFollowingComponent))
   {
      count--;
   }
   UE_CVLOG(queryingPathFollowingComponent->GetOwner(), queryingPathFollowingComponent->GetOwner(), LogTemp, Log,
            TEXT("GetNumberOfAgentsReservingNavLink for %s is %i (actual is %i)"),
            *GetNameSafe(queryingPathFollowingComponent->GetOwner()),
            count,
            actualCount);
   return count;
}

ACharacter* GetCharacterFromPathFollowingComponent(const UPathFollowingComponent* component)
{
   if(component == nullptr)
      return nullptr;
   const AController* controller = Cast<AController>(component->GetOwner());
   if(controller == nullptr)
      return nullptr;
   ACharacter* controllerCharacter = controller->GetCharacter();
   return controllerCharacter;
}

void SetCharacterCollisionWithActor(const bool enableCollision,
                                    const UPathFollowingComponent* component,
                                    const TArray<TWeakObjectPtr<const UPathFollowingComponent>>& otherAgents)
{
   if(component == nullptr)
      return;
   ACharacter* characterFromComponent = GetCharacterFromPathFollowingComponent(component);
   if(characterFromComponent == nullptr)
      return;

   if(UCharacterMovementComponent* movementComponent = Cast<UCharacterMovementComponent>(characterFromComponent->GetMovementComponent()))
   {
      movementComponent->SetAvoidanceEnabled(enableCollision);
   }
   for (const TWeakObjectPtr<const UPathFollowingComponent>& linkToAgentMap : otherAgents)
   {
      const UPathFollowingComponent* navLinkAgent = linkToAgentMap.Get();
      if(navLinkAgent && navLinkAgent != component)
      {
         if(ACharacter* character = GetCharacterFromPathFollowingComponent(navLinkAgent))
         {
            if(enableCollision)
            {
               character->MoveIgnoreActorRemove(characterFromComponent);
               characterFromComponent->MoveIgnoreActorRemove(character);
            }
            else
            {
               character->MoveIgnoreActorAdd(characterFromComponent);
               characterFromComponent->MoveIgnoreActorAdd(character);
            }
         }
      }
   }
}

void UTATNavLinkOwnerComponent::AddAgentTraversing(const UPathFollowingComponent* valueToSet)
{
   _agentsTraversing.AddUnique(valueToSet);
   SetCharacterCollisionWithActor(false, valueToSet, _agentsTraversing);
}

void UTATNavLinkOwnerComponent::RemoveAgentTraversing(const UPathFollowingComponent* valueToSet)
{
   SetCharacterCollisionWithActor(true, valueToSet, _agentsTraversing);
   _agentsTraversing.Remove(valueToSet);
}
