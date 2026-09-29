// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/CombatComponent.h"

// ose
#include "OSEProjectSettings.h"
#include "Animation/AnimMetadata_Combat.h"
#include "Character/OSECharacterBase.h"
#include "Combat/CombatFunctionLibrary.h"
#include "Combat/CombatSettings.h"
#include "Combat/OSEGameplayAbility_MeleeAttack.h"
#include "Player/OSEPlayerController.h"

// ue4
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CombatComponent)

DEFINE_LOG_CATEGORY(LogCombatComponent);

namespace CombatCVars
{
   static int DebugDrawHitboxes = 0;
   FAutoConsoleVariableRef CVarDebugCombatHitboxes(
      TEXT("OSE.Combat.DebugDrawHitboxes"),
      DebugDrawHitboxes,
      TEXT("Draw combat debug information"),
      ECVF_Default);

   static int DebugDrawAutoAim = 0;
   FAutoConsoleVariableRef CVarDebugDrawAutoAim(
      TEXT("OSE.Combat.DebugDrawAutoAim"),
      DebugDrawAutoAim,
      TEXT("Draw auto-aim debug information"),
      ECVF_Default);

   static int UseActorLocationForView = 1;
   FAutoConsoleVariableRef CVarUseActorLocationForView(
      TEXT("OSE.Combat.UseActorLocationForView"),
      UseActorLocationForView,
      TEXT("When enabled, uses the actor location for the view. Otherwise, the view location is used."),
      ECVF_Default);

   static int AimOnSwing = 1;
   FAutoConsoleVariableRef CVarAimOnSwing(
      TEXT("OSE.Combat.AimOnSwing"),
      AimOnSwing,
      TEXT("When enabled, auto-aims towards the target when a weapon is swung."),
      ECVF_Default);
}

UCombatComponent::UCombatComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = true;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
   SetIsReplicatedByDefault(true);
}

void UCombatComponent::BeginPlay()
{
   Super::BeginPlay();
   _BindToAbilitySystemComponentInit();

   const UCombatSettings& settings = UCombatSettings::Get();
   _combatSwingTraceProfile = settings.CombatSwingTraceProfile;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   #if UE_WITH_IRIS
   FDoRepLifetimeParams Params;
   Params.bIsPushBased = true;
   DOREPLIFETIME_WITH_PARAMS_FAST(UCombatComponent, _aggroCharacters, Params);
   #else
   DOREPLIFETIME(UCombatComponent, _aggroCharacters);
   #endif
}

void UCombatComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
   _TickHitboxes();
}

void UCombatComponent::OnCombatAnimationMontageStart(const UAnimSequenceBase& animation, ECombatAttackDirection attackDirection)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatAnimationMontageStart: %s"), *animation.GetName());

   // we are taking a swing:
   _lastAttackInitiatedTime = GetWorld()->GetTimeSeconds();

   // reset previous
   _ResetCombatAnimState();
   _combatAnimationInProgress = true;
   OnCombatAnimationMontageStartEvent.Broadcast();

   // setup current
   _currentAttackDirection = attackDirection;
}

void UCombatComponent::OnCombatAnimationSequenceStart(const UAnimSequenceBase& animation, const UAnimMetadata_CombatHitboxes& animMetadata)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatAnimationSequenceStart: %s"), *animation.GetName());

   // setup new hitbox anim info
   _hitboxes = animMetadata.GetHitboxMetadata();
   _combatAnimInfo = animMetadata.GetCombatAnimationInfo();
}

void UCombatComponent::OnCombatAnimationSequenceEnd(const UAnimSequenceBase& animation)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatAnimationSequenceEnd: %s"), *animation.GetName());

   if (_combatAnimationInProgress)
   {
      // from this point forward we can interrupt our combat ability and launch the next one
      _combatAnimationCanBeInterrupted = true;
      OnCombatAnimationCanBeInterruptedEvent.Broadcast();
   }
}

void UCombatComponent::OnCombatAnimationMontageEnd(const UAnimSequenceBase& animation)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatAnimationMontageEnd: %s"), *animation.GetName());

   if (_combatAnimationInProgress)
   {
      _ResetCombatAnimState();
      OnCombatAnimationMontageEndEvent.Broadcast();
   }
}

void UCombatComponent::OnCombatAnimationAutoAim(const UAnimSequenceBase& animation, const UAnimMetadata_CombatHitboxes& animMetadata)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatAnimationMontageEnd: %s"), *animation.GetName());
   _TryAutoAimOnSwing(animMetadata.GetHitboxMetadata());
}

