// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATLockableToggle.h"

// tat
#include "Variation/TATSpawnerComponent.h"

// ue5
#include "GameFramework/Character.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLockableToggle)

DEFINE_LOG_CATEGORY_STATIC(LogTATLockableToggle, Log, All);

namespace ToggleHelpers
{
   static bool MatchDirection(ETATLockableToggleDirection direction, bool isInFront)
   {
      const ETATLockableToggleDirection mask = isInFront ? ETATLockableToggleDirection::FrontOnly : ETATLockableToggleDirection::BackOnly;
      return (static_cast<uint8>(direction) & static_cast<uint8>(mask)) != 0;
   }

   static bool MatchDirection(EToggleLockDirection direction, bool isInFront)
   {
      const EToggleLockDirection mask = isInFront ? EToggleLockDirection::LockFrontOnly : EToggleLockDirection::LockBackOnly;
      return (static_cast<uint8>(direction) & static_cast<uint8>(mask)) != 0;
   }
}

ATATLockableToggle::ATATLockableToggle()
{
   _allowLockWhenOn = false;
   _allowLockWhenOff = true;
   _frontDirection = FVector(0, 1, 0);

   // mission spawning disabled by default, let the default locked bool do its job.  only drive the locked
   // state for the mission if we set this up to be driven by it.
   _lockedSpawnerComponent = CreateDefaultSubobject<UTATSpawnerComponent>(TEXT("VariationSpawner"));
   _lockedSpawnerComponent->SetSpawnType(ETATSpawnChanceType::Disabled);

   // start dormant
   // TODO: We want this to be DORM_Initial, but are running into a replication issue where if a property is changed in the level instance,
   // and then changed back to its CDO default sometime later, FlushNetDormancy does not properly replicate the change to clients
   NetDormancy = DORM_DormantAll;
}

void ATATLockableToggle::BeginPlay()
{
   Super::BeginPlay();

   // Init lock config
   _lockConfig.RandomizeLockLevel(this);
}

void ATATLockableToggle::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATLockableToggle, _locked);
   DOREPLIFETIME(ATATLockableToggle, _lockpickCurrentTrack);
   DOREPLIFETIME(ATATLockableToggle, _lockDirection);
}

void ATATLockableToggle::PostInitializeComponents()
{
   Super::PostInitializeComponents();
   _lockConfig.InitializeAlternateLock(this);

   // if this was set to anything other than disabled wait for a spawn callback to decide to lock/unlock
   if (_lockedSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled)
   {
      _lockedSpawnerComponent->AuthorityOnSpawn.AddUniqueDynamic(this, &ATATLockableToggle::_AuthorityOnLockedSpawnerSpawn);
      _lockedSpawnerComponent->AuthorityOnNotSpawn.AddUniqueDynamic(this, &ATATLockableToggle::_AuthorityOnLockedSpawnerNotSpawn);
   }
}

#if WITH_EDITOR
void ATATLockableToggle::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      FMessageLog msgLog(FName("MapCheck"));
      if (_lockedSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled && (_locked != GetClass()->GetDefaultObject<ATATLockableToggle>()->_locked))
      {
         msgLog.Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
               TEXT("LockableToggle %s is locked by default, but is using a lock spawner. This currently has probably-dormancy bugs. So start with it unlocked. The behavior will be the same (but without the bugs). But still complain about this bug so it actually gets fixed."),
               *GetActorLabel()))));
      }

      _lockConfig.CheckForErrors([this, &msgLog](FText message)
      {
         msgLog.Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(MoveTemp(message)));
      });
   }
   
}
#endif

void ATATLockableToggle::SetLocked(bool newIsLocked)
{
   if (_locked != newIsLocked)
   {
      FlushNetDormancy();
      _locked = newIsLocked;

      if (HasAuthority())
      {
         // Reset the tracks completed
         _lockpickCurrentTrack = 0;
         OnLockChanged(_locked);
      }
   }
}

void ATATLockableToggle::OnLockpickTrackCompleted(int32 trackIndex)
{
   if (trackIndex < _lockpickCurrentTrack)
   {
      UE_LOG(LogTATLockableToggle, Warning, TEXT("OnLockpickTrackCompleted() called for a track that was not active"));
      return;
   }
   else if (!IsLocked())
   {
      UE_LOG(LogTATLockableToggle, Warning, TEXT("OnLockpickTrackCompleted() called on an unlocked actor"));
      return;
   }

   FlushNetDormancy();
   _lockpickCurrentTrack = trackIndex + 1;
}

void ATATLockableToggle::ToggleForInteraction(ACharacter* interactingCharacter)
{
   Super::ToggleForInteraction(interactingCharacter);

   // unlock if moving to state where lock is not allowed to match door behavior 
   if (HasAuthority() && !_AllowLockInState(IsOn()))
   {
      Unlock();
   }
}

bool ATATLockableToggle::ShouldVisualizeLockInDirection(bool isFrontSide) const
{
   const bool isSpawnerDrivenLock = _lockedSpawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled;
   const bool potentiallyLocked =  _locked || isSpawnerDrivenLock;
   return potentiallyLocked && (_lockDirection == EToggleLockDirection::Both || (_lockDirection == EToggleLockDirection::LockFrontOnly) == isFrontSide);
}

