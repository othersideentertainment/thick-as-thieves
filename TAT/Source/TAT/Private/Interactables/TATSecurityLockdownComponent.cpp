// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATSecurityLockdownComponent.h"

// tat
#include "Breakables/TATBreakableComponent.h"
#include "AI/Squad/TATSquadAlarmStation.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSecurityLockdownComponent)

UTATSecurityLockdownComponent::UTATSecurityLockdownComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UTATSecurityLockdownComponent::OnStateChanged() const
{
   OnSecurityLockdownStateChanged.Broadcast(_state.IsInLockdown);
}

void UTATSecurityLockdownComponent::TriggerConnectedAlarms()
{
   for (const TObjectPtr<ATATSquadAlarmStation>& connectedAlarm : _connectedAlarms)
   {
      if (connectedAlarm == nullptr)
      {
         continue;
      }
      connectedAlarm->AuthorityTrigger();
   }
}

#if WITH_EDITOR
void UTATSecurityLockdownComponent::CheckForErrors()
{
   Super::CheckForErrors();
   FMessageLog msgLog(FName("MapCheck"));
   
   for (const TObjectPtr<ATATSquadAlarmStation>& connectedAlarm : _connectedAlarms)
   {
      if(connectedAlarm == nullptr)
      {
         msgLog.Error()
                     ->AddToken(FUObjectToken::Create(this))
                     ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
                        TEXT("UTATSecurityLockdownComponent '%s' (BP: '%s') has an invalid alarm set in connected alarms "),
                        *GetName(),
                        *GetClass()->GetName()))));
         break;
      }
   }
}
#endif // WITH_EDITOR

void UTATSecurityLockdownComponent::BeginPlay()
{
   Super::BeginPlay();
   for (const TObjectPtr<ATATSquadAlarmStation>& connectedAlarm : _connectedAlarms)
   {
      if(connectedAlarm == nullptr)
      {
         continue;
      }
      connectedAlarm->OnAlarmStateChanged.AddUniqueDynamic(this, &UTATSecurityLockdownComponent::OnAlarmStateChanged);
   }

   if(GetOwner()->HasAuthority())
   {
      if (UTATBreakableComponent* breakableComponent = UTATBreakableComponent::GetBreakableComponentFromActor(GetOwner()))
      {
         breakableComponent->OnBrokenAuthority.AddUObject(this, &UTATSecurityLockdownComponent::_AuthorityOnBroken);
      }
   }
}

void UTATSecurityLockdownComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   for (const TObjectPtr<ATATSquadAlarmStation>& connectedAlarm : _connectedAlarms)
   {
      if(connectedAlarm == nullptr)
      {
         continue;
      }
      connectedAlarm->OnAlarmStateChanged.RemoveAll(this);
   }
   Super::EndPlay(endPlayReason);
}

void UTATSecurityLockdownComponent::OnAlarmStateChanged(EAlarmState newState, EAlarmState previousState, bool isRecent)
{
   bool bIsAlarmTriggered = false;
   for (const TObjectPtr<ATATSquadAlarmStation>& connectedAlarm : _connectedAlarms)
   {
      if(connectedAlarm->GetState() == EAlarmState::Triggered)
      {
         bIsAlarmTriggered = true;
      }
   }
   if(_state.IsInLockdown != bIsAlarmTriggered)
   {
      // not replicated
      // GetOwner()->FlushNetDormancy();
      _state.IsInLockdown = bIsAlarmTriggered;
      OnStateChanged();
   }
}

void UTATSecurityLockdownComponent::_AuthorityOnBroken(const FTATAuthorityBreakContext& context)
{
   if(_triggerAlarmsOnBreak)
   {
      TriggerConnectedAlarms();
   }
}