void UCombatComponent::OnCombatRangedProjectileSpawn(const UAnimSequenceBase& animation)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatRangedProjectileSpawn: %s"), *animation.GetName());
   const UCombatSettings& settings = UCombatSettings::Get();
   _SendGenericAttackerEvent(settings.RangedProjectileSpawn);
}

void UCombatComponent::OnCombatRangedReload(const UAnimSequenceBase& animation)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatRangedReload: %s"), *animation.GetName());
   const UCombatSettings& settings = UCombatSettings::Get();
   _SendGenericAttackerEvent(settings.RangedProjectileReload);
}

void UCombatComponent::EnableHitbox(const UAnimSequenceBase& animation, int hitboxIndex)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("EnableHitbox: %s"), *animation.GetName());
   _enabledHitboxes.AddUnique(hitboxIndex);
}

void UCombatComponent::DisableHitbox(const UAnimSequenceBase& animation, int hitboxIndex)
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("DisableHitbox: %s"), *animation.GetName());
   _enabledHitboxes.Remove(hitboxIndex);
}

void UCombatComponent::OnCombatMeleeAttackAbilityStart(const UOSEGameplayAbility_MeleeAttack& ability)
{
   // treat this like a montage start and reset everything
   // this exists because, through montage interruption, we can get into a state where OnCombatAnimationMontageEnd is just never called
   UE_LOG(LogCombatComponent, Verbose, TEXT("OnCombatMeleeAttackAbilityStart: %s"), *ability.GetName());
   _ResetCombatAnimState();
}

bool UCombatComponent::CanCombatAnimationBeInterrupted() const
{
   if (_combatAnimationInProgress)
   {
      UE_LOG(LogCombatComponent, Verbose, TEXT("Combat Animation In Progress: %s be interrupted"), _combatAnimationCanBeInterrupted ? TEXT("Can") : TEXT("Cannot"));
      return _combatAnimationCanBeInterrupted;
   }
   
   UE_LOG(LogCombatComponent, Verbose, TEXT("Combat Animation NOT In Progress: Cannot be interrupted"));
   return false;
}

bool UCombatComponent::HasUninterruptibleCombatAnimation() const
{
   return _combatAnimationInProgress && !CanCombatAnimationBeInterrupted();
}

float UCombatComponent::SecondsSinceLastAttackInitiated() const
{
   if (_lastAttackInitiatedTime != float(INDEX_NONE))
      return GetWorld()->GetTimeSeconds() - _lastAttackInitiatedTime;
   return float(INDEX_NONE);
}

bool UCombatComponent::HasCombatDisabledStatus() const
{
   return _lastCombatDisabledTagGainedTime != float(INDEX_NONE);
}

float UCombatComponent::SecondsSinceCombatDisabledStatusGained() const
{
   if (HasCombatDisabledStatus())
      return GetWorld()->GetTimeSeconds() - _lastCombatDisabledTagGainedTime;
   return float(INDEX_NONE);
}

void UCombatComponent::AddTargetingCharacter(AOSECharacterBase* character)
{
   check(GetOwner()->HasAuthority());
   check(character);
   UE_LOG(LogCombatComponent, Verbose, TEXT("%s is now agressively targeting %s"), *character->GetName(), *GetOwner()->GetName());
   if (_aggroCharacters.AddCharacter(character))
   {
      #if UE_WITH_IRIS
      MARK_PROPERTY_DIRTY_FROM_NAME(UCombatComponent, _aggroCharacters, this);
	  #endif
      _BroadcastAggroCharactersChanged();
   }
}

void UCombatComponent::RemoveTargetingCharacter(AOSECharacterBase* character)
{
   check(GetOwner()->HasAuthority());
   check(character);
   UE_LOG(LogCombatComponent, Verbose, TEXT("%s is no longer agressively targeting %s"), *character->GetName(), *GetOwner()->GetName());
   if (_aggroCharacters.RemoveCharacter(character))
   {
      #if UE_WITH_IRIS
      MARK_PROPERTY_DIRTY_FROM_NAME(UCombatComponent, _aggroCharacters, this);
	  #endif
      _BroadcastAggroCharactersChanged();
   }
}

void UCombatComponent::_ResetCombatAnimState()
{
   _combatAnimationInProgress = false;
   _combatAnimationCanBeInterrupted = false;
   _hitboxes.Reset();
   _enabledHitboxes.Reset();
   _hitActors.Reset();
   _combatAnimInfo = FCombatAnimationInfo();
   _currentAttackDirection = ECombatAttackDirection::None;
}

