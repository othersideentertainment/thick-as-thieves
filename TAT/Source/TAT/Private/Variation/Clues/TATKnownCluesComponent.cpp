// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATKnownCluesComponent.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Variation/Clues/TATClueFact.h"
#include "Variation/Clues/TATClueLocationInterface.h"
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/Clues/TATLocalClueFactSubsystem.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Online/TATGameState.h"
#include "UI/TATUIFunctionLibrary.h"

// ose
#include "Player/OSEPlayerState.h"

// ue
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Quests/TATPlayerQuestComponent.h"
#include "Quests/TATQuestTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATKnownCluesComponent)

void FTATKnownClueArray::PostReplicatedAdd(const TArrayView<int32>& addedIndices, int32 finalSize)
{
   if(ensure(Owner))
   {
      for(int32 addedIndex : addedIndices)
      {
         Owner->_OnClueAdded(Items[addedIndex]);
      }
   }
}

UTATKnownCluesComponent::UTATKnownCluesComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   
   SetIsReplicatedByDefault(true);
}

void UTATKnownCluesComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.Condition = COND_OwnerOnly;

   DOREPLIFETIME_WITH_PARAMS_FAST(UTATKnownCluesComponent, _knownClues, params);
}

UTATKnownCluesComponent* UTATKnownCluesComponent::Get(const APlayerState* playerState)
{
   if(playerState)
   {
      return playerState->FindComponentByClass<UTATKnownCluesComponent>();
   }
   return nullptr;
}

void UTATKnownCluesComponent::OnRegister()
{
   Super::OnRegister();
   
   _knownClues.Owner = this;
}

void UTATKnownCluesComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority())
   {
      if (const ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
      {
         if (UTATMapVariationMgrComponent* mapVariationMgr = gameState->GetMapVariationMgr())
         {
            mapVariationMgr->AuthorityCallOrWaitForComplete(FSimpleDelegate::CreateWeakLambda(this, [this, mapVariationMgr]() {
               mapVariationMgr->ForEachInitialClueThunk([this](const FTATClueFactThunk& thunk) { AddClueFacts(&thunk); });
            }));
         }
      }
   }

}

void UTATKnownCluesComponent::RecordKnownClueFacts(const APlayerState* playerState, const FTATClueFactThunk* facts)
{
   if(UTATKnownCluesComponent* knownClues = Get(playerState))
   {
      knownClues->AddClueFacts(facts);
   }
}

void UTATKnownCluesComponent::BP_RecordKnownClueFacts(const APlayerState* playerState, FGameplayTag sourceTag, const TArray<FDataTableRowHandle>& factHandles)
{
   if(UTATKnownCluesComponent* knownClues = Get(playerState))
   {
      // make a partial clue context
      const FTATClueContext dummyContext = {
         .Location = UTATDummyClueLocation::Get(),
         .SourceTag = sourceTag
      };
      const FTATClueFactThunk factThunk(factHandles, dummyContext);
      knownClues->AddClueFacts(&factThunk);
   }
}

void UTATKnownCluesComponent::AddClueFacts(const FTATClueFactThunk* facts)
{
   if(facts == nullptr || !GetOwner()->HasAuthority())
   {
      return;
   }

   const int startingKnownCount = _knownClues.Items.Num();
   const int factCount = facts->GetFactCount();
   const FGameplayTag sourceTag = facts->GetSourceTag();
   const int32 sourceIndex = facts->GetSourceIndex();
   for(int i = 0; i < factCount; ++i)
   {
      const FGameplayTag factTag = facts->GetFactTagAt(i);
      if(!_knownClues.Items.Contains(FNamespacedFactTag {sourceTag, sourceIndex, factTag}))
      {
         const FTATClueFactSpec& spec = facts->GetFactSpecAt(i);
         FTATKnownClue& newEntry = _knownClues.Items.Emplace_GetRef();
         newEntry.ClueText = facts->GetFactTextAt(i, this);
         newEntry.SourceTag = sourceTag;
         newEntry.SourceIndex = sourceIndex;
         newEntry.FactTag = factTag;
         newEntry.CategoryTag = spec.Category;
         newEntry.DisplayPriority = spec.DisplayPriority;
         newEntry.SuppressedByFactTags = spec.SuppressedByFacts;
         _OnClueAdded(newEntry);
         _knownClues.MarkItemDirty(newEntry);
      }
   }

   if(_knownClues.Items.Num() > startingKnownCount)
   {
      _AuthorityHandleAddedFacts(startingKnownCount);
   }
}

