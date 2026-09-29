// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATLocalClueFactSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLocalClueFactSubsystem)

void UTATLocalClueFactSubsystem::CallOrRegisterFactDelegate(FGameplayTag factTag, FSimpleMulticastDelegate::FDelegate&& delegate)
{
   FFactEntry& entry = _listenersByTag.FindOrAdd(factTag);
   if(entry.IsKnown)
   {
      delegate.Execute();
   }
   else
   {
      entry.Listeners.Add(MoveTemp(delegate));
   }
}

void UTATLocalClueFactSubsystem::UnregisterFactDelegate(FGameplayTag factTag, const UObject* source)
{
   if(FFactEntry* entry = _listenersByTag.Find(factTag))
   {
      entry->Listeners.RemoveAll(source);
   }
}

void UTATLocalClueFactSubsystem::CallOrRegisterFactDelegateMulti(const FGameplayTagContainer& factTags, FSimpleMulticastDelegate::FDelegate&& delegate)
{
   for (const FGameplayTag& tag : factTags)
   {
      CallOrRegisterFactDelegate(tag, CopyTemp(delegate));
   }
}

void UTATLocalClueFactSubsystem::UnregisterFactDelegateMulti(const FGameplayTagContainer& factTags, const UObject* source)
{
   for (const FGameplayTag& tag : factTags)
   {
      UnregisterFactDelegate(tag, source);
   }
}

void UTATLocalClueFactSubsystem::CallOrRegisterNamespacedFactDelegate(FGameplayTag factTag, const FTATClueFactNamespace& space,
   FSimpleMulticastDelegate::FDelegate&& delegate)
{
   FFactEntry& entry = _listenersByNamespacedTag.FindOrAdd({factTag, space});
   if(entry.IsKnown)
   {
      delegate.Execute();
   }
   else
   {
      entry.Listeners.Add(MoveTemp(delegate));
   }
}

void UTATLocalClueFactSubsystem::UnregisterNamespacedFactDelegate(FGameplayTag factTag, const FTATClueFactNamespace& space, const UObject* source)
{
   if(FFactEntry* entry = _listenersByNamespacedTag.Find({factTag, space}))
   {
      entry->Listeners.RemoveAll(source);
   }
}

void UTATLocalClueFactSubsystem::CallOrRegisterNamespacedFactDelegateMulti(const FGameplayTagContainer& factTags, const FTATClueFactNamespace& space,
   FSimpleMulticastDelegate::FDelegate&& delegate)
{
   for (const FGameplayTag& tag : factTags)
   {
      CallOrRegisterNamespacedFactDelegate(tag, space, CopyTemp(delegate));
   }
}

void UTATLocalClueFactSubsystem::UnregisterNamespacedFactDelegateMulti(const FGameplayTagContainer& factTags, const FTATClueFactNamespace& space, const UObject* source)
{
   for (const FGameplayTag& tag : factTags)
   {
      UnregisterNamespacedFactDelegate(tag, space, source);
   }
}

void UTATLocalClueFactSubsystem::NotifyFactKnown(FGameplayTag factTag, const FTATClueFactNamespace& space)
{
   // Currently not marking parent tags, but could if there is
   // utility, and we know that we are not using parent tags as
   // distinct facts.
   auto markKnown = [](FFactEntry& entry)
   {
      if(!entry.IsKnown)
      {
         entry.IsKnown = true;
         entry.Listeners.Broadcast();
         entry.Listeners.Clear();
      }
   };

   // Need to always create entry in case listener is added later
   markKnown(_listenersByTag.FindOrAdd(factTag));
   markKnown(_listenersByNamespacedTag.FindOrAdd({factTag, space}));
}

bool UTATLocalClueFactSubsystem::IsFactKnown(FGameplayTag factTag) const
{
   const FFactEntry* entry = _listenersByTag.Find(factTag);
   return entry && entry->IsKnown;
}

bool UTATLocalClueFactSubsystem::AreAllFactsKnown(const FGameplayTagContainer& factTags) const
{
   for (const FGameplayTag& tag : factTags)
   {
      if (!IsFactKnown(tag))
      {
         return false;
      }
   }

   return true;
}

bool UTATLocalClueFactSubsystem::IsFactKnown(FGameplayTag factTag, const FTATClueFactNamespace& space) const
{
   const FFactEntry* entry = _listenersByNamespacedTag.Find({factTag, space});
   return entry && entry->IsKnown;
}

bool UTATLocalClueFactSubsystem::AreAllFactsKnown(const FGameplayTagContainer& factTags, const FTATClueFactNamespace& space) const
{
   for (const FGameplayTag& tag : factTags)
   {
      if (!IsFactKnown(tag, space))
      {
         return false;
      }
   }

   return true;
}
