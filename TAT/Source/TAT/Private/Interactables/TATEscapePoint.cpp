// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATEscapePoint.h"

// tat
#include "Character/TATTeams.h"
#include "Character/TATTeamsSubsystem.h"
#include "Compass/TATGenericIndicator.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/EscapeRoutes/TATEscapeRouteWorldSubsystem.h"
#include "Environment/TATOverlapTargetTriggerComponent.h"
#include "Variation/TATSpawnerComponent.h"
#include "Online/TATGameState.h"
#include "Online/TATPvPGameMode.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerState.h"
#include "WorldMap/TATMapActorComponent.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEscapePoint)

DEFINE_LOG_CATEGORY_STATIC(LogTATEscapePoint, Log, All);


#define LOCTEXT_NAMESPACE "TATEscapePoint" 

namespace EscapeHelpers
{
   static ATATPlayerState* GetPlayerStateForTarget(AActor* actor)
   {
      if (const auto* pawn = Cast<APawn>(actor))
      {
         return pawn->GetPlayerState<ATATPlayerState>();
      }

      return nullptr;
   }
}

ATATEscapePoint::ATATEscapePoint()
{
   PrimaryActorTick.bCanEverTick = false;
   bReplicates = true;
   NetDormancy = DORM_DormantAll;

   // Distant players should still be able to see if an escape route has been taken
   bAlwaysRelevant = true;

   _mapActorComponent = CreateDefaultSubobject<UTATMapActorComponent>(TEXT("MapActorComponent"));
   _usableInEscapeSpawner = CreateDefaultSubobject<UTATSpawnerComponent>("EscapeSpawner");
   _summonOverlapTrigger = CreateDefaultSubobject<UTATOverlapTargetTriggerComponent>("SummonOverlapTrigger");
}

#if WITH_EDITOR
EDataValidationResult ATATEscapePoint::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   bool mapSpriteTagsValid = true;
   auto validateMapSpriteEntryTag = [&](const FGameplayTag& spriteEntryTag, const FString& memberName)
   {
      bool result = true;

      // Invalid tag
      if (!spriteEntryTag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has unassigned %s!"), *GetName(), *memberName)));
         result = false;
      }
      // Tag doesn't correspond to map sprite entry
      else
      {
         const UTATProjectSettings& tatProjectSettings = UTATProjectSettings::Get();
         if (const UTATMapSpriteDataAsset* mapSpriteDataAsset = tatProjectSettings.MapSpriteData.LoadSynchronous())
         {
            if(!mapSpriteDataAsset->SpriteTable.FindByKey(spriteEntryTag))
            {
               context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has %s tag that doesn't correspond to an entry in UTATProjectSettings's MapSpriteData!"), *GetName(), *memberName)));
               result = false;
            }
         }
      }
      return result;
   };

   mapSpriteTagsValid &= validateMapSpriteEntryTag(_escapeRoutePendingMapSpriteEntry, GET_MEMBER_NAME_STRING_CHECKED(ATATEscapePoint, _escapeRoutePendingMapSpriteEntry));
   mapSpriteTagsValid &= validateMapSpriteEntryTag(_escapeRouteOpenMapSpriteEntry, GET_MEMBER_NAME_STRING_CHECKED(ATATEscapePoint, _escapeRouteOpenMapSpriteEntry));
   mapSpriteTagsValid &= validateMapSpriteEntryTag(_escapeRouteTakenMapSpriteEntry, GET_MEMBER_NAME_STRING_CHECKED(ATATEscapePoint, _escapeRouteTakenMapSpriteEntry));

   if (!mapSpriteTagsValid)
   {
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif // WITH_EDITOR

void ATATEscapePoint::BeginPlay()
{
   Super::BeginPlay();

   if (UWorld* world = GetWorld())
   {
      FActorSpawnParameters spawnParams;
      spawnParams.Owner = this;
      spawnParams.ObjectFlags = RF_Transient | RF_DuplicateTransient;
      spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
      spawnParams.bNoFail = true;
      _compassIndicator = world->SpawnActor<ATATGenericIndicator>(_compassIndicatorClass, GetTransform(), spawnParams);
      
      if (UTATEscapeRouteWorldSubsystem* escapeRouteSubsystem = world->GetSubsystem<UTATEscapeRouteWorldSubsystem>())
      {
         escapeRouteSubsystem->RegisterEscapePoint(this);

         if (IsEscapeUsableNow())
         {
            escapeRouteSubsystem->OnEscapeRouteOpened.Broadcast(this);
         }
      }
   }

   _UpdateEscapeForState();
}

void ATATEscapePoint::EndPlay(EEndPlayReason::Type endPlayReason)
{
   if (const UWorld* world = GetWorld())
   {
      if (UTATEscapeRouteWorldSubsystem* escapeRouteSubsystem = world->GetSubsystem<UTATEscapeRouteWorldSubsystem>())
      {
         escapeRouteSubsystem->UnregisterEscapePoint(this);
      }

      if (ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>())
      {
         tatGS->OnPhaseTimerChanged.RemoveAll(this);
      }
   }

   if (_compassIndicator)
   {
      _compassIndicator->Destroy();
   }

   Super::EndPlay(endPlayReason);
}

void ATATEscapePoint::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ATATEscapePoint, _openState);
   DOREPLIFETIME(ATATEscapePoint, _summonState);
}

