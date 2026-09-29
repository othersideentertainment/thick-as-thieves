// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "UI/TATHUDIndicatorSubsystem.h"

// tat
#include "UI/TATHUD.h"
#include "Player/TATPlayerState.h"
#include "Abilities/TATGameplayEffectUIData_HUDIndicator.h"
#include "UI/TATHUDIndicatorWidgetInterface.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

// wwise
#include "AkAudioEvent.h"

// ue
#include "Blueprint/UserWidget.h"
#include "Logging/LogVerbosity.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHUDIndicatorSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATHUDIndicatorSubsystem, Log, All);

void UTATHUDIndicatorSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATHUDIndicatorSubsystem::Deinitialize()
{
   Super::Deinitialize();

   if (UOSEAbilitySystemComponent* asc = _abilitySystemComponent.Get())
   {
      _UnregisterFromPlayerEvents(asc);
   }
}

void UTATHUDIndicatorSubsystem::PlayerControllerChanged(APlayerController* newPlayerController)
{
   Super::PlayerControllerChanged(newPlayerController);

   if (newPlayerController != nullptr)
   {
      if (APlayerState* newPlayerState = newPlayerController->GetPlayerState<APlayerState>())
      {
         _InitSubsystemWithAbilitySystemComponent(UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(newPlayerState));
      }
   }
}

void UTATHUDIndicatorSubsystem::OnLocalPlayerStateReady(APlayerState* newPlayerState)
{
   if (!ensure(newPlayerState != nullptr))
   {
      return;
   }

   if (!_abilitySystemComponent.IsValid())
   {
      _InitSubsystemWithAbilitySystemComponent(UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(newPlayerState));
   }
}

bool UTATHUDIndicatorSubsystem::GetIndicatorByIndex(int32 index, FTATHUDIndicatorInfo& indicatorInfo) const
{
   if (_indicators.IsValidIndex(index))
   {
      indicatorInfo = _indicators[index];
      return true;
   }
   indicatorInfo = FTATHUDIndicatorInfo{};
   return false;
}

void UTATHUDIndicatorSubsystem::ForEachIndicator(TFunctionRef<void(int32, const FTATHUDIndicatorInfo&)> callback) const
{
   for (int32 i = 0; i < _indicators.Num(); i++)
   {
      callback(i, _indicators[i]);
   }
}

// static
bool UTATHUDIndicatorSubsystem::GetHUDIndicatorTimer(const UObject* contextObject, const FTATHUDIndicatorState& state, float& elapsedSeconds, float& normalizedValue)
{
   if (state.TimerDuration > 0.0f && state.TimerStartTime > 0.0f)
   {
      if (UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull))
      {
         const double now = world->GetTimeSeconds();
         elapsedSeconds = state.GetTimerElapsedSeconds(now);
         normalizedValue = state.GetTimerNormalizedValue(now);
         return true;
      }
   }
   elapsedSeconds = 0.0f;
   normalizedValue = 0.0f;
   return false;
}

void UTATHUDIndicatorSubsystem::_InitSubsystemWithAbilitySystemComponent(UOSEAbilitySystemComponent* newAsc)
{
   UOSEAbilitySystemComponent* prevAsc = _abilitySystemComponent.Get();
   if (prevAsc == newAsc)
   {
      return;
   }

   // If we have a previous ASC and it's not the same as the new one, reset all current state
   if (!_abilitySystemComponent.IsExplicitlyNull())
   {
      if (prevAsc != nullptr)
      {
         _UnregisterFromPlayerEvents(prevAsc);
      }
      // TODO: more proactively remove indicators on map change
      _RemoveAllIndicators();
      _abilitySystemComponent.Reset();
   }

   if (newAsc != nullptr)
   {
      _RegisterForPlayerEvents(newAsc);
   }
}

ATATHUD* UTATHUDIndicatorSubsystem::_GetHUD() const
{
   if (ULocalPlayer* localPlayer = GetLocalPlayer())
   {
      if (APlayerController* pc = localPlayer->GetPlayerController(GetWorld()))
      {
         return pc->GetHUD<ATATHUD>();
      }
   }
   return nullptr;
}

bool UTATHUDIndicatorSubsystem::_GameplayEffectToIndicatorState(const UTATGameplayEffectUIData_HUDIndicator& indicatorData, const FActiveGameplayEffect& effect, FTATHUDIndicatorState& outState) const
{
   outState.Icon = indicatorData.Icon;
   outState.Style = indicatorData.Style;
   outState.Progress = 0.0f;
   if (indicatorData.ShowTimer)
   {
      outState.TimerStartTime = effect.StartWorldTime;
      outState.TimerDuration = effect.GetDuration();
      outState.TimerDirection = indicatorData.TimerDirection;
   }
   else
   {
      outState.TimerStartTime = 0.0f;
      outState.TimerDuration = 0.0f;
      outState.TimerDirection = ETATHUDIndicatorTimerDirection::Decreasing;
   }
   outState.Count = indicatorData.ShowStackCount ? effect.Spec.GetStackCount() : 0;
   outState.Highlight = false;
   outState.Label = indicatorData.Label;
   return true;
}

