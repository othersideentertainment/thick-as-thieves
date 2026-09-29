// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATElectrotypeBooth.h"

// tat
#include "Variation/Clues/TATClueType.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Online/TATGameState.h"
#include "Player/TATLocalPlayerStateWorldSubsystem.h"
#include "Player/TATPlayerState.h"
#include "Quests/TATQuestTags.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue
#include "Algo/Partition.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATElectrotypeBooth)
DEFINE_LOG_CATEGORY_STATIC(LogTATElectrotypeBooth, Log, All);

namespace ElectrotypeHelpers
{
   // Only replicate clue content to client with matching character
   static bool ShouldReplicateClue(TWeakObjectPtr<ACharacter> interactor, UPackageMap* map)
   {
      ACharacter* character = interactor.Get();
      if(!character) return false;

      // This is present on both client as server (possibly could be checked cast, as is done in other places)
      UPackageMapClient* mapClient = Cast<UPackageMapClient>(map);
      if(!mapClient) return false;

      UNetConnection* netConnection = mapClient->GetConnection();

      return netConnection == character->GetNetConnection();
   }

   static int32 GetSeedForCharacter(const ACharacter* character)
   {
      check(character);
      const ATATGameState* gameState = character->GetWorld()->GetGameState<ATATGameState>();
      const APlayerState* playerState = character->GetPlayerState();
      if(!gameState || !playerState)
      {
         return 0;
      }

      // NOTE: this doesn't actually vary with the booth, but that shouldn't matter,
      //       since the clues at each booth are random anyways. So if the clues are
      //       filtered in the same pattern, that would not make a difference.
      // TODO: is playerId enough difference in bits to vary enough
      return SeedHelpers::CombineSeed(gameState->GetMapSeed(), playerState->GetPlayerId());
   }
}

bool FTATActiveElectrotypeClue::NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess)
{
   bool replicateClueData = IsActive() && ar.IsSaving() && ElectrotypeHelpers::ShouldReplicateClue(Interactor, map);
   ar << ClueState;
   if(IsActive())
   {
      ar << replicateClueData;
      ar << ClueIndex;
   }
   else if (ar.IsLoading())
   {
      ClueIndex = NoClue;
   }

   if (replicateClueData)
   {
      ar << Interactor;
      ar << ClueText;
   }
   else if(ar.IsLoading())
   {
      Interactor.Reset();
      ClueText = FText();
   }

   outSuccess = true;
   return true;
}

UTATElectrotypeClueComponent::UTATElectrotypeClueComponent()
{
   _clueType = ETATClueType::Electrotype;
}

void UTATElectrotypeClueComponent::AddClue(const FTATElectrotypeClueData& clue)
{
   CastChecked<ATATElectrotypeBooth>(GetOwner())->AddClue(clue);
}

void UTATElectrotypeClueComponent::OnAllCluesApplied()
{
   CastChecked<ATATElectrotypeBooth>(GetOwner())->OnAllCluesApplied();
}

ATATElectrotypeBooth::ATATElectrotypeBooth()
{
   PrimaryActorTick.bStartWithTickEnabled = false;
   bReplicates = true;
   NetDormancy = DORM_Initial;
   
   _clueComponent = CreateDefaultSubobject<UTATElectrotypeClueComponent>("ClueSpawner");
}

void ATATElectrotypeBooth::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATElectrotypeBooth, _clueSources);
   DOREPLIFETIME(ATATElectrotypeBooth, _activeClue);
   DOREPLIFETIME(ATATElectrotypeBooth, _interactingCharacter);
   DOREPLIFETIME(ATATElectrotypeBooth, _interactAnimState);
}