void ATATEscapePoint::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   _usableInEscapeSpawner->AuthorityOnSpawn.AddUniqueDynamic(this, &ATATEscapePoint::_AuthorityOnEscapeSpawnerSpawned);
}

void ATATEscapePoint::_OnRep_OpenState(FTATEscapePointOpenState oldOpenState)
{
   if(oldOpenState.State != _openState.State)
   {
      _OnStateChanged(oldOpenState.State);
   }
}

void ATATEscapePoint::_OnRep_SummonState(const FTATEscapePointSummonState& previous)
{
   if (_summonState.State == previous.State)
   {
      return;
   }

   bool isRecent = UOSEInteractionHelpers::IsOld(this, _summonState.StartedAt);
   BP_OnSummonStateChanged(_summonState.State, previous.State, isRecent);
}

void ATATEscapePoint::_AuthorityOnEscapeSpawnerSpawned(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   check(HasAuthority());

   ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>();
   check(tatGS);

   tatGS->CallOrRegisterMatchStartDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ATATEscapePoint::_AuthorityOnMatchStart));
}

void ATATEscapePoint::_AuthorityOnMatchTimerUpdated(ETATMatchPhase phase)
{
   check(HasAuthority());

   // If we've already opened, or never will open, match time changes don't affect us
   if (_openState.State >= ETATEscapePointState::Summonable)
   {
      return;
   }

   // TODO: cancel stuff if match ends?
   if(phase != ETATMatchPhase::Endgame)
   {
      return;
   }

   ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>();
   check(tatGS);

   // We are given a world time that is when the match will end. We convert that to a remaining duration until our escape point should open,
   // since _escapeOpenStartTime is a relative duration from the match start to when we should open
   const float timeThatEscapeOpens = _escapeOpenStartTimeFromEndgame + tatGS->GetCurrentPhaseStart();
   const float timeUntilEscapeOpens = timeThatEscapeOpens - tatGS->GetServerWorldTimeSeconds();

   GetWorld()->GetTimerManager().ClearTimer(_nextStateTimer);

   if (timeUntilEscapeOpens > 0)
   {
      GetWorld()->GetTimerManager().SetTimer(
         _nextStateTimer,
         FTimerDelegate::CreateUObject(this, &ATATEscapePoint::_AuthorityOnEscapeBegin),
         timeUntilEscapeOpens, false);
   }
   else
   {
      // The match time has likely been updated past our opening time: in that case,
      // we want to open now, and stay open for our full duration
      _AuthorityOnEscapeBegin();
   }
}

void ATATEscapePoint::_AuthorityOnMatchStart()
{
   check(HasAuthority());
   check(_openState.State == ETATEscapePointState::Dormant);

   // TODO: this leaks information if we longer want to display this
   _AuthoritySetState(ETATEscapePointState::Pending);

   ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>();
   check(tatGS);

   _AuthorityOnMatchTimerUpdated(tatGS->GetCurrentPhase());

   tatGS->OnPhaseTimerChanged.AddUniqueDynamic(this, &ThisClass::_AuthorityOnMatchTimerUpdated);
}

void ATATEscapePoint::_AuthorityOnEscapeBegin()
{
   check(HasAuthority());

   if (_openState.State != ETATEscapePointState::Pending)
   {
      return;
   }

   _AuthoritySetState(ETATEscapePointState::Summonable);
}

void ATATEscapePoint::_AuthorityOpenEscape()
{
   check(HasAuthority());
   _AuthoritySetState(ETATEscapePointState::Open);

   // NB: net dormancy already flushed in _AuthoritySetState
   _openState.ServerTimeOpened = GetWorld()->GetTimeSeconds();

   if (_escapeClosesOnTimer && _escapeOpenDuration > 0)
   {
      GetWorld()->GetTimerManager().SetTimer(
         _nextStateTimer,
         FTimerDelegate::CreateUObject(this, &ATATEscapePoint::_AuthorityOnEscapeEnd),
         _escapeOpenDuration, false);
   }
}