void UCombatComponent::_TickHitboxes()
{
   if (!_enabledHitboxes.Num())
      return;

   AOSECharacterBase* attacker = CastChecked<AOSECharacterBase>(GetOwner());

   for(int hitboxIndex : _enabledHitboxes)
   {
      if (!_hitboxes.Contains(hitboxIndex))
         continue;

      const FCombatHitboxMetadata& hitboxMetadata = _hitboxes[hitboxIndex];
      TArray<FHitResult> outHits;
      _TraceHitbox(hitboxMetadata, outHits, bool(CombatCVars::DebugDrawHitboxes));

      for (const FHitResult& hit : outHits)
      {
         if (!hit.HasValidHitObjectHandle())
            continue;

         const FName bucket = _GetBucketForHit(hit);
         const FActorAndBucket actorWithBucket(hit.GetActor(), bucket);

         if (!_hitActors.Contains(actorWithBucket))
         {
            bool handled = true;
            AActor* defenderActor = hit.GetActor();
            AOSECharacterBase* defender = Cast<AOSECharacterBase>(hit.GetActor());
            if (defender)
            {
               // We hit a character, is it a valid character to be hit in combat?  It's not
               // already dead, it's actually an enemy, etc etc
               if (_IsTargetValid(*attacker, *defender))
               {
                  _OnCombatHitValidTargetCharacter(*attacker, *defender, hit);
               }
               else
               {
                  _OnCombatHitInvalidTargetCharacter(*attacker, *defender, hit);
               }
            }
            else if (defenderActor)
            {
               handled = _OnCombatHitNonCharacterActor(*attacker, *defenderActor, hit);
            }

            // add them whether they're valid or not (but handled one way of the other) so we don't have to check again this swing
            if (handled)
            {
               _hitActors.Add(actorWithBucket);
            }
         }
      }
   }
}

void UCombatComponent::_TraceHitbox(const FCombatHitboxMetadata& hitboxMetadata, TArray<FHitResult>& outHitResults, bool debugDraw)
{
   UWorld* world = GetWorld();
   AOSECharacterBase* attacker = CastChecked<AOSECharacterBase>(GetOwner());
   FTransform hitboxWorldXfm = _GetHitboxWorldXfm(hitboxMetadata);

   FCollisionQueryParams params(SCENE_QUERY_STAT(TraceCombatHitbox), false);
   params.AddIgnoredActor(attacker);
   params.bIgnoreTouches = true;
   params.bReturnPhysicalMaterial = true; //< kismet did this by default, and might plausibly use for sounds
   const bool hasHit = world->SweepMultiByProfile(outHitResults, hitboxWorldXfm.GetLocation(), hitboxWorldXfm.GetLocation(), FQuat::Identity, _combatSwingTraceProfile.Name, FCollisionShape::MakeSphere(hitboxMetadata.Radius), params);

#if ENABLE_DRAW_DEBUG
   if (debugDraw)
   {
      ::DrawDebugSphere(world, hitboxWorldXfm.GetLocation(), hitboxMetadata.Radius, 20, (hasHit ? hitboxMetadata.DebugTraceHitColor : hitboxMetadata.DebugTraceColor).ToFColor(true));

      // draw hits
      for (FHitResult const& hit : outHitResults)
      {
         ::DrawDebugPoint(world, hit.ImpactPoint, 16, (hit.bBlockingHit ? hitboxMetadata.DebugTraceColor.ToFColor(true) : hitboxMetadata.DebugTraceHitColor.ToFColor(true)));
      }
   }
#endif

   // Now that there are non-character targets, some things, like doors, are thin enough so that the hit
   //    boxes are on the opposite side of it, resulting in impossible hit directions. In that case, try
   //    to trace back from the near edge of the sphere.
   const FVector attackerToBox = (hitboxWorldXfm.GetLocation() - attacker->GetActorLocation()).GetSafeNormal2D();
   const FVector nearEdge = hitboxWorldXfm.GetLocation() - (attackerToBox * hitboxMetadata.Radius);
   for (auto it = outHitResults.CreateIterator(); it; ++it)
   {
      FHitResult& hit = (*it);
      FVector normal2d = hit.Normal;
      normal2d.Z = 0;

      if ((normal2d | attackerToBox) > 0)
      {
         if (UPrimitiveComponent* primitive = hit.GetComponent())
         {
            FHitResult newHit;
            if (primitive->LineTraceComponent(newHit, nearEdge, hit.Location, params))
            {
               hit = newHit;
            }
         }
      }
   }
}