bool UTATHUDIndicatorSubsystem::_GameplayEffectToIndicatorState(FActiveGameplayEffectHandle handle, FTATHUDIndicatorState& outState) const
{
   UOSEAbilitySystemComponent* asc = _abilitySystemComponent.Get();
   if (asc == nullptr)
   {
      outState = FTATHUDIndicatorState{};
      return false;
   }

   const FActiveGameplayEffect* activeEffect = asc->GetActiveGameplayEffect(handle);
   if (activeEffect == nullptr || activeEffect->Spec.Def == nullptr)
   {
      outState = FTATHUDIndicatorState{};
      return false;
   }

   const UTATGameplayEffectUIData_HUDIndicator* indicatorData = activeEffect->Spec.Def->FindComponent<UTATGameplayEffectUIData_HUDIndicator>();
   if (indicatorData == nullptr)
   {
      outState = FTATHUDIndicatorState{};
      return false;
   }

   return _GameplayEffectToIndicatorState(*indicatorData, *activeEffect, outState);
}

void UTATHUDIndicatorSubsystem::_PostLoadMap(UWorld* world)
{
   // TODO: Audit where/if this is needed. Is this to cover replication delays on the player state on clients?
   //       Could that be solved more directly?
   check(world != nullptr);
   if (_abilitySystemComponent.IsValid())
   {
      return;
   }
   if (ULocalPlayer* localPlayer = GetLocalPlayer())
   {
      if (APlayerController* pc = localPlayer->GetPlayerController(world))
      {
         if (UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(pc->GetPlayerState<APlayerState>()))
         {
            _InitSubsystemWithAbilitySystemComponent(asc);
         }
      }
   }
}

void UTATHUDIndicatorSubsystem::_OnWorldBeginPlay()
{
   // TODO: Audit where/if this is needed. Is this to cover replication delays on the player state on clients?
   //       Could that be solved more directly?
   if (_abilitySystemComponent.IsValid())
   {
      return;
   }
   if (ULocalPlayer* localPlayer = GetLocalPlayer())
   {
      if (APlayerController* pc = localPlayer->GetPlayerController(GetWorld()))
      {
         if (UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(pc->GetPlayerState<APlayerState>()))
         {
            _InitSubsystemWithAbilitySystemComponent(asc);
         }
      }
   }
}

int32 UTATHUDIndicatorSubsystem::_AddIndicatorFromGameplayEffect(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle)
{
   // Don't create any widgets if the world is tearing down.
   UWorld* world = GetWorld();
   if (!IsValid(world) || world->bIsTearingDown)
   {
      return INDEX_NONE;
   }

   if (asc == nullptr || !handle.IsValid() || spec.Def == nullptr)
   {
      return INDEX_NONE;
   }

   const UTATGameplayEffectUIData_HUDIndicator* indicatorData = spec.Def->FindComponent<UTATGameplayEffectUIData_HUDIndicator>();
   if (indicatorData == nullptr)
   {
      return INDEX_NONE;
   }

   const FActiveGameplayEffect* activeEffect = asc->GetActiveGameplayEffect(handle);
   if (activeEffect == nullptr)
   {
      return INDEX_NONE;
   }

   // If this handle already has an entry, just update it if needed
   const int32 existingIndex = _FindExistingIndicatorIndex(handle);
   if (existingIndex != INDEX_NONE)
   {
      FTATHUDIndicatorState newState{};
      if (_GameplayEffectToIndicatorState(*indicatorData, *activeEffect, newState) && newState != _indicators[existingIndex].State)
      {
         _indicators[existingIndex].State = newState;
         _UpdateIndicatorByIndex(existingIndex);
      }
      return existingIndex;
   }

   const int32 newIndex = _indicators.Emplace();
   FTATHUDIndicatorInfo& newIndicator = _indicators[newIndex];
   newIndicator.EffectHandle = handle;
   _GameplayEffectToIndicatorState(*indicatorData, *activeEffect, newIndicator.State);

   // Register for effect-specific events
   if (FOnActiveGameplayEffectTimeChange* delegate = asc->OnGameplayEffectTimeChangeDelegate(handle))
   {
      delegate->AddUObject(this, &UTATHUDIndicatorSubsystem::_OnGameplayEffectTimeChange);
   }
   if (FOnActiveGameplayEffectStackChange* delegate = asc->OnGameplayEffectStackChangeDelegate(handle))
   {
      delegate->AddUObject(this, &UTATHUDIndicatorSubsystem::_OnGameplayEffectStackChange);
   }

   // Let the HUD create a widget for the indicator
   if (ATATHUD* hud = _GetHUD())
   {
      newIndicator.Widget = hud->CreateWidgetForHUDIndicator(newIndicator.State);
   }

   return newIndex;
}