bool ATATElectrotypeBooth::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   // Don't allow multiple characters to interact at once (but also don't hide interaction prompt for interacting character)
   if (_interactingCharacter.IsValid() && interactingCharacter != _interactingCharacter.Get())
   {
      return false;
   }

   switch (_activeClue.ClueState)
   {
   case ETATActiveElectrotypeClueState::NotActive:
      return !_activeClue.IsActive() && _clueSources.IsValidIndex(_GetNextClueIndex(interactingCharacter));

   case ETATActiveElectrotypeClueState::Pending:
      // Interaction only "allowed" to present _pendingCluePrompt
      return interactingCharacter == _activeClue.Interactor;

   case ETATActiveElectrotypeClueState::Dispensed:
      // Must wait for clue text to replicate to client before they can take the clue!
      if (_activeClue.ClueText.IsEmpty())
      {
         return false;
      }
      return interactingCharacter == _activeClue.Interactor;
   }

   return false;
}

FInteractStartResult ATATElectrotypeBooth::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result;

   switch (_activeClue.ClueState)
   {
   case ETATActiveElectrotypeClueState::NotActive:
      // Handle instant interaction
      if (_heldInteractDuration <= 0)
      {
         result.InstantAnimationTag = _interactPressAnimationTag;
         _PresentNextClue(interactingCharacter);
      }
      // Handle held interaction (if no other interaction in progress)
      else if (!_interactingCharacter.IsValid())
      {
         // Cache character to prevent others from interacting
         FlushNetDormancy();
         _interactingCharacter = interactingCharacter;

         result.HoldAnimationTag = _interactHoldAnimationTag;
         result.HoldActionCues = _interactHoldActionCues;
         result.bWaitForDelay = true;
         result.Delay = _heldInteractDuration;

         _SetInteractAnimationState(ETATElectrotypeInteractAnimationState::HeldInteractInProgress);
      }
      break;

   case ETATActiveElectrotypeClueState::Pending:
      // No interaction allowed while pending
      break;

   case ETATActiveElectrotypeClueState::Dispensed:
      // Handle press-interact to take clue if dispensed for interacting player
      if (_activeClue.Interactor == interactingCharacter)
      {
         result.InstantAnimationTag = _interactPressAnimationTag;
         _TakeActiveClue(interactingCharacter);
      }
      break;
   }

   return result;
}

bool ATATElectrotypeBooth::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if(interactingCharacter != _interactingCharacter.Get() && _interactingCharacter.IsValid())
   {
      return false;
   }

   if (context.IsComplete())
   {
      _SetInteractAnimationState(ETATElectrotypeInteractAnimationState::HeldInteractCompleted);
      _PresentNextClue(interactingCharacter);
   }
   else
   {
      _SetInteractAnimationState(ETATElectrotypeInteractAnimationState::InitialState);
   }

   // Clear interacting character to allow another interaction
   FlushNetDormancy();
   _interactingCharacter.Reset();
   return false;
}

void ATATElectrotypeBooth::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   switch (_activeClue.ClueState)
   {
   case ETATActiveElectrotypeClueState::NotActive:
      if (_heldInteractDuration > 0)
      {
         outPrompt.HoldAction = _generateCluePrompt;
      }
      else
      {
         outPrompt.PressAction = _generateCluePrompt;
      }
      break;

   case ETATActiveElectrotypeClueState::Dispensed:
      outPrompt.PressAction = _takeCluePrompt;
      break;

   case ETATActiveElectrotypeClueState::Pending:
      // Use ErrorMessage to indicate no allowed action
      outPrompt.ErrorMessage = _pendingCluePrompt;
      break;
   }
}

void ATATElectrotypeBooth::AddClue(const FTATElectrotypeClueData& clue)
{
   check(HasAuthority());
   _authorityClues.Add(clue);
}

void ATATElectrotypeBooth::OnAllCluesApplied()
{
   check(HasAuthority());
   if(_authorityClues.IsEmpty())
   {
      return;
   }
   
   // partition so mission-related clues are first
   // This allows all players to see their clues in the same order, but see the mission clues first
   Algo::Partition(_authorityClues.GetData(), _authorityClues.Num(), [](const FTATElectrotypeClueData& clue) { return clue.SourceTag.MatchesTag(TAG_Mission);});
   
   // copy source tags to replicated array
   FlushNetDormancy();
   _clueSources.Reserve(_authorityClues.Num());
   Algo::Transform(_authorityClues, _clueSources, [](const FTATElectrotypeClueData& clue) { return clue.SourceTag; });
   _InitLocalClues();
}