FTransform UCombatComponent::_GetHitboxWorldXfm(const FCombatHitboxMetadata& hitboxMetadata) const
{
   AOSECharacterBase* attacker = CastChecked<AOSECharacterBase>(GetOwner());

   // Cache actor and mesh transforms
   const FTransform actorXfm = attacker->GetTransform();
   const FTransform meshXfm = attacker->GetMesh()->GetComponentTransform();

   // This is what is used for aiming: the eye location and view rotation.
   // View rotation is essentially control rotation. It is replicated to
   // simulated proxies via OSECharacterBase
   FVector viewLocation;
   FRotator viewRotation;
   attacker->GetActorEyesViewPoint(viewLocation, viewRotation);

   if (CombatCVars::UseActorLocationForView)
   {
      // Effectively ignores the view location when this is set
      viewLocation = attacker->GetActorLocation();
   }

   const FVector& hitLocationAnim = hitboxMetadata.Location;
   if(_UseViewRotationWhenCalculatingHitBoxes)
   {
      const FTransform viewXfm = FTransform(viewRotation, viewLocation);
      // Delta relative to the actor for view
      const FTransform viewDeltaXfm = viewXfm.GetRelativeTransform(actorXfm);

      // Mesh to actor space and apply the view delta
      const FTransform meshToActorXfm = meshXfm.GetRelativeTransform(actorXfm);
      const FTransform meshToViewXfm = meshToActorXfm * viewDeltaXfm;

      // Transforms actor-relative mesh transform with view applied to world space
      const FTransform hitBoxToWorldXfm = meshToViewXfm * actorXfm;
      return FTransform(hitLocationAnim) * hitBoxToWorldXfm;
   }
   // If we're not using the view rotation, multiply against the mesh transform
   return  FTransform(hitLocationAnim) * meshXfm;
}

void UCombatComponent::_TryAutoAimOnSwing(const TMap<int, FCombatHitboxMetadata>& hitBoxes)
{
   if (!CombatCVars::AimOnSwing)
      return;
   
   // if any of the hitboxes in our swing would currently intersect with an enemy, then auto-aim towards the target
   AOSECharacterBase& attacker = *CastChecked<AOSECharacterBase>(GetOwner());
   if (AOSEPlayerController* playerController = Cast<AOSEPlayerController>(attacker.GetController()))
   {
      TArray<AOSECharacterBase*> hitDefenders;
      FHitResult firstDefenderHit;

      for(auto& entry : hitBoxes)
      {
         const FCombatHitboxMetadata& hitboxMetadata = entry.Value;
         TArray<FHitResult> outHits;
         _TraceHitbox(hitboxMetadata, outHits, bool(CombatCVars::DebugDrawAutoAim));

         // figure out all of our potential hit targets
         for (const FHitResult& hit : outHits)
         {
            AActor* defenderActor = hit.GetActor();
               
            if (AOSECharacterBase* hitDefender = Cast<AOSECharacterBase>(hit.GetActor()))
            {
               if (!hitDefenders.Contains(hitDefender))
               {
                  if (_IsTargetValid(attacker, *hitDefender, AutoAimForwardDotTolerance))
                  {
                     if (hitDefenders.Num() == 0)
                     {
                        firstDefenderHit = hit;
                     }
                     hitDefenders.Add(hitDefender);
                  }
               }
            }
         }
      }

      // more than one hit means there's multiple targets roughly in our view cone, so let's not do any aiming
      if (hitDefenders.Num() == 1)
      {
         // eyes
         FVector attackerLoc;
         FRotator attackerRot;
         attacker.GetActorEyesViewPoint(attackerLoc, attackerRot);

         AActor* hitActor = firstDefenderHit.GetActor();
         check(hitActor);

         FVector actorLocation = hitActor->GetActorLocation();
         actorLocation.Z = FMath::Max(firstDefenderHit.ImpactPoint.Z, actorLocation.Z);
         const FVector impactDirection = (actorLocation - attackerLoc).GetSafeNormal2D();
         const FRotator impactRotation = impactDirection.ToOrientationRotator();

         FRotator nextRotation = playerController->GetControlRotation().GetNormalized();
         nextRotation.Pitch = nextRotation.Pitch;
         nextRotation.Yaw = impactRotation.Yaw;

         const FRotator deltaRotation = playerController->CalculateExtraRotationInput(nextRotation);
         playerController->AddExtraRotationInput(deltaRotation);
      }
   }
}

