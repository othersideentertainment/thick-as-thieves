// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATNavLinkCustomComponent.h"

// ue
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"

#include "TATNavLinkOwnerComponent.generated.h"

class UTATPathFollowingComponent;
class UTATNavLinkCustomComponent;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATNavLinkOwnerComponent : public UActorComponent
{
   GENERATED_BODY()

   DECLARE_MULTICAST_DELEGATE_OneParam(FTATNavLinkReservationChange, int32 /*registeredAgentCount*/);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATNavLinkNavAgentEvent, const UPathFollowingComponent*, agent);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTATNavLinkNavAgentChangeEvent, const UPathFollowingComponent*, newAgent, const UPathFollowingComponent*, oldAgent);

public:
   UTATNavLinkOwnerComponent();

   void RegisterNavLink(UTATNavLinkCustomComponent* navLink);
   void UnregisterNavLink(UTATNavLinkCustomComponent* navLink);

   virtual void ReserveForNavAgent(UPathFollowingComponent* pathFollowingComponent);
   virtual void UnReserveForNavAgent(UPathFollowingComponent* pathFollowingComponent);

   // Sets the passed-in agent as the traverser only if:
   // 1. There is no other agent currently traversing
   // 2. If this agent is closer to the nav-link than the agent already traversing
   UFUNCTION(BlueprintCallable)
   virtual bool TrySetNavAgentTraversingNavLink(UPathFollowingComponent* pathFollowingComponent);
   UFUNCTION(BlueprintCallable)
   virtual void RemoveNavAgentTraversingNavLink(UPathFollowingComponent* pathFollowingComponent);
   UFUNCTION(BlueprintCallable)
   virtual bool IsThisNavAgentTraversingNavLink(UPathFollowingComponent* pathFollowingComponent) const;

   UPROPERTY(BlueprintAssignable)
   FTATNavLinkNavAgentChangeEvent OnTraversingNavLinkAgentChanged;

   UPROPERTY(BlueprintAssignable)
   FTATNavLinkNavAgentEvent OnNavLinkAgentTraversalStarted;
   
   UPROPERTY(BlueprintAssignable)
   FTATNavLinkNavAgentEvent OnNavLinkAgentTraversalFinished;

   UFUNCTION(BlueprintCallable)
   FORCEINLINE int GetNumberOfAgentsReservingNavLink() const { return _agentsReservingNavLink.Num(); }
   
   UFUNCTION(BlueprintCallable)
   int GetNumberOfOtherAgentsReservingNavLink(UPathFollowingComponent* queryingPathFollowingComponent) const;

   FTATNavLinkReservationChange OnNavLinkReservationChanged;
private:
   UPROPERTY(Transient)
   TSet<UTATNavLinkCustomComponent*> _navLinks;

   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<const UPathFollowingComponent>> _agentsTraversing;

   UPROPERTY(Transient)
   TSet<UPathFollowingComponent*> _agentsReservingNavLink;

   void AddAgentTraversing(const UPathFollowingComponent* valueToSet);
   void RemoveAgentTraversing(const UPathFollowingComponent* valueToSet);
};
