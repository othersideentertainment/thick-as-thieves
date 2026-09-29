// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Developer/TATEditorActorDependencySubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEditorActorDependencySubsystem)



bool UTATEditorActorDependencySubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Editor;
}

#if WITH_EDITOR
TConstArrayView<TWeakObjectPtr<AActor>> UTATEditorActorDependencySubsystem::GetDependencies(FName groupKey, TWeakObjectPtr<AActor> actor) const
{
   if(const FDependencyGroup* group = _dependencyGroups.Find(groupKey))
   {
      if(const TArray<TWeakObjectPtr<AActor>>* found = group->Dependencies.Find(actor))
      {
         return MakeConstArrayView(*found);
      }
   }

   return {};
}

TConstArrayView<TWeakObjectPtr<AActor>> UTATEditorActorDependencySubsystem::GetReverseDependencies(FName groupKey,
   TWeakObjectPtr<AActor> actor) const
{
   if(const FDependencyGroup* group = _dependencyGroups.Find(groupKey))
   {
      if(const TArray<TWeakObjectPtr<AActor>>* found = group->ReverseDependencies.Find(actor))
      {
         return MakeConstArrayView(*found);
      }
   }

   return {};
}

void UTATEditorActorDependencySubsystem::UpdateDependencies(FName groupKey, TWeakObjectPtr<AActor> actor, TConstArrayView<AActor*> dependencies)
{
   _dependencyGroups.FindOrAdd(groupKey).UpdateDependencies(actor, dependencies);
}

// This is a little involved due to:
// 1. Being the more complicated many-to-many
// 2. Being a template to allow both weak and strong pointers as the input array
template <typename TContainer>
void UTATEditorActorDependencySubsystem::FDependencyGroup::UpdateDependencies(TWeakObjectPtr<AActor> dependingActor, const TContainer& newDependencies)
{
   const TArray<TWeakObjectPtr<AActor>>* previousPointer = Dependencies.Find(dependingActor);
   TConstArrayView<TWeakObjectPtr<AActor>> previous = previousPointer ? TConstArrayView<TWeakObjectPtr<AActor>>(*previousPointer) : TConstArrayView<TWeakObjectPtr<AActor>>();
   using NewElemType = typename TContainer::ElementType;

   // remove reverse dependencies no longer in the new array
   for(TWeakObjectPtr<AActor> previousActor : previous)
   {
      if(!newDependencies.ContainsByPredicate([previousActor](const NewElemType& newActor){ return TWeakObjectPtr<AActor>(newActor) == previousActor; }))
      {
         if(TArray<TWeakObjectPtr<AActor>>* found = ReverseDependencies.Find(previousActor))
         {
            found->Remove(dependingActor);
         }
      }
   }

   // add reverse dependencies in the new array
   for(const NewElemType& newActor : newDependencies)
   {
      if(!previous.Contains(newActor))
      {
         ReverseDependencies.FindOrAdd(newActor).Add(dependingActor);
      }
   }

   // Set the forward dependencies
   if(newDependencies.Num() > 0)
   {
      TArray<TWeakObjectPtr<AActor>>& current = Dependencies.FindOrAdd(dependingActor);
      current.Reset();
      current.Reserve(newDependencies.Num());
      for(const NewElemType& newDependency : newDependencies)
      {
         TWeakObjectPtr<AActor> asWeakObject = newDependency;
         if(asWeakObject.IsValid())
         {
            current.Add(newDependency);
         }
      }
   }
   else
   {
      Dependencies.Remove(dependingActor);
   }
}

#endif
