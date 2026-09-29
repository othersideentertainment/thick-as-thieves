// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue
#include "CoreMinimal.h"
#include "NavLinkCustomComponent.h"

#include "TATNavLinkCustomComponent.generated.h"

class UPathFollowingComponent;
class UTATNavLinkOwnerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTATNavLinkReachedEndPoint, const UObject*, pathFollowingComponent);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATNavLinkCustomComponent : public UNavLinkCustomComponent
{
   GENERATED_BODY()

   struct FNavAgentStateData
   {
      bool IsMoveActive = false;

      // Assume a nav-agent can move until told otherwise.
      bool CanAgentMove = false;
      bool CanAgentUpdateNavSegment = true;
      bool AgentReachedEndOfNavLink = false;

      void MoveStarted()
      {
         IsMoveActive = true;
      }

      void MoveEnded()
      {
         IsMoveActive = false;

         // Reset values back to default.
         CanAgentMove = true;
         CanAgentUpdateNavSegment = true;
         AgentReachedEndOfNavLink = false;
      }

      bool IsWaitingToTraverse() const
      {
         return IsMoveActive && !CanAgentMove;
      }
   };

public:
   // from UActorComponent
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintCallable)
   bool CanAgentMoveTowardsNavLinkEnd(const UObject* pathComp) const;
   UFUNCTION(BlueprintCallable)
   void SetCanAgentMoveTowardsNavLinkEnd(const UObject* pathComp, const bool canAgentMove);

   UFUNCTION(BlueprintCallable)
   bool CanAgentUpdateNavSegment(const UObject* pathComp) const;
   UFUNCTION(BlueprintCallable)
   void SetAgentCanUpdateNavSegment(const UObject* pathComp, const bool canAgentUpdateNavSegment);

   bool TriggerNavLinkReachedEndPoint(const UObject* pathComp);
   bool CheckIfReachedEndOfNavLink(const UObject* pathComp) const;

   // from UNavLinkCustomComponent
   virtual bool OnLinkMoveStarted(UObject* pathComp, const FVector& destPoint) override;
   virtual void OnLinkMoveFinished(UObject* pathComp) override;
   virtual void GetNavigationData(FNavigationRelevantData& data) const override;
   virtual bool IsLinkUsingCustomReachCondition(const UObject* pathComp) const override { return true; }
   virtual bool HasReachedLinkStart(const UObject* pathComp, const FVector& currentLocation, const FNavPathPoint& linkStart, const FNavPathPoint& linkEnd) const override;

   UFUNCTION(BlueprintCallable)
   FORCEINLINE UTATNavLinkOwnerComponent* GetNavLinkOwner() const { return _ownerComponent; }
   
   UPROPERTY(BlueprintAssignable)
   FOnTATNavLinkReachedEndPoint OnTATNavLinkReachedEndPoint;
   
   UPROPERTY(EditAnywhere, Category="Behavior")
   TSubclassOf<UOSEGameplayAbility> NavLinkBehavior { nullptr };

   UPROPERTY(EditAnywhere, Category="Snapping", meta=(DisplayPriority=2))
   float SnapHeight { 100.f };
   
   UPROPERTY(EditAnywhere, Category="Snapping", meta=(DisplayPriority=2))
   float SnapRadius { 100.f };

   // Float curve which defines additional acceptance radius amount for agents arriving
   // to this nav-link's starting position per the number of agents already waiting.
   // Useful to space out agents from other waiting agents.
   UPROPERTY(EditAnywhere, Category = "Config", meta = (DisplayPriority = 2))
   TObjectPtr<UCurveFloat> AcceptanceRadiusDeltaByWaitingCount = nullptr;

private:
   
   UPROPERTY(Transient)
   TObjectPtr<UTATNavLinkOwnerComponent> _ownerComponent;

   TMap<TWeakObjectPtr<const UObject>, FNavAgentStateData> _agentStateData;

   // The number of nav agents that have arrived at this nav-link's starting location
   // but are not yet able to move.
   int _numAgentsWaitingToTraverse = 0;

   FNavAgentStateData* GetStateDataForAgent(const UObject* agent);
   const FNavAgentStateData* GetStateDataForAgent(const UObject* agent) const;

   // Acceptance radius for AI navigating to the nav-link starting location.
   // Using 'AcceptanceRadiusDeltaByWaitingCount' we expand the radius to space AI out
   // from one another when waiting to traverse.
   static float _GetAcceptanceRadiusForNavAgent(const UPathFollowingComponent* agent);
};