void UTATHUDIndicatorSubsystem::_UpdateIndicatorByIndex(int32 index)
{
   check(_indicators.IsValidIndex(index));
   FTATHUDIndicatorInfo& indicator = _indicators[index];

   // Notify the HUD
   if (ATATHUD* hud = _GetHUD())
   {
      hud->OnHUDIndicatorChanged(indicator);
   }

   // Notify the widget as well if it has the appropriate interface
   if (indicator.Widget && indicator.Widget->Implements<UTATHUDIndicatorWidgetInterface>())
   {
      ITATHUDIndicatorWidgetInterface::Execute_OnHUDIndicatorUpdated(indicator.Widget, indicator.State);
   }
}

void UTATHUDIndicatorSubsystem::_RemoveIndicatorByIndex(int32 index, bool removeFromArray)
{
   check(_indicators.IsValidIndex(index));
   FTATHUDIndicatorInfo& indicator = _indicators[index];

   // Let the HUD know the indicator is going away so it can remove the widget
   if (ATATHUD* hud = _GetHUD())
   {
      hud->OnHUDIndicatorRemoved(indicator);
   }

   // Notify the widget as well if it has the appropriate interface
   if (indicator.Widget && indicator.Widget->Implements<UTATHUDIndicatorWidgetInterface>())
   {
      ITATHUDIndicatorWidgetInterface::Execute_OnHUDIndicatorRemoved(indicator.Widget);
   }

   // Remove from the indicators array (optional to make bulk-removal easier)
   if (removeFromArray)
   {
      _indicators.RemoveAt(index);
   }
}

void UTATHUDIndicatorSubsystem::_RemoveAllIndicators()
{
   static constexpr bool removeFromArray = false;
   for (int32 i = 0; i < _indicators.Num(); i++)
   {
      _RemoveIndicatorByIndex(i, removeFromArray);
   }
   _indicators.Reset();
}

int32 UTATHUDIndicatorSubsystem::_FindExistingIndicatorIndex(FActiveGameplayEffectHandle handle) const
{
   for (int32 i = 0; i < _indicators.Num(); i++)
   {
      if (_indicators[i].EffectHandle == handle)
      {
         return i;
      }
   }
   return INDEX_NONE;
}

void UTATHUDIndicatorSubsystem::_RegisterForPlayerEvents(UOSEAbilitySystemComponent* asc)
{
   check(asc != nullptr);
   check(_abilitySystemComponent.IsExplicitlyNull());
   _abilitySystemComponent = asc;

   asc->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &ThisClass::_OnGameplayEffectWithDurationAdded);
   asc->OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &ThisClass::_OnAnyGameplayEffectAdded);
   asc->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::_OnGameplayEffectRemoved);
}

void UTATHUDIndicatorSubsystem::_UnregisterFromPlayerEvents(UOSEAbilitySystemComponent* asc)
{
   check(asc != nullptr);
   check(_abilitySystemComponent.Get() == asc);
   asc->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
   asc->OnGameplayEffectAppliedDelegateToSelf.RemoveAll(this);
   asc->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
   _abilitySystemComponent.Reset();
}

void UTATHUDIndicatorSubsystem::_OnGameplayEffectWithDurationAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle)
{
   _AddIndicatorFromGameplayEffect(asc, spec, handle);
}

void UTATHUDIndicatorSubsystem::_OnAnyGameplayEffectAdded(UAbilitySystemComponent* asc, const FGameplayEffectSpec& spec, FActiveGameplayEffectHandle handle)
{
   _AddIndicatorFromGameplayEffect(asc, spec, handle);
}

void UTATHUDIndicatorSubsystem::_OnGameplayEffectRemoved(const FActiveGameplayEffect& activeEffect)
{
   const int32 idx = _FindExistingIndicatorIndex(activeEffect.Handle);
   if (idx != INDEX_NONE)
   {
      static constexpr bool removeFromArray = true;
      _RemoveIndicatorByIndex(idx, removeFromArray);
   }
}

void UTATHUDIndicatorSubsystem::_OnGameplayEffectTimeChange(FActiveGameplayEffectHandle handle, float newStartTime, float newDuration)
{
   const int32 idx = _FindExistingIndicatorIndex(handle);
   if (idx == INDEX_NONE)
   {
      return;
   }
   FTATHUDIndicatorInfo& indicator = _indicators[idx];
   FTATHUDIndicatorState newState{};
   if (_GameplayEffectToIndicatorState(handle, newState) && newState != indicator.State)
   {
      indicator.State = newState;
      _UpdateIndicatorByIndex(idx);
   }
}

void UTATHUDIndicatorSubsystem::_OnGameplayEffectStackChange(FActiveGameplayEffectHandle handle, int32 newStackCount, int32 previousStackCount)
{
   const int32 idx = _FindExistingIndicatorIndex(handle);
   if (idx == INDEX_NONE)
   {
      return;
   }
   FTATHUDIndicatorInfo& indicator = _indicators[idx];
   FTATHUDIndicatorState newState{};
   if (_GameplayEffectToIndicatorState(handle, newState) && newState != indicator.State)
   {
      indicator.State = newState;
      _UpdateIndicatorByIndex(idx);
   }
}
