// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/TATCombatComponent.h"

// tat
#include "Abilities/TATGameplayAbilityTargetData_Dodge.h"
#include "Character/TATCharacterMovement.h"
#include "Character/TATTeams.h"
#include "Combat/TATCombatFunctionLibrary.h"
#include "Combat/TATCombatSettings.h"
#include "Damage/TATDamageFunctionLibrary.h"
#include "Developer/TATProjectSettings.h"
#include "Developer/TATToolSettings.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Tools/TATToolFunctionLibrary.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Character/OSECharacterMovement.h"
#include "Combat/CombatFunctionLibrary.h"
#include "Combat/CombatSettings.h"
#include "Items/ToolSetComponent.h"

// ue4
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCombatComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATCombatComponent, Log, All)

UTATCombatComponent::UTATCombatComponent()
   : Super()
{
}

void UTATCombatComponent::BeginPlay()
{
   Super::BeginPlay();

   // For now, only do these on authority to simplify the logic
   if (GetOwner()->HasAuthority())
   {
      if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
      {
            // Listen for bumped into
            ownerCharacter->OnBumpedIntoEvent.AddUniqueDynamic(this, &UTATCombatComponent::_OnBumpedInto);

         if (StaggerOnLandOnCharacter)
         {
            // Listen for LandedOn
            ownerCharacter->LandedDelegate.AddUniqueDynamic(this, &UTATCombatComponent::_OnOwnerLanded);
         }
      }
      else
      {
         UE_LOG(LogTATCombatComponent, Warning, TEXT("TATCombatComponent on '%s' expected to be on an OSECharacterBase"),
            *GetOwner()->GetName());
      }
   }

   if (UToolSetComponent* toolSetComponent = UTATToolFunctionLibrary::GetToolSetComponentFromActor(GetOwner()))
   {
      toolSetComponent->OnEquippedToolChanged.AddDynamic(this, &UTATCombatComponent::_OnEquippedToolChanged);
   }
   else
   {
      UE_LOG(LogTATCombatComponent, Error, TEXT("TATCombatComponent on '%s' expected owner to have a UToolSetComponent!"), *GetOwner()->GetName());
   }
}

void UTATCombatComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UToolSetComponent* toolSetComponent = UTATToolFunctionLibrary::GetToolSetComponentFromActor(GetOwner()))
   {
      toolSetComponent->OnEquippedToolChanged.RemoveAll(this);
   }

   Super::EndPlay(endPlayReason);
}

void UTATCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.Condition = COND_OwnerOnly;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _lastChainAttackIndex, params);
}

void UTATCombatComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
}

int32 UTATCombatComponent::GetNextChainAttackIndex() const
{
   return _GetAllowedChainAttackIndex(false);
}

bool UTATCombatComponent::NotifyStartAttack(FGameplayTag usageTag, const UTATMeleeWeaponToolComponent* equippedWeapon, const UAnimMontage* animMontage, int32 chainIndex)
{
   check(IsValid(animMontage));
   check(equippedWeapon);
   const FTATMeleeWeaponAttack* attack = equippedWeapon->GetMeleeWeaponAttackForUsage(usageTag);
   if (!ensure(attack))
   {
      return false;
   }

   constexpr bool includeFudge = true;
   const int32 allowedChainAttack = _GetAllowedChainAttackIndex(includeFudge);

   _ResetChainAttackState();

   if (!attack->IsChainAttack)
   {
      return true;
   }

   // NOTE: Since _lastChainAttackIndex is replicated, it is possible for it to clobber
   //       a more recent client value if the latency exceeds the time between two attacks
   const bool indexIsValid = chainIndex <= allowedChainAttack;

   _lastChainAttackIndex = FMath::Min(chainIndex, allowedChainAttack);
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _lastChainAttackIndex, this);

   // TODO: should we use the montage length here, or just start the time at the end of the attack?
   _chainAttackState.ChainExpirationTime = GetWorld()->GetTimeSeconds() + animMontage->GetPlayLength() + attack->ChainAttackMaxSecondsBetweenHits;
   UE_LOG(LogCombatComponent, Verbose, TEXT("[%s] Chain index now %d")
      , GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT")
      , _lastChainAttackIndex);
   return indexIsValid;
}

