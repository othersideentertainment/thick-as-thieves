// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATClueFact.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATLocalClueFactSubsystem.generated.h"

// A subsystem that keeps track of the clue facts that are known by the local player
//
// NOTE: assumption that a single tag is sufficient will not last post-TOD
UCLASS()
class TAT_API UTATLocalClueFactSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:

   // delegate will be called when the fact becomes known by the local player
   void CallOrRegisterFactDelegate(FGameplayTag factTag, FSimpleMulticastDelegate::FDelegate&& delegate);
   void UnregisterFactDelegate(FGameplayTag factTag, const UObject* source);

   void CallOrRegisterFactDelegateMulti(const FGameplayTagContainer& factTags, FSimpleMulticastDelegate::FDelegate&& delegate);
   void UnregisterFactDelegateMulti(const FGameplayTagContainer& factTags, const UObject* source);

   void CallOrRegisterNamespacedFactDelegate(FGameplayTag factTag, const FTATClueFactNamespace& space, FSimpleMulticastDelegate::FDelegate&& delegate);
   void UnregisterNamespacedFactDelegate(FGameplayTag factTag, const FTATClueFactNamespace& space, const UObject* source);

   void CallOrRegisterNamespacedFactDelegateMulti(const FGameplayTagContainer& factTags, const FTATClueFactNamespace& space, FSimpleMulticastDelegate::FDelegate&& delegate);
   void UnregisterNamespacedFactDelegateMulti(const FGameplayTagContainer& factTags, const FTATClueFactNamespace& space, const UObject* source);

   // a fact tag is known to the local player
   void NotifyFactKnown(FGameplayTag factTag, const FTATClueFactNamespace& space);

   // TODO: Remove?
   bool IsFactKnown(FGameplayTag factTag) const;
   bool AreAllFactsKnown(const FGameplayTagContainer& factTags) const;
   
   bool IsFactKnown(FGameplayTag factTag, const FTATClueFactNamespace& space) const;
   bool AreAllFactsKnown(const FGameplayTagContainer& factTags, const FTATClueFactNamespace& space) const;

private:
   struct FFactEntry
   {
      FSimpleMulticastDelegate Listeners;
      bool IsKnown = false;
   };

   TMap<FGameplayTag, FFactEntry> _listenersByTag;


   struct FNamespacedFact
   {
      FGameplayTag FactTag;
      FTATClueFactNamespace Namespace;

      bool operator==(const FNamespacedFact&) const = default;
      friend uint32 GetTypeHash(const FNamespacedFact& n)
      {
         return HashCombineFast(GetTypeHash(n.FactTag), GetTypeHash(n.Namespace));
      }
   };

   TMap<FNamespacedFact, FFactEntry> _listenersByNamespacedTag;
};