void ATATEscapePoint::_AuthorityOnEscapeEnd()
{
   check(HasAuthority());
   if (_openState.State == ETATEscapePointState::Open)
   {
      _AuthoritySetState(ETATEscapePointState::Closed);
   }
}

void ATATEscapePoint::_AuthoritySetState(ETATEscapePointState state)
{
   check(HasAuthority());

   if (_openState.State == state)
   {
      return;
   }

   FlushNetDormancy();

   const ETATEscapePointState oldState = _openState.State;
   _openState.State = state;

   GetWorld()->GetTimerManager().ClearTimer(_nextStateTimer);

   _OnStateChanged(oldState);
}

void ATATEscapePoint::_UpdateEscapeForState()
{
   UpdateEscapeRouteVisuals(IsEscapeUsableNow(), _openState.State);
   _UpdateMapSprite();
}

void ATATEscapePoint::_UpdateMapSprite()
{
   if (!IsValid(_mapActorComponent))
   {
      UE_LOG(LogTATEscapePoint, Warning, TEXT("%s | _UpdateMapSprite() failed due to invalid _mapActorComponent!"), *GetName());
      return;
   }

   // TODO: align more with state in a future pass
   if (_openState.State == ETATEscapePointState::Closed)
   {
      _mapActorComponent->UpdateMapSprite(_escapeRouteTakenMapSpriteEntry);
   }
   else if (_openState.State == ETATEscapePointState::Open)
   {
      _mapActorComponent->UpdateMapSprite(_escapeRouteOpenMapSpriteEntry);
   }
   else if (_openState.State == ETATEscapePointState::Pending)
   {
      _mapActorComponent->UpdateMapSprite(_escapeRoutePendingMapSpriteEntry);
   }
   else if (_openState.State == ETATEscapePointState::Summonable)
   {
      _mapActorComponent->UpdateMapSprite(_escapeRouteSummonableMapSpriteEntry);
   }
   else if (_openState.State == ETATEscapePointState::Dormant)
   {
      _mapActorComponent->UpdateMapSprite(_escapeRouteDormantMapSpriteEntry);
   }
}

void ATATEscapePoint::_OnStateChanged(ETATEscapePointState previousState)
{
   _UpdateEscapeForState();

   BP_OnStateChanged(_openState.State, previousState);

   // a quick approximation to count both open and summonable
   auto isStateUsable = [](ETATEscapePointState state) {
      return state == ETATEscapePointState::Open || state == ETATEscapePointState::Summonable;
   };
   const bool canBeUsedNow = isStateUsable(_openState.State);
   const bool couldBeUsedBefore = isStateUsable(previousState);

   if (canBeUsedNow != couldBeUsedBefore)
   {
      if (_compassIndicator.Get())
      {
         _compassIndicator->SetIndicatorEnabled(canBeUsedNow);
      }
      if (const UWorld* world = GetWorld())
      {
         if (UTATEscapeRouteWorldSubsystem* escapeRouteSubsystem = world->GetSubsystem<UTATEscapeRouteWorldSubsystem>())
         {
            if (canBeUsedNow)
            {
               escapeRouteSubsystem->OnEscapeRouteOpened.Broadcast(this);
            }
            else
            {
               escapeRouteSubsystem->OnEscapeRouteClosed.Broadcast(this);
            }
         }
      }
   }

   if (_openState.State == ETATEscapePointState::Summonable && _summonState.State == ETATEscapeSummonState::None)
   {
      BP_OnSummonStateChanged(ETATEscapeSummonState::None, ETATEscapeSummonState::None, false);
   }

   if (HasAuthority())
   {
      if (_openState.State == ETATEscapePointState::Summonable)
      {
         _summonOverlapTrigger->OnTargetFound.AddUObject(this, &ThisClass::_AuthorityOnSummonTargetFound);
         _summonOverlapTrigger->OnTargetLost.AddUObject(this, &ThisClass::_AuthorityOnSummonTargetLost);
         _summonOverlapTrigger->StartTracking();
      }
      else if(previousState == ETATEscapePointState::Summonable)
      {
         _summonOverlapTrigger->StopTracking();
         _actorsWithEffectApplied.CancelAll();
      }
   }
}