void ATATElectrotypeBooth::AuthorityOnClueFinishedDispensing()
{
   check(HasAuthority());
   if (_activeClue.ClueState != ETATActiveElectrotypeClueState::Pending)
   {
      UE_LOG(LogTATElectrotypeBooth, Error, TEXT("[%s] AuthorityOnClueFinishedDispensing() called with active clue in invalid state %s!"), *GetName(), *UEnum::GetValueAsString(_activeClue.ClueState));
      return;
   }

   UE_LOG(LogTATElectrotypeBooth, Verbose, TEXT("[%s] finished dispensing clue for character %s"), *GetName(), *GetNameSafe(_activeClue.Interactor.Get()));
   
   const int32 clueIndex = _activeClue.ClueIndex;
   check(_clueSources.IsValidIndex(clueIndex));

   FlushNetDormancy();
   _activeClue.ClueState = ETATActiveElectrotypeClueState::Dispensed;
   _activeClue.ClueText = _authorityClues[clueIndex].ClueText;

   GetWorldTimerManager().SetTimer(_expireClueTimer, FTimerDelegate::CreateUObject(this, &ThisClass::_ExpireActiveClue), _clueDisplayDuration, false);
}

void ATATElectrotypeBooth::_SetInteractAnimationState(ETATElectrotypeInteractAnimationState newInteractAnimState)
{
   if (newInteractAnimState == _interactAnimState)
   {
      return;
   }

   UE_LOG(LogTATElectrotypeBooth, Verbose, TEXT("[%s] _interactAnimState changed (%s => %s)")
      , *GetName()
      , *UEnum::GetValueAsString(_interactAnimState)
      , *UEnum::GetValueAsString(newInteractAnimState));

   const ETATElectrotypeInteractAnimationState oldState = _interactAnimState;
   FlushNetDormancy();
   _interactAnimState = newInteractAnimState;
   _OnRep_InteractState(oldState);
}

void ATATElectrotypeBooth::_PresentNextClue(ACharacter* interactingCharacter)
{
   UE_LOG(LogTATElectrotypeBooth, Verbose, TEXT("[%s] Dispensing clue for character %s..."), *GetName(), *interactingCharacter->GetName());

   FTATActiveElectrotypeClue newClue;
   newClue.ClueState = ETATActiveElectrotypeClueState::Pending;
   newClue.Interactor = interactingCharacter;
   if(HasAuthority())
   {
      const int32 clueIndex = _GetNextClueIndex(interactingCharacter);
      check(_clueSources.IsValidIndex(clueIndex));
      newClue.ClueIndex = clueIndex;
      _SetLastClueIndex(interactingCharacter, clueIndex);
   }

   // client and server can predictively set the fact of the clue, but not the text
   _SetActiveClue(newClue);
}

void ATATElectrotypeBooth::_OnRep_ActiveClue(const FTATActiveElectrotypeClue& previous)
{
   if(previous.IsActive() != _activeClue.IsActive())
   {
      BP_OnClueActive(_activeClue.IsActive());

      if(!_activeClue.IsActive() && _expireClueTimer.IsValid())
      {
         GetWorldTimerManager().ClearTimer(_expireClueTimer);
      }

      if (previous.ClueState == ETATActiveElectrotypeClueState::Dispensed)
      {
         ACharacter* clueOwner = previous.Interactor.Get();
         BP_OnClueTakenOrExpired(clueOwner);
      }
   }

   ACharacter* character = _activeClue.Interactor.Get();
   if(_activeClue.ClueIndex != FTATActiveElectrotypeClue::NoClue && character && character->IsLocallyControlled())
   {
      // the way this could get out sync is if the client never sees the clue it triggered itself
      // if that is a concern, could do some replicated tricks to have per-client replication, but the machinery for that is decently sized
      // Seems less slightly less risky, where a race between players to activate a clue is vaguely plausibly
      _SetLastClueIndex(character, _activeClue.ClueIndex);
   }

   if (!IsNetMode(NM_DedicatedServer) && _activeClue.Interactor != previous.Interactor)
   {
      _UpdateHasNextOrActiveClue();
   }
}

