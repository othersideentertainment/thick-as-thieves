// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Alertness/OSEAlertnessComponent.h"

// ose
#include "AI/OSEAIController.h"
#include "AI/Alertness/OSEAlertnessAsset.h"

// ue4
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAlertnessComponent)

DEFINE_LOG_CATEGORY(LogAlertnessComponent);

UOSEAlertnessComponent::UOSEAlertnessComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   SetIsReplicatedByDefault(true);
}

void UOSEAlertnessComponent::BeginPlay()
{
   Super::BeginPlay();
   // sanity BeginPlay check
   const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(GetOwner());
   checkf(alertnessInterface != nullptr, TEXT("Please implement IOSEAlertnessInterface on the owner of alertness component."));
}

void UOSEAlertnessComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _alertnessLevel, params);
}

void UOSEAlertnessComponent::SetMinAndMaxAlertnessLevel(const EAlertnessLevel minAlertnessLevel, const EAlertnessLevel maxAlertnessLevel)
{
   MinAlertnessLevel = minAlertnessLevel;
   MaxAlertnessLevel = maxAlertnessLevel;
}

bool CanChangeAlertness(const AActor* owner, const UOSEAlertnessSettingsAsset* alertnessSettingsAsset)
{
   if(IsValid(owner) == false)
   {
      UE_LOG(LogAlertnessComponent, Error, TEXT("AuthorityRaiseAlertLevelToAtLeast called with an invalid owner!"));
      return false;
   }
   if(owner->HasAuthority() == false)
   {
      UE_LOG(LogAlertnessComponent, Error, TEXT("%s AuthorityRaiseAlertLevelToAtLeast called on a client!"), *GetNameSafe(owner));
      return false;
   }
   if (IsValid(alertnessSettingsAsset) == false)
   {
      UE_LOG(LogAlertnessComponent, Error, TEXT("%s AuthorityRaiseAlertLevelToAtLeast on UOSEAlertnessComponent but no alertness settings configured!"), *GetNameSafe(owner));
      return false;
   }
   return true;
}

void UOSEAlertnessComponent::AuthorityRaiseAlertnessLevelToAtLeast(EAlertnessLevel alertnessLevel, AActor* optionalInstigator /* = nullptr */)
{
   if(CanChangeAlertness(GetOwner(), AlertnessSettingsAsset) == false)
      return;
   
   // whether or not we actually raise the level, hit the callback; something wanted us to stay in this alert level or higher
   if (alertnessLevel >= GetAlertnessLevel())
   {
      _OnAlertnessLevelChangeRequested();
   }

   // raise the level if it's higher
   if (alertnessLevel > GetAlertnessLevel())
   {
      AActor* instigator = optionalInstigator ? optionalInstigator : GetOwner();
      _SetAlertnessLevel(instigator, alertnessLevel);
   }
}

void UOSEAlertnessComponent::AuthorityLowerAlertnessLevelToAtMost(const EAlertnessLevel alertnessLevel, AActor* optionalInstigator /* = nullptr */)
{
   if(CanChangeAlertness(GetOwner(), AlertnessSettingsAsset) == false)
      return;

   // whether or not we actually lower the level, hit the callback; something wanted us to stay in this alert level or lower
   if (alertnessLevel <= GetAlertnessLevel())
   {
      _OnAlertnessLevelChangeRequested();
   }

   // lower the level if it's lower
   if (alertnessLevel < GetAlertnessLevel())
   {
      AActor* instigator = optionalInstigator ? optionalInstigator : GetOwner();
      _SetAlertnessLevel(instigator, alertnessLevel);
   }
}

void UOSEAlertnessComponent::_OnRep_AlertLevel(const EAlertnessLevel oldAlertnessLevel)
{
   _BroadcastAlertLevelChanged(oldAlertnessLevel);
}

void UOSEAlertnessComponent::_BroadcastAlertLevelChanged(const EAlertnessLevel oldAlertnessLevel)
{
   UE_LOG(LogAlertnessComponent, Verbose, TEXT("%s alertness level is %s (was: %s)"), *GetOwner()->GetName(), *UEnum::GetValueAsString(_alertnessLevel), *UEnum::GetValueAsString(oldAlertnessLevel));
   _OnAlertnessLevelChanged(oldAlertnessLevel, _alertnessLevel);
   OnAlertnessLevelChanged.Broadcast(oldAlertnessLevel, _alertnessLevel);
}

void UOSEAlertnessComponent::_AuthorityBroadcastAlertLevelChanged(const EAlertnessLevel oldAlertnessLevel, AActor* instigator) const
{
   UE_LOG(LogAlertnessComponent, Verbose, TEXT("[Authority] %s alertness level is %s (was: %s) caused by %s"), *GetOwner()->GetName(), *UEnum::GetValueAsString(_alertnessLevel), *UEnum::GetValueAsString(oldAlertnessLevel), *AActor::GetDebugName(instigator));
   // server-only broadcast that can come along w/ the instigator of the alertness change.  useful for targeting the
   // actor that caused the alertness bump in reaction to the bump
   AuthorityOnAlertnessLevelChanged.Broadcast(oldAlertnessLevel, _alertnessLevel, instigator);
}

void UOSEAlertnessComponent::_AuthorityResetAlertLevel()
{
   _SetAlertnessLevel(GetOwner(), EAlertnessLevel::Neutral);
}

bool UOSEAlertnessComponent::_SetAlertnessLevel(AActor* instigator, const EAlertnessLevel level)
{
   ensure(instigator);
   check(GetOwner()->HasAuthority());
   if (level < MinAlertnessLevel || level > MaxAlertnessLevel)
   {
      return false;
   }

   const EAlertnessLevel oldAlertnessLevel = _alertnessLevel;
   _alertnessLevel = level;

   if(_alertnessLevel != oldAlertnessLevel)
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _alertnessLevel, this);

      _OnAlertnessLevelChangeRequested();
      _BroadcastAlertLevelChanged(oldAlertnessLevel);
      _AuthorityBroadcastAlertLevelChanged(oldAlertnessLevel, instigator);
      return true;
   }

   _OnAlertnessLevelChangeRefreshed();
   return false;
}
