// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/OSEPlayerState.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Online/OSEGameState.h"
#include "Player/OSEPlayerController.h"
#include "Utl/OSEUtlFunctionLibrary.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPlayerState)

DEFINE_LOG_CATEGORY_STATIC(LogOSEPlayerState, Log, All);

AOSEPlayerState::AOSEPlayerState(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
   // Spawn the ability system component here and set as replicated so it gets sent down in initial packet

   // Create ability system component, and set it to be explicitly replicated
   AbilitySystemComponent = CreateDefaultSubobject<UOSEAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
   AbilitySystemComponent->SetIsReplicated(true);

   // Create the attribute set, this replicates by default. Adding it as a subobject of the owning actor
   // of an AbilitySystemComponent automatically registers the AttributeSet with the AbilitySystemComponent
   BaseAttributeSet = CreateDefaultSubobject<UAttributeBaseSet>(TEXT("BaseAttributeSet"));

   // Need to bump net frequency because ASC doesn't force replicate properly and depends on a high frequency
   SetNetUpdateFrequency(100.0f);
   NetPriority = 3.0f;
}

/* static */
AOSEPlayerState* AOSEPlayerState::GetOSEPlayerState(const UObject* contextObj, int index)
{
   // the array of player states owned by AOSEGameState is sorted by a unique per-player value so we can have
   // a stable index lookup on server + all clients
   if (AOSEGameState* gs = AOSEGameState::GetOSEGameState(contextObj))
   {
      const TArray<AOSEPlayerState*>& osePlayerStates = gs->GetOSEPlayerStates();
      if (index < osePlayerStates.Num())
         return osePlayerStates[index];
   }
   return nullptr;
}

/* static */
AOSEPlayerState* AOSEPlayerState::GetLocalOSEPlayerState(const UObject* contextObj)
{
   if (AOSEPlayerController* localPC = AOSEPlayerController::GetLocalOSEPlayerController(contextObj))
   {
      return localPC->GetOSEPlayerState();
   }
   return nullptr;
}

void AOSEPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   #if UE_WITH_IRIS
   FDoRepLifetimeParams DefaultParams;
   DefaultParams.bIsPushBased = true;
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSEPlayerState, _team, DefaultParams);
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSEPlayerState, _playerStats, DefaultParams);
   DOREPLIFETIME_WITH_PARAMS_FAST(AOSEPlayerState, _damageLog, DefaultParams);
   #else
   DOREPLIFETIME(AOSEPlayerState, _playerStats);
   DOREPLIFETIME(AOSEPlayerState, _damageLog);
   DOREPLIFETIME(AOSEPlayerState, _team);
   #endif  
}

UAbilitySystemComponent* AOSEPlayerState::GetAbilitySystemComponent() const
{
   return AbilitySystemComponent;
}

UOSEAbilitySystemComponent* AOSEPlayerState::GetAbilitySystemComponentFromActor() const
{
   return UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(this);
}

UAttributeBaseSet* AOSEPlayerState::GetBaseAttributeSet() const
{
   return BaseAttributeSet;
}

TScriptInterface< IAttributeBaseInterface > AOSEPlayerState::GetBaseAttributeInterface() const
{
   return GetBaseAttributeSet();
}

// IAttributeBaseInterface (Health)
float AOSEPlayerState::GetHealth() const { return BaseAttributeSet ? BaseAttributeSet->GetHealth() : 0.0f; }
float AOSEPlayerState::GetHealthMax() const { return BaseAttributeSet ? BaseAttributeSet->GetHealthMax() : 1.0f; }
float AOSEPlayerState::GetHealthPercent() const { return BaseAttributeSet ? BaseAttributeSet->GetHealthPercent() : 0.0f; }
float AOSEPlayerState::GetHealthRegenRate() const { return BaseAttributeSet ? BaseAttributeSet->GetHealthRegenRate() : 0.0f; }

// IAttributeBaseInterface (Energy)
float AOSEPlayerState::GetEnergy() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergy() : 0.0f; }
float AOSEPlayerState::GetEnergyMax() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergyMax() : 1.0f; }
float AOSEPlayerState::GetEnergyPercent() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergyPercent() : 0.0f; }
float AOSEPlayerState::GetEnergyRegenRate() const { return BaseAttributeSet ? BaseAttributeSet->GetEnergyRegenRate() : 0.0f; }

// IAttributeBaseInterface (Damage)
float AOSEPlayerState::GetHealthDamage() const { return BaseAttributeSet ? BaseAttributeSet->GetHealthDamage() : 0.0f; }
float AOSEPlayerState::GetAttackDamage() const { return BaseAttributeSet ? BaseAttributeSet->GetAttackDamage() : 0.0f; }
float AOSEPlayerState::GetAttackDamageMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetAttackDamageMultiplier() : 0.0f; }
float AOSEPlayerState::GetDamageReductionMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetDamageReductionMultiplier() : 0.0f; }