void UTATCombatComponent::HandleAttackConnectWithTarget(const UTATMeleeWeaponToolComponent* meleeWeaponTool, FGameplayTag usageTag, int32 chainIndex, const FHitResult& hitResult)
{
   if (!meleeWeaponTool)
   {
      UE_LOG(LogCombatComponent, Error, TEXT("HandleAttackConnectWithTarget() called without valid meleeWeaponTool!"));
      return;
   }
   else if(!meleeWeaponTool->IsEquipped())
   {
      UE_LOG(LogCombatComponent, Error, TEXT("HandleAttackConnectWithTarget() called with unequipped meleeWeaponTool %s!"), *meleeWeaponTool->GetName());
      return;
   }

   // This can get called for multiple impacts from a single swing (even with the same actor), so early out after first call within a swing
   if (_chainAttackState.SwingLanded)
   {
      return;
   }

   // Cache hit location
   _lastConnectingHitLocation = hitResult.ImpactPoint;

   const FTATMeleeWeaponAttack* attack = meleeWeaponTool->GetMeleeWeaponAttackForUsage(usageTag);
   if (attack == nullptr || !attack->IsChainAttack)
   {
      // Handle chain attack interrupted by non-chain attack
      if (GetOwner()->HasAuthority() && _authorityChainAttackExpirationTimer.IsValid())
      {
         constexpr bool chainAttackCompleted = false;
         _AuthorityOnChainAttackEnded(chainAttackCompleted);
      }
      return;
   }

   _chainAttackState.SwingLanded = true;
   _chainAttackState.ChainIndexForLandedSwing = chainIndex;

   if (GetOwner()->HasAuthority())
   {
      // Clear chain attack expiration timer
      const bool chainAttackCompleted = chainIndex >= meleeWeaponTool->GetMaxChainAttackLength() - 1;
      if (chainAttackCompleted)
      {
         _AuthorityOnChainAttackEnded(chainAttackCompleted);
      }
      // Only begin the chain attack expiration timer if at least 2 hits have landed
      else if (chainIndex >= 1)
      {
         // Set chian attack expiration timer
         constexpr bool includeFudge = true;
         const float secondsUntilExpiration = _GetSecundsUntilChainAttackExpiration(includeFudge);
         
         // Note: SetTimer() will clear any existing timer
         FTimerDelegate chainAttackExpirationDelegate = FTimerDelegate::CreateUObject(this, &UTATCombatComponent::_AuthorityOnChainAttackEnded, chainAttackCompleted);
         constexpr bool looping = false;
         GetWorld()->GetTimerManager().SetTimer(_authorityChainAttackExpirationTimer, chainAttackExpirationDelegate, secondsUntilExpiration, looping);
         UE_LOG(LogCombatComponent, Verbose, TEXT("Setting chain attack expiration timer (%f seconds...)"), secondsUntilExpiration);
      }
   }
}

void UTATCombatComponent::SetLastChargeAttackDurationNormalized(float durationNormalized)
{
   _lastChargeAttackDurationNormalized = FMath::Clamp(durationNormalized, 0.f, 1.f);
   UE_CLOG(durationNormalized != _lastChargeAttackDurationNormalized, LogCombatComponent, Error, 
      TEXT("SetLastChargeAttackDurationNormalized() called with value %f outside range 0 <-> 1! Clamping to %f")
      , durationNormalized
      , _lastChargeAttackDurationNormalized);
}

void UTATCombatComponent::_OnCombatHitValidTargetCharacter(AOSECharacterBase& attacker, AOSECharacterBase& defender, const FHitResult& hit)
{
   Super::_OnCombatHitValidTargetCharacter(attacker, defender, hit);
   _SendEventsToAttackerAndDefender(*attacker.GetAbilitySystemComponent(), &defender, hit);
}

bool UTATCombatComponent::_OnCombatHitNonCharacterActor(AOSECharacterBase& attacker, AActor& defender, const FHitResult& hit)
{
   // if we hit an actor which has an ability system, we want to send damage events onto them
   // just so that we can have character -> non-character interactions
   if(UTATDamageFunctionLibrary::IsActorPossiblyDamageable(&defender))
   {
      // If not valid for location reasons, don't do the hit, but don't count it as handled yet
      if (!_IsNonCharacterTargetValid(attacker, defender, hit))
      {
         return false;
      }

      _SendEventsToAttackerAndDefender(*attacker.GetAbilitySystemComponent(), &defender, hit);
   }
   else
   {
      // send an event to our owner actor, who is the attacker
      const UCombatSettings& settings = UCombatSettings::Get();
      FGameplayAbilityTargetDataHandle targetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(hit);

      FGameplayEventData payload;
      payload.Instigator = &attacker;
      payload.Target = &defender;
      payload.TargetData = targetDataHandle;
      payload.EventTag = settings.GeometryHitEventTag;

      UE_LOG(LogCombatComponent, Verbose, TEXT("%s sending event %s to attacker %s"), attacker.HasAuthority() ? TEXT("Server") : TEXT("Client"), *settings.GeometryHitEventTag.ToString(), *attacker.GetName());
      UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(&attacker, settings.GeometryHitEventTag, payload);
   }

   return true;
}