void ATATEscapePoint::_AuthorityOnSummonTargetFound(ETATOverlapTargetTriggerReason reason)
{
   _AuthorityUpdateSummonState();
   _ApplySummonEffectsToTargets();
   _AuthorityAddTeamChangeListeners();
}

void ATATEscapePoint::_AuthorityOnSummonTargetLost(AActor* lostTarget)
{
   _actorsWithEffectApplied.CancelByActor(lostTarget);

   if (ATATPlayerState* playerState = EscapeHelpers::GetPlayerStateForTarget(lostTarget))
   {
      playerState->OnIsInATeamChanged.RemoveAll(this);
   }

   _AuthorityUpdateSummonState();
}

void ATATEscapePoint::_AuthorityUpdateSummonState()
{
   auto getNewState = [this](TConstArrayView<TWeakObjectPtr<AActor>> targets) {
      if (targets.IsEmpty())
      {
         return ETATEscapeSummonState::None;
      }

      // check target compatibility
      TArray<const AActor*, TInlineAllocator<8>> existingTargets;
      for (TWeakObjectPtr<AActor> weakTarget : targets)
      {
         const AActor* target = weakTarget.Get();
         if(target == nullptr) continue;

         // If any are not allowed class, disallow
         if (_requiredFinalActorClass && !target->IsA(_requiredFinalActorClass))
         {
            return ETATEscapeSummonState::Error;
         }

         // if not coop with other targets, disallow that
         // (should check that explicitly rather than attitude?)
         // NOTE: not currently handling if they become allied while in the zone
         auto isUnfriendly = [target](const AActor* other) {
            return UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(target, other, ETATTeamDisguiseHandling::UseOriginalTeam) != EOSETeamAttitude::Friendly;
         };
         if (existingTargets.ContainsByPredicate(isUnfriendly))
         {
            return ETATEscapeSummonState::Error;
         }
         existingTargets.Add(target);
      }

      if (existingTargets.IsEmpty())
      {
         return ETATEscapeSummonState::None;
      }

      const AActor* firstTarget = existingTargets[0];
      const IOSETeamInterface* firstTeamInterface = Cast<IOSETeamInterface>(firstTarget);
      if (!ensure(firstTeamInterface))
      {
         return ETATEscapeSummonState::Error;
      }
      
      // Check that all members of the team are in the zone
      // TODO: This doesn't account for disconnects of a player outside the zone. Should this just poll on tick instead
      // TODO: Probably surface the counts somewhere
      const uint8 team = UTATTeamAttitudeSolver::GetOriginalTeam(firstTeamInterface->GetTeam());
      const UTATTeamsSubsystem* teamsSubsystem = GetWorld()->GetSubsystem<UTATTeamsSubsystem>();
      check(teamsSubsystem);
      for (TWeakObjectPtr<ATATCharacter> weakCharacter : teamsSubsystem->GetAllMembersOfTeam(team))
      {
         ATATCharacter* character = weakCharacter.Get();
         // TODO: some carve-out for already KOed, if a thing?
         if (character && !existingTargets.Contains(character))
         {
            return ETATEscapeSummonState::PartialTeam;
         }
      }

      // if no conflicts, then be summoning
      return ETATEscapeSummonState::Summoning;
   };

   const ETATEscapeSummonState newState = getNewState(_summonOverlapTrigger->GetCurrentTargets());
   if (_summonState.State == newState)
   {
      return;
   }

   const FTATEscapePointSummonState previous = _summonState;
   FlushNetDormancy();
   _summonState.State = newState;
   _summonState.StartedAt = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   _OnRep_SummonState(previous);

   // update timer
   GetWorld()->GetTimerManager().ClearTimer(_nextStateTimer);
   if (newState == ETATEscapeSummonState::Summoning)
   {
      GetWorld()->GetTimerManager().SetTimer(
         _nextStateTimer,
         FTimerDelegate::CreateUObject(this, &ATATEscapePoint::_AuthorityOnSummonComplete),
         _summonDuration, false);
   }

   // update effects
   _actorsWithEffectApplied.CancelAll();
   _ApplySummonEffectsToTargets();
}