void UTATKnownCluesComponent::AddSyntheticFact(FGameplayTag sourceTag, FGameplayTag factTag)
{
   if (!factTag.IsValid() || !GetOwner()->HasAuthority())
   {
      return;
   }

   // Leave source index for synthetic facts as 0 until there is a use-case to use SourceIndex for a synthetic fact
   if (!_knownClues.Items.Contains(FNamespacedFactTag { .SourceTag = sourceTag, .FactTag = factTag}))
   {
      FTATKnownClue& newEntry = _knownClues.Items.Emplace_GetRef();
      newEntry.SourceTag = sourceTag;
      newEntry.FactTag = factTag;
      _OnClueAdded(newEntry);
      _knownClues.MarkItemDirty(newEntry);
      _AuthorityHandleAddedFacts(_knownClues.Items.Num() - 1);
   }
}

void UTATKnownCluesComponent::BP_RecordSyntheticClueFact(const APlayerState* playerState, FGameplayTag sourceTag, FGameplayTag factTag)
{
   if(UTATKnownCluesComponent* knownClues = Get(playerState))
   {
      knownClues->AddSyntheticFact(sourceTag, factTag);
   }
}

void UTATKnownCluesComponent::AuthorityAddFactsFromAlly(TConstArrayView<FTATKnownClue> facts)
{
   check(GetOwner()->HasAuthority());

   const int startingKnownCount = _knownClues.Items.Num();
   for(const FTATKnownClue& fact : facts)
   {
      if(!_knownClues.Items.Contains(FNamespacedFactTag {fact.SourceTag, fact.SourceIndex, fact.FactTag}))
      {
         FTATKnownClue& newEntry = _knownClues.Items.Emplace_GetRef(fact);
         newEntry.SharedFromAlly = true;
         _OnClueAdded(newEntry);
         _knownClues.MarkItemDirty(newEntry);
      }
   }

   if(_knownClues.Items.Num() > startingKnownCount)
   {
      constexpr bool fromAlly = true;
      _AuthorityHandleAddedFacts(startingKnownCount, fromAlly);
   }
}

const FText& UTATKnownCluesComponent::GetClueText(int32 clueIndex) const
{
   if(_filteredClueIndices.IsValidIndex(clueIndex))
   {
      return _knownClues.Items[_filteredClueIndices[clueIndex]].ClueText;
   }
   return FText::GetEmpty();
}

int32 UTATKnownCluesComponent::GetClueCount() const
{
   return _filteredClueIndices.Num();
}

bool UTATKnownCluesComponent::HasFactForSource(FGameplayTag sourceTag) const
{
   // Can cache this if it is high traffic
	return _knownClues.Items.ContainsByPredicate([sourceTag](const FTATKnownClue& knownClue) { return knownClue.SourceTag == sourceTag; });
}

bool UTATKnownCluesComponent::HasFactWithTag(FGameplayTag sourceTag, FGameplayTag factTag) const
{
   // Can cache this if it is high traffic
   return _knownClues.Items.ContainsByPredicate([sourceTag, factTag](const FTATKnownClue& knownClue) { return knownClue.SourceTag == sourceTag && knownClue.FactTag == factTag; });
}

void UTATKnownCluesComponent::GetClueFactsWithCategory(FGameplayTag categoryTag, TArray<FText>& outFacts)
{
   outFacts.Reset();
   for(const int clueIndex : _filteredClueIndices)
   {
      const FTATKnownClue& knownClue = _knownClues.Items[clueIndex];
      if(knownClue.CategoryTag == categoryTag)
      {
         outFacts.Add(knownClue.ClueText);
      }
   }
}

void UTATKnownCluesComponent::_AuthorityShareCluesWithAllies(TConstArrayView<FTATKnownClue> facts)
{
   check(GetOwner()->HasAuthority());

   const AOSEPlayerState* ownerState = GetOwner<AOSEPlayerState>();
   if(!ensure(ownerState))
   {
      return;
   }
   const uint8 ownerTeam = ownerState->GetTeam();

   if (const AOSEGameState* gameState = GetWorld()->GetGameState<AOSEGameState>())
   {
      for(const AOSEPlayerState* playerState : gameState->GetOSEPlayerStates())
      {
         if(playerState->GetTeam() != ownerTeam || playerState == ownerState)
         {
            continue;
         }

         if(UTATKnownCluesComponent* cluesComponent = playerState->GetComponentByClass<UTATKnownCluesComponent>())
         {
            cluesComponent->AuthorityAddFactsFromAlly(facts);
         }
      }
   }
}