bool UTATCombatComponent::_CanAttackerBeHostileToDefenderTeam(AOSECharacterBase& attacker, AOSECharacterBase& defender) const
{
   if (attacker.Implements<UOSETeamInterface>() && defender.Implements<UOSETeamInterface>())
   {
      // only allow attacks if we are hostile to their actual team
      const EOSETeamAttitude minimumAttitude = UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(
         &attacker,
         &defender,
         ETATTeamDisguiseHandling::UseOriginalTeam
      );
      return minimumAttitude != EOSETeamAttitude::Friendly;
   }
   else
   {
      // one of the actors doesn't implement the team interface, so assume we cannot target them.
      return false;
   }
}


bool UTATCombatComponent::_IsNonCharacterTargetValid(const AOSECharacterBase& attacker, const AActor& defender, const FHitResult& hit) const
{
   // NOTE: If this ever returns false for reasons that are dependent on the hit result,
   //       change this method to return an enum of (Valid,Invalid,InvalidAtLocation), so it is only not-handled
   //       in the last case.

   // defender is in front of the attacker -- even if the hitboxes clip something behind you, you gotta face them!
   if (!UCombatFunctionLibrary::IsLocationInFrontOfAttacker(&attacker, hit.ImpactPoint))
      return false;

   // attacker can see the defender
   // NOTE: since this is potentially going to happen a lot more than the character one, just to a line trace rather than a sphere trace
   constexpr float radius = 0;
   if (!UCombatFunctionLibrary::DoesAttackerHaveHitPathToDefenderLocation(&attacker, &defender, hit.ImpactPoint, radius))
      return false;

   // everything passed; valid target!
   return true;
}

void UTATCombatComponent::_SendEventsToAttackerAndDefender(const UAbilitySystemComponent& attacker, AActor* defenderActor, const FHitResult& hit)
{
   const UCombatSettings& settings = UCombatSettings::Get();
   const UTATCombatSettings& tatSettings = UTATCombatSettings::Get();

   AActor* attackerActor = attacker.GetAvatarActor();
   const bool hasAuthority = attackerActor->HasAuthority();
   const bool hitShielded = UTATCombatFunctionLibrary::IsDefenderShielded(defenderActor);
   const bool hitParried = UTATCombatFunctionLibrary::IsDefenderParrying(attackerActor, defenderActor);
   bool hitBlocked = UTATCombatFunctionLibrary::IsDefenderBlocking(attackerActor, defenderActor);
   
   // Negate blocking for charge attacks/shoves as specified in settings
   if (hitBlocked && UTATCombatFunctionLibrary::IsPerformingChargeAttack(attackerActor))
   {
      const float chargePercentage = _lastChargeAttackDurationNormalized * 100.f;
      hitBlocked &= tatSettings.CanBlockChargeAttacks && chargePercentage < tatSettings.RequiredChargeAttackPercentageToNegateBlock;
   }
   if (hitBlocked && !tatSettings.CanBlockShoves)
   {
      hitBlocked &= !UTATCombatFunctionLibrary::IsPerformingShove(attackerActor);
   }
   
   FGameplayTagContainer attackerTags;
   attacker.GetOwnedGameplayTags(attackerTags);
   if (_combatAnimInfo.AttackTag.IsValid())
   {
      // adds a tag so that responses know what kind of attack was performed
      attackerTags.AddTag(_combatAnimInfo.AttackTag);
   }

   FGameplayTagContainer defenderTags;
   if (IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(defenderActor))
   {
      tagInterface->GetOwnedGameplayTags((defenderTags));
   }

   const FGameplayAbilityTargetDataHandle targetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(hit);

   // send an event to our owner actor, who is the attacker
   FGameplayEventData payload;
   payload.Instigator = attackerActor;
   payload.InstigatorTags = attackerTags;
   payload.Target = defenderActor;
   payload.TargetData = targetDataHandle;
   payload.TargetTags = defenderTags;
   payload.OptionalObject = this; // send in a reference to ourselves so that handling abilities can work backwards to where the event came from

   FGameplayTag attackerEventTag;
   FGameplayTag defenderEventTag;
   // Priority is Shielded -> Parried -> Blocked
   if (hitShielded)
   {
      attackerEventTag = tatSettings.AttackerShieldEventTag;
      defenderEventTag = tatSettings.DefenderShieldEventTag;
   }
   else if (hitParried)
   {
      attackerEventTag = tatSettings.AttackerParryEventTag;
      defenderEventTag = tatSettings.DefenderParryEventTag;
   }
   else if (hitBlocked)
   {
      attackerEventTag = tatSettings.AttackerBlockEventTag;
      defenderEventTag = tatSettings.DefenderBlockEventTag;
      payload.TargetTags.AddTag(TAG_DamageContext_Blocked);
   }
   else
   {
      attackerEventTag = settings.AttackerHitEventTag;
      defenderEventTag = settings.DefenderEventTag;
   }

   // attacker event
   {
      payload.EventTag = attackerEventTag;
      UE_LOG(LogCombatComponent, Verbose, TEXT("%s sending event %s to attacker %s"), hasAuthority ? TEXT("Server") : TEXT("Client"), *attackerEventTag.ToString(), *attacker.GetName());
      UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(attackerActor, attackerEventTag, payload);
   }

   // defender event
   // TODO: This is only used by the AI knowledge component, revisit?
   {
      payload.EventTag = defenderEventTag;
      UE_LOG(LogCombatComponent, Verbose, TEXT("%s sending event %s to defender %s"), hasAuthority ? TEXT("Server") : TEXT("Client"), *defenderEventTag.ToString(), *defenderActor->GetName());
      UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(defenderActor, defenderEventTag, payload);
   }
}

