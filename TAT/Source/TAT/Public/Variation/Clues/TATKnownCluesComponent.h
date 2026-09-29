// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "TATKnownCluesComponent.generated.h"

struct FTATClueFactThunk;

// A textual clue that can be remembered
// TODO: Consider/measure ffast-array? (wouldn't help bandwidth, just maybe-server-cpu)
// TODO: rename to KnownClueFact?
USTRUCT()
struct FTATKnownClue : public FFastArraySerializerItem
{
   GENERATED_BODY()
   
   UPROPERTY()
   FText ClueText;
   UPROPERTY()
   FGameplayTag SourceTag;
   UPROPERTY()
   int32 SourceIndex = 0;
   UPROPERTY()
   FGameplayTag FactTag;
   UPROPERTY()
   FGameplayTag CategoryTag;
   UPROPERTY()
   FGameplayTagContainer SuppressedByFactTags;
   // ascending
   UPROPERTY()
   int32 DisplayPriority = 0;
   UPROPERTY()
   bool SharedFromAlly = false;
};

USTRUCT()
struct FTATKnownClueArray : public FFastArraySerializer
{
   GENERATED_BODY()

   UPROPERTY()
   TArray<FTATKnownClue> Items;

   UPROPERTY(Transient, NotReplicated)
   TObjectPtr<class UTATKnownCluesComponent> Owner = nullptr;

   bool NetDeltaSerialize(FNetDeltaSerializeInfo& deltaParms)
   {
      return FFastArraySerializer::FastArrayDeltaSerialize<FTATKnownClue, FTATKnownClueArray>(Items, deltaParms, *this);
   }

   void PostReplicatedAdd(const TArrayView<int32>& addedIndices, int32 finalSize);
};

template<>
struct TStructOpsTypeTraits<FTATKnownClueArray> : public TStructOpsTypeTraitsBase2<FTATKnownClueArray>
{
   enum
   {
      WithNetDeltaSerializer = true,
   };
};

// Keeps track of (textual) clues that the player has seen through direct interaction
//
// Now set on authority, and replicated to the client
//
// Clue fact identity is now namespaced by the clue source tag. This affects suppression
// by other facts, but does not affect highlights. The utility of "synthetic" facts is questionable
// with this namespacing, but it also is necessary if there are any facts that might be used multiple
// times at once. Major loot clues would be the obvious example, although there may be others.
UCLASS()
class TAT_API UTATKnownCluesComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATKnownCluesComponent();

   static UTATKnownCluesComponent* Get(const APlayerState* playerState);

   virtual void OnRegister() override;
   virtual void BeginPlay() override;

   static void RecordKnownClueFacts(const APlayerState* playerState, const FTATClueFactThunk* facts);

   // Limited BP wrapper function
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Clues, meta=(DisplayName="Record Known Clue Facts"))
   static void BP_RecordKnownClueFacts(const APlayerState* playerState, FGameplayTag sourceTag, const TArray<FDataTableRowHandle>& factHandles);
   
   void AddClueFacts(const FTATClueFactThunk* facts);
   // Adds a fact with just the tag, and no other payload
   // (used for other facts, etc to key off of)
   void AddSyntheticFact(FGameplayTag sourceTag, FGameplayTag factTag);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Clues, meta=(DisplayName="Record Synthetic Clue Fact"))
   static void BP_RecordSyntheticClueFact(const APlayerState* playerState, FGameplayTag sourceTag, UPARAM(meta=(GameplayTagFilter = "ClueFact")) FGameplayTag factTag);

   void AuthorityAddFactsFromAlly(TConstArrayView<FTATKnownClue> facts);

   UFUNCTION(BlueprintPure, Category=Clues)
   const FText& GetClueText(int32 clueIndex) const;
   UFUNCTION(BlueprintPure, Category=Clues)
   int32 GetClueCount() const;

   UFUNCTION(BlueprintCallable, Category=Clues)
   bool HasFactForSource(FGameplayTag sourceTag) const;
   bool HasFactWithTag(FGameplayTag sourceTag, FGameplayTag factTag) const;

   // Get the facts that have the exact category
   // (no partial matching, as supporting the empty tag)
   UFUNCTION(BlueprintCallable, Category=Clues, meta=(GameplayTagFilter="ClueFactCategory"))
   void GetClueFactsWithCategory(FGameplayTag categoryTag, TArray<FText>& outFacts);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCluesUpdated, UTATKnownCluesComponent*, knownClues);
   UPROPERTY(BlueprintAssignable)
   FOnCluesUpdated OnCluesUpdated;

private:
   // for Contains/FindWithKey on the array containing these
   struct FNamespacedFactTag
   {
      FGameplayTag SourceTag;
      int32 SourceIndex = 0;
      FGameplayTag FactTag;

      friend bool operator==(const FTATKnownClue& knownClue, const FNamespacedFactTag& tag)
      {
         return tag.SourceTag == knownClue.SourceTag && tag.SourceIndex == knownClue.SourceIndex && tag.FactTag == knownClue.FactTag;
      }
   };

   void _AuthorityShareCluesWithAllies(TConstArrayView<FTATKnownClue> facts);
   
   void _AuthorityHandleAddedFacts(int32 startingIndex, bool fromAlly = false);
   bool _IsLocalPlayer() const;
   void _NotifyKnownFactsFrom(int32 startingIndex);
   void _UpdateFilteredClues();
   void _NotifyCluesUpdated();

   void _OnClueAdded(const FTATKnownClue& knownClue);

   UFUNCTION()
   void _OnRep_KnownClues();
   
   UPROPERTY(Transient, ReplicatedUsing=_OnRep_KnownClues)
   FTATKnownClueArray _knownClues;
   friend FTATKnownClueArray;

   TArray<int32> _filteredClueIndices;
};