void UCombatComponent::_SendGenericAttackerEvent(const FGameplayTag& eventTag) const
{
   // send an event to our owner actor, who is the attacker
   AOSECharacterBase& attacker = *CastChecked<AOSECharacterBase>(GetOwner());
   const bool hasAuthority = attacker.HasAuthority();

   FGameplayEventData payload;
   payload.Instigator = &attacker;
   payload.EventTag = eventTag;
   UE_LOG(LogCombatComponent, Verbose, TEXT("%s sending event %s to attacker %s"), hasAuthority ? TEXT("Server") : TEXT("Client"), *payload.EventTag.ToString(), *attacker.GetName());
   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(&attacker, payload.EventTag, payload);
}

void UCombatComponent::_BindToAbilitySystemComponentInit()
{
   if (AOSECharacterBase* character = Cast<AOSECharacterBase>(GetOwner()))
   {
      character->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UCombatComponent::_OnAbilitySystemComponentInit));
   }
}

void UCombatComponent::_OnAbilitySystemComponentInit()
{
   UAbilitySystemComponent* ownerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
   check(ownerASC);

   const UCombatSettings& combatSettings = UCombatSettings::Get();
   ownerASC->RegisterGameplayTagEvent(combatSettings.StatusCombatDisabled).AddUObject(this, &UCombatComponent::_OnStatusCombatDisabledChanged);
}

void UCombatComponent::_OnStatusCombatDisabledChanged(const FGameplayTag tag, int32 newTagCount)
{
   const bool newIsCombatDisabledStatus = newTagCount > 0;
   const bool oldIsCombatDisabledStatus = _lastCombatDisabledTagGainedTime != float(INDEX_NONE);
   if (newIsCombatDisabledStatus != oldIsCombatDisabledStatus)
   {
      if (newIsCombatDisabledStatus)
         _lastCombatDisabledTagGainedTime = GetWorld()->GetTimeSeconds();
      else
         _lastCombatDisabledTagGainedTime = float(INDEX_NONE);
      OnCombatDisabledStatusChanged.Broadcast(newIsCombatDisabledStatus);
   }
}

void UCombatComponent::OnRep_AggroCharacters()
{
   _BroadcastAggroCharactersChanged();   
}

void UCombatComponent::_BroadcastAggroCharactersChanged()
{
   UE_LOG(LogCombatComponent, Verbose, TEXT("%d actors are aggressive towards %s"), _aggroCharacters.CharacterInfos.Num(), *AActor::GetDebugName(GetOwner()));
   OnAggressiveCharacterListChanged.Broadcast(_aggroCharacters.CharacterInfos);
}

bool UCombatComponent::_IsTargetValid(AOSECharacterBase& attacker, AOSECharacterBase& defender, float forwardDotTolerance /*= float(INDEX_NONE) */) const
{
   // target is dead/unconscious
   if (defender.IsLyingDown())
      return false;

   // Check if the teams are compatible
   if (!_CanAttackerBeHostileToDefenderTeam(attacker, defender))
   {
      return false;
   }

   // defender is in front of the attacker -- even if the hitboxes clip something behind you, you gotta face them!
   if (!UCombatFunctionLibrary::IsDefenderInFrontOfAttacker(&attacker, &defender, forwardDotTolerance))
      return false;

   // attacker can see the defender
   const UCombatSettings& settings = UCombatSettings::Get();
   if (!UCombatFunctionLibrary::DoesAttackerHaveHitPathToDefender(&attacker, &defender, settings.CombatAttackerToDefenderPathTraceSphereRadius))
      return false;

   // everything passed; valid target!
   return true;
}

bool UCombatComponent::_CanAttackerBeHostileToDefenderTeam(AOSECharacterBase& attacker, AOSECharacterBase& defender) const
{
   // Check if attacker is hostile to target
   if (defender.Implements<UOSETeamInterface>())
   {
      EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(&attacker, &defender);

      // only allow attacks if we are hostile to them, friendly/neutral do not
      return attitude == EOSETeamAttitude::Hostile;
   }
   else
   {
      // target doesn't implement the team interface, which is required to check for hostility, so assume we cannot target them.
      return false;
   }
}

void UCombatComponent::_OnCombatHitValidTargetCharacter(AOSECharacterBase& attacker, AOSECharacterBase& defender, const FHitResult& hit)
{
   OnCombatHitValidTargetCharacter.Broadcast(&attacker, &defender, hit);
}

FName UCombatComponent::_GetBucketForHit(const FHitResult& hit) const
{
   return FName();
}