FName UTATCombatComponent::_GetBucketForHit(const FHitResult& hit) const
{
   // Hits to non-damagable portions should be bucketed separately, so that a
   // a melee hit that hits a non-damageable part of an actor first (and then the damageable part)
   // does not cause the second hit to be swallowed.
   static const FName kInvalidDamageTag(TEXT("Invalid"));
   return UTATDamageFunctionLibrary::IsComponentInvalidDamageTarget(hit.GetComponent()) ? kInvalidDamageTag : FName();
}

void UTATCombatComponent::_OnBumpedInto(AOSECharacterBase* defender, const FHitResult& impact)
{
   // If no shove, then don't try
   if (!_shoveSettings.ShoveAbilityGameplayEvent.IsValid())
   {
      return;
   }

   // Ensure shove cooldown elapsed
   const float timeElapsedSinceLastShove = GetWorld()->GetTimeSeconds() - _authorityLastShoveTimeSeconds;
   if (timeElapsedSinceLastShove < _shoveSettings.ShoveDispatchCooldownSeconds)
   {
      UE_LOG(LogCombatComponent, VeryVerbose, TEXT("_OnBumpedInto() | shove cooldown not elapsed, %f seconds remaining"), _shoveSettings.ShoveDispatchCooldownSeconds - timeElapsedSinceLastShove);
      return;
   }
   
   const AOSECharacterBase* ownerCharacter = CastChecked<AOSECharacterBase>(GetOwner());
   UAbilitySystemComponent* asc = ownerCharacter->GetAbilitySystemComponent();
   check(asc);

   const FGameplayAbilityTargetDataHandle targetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(impact);

   FGameplayEventData payload;
   payload.EventTag = _shoveSettings.ShoveAbilityGameplayEvent;
   payload.Target = defender;
   payload.Instigator = GetOwner();
   payload.OptionalObject = _SelectShoveMontage();
   payload.TargetData = targetDataHandle;

   if (asc->HandleGameplayEvent(_shoveSettings.ShoveAbilityGameplayEvent, &payload) > 0)
   {
      UE_LOG(LogCombatComponent, Verbose, TEXT("%s shoved %s"), *GetOwner()->GetName(), *defender->GetName());
      _authorityLastShoveTimeSeconds = GetWorld()->GetTimeSeconds();
   }
   else
   {
      UE_LOG(LogCombatComponent, Verbose, TEXT("%s failed to shove %s - gameplay event %s did not activate any abilities!")
         , *GetOwner()->GetName()
         , *defender->GetName()
         , *_shoveSettings.ShoveAbilityGameplayEvent.ToString());
   }
}

void UTATCombatComponent::_OnOwnerLanded(const FHitResult& impact)
{
   if (AOSECharacterBase* landedCharacter = Cast<AOSECharacterBase>(impact.GetActor()))
   {
      _SendStaggerEvent(LandOnStaggerEventTag, landedCharacter, impact);
   }
}

void UTATCombatComponent::_SendStaggerEvent(FGameplayTag eventTag, AActor* defender, const FHitResult& impact)
{
   const FGameplayAbilityTargetDataHandle targetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(impact);

   FGameplayEventData payload;
   payload.EventTag = eventTag;
   payload.Target = defender;
   payload.TargetData = targetDataHandle;

   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), eventTag, payload);
}