void ATATEscapePoint::_AuthorityOnSummonComplete()
{
   // Summon = escape for now

   TArray<APlayerController*, TInlineAllocator<8>> existingTargets;
   for (TWeakObjectPtr<AActor> weakTarget : _summonOverlapTrigger->GetCurrentTargets())
   {
      const APawn* target = Cast<APawn>(weakTarget.Get());
      if (target == nullptr) continue;

      if (APlayerController* controller = target->GetController<APlayerController>())
      {
         existingTargets.Add(controller);
      }
   }

   if (ATATSessionGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATSessionGameMode>())
   {
      gameMode->HandlePlayersEscaped(existingTargets, this);
   }

   if (UTATEscapeRouteWorldSubsystem* escapeRouteSubsystem = GetWorld()->GetSubsystem<UTATEscapeRouteWorldSubsystem>())
   {
      for (APlayerController* controller : existingTargets)
      {
         if (ATATPlayerState* playerState = controller->GetPlayerState<ATATPlayerState>())
         {
            escapeRouteSubsystem->AuthorityBroadcastEscapeRouteUsed(this, playerState);
         }
      }
   }
}

void ATATEscapePoint::_ApplySummonEffectsToTargets()
{
   auto getEffectForState = [this](ETATEscapeSummonState state) -> TSubclassOf<UGameplayEffect> {
      switch (state) {
      case ETATEscapeSummonState::None:
      default:
         return nullptr;
      case ETATEscapeSummonState::Summoning:
         return _summoningEffect;
      case ETATEscapeSummonState::PartialTeam:
         return _partialTeamEffect;
      case ETATEscapeSummonState::Error:
         return _errorEffect;
      }
      };
   TSubclassOf<UGameplayEffect> effectForState = getEffectForState(_summonState.State);
   if (effectForState)
   {
      for (TWeakObjectPtr<AActor> weakTarget : _summonOverlapTrigger->GetCurrentTargets())
      {
         AActor* targetActor = weakTarget.Get();
         if (targetActor == nullptr) continue;

         // Assume actors with effects already have the right one
         if (_actorsWithEffectApplied.EffectEntries.ContainsByPredicate(
            [targetActor](const FOSEActorWithAppliedEffectEntry& entry) { return entry.Actor == targetActor; }))
         {
            continue;
         }

         UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(targetActor, false);
         if (asc == nullptr) continue;

         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         effectContext.AddInstigator(this, this);
         FGameplayEffectSpec effectSpec(effectForState.GetDefaultObject(), effectContext);

         if (_summonState.State == ETATEscapeSummonState::Summoning)
         {
            effectSpec.SetDuration(GetSummonTimeRemaining(), false);
         }

         FActiveGameplayEffectHandle activeEffectHandle = asc->ApplyGameplayEffectSpecToSelf(effectSpec);
         _actorsWithEffectApplied.Add(targetActor, activeEffectHandle);
      }
   }
}

void ATATEscapePoint::_AuthorityAddTeamChangeListeners()
{
   // Listen for changes to is-in-team to handle becoming allies while in the summon zone
   // NOTE: not the ideal event to listen for, but didn't want to do further surgery
   for (TWeakObjectPtr<AActor> weakTarget : _summonOverlapTrigger->GetCurrentTargets())
   {
      if (ATATPlayerState* playerState = EscapeHelpers::GetPlayerStateForTarget(weakTarget.Get()))
      {
         playerState->OnIsInATeamChanged.AddUniqueDynamic(this, &ThisClass::_AuthorityHandleTargetInTeamChange);
      }
   }
}

void ATATEscapePoint::_AuthorityHandleTargetInTeamChange(bool inTeam)
{
   _AuthorityUpdateSummonState();
}

bool ATATEscapePoint::IsEscapeUsableNow() const
{
   return _openState.State == ETATEscapePointState::Open;
}

float ATATEscapePoint::GetOpenTimeRemaining() const
{
   if (_openState.State == ETATEscapePointState::Open)
   {
      if (ATATGameState* tatGS = GetWorld()->GetGameState<ATATGameState>())
      {
         if (_escapeClosesOnTimer && _escapeOpenDuration > 0)
         {
            const float timeSinceOpening = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - _openState.ServerTimeOpened;
            return _escapeOpenDuration - timeSinceOpening;
         }
         else if(tatGS->GetCurrentPhase() == ETATMatchPhase::Endgame)
         {
            // No explicit end duration, so it will close when the match ends
            return tatGS->GetTimeLeftInPhase();
         }
      }
   }

   return 0.0f;
}

float ATATEscapePoint::GetSummonTimeRemaining() const
{
   if (_summonState.State == ETATEscapeSummonState::Summoning)
   {
      const float timeSinceStart = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() - _summonState.StartedAt;
      return FMath::Max(0, _summonDuration - timeSinceStart);
   }

   return 0.f;
}

#undef LOCTEXT_NAMESPACE