// IAttributeBaseInterface (Damage)
float AOSEPlayerState::GetMovementMaxSpeedMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetMovementMaxSpeedMultiplier() : 1.0f; }
float AOSEPlayerState::GetMovementFrictionMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetMovementFrictionMultiplier() : 1.0f; }
float AOSEPlayerState::GetMovementBrakingDecelerationMultiplier() const { return BaseAttributeSet ? BaseAttributeSet->GetMovementBrakingDecelerationMultiplier() : 1.0f; }
float AOSEPlayerState::GetGravityScale() const { return BaseAttributeSet ? BaseAttributeSet->GetGravityScale() : 1.0f; }

void AOSEPlayerState::AuthoritySetTeam(uint8 team)
{
   check(HasAuthority());
   if (_team != team)
   {
      _team = team;
#if UE_WITH_IRIS
      MARK_PROPERTY_DIRTY_FROM_NAME(AOSEPlayerState, _team, this);
#endif
      _HandleTeamChanged(team);
   }
}

TScriptInterface<IToolSetInterface> AOSEPlayerState::GetToolSetInterface() const
{
   // implemented for a convenient pass-through from player state to pawn
   if (IToolSetSystemInterface* toolSetSystemInterface = Cast<IToolSetSystemInterface>(GetPawn()))
   {
      return toolSetSystemInterface->GetToolSetInterface();
   }
   return nullptr;
}

bool AOSEPlayerState::IsLocalPlayerState() const
{
   if (AController* ownerController = GetOwner<AController>())
   {
      return ownerController->IsLocalController();
   }
   return false;
}

void AOSEPlayerState::AuthorityUpdatePlayerStatInt(FGameplayTag tag, int updateValue)
{
   check(HasAuthority());
   FOSEPlayerStat& stat = _playerStats.GetOrAddStat(tag);
   stat.IntValue += updateValue;
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSEPlayerState, _playerStats, this);
   #endif
   _BroadcastPlayerStatsChanged();
}

bool AOSEPlayerState::AuthorityUpdatePlayerStatUniqueByName(FGameplayTag tag, FName key)
{
   check(HasAuthority());

   TArray<FName>& currentKeys = _playerStatUniqueNames.FindOrAdd(tag);
   if(!currentKeys.Contains(key))
   {
      currentKeys.Add(key);

      _playerStats.GetOrAddStat(tag).IntValue = currentKeys.Num();
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _playerStats, this);

      _BroadcastPlayerStatsChanged();
      return true;
   }

   return false;
}

bool AOSEPlayerState::AuthorityUpdatePlayerStatUniqueByObject(FGameplayTag tag, FObjectKey key)
{
   check(HasAuthority());

   TArray<FObjectKey>& currentKeys = _playerStatUniqueObjects.FindOrAdd(tag);
   if(!currentKeys.Contains(key))
   {
      currentKeys.Add(key);

      _playerStats.GetOrAddStat(tag).IntValue = currentKeys.Num();
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _playerStats, this);

      _BroadcastPlayerStatsChanged();
      return true;
   }

   return false;
}

void AOSEPlayerState::AuthorityAddIncomingDamageLogEntry(AActor* fromActor, float damageAmount)
{
   check(HasAuthority());

   FOSEDamageLogEntry entry;
   entry.IsIncomingDamage = true;
   entry.DamageAmount = damageAmount;
   entry.FromCharacterName = UOSEUtlFunctionLibrary::GetPlayerFacingActorName(fromActor);
   entry.ToCharacterName = GetPlayerName();
   entry.TimeStamp = FDateTime::UtcNow();
   _damageLog.AddDamageEntry(entry, DamageLogMaxEntries);
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSEPlayerState, _damageLog, this);
   #endif
}

void AOSEPlayerState::AuthorityAddOutgoingDamageLogEntry(AActor* toActor, float damageAmount)
{
   check(HasAuthority());

   FOSEDamageLogEntry entry;
   entry.IsIncomingDamage = false;
   entry.DamageAmount = damageAmount;
   entry.ToCharacterName = UOSEUtlFunctionLibrary::GetPlayerFacingActorName(toActor);
   entry.FromCharacterName = GetPlayerName();
   entry.TimeStamp = FDateTime::UtcNow();
   _damageLog.AddDamageEntry(entry, DamageLogMaxEntries);
   #if UE_WITH_IRIS
   MARK_PROPERTY_DIRTY_FROM_NAME(AOSEPlayerState, _damageLog, this);
   #endif
}

void AOSEPlayerState::_OnRep_PlayerStats()
{
   _BroadcastPlayerStatsChanged();
}

void AOSEPlayerState::_BroadcastPlayerStatsChanged()
{
   UE_LOG(LogOSEPlayerState, Verbose, TEXT("Players %s stats changed!"), *GetPlayerName());
   OnPlayerStatsChanged.Broadcast();
}

void AOSEPlayerState::_OnRep_DamageLog()
{
   _BroadcastDamageLogChanged();
}

void AOSEPlayerState::_BroadcastDamageLogChanged()
{
   UE_LOG(LogOSEPlayerState, Verbose, TEXT("Players %s damage log changed!"), *GetPlayerName());
   OnDamageLogEntryChanged.Broadcast();
}