void UTATCombatComponent::_AuthorityOnChainAttackEnded(bool completed)
{
   check(GetOwner()->HasAuthority());
   GetWorld()->GetTimerManager().ClearTimer(_authorityChainAttackExpirationTimer);
   UE_LOG(LogCombatComponent, Verbose, TEXT("Chain attack %s at index %d")
      , completed ? TEXT("completed") : TEXT("expired")
      , _lastChainAttackIndex);

   _AuthoritySpawnChainAttackThiefVisionGlyph();
}

void UTATCombatComponent::_AuthoritySpawnChainAttackThiefVisionGlyph()
{
   check(GetOwner()->HasAuthority());
   if (_chainAttackThiefVisionIndicator.IsValid())
   {
      if (UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
      {
         UE_LOG(LogCombatComponent, Verbose, TEXT("[%s] Spawning chain attack thief vision indicator..."), *GetOwner()->GetName());

         // Spawn indicator where last attack landed
         const FVector spawnLocation = _lastConnectingHitLocation + _chainAttackThiefVisionIndicatorSpawnOffset;
         const FTransform spawnTransform = FTransform(FRotator::ZeroRotator, spawnLocation, FVector::OneVector);
         constexpr float deduplicateDistance = 0.f;
         AActor* instigator = GetOwner();
         thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(_chainAttackThiefVisionIndicator, spawnTransform, deduplicateDistance, instigator);
      }
   }
   else
   {
      UE_LOG(LogCombatComponent, Warning, TEXT("[%s] | _chainAttackThiefVisionIndicator unassigned! Could not spawn thief vision indicator"), *GetName());
   }
}

float UTATCombatComponent::_GetSecundsUntilChainAttackExpiration(bool includeFudge) const
{
   const float now = GetWorld()->GetTimeSeconds();
   float expiration = _chainAttackState.ChainExpirationTime;
   if (includeFudge)
   {
      expiration += UTATToolSettings::Get().ChainAttackServerFudgeWindowSeconds;
   }

   return expiration - now;
}

int32 UTATCombatComponent::_GetAllowedChainAttackIndex(bool includeFudge) const
{
   if (!_chainAttackState.SwingLanded || _chainAttackState.ChainIndexForLandedSwing != _lastChainAttackIndex)
   {
      return 0;
   }

   if (_chainAttackState.ChainExpirationTime == 0)
   {
      return 0;
   }
   
   const float secondsUntilExpiration = _GetSecundsUntilChainAttackExpiration(includeFudge);
   if (secondsUntilExpiration <= 0)
   {
      UE_LOG(LogCombatComponent, Verbose, TEXT("[%s] Missed chain window by %f")
         , GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT")
         , FMath::Abs(secondsUntilExpiration));
      return 0;
   }

   const UTATMeleeWeaponToolComponent* equippedWeapon = UTATToolFunctionLibrary::GetEquippedMeleeWeaponToolFromActor(GetOwner());
   if (!equippedWeapon)
   {
      return 0;
   }
   
   const int32 nextIndex = _lastChainAttackIndex + 1;
   return nextIndex < equippedWeapon->GetMaxChainAttackLength() ? nextIndex : 0;
}

void UTATCombatComponent::_ResetChainAttackState()
{
   _lastChainAttackIndex = 0;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _lastChainAttackIndex, this);

   _chainAttackState = FTATChainAttackState();
}

void UTATCombatComponent::_OnEquippedToolChanged()
{
   _ResetChainAttackState();
   GetWorld()->GetTimerManager().ClearTimer(_authorityChainAttackExpirationTimer);
}

const UAnimMontage* UTATCombatComponent::_SelectShoveMontage() const
{
   const TArray<UAnimMontage*>& shoveMontages = _shoveSettings.ShoveMontages;
   if (shoveMontages.Num() > 0)
   {
      const uint32 animMontageIndex = FMath::RandHelper(shoveMontages.Num());
      const UAnimMontage* shoveMontage = shoveMontages[animMontageIndex];
      UE_CLOG(shoveMontage == nullptr, LogCombatComponent, Error, TEXT("[%s] | _SelectShoveMontage() selected invalid AnimMontage at index %d!"), *GetName(), animMontageIndex);
      return shoveMontage;
   }

   UE_LOG(LogCombatComponent, Error, TEXT("[%s] has no entries in ShoveMontages!"), *GetName());
   return nullptr;
}
