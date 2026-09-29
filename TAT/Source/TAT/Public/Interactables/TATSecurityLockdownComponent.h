// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue 
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATSecurityLockdownComponent.generated.h"


class ATATSquadAlarmStation;
struct FTATAuthorityBreakContext;

USTRUCT()
struct FSecurityLockdownState
{
   GENERATED_BODY()

   UPROPERTY(Transient)
   bool IsInLockdown { false };
};

// This component will fire an event when its connected alarms activate, and will fire another event when
// all of the alarms are deactivated. Currently used on Doors and Windows as a way of forcing them to close and lock
UCLASS(BlueprintType, hideCategories = (ComponentTick, ComponentReplication, Replication, Activation, Collision, Sockets, Tags, Cooking), Meta = (BlueprintSpawnableComponent))
class TAT_API UTATSecurityLockdownComponent : public UActorComponent
{
   GENERATED_BODY()
public:
   UTATSecurityLockdownComponent();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSecurityLockdownStateChanged, bool, bLockedDown);
   UPROPERTY(BlueprintAssignable)
   FSecurityLockdownStateChanged OnSecurityLockdownStateChanged;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void SetTriggerAlarmsOnBreak(bool shouldTrigger) { _triggerAlarmsOnBreak = shouldTrigger; }

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void TriggerConnectedAlarms();

   TConstArrayView<TObjectPtr<ATATSquadAlarmStation>> GetConnectedAlarms() const { return _connectedAlarms; }
   
#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR
protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UPROPERTY(EditInstanceOnly)
   TArray<TObjectPtr<ATATSquadAlarmStation>> _connectedAlarms;

   // If true, will trigger if connected alarms when this actor is broken
   UPROPERTY(EditAnywhere)
   bool _triggerAlarmsOnBreak = true;

   UFUNCTION()
   void OnAlarmStateChanged(EAlarmState newState, EAlarmState previousState, bool isRecent);
   void _AuthorityOnBroken(const FTATAuthorityBreakContext& context);
   void OnStateChanged() const;
private:
   UPROPERTY(Transient)
   FSecurityLockdownState _state;
};