void ATATElectrotypeBooth::_OnRep_ClueSources()
{
   _InitLocalClues();
}

void ATATElectrotypeBooth::_OnRep_InteractState(ETATElectrotypeInteractAnimationState previous)
{
   // Should be guaranteed but just in case
   check(_interactAnimState != previous);
   BP_HandleInteractAnimationState(previous, _interactAnimState);
}

void ATATElectrotypeBooth::_SetActiveClue(const FTATActiveElectrotypeClue& newClue)
{
   const FTATActiveElectrotypeClue previousClue = _activeClue;
   FlushNetDormancy();
   _activeClue = newClue;
   _OnRep_ActiveClue(previousClue);
}

void ATATElectrotypeBooth::_ClearActiveClue()
{
   _SetActiveClue({});

   // Restore interact-animation state to initial value so we can handle another press-and-hold interaction
   _SetInteractAnimationState(ETATElectrotypeInteractAnimationState::InitialState);
}

void ATATElectrotypeBooth::_ExpireActiveClue()
{
   UE_LOG(LogTATElectrotypeBooth, Verbose, TEXT("[%s] Clue expired"), *GetName());
   _expireClueTimer.Invalidate();
   _ClearActiveClue();
}

void ATATElectrotypeBooth::_TakeActiveClue(ACharacter* interactingCharacter)
{
   check(interactingCharacter);
   UE_LOG(LogTATElectrotypeBooth, Verbose, TEXT("[%s] %s taking clue"), *GetName(), *interactingCharacter->GetName());

   // Present clue text
   if (interactingCharacter->IsLocallyControlled())
   {
      BP_HandleClueText(_activeClue.ClueText);
   }

   if (HasAuthority())
   {
      const int32 clueIndex = _activeClue.ClueIndex;
      _AuthorityRecordSeenClue(interactingCharacter, _authorityClues[clueIndex]);
   }

   _ClearActiveClue();
}

void ATATElectrotypeBooth::_InitLocalClues()
{
   if (IsNetMode(NM_DedicatedServer))
   {
      return;
   }

   if (auto* localPlayerStateSubsystem = GetWorld()->GetSubsystem<UTATLocalPlayerStateWorldSubsystem>())
   {
      _OnLocalQuestChanged(localPlayerStateSubsystem->GetLocalQuests());
      localPlayerStateSubsystem->OnLocalQuestChanged.AddUObject(this, &ThisClass::_OnLocalQuestChanged);
   }
}

void ATATElectrotypeBooth::_OnLocalQuestChanged(TConstArrayView<FGameplayTag> questTags)
{
   // NB: Do not need to do random filtering here, because it will never filter to zero clues from
   //     the starting index, if there are clues to show.
   _SetHasNextClue(!_CalculateClueIndicesForMissions(questTags).IsEmpty());
}

TConstArrayView<FGameplayTag> ATATElectrotypeBooth::_GetQuestTagsForCharacter(const ACharacter* character) const
{
   const ATATPlayerState* playerState = character->GetPlayerState<ATATPlayerState>();
   return playerState ? playerState->GetActiveQuestTags() : TConstArrayView<FGameplayTag>();
}

ATATElectrotypeBooth::FClueIndices ATATElectrotypeBooth::_CalculateClueIndicesForMissions(TConstArrayView<FGameplayTag> questTags) const
{
   FClueIndices result;
   auto shouldSkip = [questTags](FGameplayTag sourceTag) {
      return sourceTag.MatchesTag(TAG_Mission) && !questTags.Contains(sourceTag);
   };

   for(int i = 0; i < _clueSources.Num(); ++i)
   {
      if(!shouldSkip(_clueSources[i]))
      {
         result.Add(i);
      }
   }
   return result;
}

