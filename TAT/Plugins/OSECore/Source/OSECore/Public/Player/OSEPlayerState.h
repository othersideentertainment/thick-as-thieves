// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"

// ose
#include "Character/OSETeamInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Player/OSEPlayerStats.h"

// ue4
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Attributes/AttributeBaseInterface.h"
#include "Abilities/Attributes/AttributeBaseSystemInterface.h"

#include "OSEPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class OSECORE_API AOSEPlayerState : public APlayerState
   , public IAbilitySystemInterface
   , public IAttributeBaseInterface
   , public IAttributeBaseSystemInterface
   , public IOSETeamInterface
   , public IToolSetSystemInterface
{
   GENERATED_BODY()

public:   
   AOSEPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

   // statics
   UFUNCTION(BlueprintPure, Category = "PlayerState|OSE", meta = (WorldContext = "contextObj"))
   static AOSEPlayerState* GetOSEPlayerState(const UObject* contextObj, int index);
   UFUNCTION(BlueprintPure, Category = "PlayerState|OSE", meta = (WorldContext = "contextObj"))
   static AOSEPlayerState* GetLocalOSEPlayerState(const UObject* contextObj);

   // from APlayerState
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   /// IAbilitySystemInterface
   virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
   virtual class UOSEAbilitySystemComponent* GetAbilitySystemComponentFromActor() const;

   /// Returns the base attribute set
   virtual class UAttributeBaseSet* GetBaseAttributeSet() const;

   // IAttributeBaseSystemInterface
   virtual TScriptInterface< IAttributeBaseInterface > GetBaseAttributeInterface() const override;
   
   // IAttributeBaseInterface (Health)
   virtual float GetHealth() const override;
   virtual float GetHealthMax() const override;
   virtual float GetHealthPercent() const override;
   virtual float GetHealthRegenRate() const override;
   
   // IAttributeBaseInterface (Energy)
   virtual float GetEnergy() const override;
   virtual float GetEnergyMax() const override;
   virtual float GetEnergyPercent() const override;
   virtual float GetEnergyRegenRate() const override;
   
   // IAttributeBaseInterface (Damage)
   virtual float GetHealthDamage() const override;
   virtual float GetAttackDamage() const override;
   virtual float GetAttackDamageMultiplier() const override;
   virtual float GetDamageReductionMultiplier() const override;

   // IAttributeBaseInterface (Movement)
   virtual float GetMovementMaxSpeedMultiplier() const override;
   virtual float GetMovementFrictionMultiplier() const override;
   virtual float GetMovementBrakingDecelerationMultiplier() const override;
   virtual float GetGravityScale() const override;

   // IOSETeamInterface
   virtual uint8 GetTeam() const override final { return _team; }
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PlayerState|OSE|Team")
   void AuthoritySetTeam(uint8 team);

   /// IToolSetSystemInterface
   virtual TScriptInterface<IToolSetInterface> GetToolSetInterface() const override;

   // Is this the player state for the local player?  Only returns true once our player controller exists, will return false till then.
   UFUNCTION(BlueprintPure, Category = "PlayerState|OSE")
   bool IsLocalPlayerState() const;

   // get/set player stats
   UFUNCTION(BlueprintPure, Category = "PlayerState|OSE|Player Stats")
   const FOSEPlayerStats& GetPlayerStats() const { return _playerStats; }
   void AuthorityUpdatePlayerStatInt(FGameplayTag tag, int updateValue);
   bool AuthorityUpdatePlayerStatUniqueByName(FGameplayTag tag, FName key);
   bool AuthorityUpdatePlayerStatUniqueByObject(FGameplayTag tag, FObjectKey key);
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerStatsChanged);
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|OSE|Player Stats")
   FOnPlayerStatsChanged OnPlayerStatsChanged;

   // damage log
   UPROPERTY(EditDefaultsOnly, Category = "PlayerState|OSE|Damage Log")
   int DamageLogMaxEntries = 15;
   UFUNCTION(BlueprintPure, Category = "PlayerState|OSE|Damage Log")
   const FOSEDamageLog& GetDamageLog() const { return _damageLog; }
   void AuthorityAddIncomingDamageLogEntry(AActor* fromActor, float damageAmount);
   void AuthorityAddOutgoingDamageLogEntry(AActor* toActor, float damageAmount);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDamageLogEntryChanged);
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|OSE|Damage Log")
   FOnDamageLogEntryChanged OnDamageLogEntryChanged;
   
protected:
   /// Ability system component
   UPROPERTY(EditDefaultsOnly)
   class UAbilitySystemComponent* AbilitySystemComponent;

   /// Base ability attribute set
   UPROPERTY()
   class UAttributeBaseSet* BaseAttributeSet;

private:
   UFUNCTION()
   void _OnRep_PlayerStats();
   void _BroadcastPlayerStatsChanged();

   UFUNCTION()
   void _OnRep_DamageLog();
   void _BroadcastDamageLogChanged();

protected:

   virtual void _HandleTeamChanged(uint8 team) {}
   UFUNCTION()
   virtual void _OnRep_Team() { _HandleTeamChanged(_team); };

private:
   // our player stats
   UPROPERTY(ReplicatedUsing = _OnRep_PlayerStats)
   FOSEPlayerStats _playerStats;

   // basic book-keeping for stats for number _distinct_ things foozled
   // Using array for now, but could switch to set if they get large in practice
   TMap<FGameplayTag, TArray<FName>> _playerStatUniqueNames;
   TMap<FGameplayTag, TArray<FObjectKey>> _playerStatUniqueObjects;

   // our damage log of incoming and outgoing damage
   UPROPERTY(ReplicatedUsing = _OnRep_DamageLog)
   FOSEDamageLog _damageLog;

   // which team are we on?
   UPROPERTY(Replicated, ReplicatedUsing= _OnRep_Team)
   uint8 _team = IOSETeamInterface::kInvalidTeam;
};