void ATATLockableToggle::SetLockDirection(EToggleLockDirection lockDirection)
{
   if (lockDirection != _lockDirection)
   {
      FlushNetDormancy();
      _lockDirection = lockDirection;
   }
}

FTATLockableToggleAllowedDirections ATATLockableToggle::_GetAllowedDirections() const
{
   return _normalAllowedDirections;
}

void ATATLockableToggle::OnRep_Locked()
{
   OnLockChanged(_locked);
}

FLockInteractContext ATATLockableToggle::_MakeLockContext(ACharacter* interactingCharacter) const
{
   const bool isInFront = _IsOnFrontSide(interactingCharacter->GetActorLocation());
   const FTATLockableToggleAllowedDirections allowedDirections = _GetAllowedDirections();
   
   FLockInteractContext ctx;
   ctx.bIsLocked = _locked;
   ctx.bIsLockRelevant = _AllowLockInState(IsOn());
   ctx.bAllowsKey = _lockConfig.KeyTag.IsValid() && ToggleHelpers::MatchDirection(allowedDirections.KeyDirection, isInFront);
   ctx.bHasKey = ctx.bIsLockRelevant && _lockConfig.DoesCharacterHaveKey(interactingCharacter);
   ctx.bCanInteractorLockpick = _lockConfig.CanBeLockpicked && FTATLockConfig::CanActorLockpick(interactingCharacter);
   ctx.bCanBePickedInCurrentDirection = ToggleHelpers::MatchDirection(allowedDirections.LockpickableDirection, isInFront);
   ctx.bIsLockedInCurrentDirection = _locked && ctx.bIsLockRelevant && ToggleHelpers::MatchDirection(_lockDirection, isInFront);
   ctx.bCanBeRelockedInCurrentDirection = !_locked && ctx.bIsLockRelevant && ToggleHelpers::MatchDirection(allowedDirections.RelockDirection, isInFront);
   // the above two conditions are mutually exclusive, so it will only evaluate lock direction at most once
   ctx.bAreAllSidesLocked = _lockDirection == EToggleLockDirection::Both;
   return ctx;
}

bool ATATLockableToggle::_AllowLockInState(bool isOn) const
{
   return isOn ? _allowLockWhenOn : _allowLockWhenOff;
}

bool ATATLockableToggle::_IsOnFrontSide(const FVector& characterPosition) const
{
   FVector localCharacterPosition = GetActorTransform().InverseTransformPositionNoScale(characterPosition);
   localCharacterPosition -= _centerOffset;
   return FVector::DotProduct(localCharacterPosition, _frontDirection) > 0;
}

bool ATATLockableToggle::_BlockedInDirection(const ACharacter* interactingCharacter) const
{
   return !IsOn() && !ToggleHelpers::MatchDirection(_GetAllowedDirections().AllowedTurnOnDirection,
                                                   _IsOnFrontSide(interactingCharacter->GetActorLocation()));
}

bool ATATLockableToggle::_TryPriorityInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if (_BlockedInDirection(interactingCharacter))
   {
      prompt.ErrorMessage = _cannotTurnOnFromDirectionMessage;
      return true;
   }

   return Super::_TryPriorityInteractPrompt(interactingCharacter, prompt);
}

void ATATLockableToggle::_AddNormalInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   const FLockInteractContext lockContext = _MakeLockContext(interactingCharacter);
   if (!lockContext.bIsLockedInCurrentDirection)
   {
      Super::_AddNormalInteractPrompt(interactingCharacter, prompt);
   }

   _lockConfig.AddToPrompt(prompt, lockContext, interactingCharacter);
}

bool ATATLockableToggle::_TryPriorityStartInteract(ACharacter* interactingCharacter, FInteractStartResult& outResult)
{
   if (_BlockedInDirection(interactingCharacter))
   {
      return true;
   }
   
   return Super::_TryPriorityStartInteract(interactingCharacter, outResult);
}

FInteractStartResult ATATLockableToggle::_NormalStartInteract(ACharacter* interactingCharacter)
{
   const FLockInteractContext lockContext = _MakeLockContext(interactingCharacter);
   FInteractStartResult result;
   if (_lockConfig.TryHandleInteractStart(this, interactingCharacter, lockContext, result))
   {
      return result;
   }
   else if (!lockContext.bIsLockedInCurrentDirection)
   {
      return Super::_NormalStartInteract(interactingCharacter);
   }

   return FInteractStartResult();
}

bool ATATLockableToggle::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   const FLockInteractContext lockContext = _MakeLockContext(interactingCharacter);
   if (context.IsComplete())
   {
      _lockConfig.HandleInteractComplete(this, interactingCharacter, lockContext);
   }
   else if (context.IsProbablyInstant() && !lockContext.bIsLockedInCurrentDirection)
   {
      ToggleForInteraction(interactingCharacter);
      // TODO: how do we handle instant animation tags triggered on end?
   }

   return false;
}

void ATATLockableToggle::_AuthorityOnLockedSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // if we "spawn" from the spawner system that means lock
   SetLocked(true);
}

void ATATLockableToggle::_AuthorityOnLockedSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // if we "don't spawn" from the spawner system that means unlock
   SetLocked(false);
}