ATATElectrotypeBooth::FClueIndices ATATElectrotypeBooth::_CalculateClueIndicesForCharacter(const ACharacter* character) const
{
   // randomly (but deterministically) select the subset of clue indices to show to the player
   
   // NOTE(zkamsler): I would normally use reservoir sampling here, but I already kinda needed
   //                 _CalculateClueIndicesForQuests for a different codepath. So might as well
   //                 use it here, as long as the total number is likely to be fewer than the inline
   //                 size.
   FClueIndices candidates = _CalculateClueIndicesForMissions(_GetQuestTagsForCharacter(character));
   FRandomStream randomStream(ElectrotypeHelpers::GetSeedForCharacter(character));

   const int32 desiredCluesCount = FMath::Min(_maxCluesToDisplay, candidates.Num());
   FClueIndices result;

   // Prioritize keeping clues that are mission-related. First-come-first-serve for now
   // NB: Mission-related clues are already sorted to the beginning of the array
   const int32 firstNonMission = candidates.IndexOfByPredicate([this](uint8 index) { return !_clueSources[index].MatchesTag(TAG_Mission); });
   if(firstNonMission > 0)
   {
      const int32 clampedCount = FMath::Min(desiredCluesCount, firstNonMission);
      result = TArrayView<uint8>(candidates).Slice(0, clampedCount);
      candidates.RemoveAt(0, clampedCount);
   }

   // randomly select from the rest
   for(int i = result.Num(); i < desiredCluesCount; ++i)
   {
      const int32 indexToRemove = randomStream.RandHelper(candidates.Num());
      result.Add(candidates[indexToRemove]);
      candidates.RemoveAtSwap(indexToRemove);
   }
   return result;
}

int32 ATATElectrotypeBooth::_GetNextClueIndex(const ACharacter* character) const
{
   const int32* found = _clueIndexByPlayer.Find(character);
   const int32 prevIndex = found ? *found : INDEX_NONE;
   FClueIndices allowedIndices = _CalculateClueIndicesForCharacter(character);

   return _GetNextClueIndex(prevIndex, allowedIndices);
}

int32 ATATElectrotypeBooth::_GetNextClueIndex(int32 lastIndex, TConstArrayView<uint8> allowedIndices) const
{
   int32 nextIndex = lastIndex + 1;

   check(_clueSources.Num() < 256);
   while (_clueSources.IsValidIndex(nextIndex) && !allowedIndices.Contains(nextIndex))
   {
      ++nextIndex;
   }

   return nextIndex;
}

void ATATElectrotypeBooth::_SetLastClueIndex(const ACharacter* character, int32 index)
{
   check(character);
   _clueIndexByPlayer.Add(character, index);
   if (character->IsLocallyControlled())
   {
      _SetHasNextClue(_clueSources.IsValidIndex(_GetNextClueIndex(character)));
   }
}

void ATATElectrotypeBooth::_AuthorityRecordSeenClue(const ACharacter* character, const FTATElectrotypeClueData& clueData) const
{
   check(HasAuthority());
   if(character)
   {
      UTATKnownCluesComponent::RecordKnownClueFacts(character->GetPlayerState(), clueData.Facts.Get());
   }
}

void ATATElectrotypeBooth::_SetHasNextClue(bool hasNext)
{
   check(!IsNetMode(NM_DedicatedServer));
   if (_localPlayerClues.HasNextClue != hasNext)
   {
      _localPlayerClues.HasNextClue = hasNext;
      _UpdateHasNextOrActiveClue();
   }
}

void ATATElectrotypeBooth::_UpdateHasNextOrActiveClue()
{
   check(!IsNetMode(NM_DedicatedServer));

   const bool hasLocalActiveClue = _activeClue.Interactor.IsValid() && _activeClue.Interactor->IsLocallyControlled();
   const bool hasEither = _localPlayerClues.HasNextClue || hasLocalActiveClue;

   if (hasEither != _localPlayerClues.HasNextOrActiveClue)
   {
      _localPlayerClues.HasNextOrActiveClue = hasEither;
      BP_OnLocalClueAvailabilityChanged(hasEither);
   }
}