void UTATKnownCluesComponent::_AuthorityHandleAddedFacts(int32 startingIndex, bool fromAlly)
{
   MARK_PROPERTY_DIRTY_FROM_NAME(UTATKnownCluesComponent, _knownClues, this);
   _UpdateFilteredClues();
   if (_IsLocalPlayer())
   {
      _NotifyKnownFactsFrom(startingIndex);
   }
   // probably only need if local, but just in case
   _NotifyCluesUpdated();
   if(!fromAlly)
   {
      _AuthorityShareCluesWithAllies(MakeArrayView(_knownClues.Items).Mid(startingIndex));
   }
}

bool UTATKnownCluesComponent::_IsLocalPlayer() const
{
   const auto* ps = GetOwner<AOSEPlayerState>();
   return ps && ps->IsLocalPlayerState();
}

void UTATKnownCluesComponent::_NotifyKnownFactsFrom(int32 startingIndex)
{
   if(UTATLocalClueFactSubsystem* factSubsystem = GetWorld()->GetSubsystem<UTATLocalClueFactSubsystem>())
   {
      for(int i = startingIndex; i < _knownClues.Items.Num(); ++i)
      {
         const FTATKnownClue& knownClue = _knownClues.Items[i];
         factSubsystem->NotifyFactKnown(knownClue.FactTag, {.SourceTag = knownClue.SourceTag, .SourceIndex = knownClue.SourceIndex});
      }
   }
}

void UTATKnownCluesComponent::_UpdateFilteredClues()
{
   // TODO: if there are ever events fired when this updates, add some sort of scope to allow batching
   //       Or even before then
   _filteredClueIndices.Reset();
   _filteredClueIndices.Reserve(_knownClues.Items.Num());
   
   // keep facts that are not suppressed by another fact
   // (Could have combined known tags into a single container, but this is also fine)
   auto shouldKeep = [this](const FTATKnownClue& knownClue) {
      if (knownClue.ClueText.IsEmpty())
      {
         return false;
      }

      if (knownClue.SuppressedByFactTags.IsValid() &&
         _knownClues.Items.ContainsByPredicate([source = knownClue.SourceTag, &tags = knownClue.SuppressedByFactTags](const FTATKnownClue& c) { return c.SourceTag == source && c.FactTag.MatchesAny(tags); }))
      {
         return false;
      }

      return true;
   };

   const int clueCount = _knownClues.Items.Num();
   for (int i = 0; i < clueCount; ++i)
   {
      if (shouldKeep(_knownClues.Items[i]))
      {
         _filteredClueIndices.Add(i);
      }
   }
   _filteredClueIndices.StableSort([this](int32 a, int32 b)
   {
      const int priorityDiff = _knownClues.Items[a].DisplayPriority - _knownClues.Items[b].DisplayPriority;
      if (priorityDiff != 0)
      {
         return priorityDiff < 0;
      }
      return _knownClues.Items[a].ReplicationID < _knownClues.Items[b].ReplicationID;
   });
}

void UTATKnownCluesComponent::_NotifyCluesUpdated()
{
   OnCluesUpdated.Broadcast(this);
}

void UTATKnownCluesComponent::_OnClueAdded(const FTATKnownClue& knownClue)
{
   if(knownClue.SharedFromAlly && !knownClue.ClueText.IsEmpty() && _IsLocalPlayer())
   {
      // NOTE: Design is not currently planning to use the formatted toast text,
      //       so jettison that code-path if it becomes slightly inconvenient (and is still off)
      const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
      FText toastText = projectSettings.IncludeFactWithSharedClueToast ?
         FText::FormatNamed(projectSettings.SharedClueToastText, TEXT("Fact"), knownClue.ClueText) :
         projectSettings.SharedClueToastText;
      UTATUIFunctionLibrary::RequestToastIfLocallyControlled(GetOwner(), projectSettings.SharedClueToastTag,
         toastText);
   }
}

void UTATKnownCluesComponent::_OnRep_KnownClues()
{
   // could track which index to start from, but probably not worth it
   _UpdateFilteredClues();
   _NotifyKnownFactsFrom(0);
   _NotifyCluesUpdated();
}
