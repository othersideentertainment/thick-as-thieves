// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATEditorActorDependencySubsystem.generated.h"

// An editor-only subsystem for keeping track of reverse dependencies
// between actors, so that we can draw editor-vis for them.
//
// I have written one-off versions of this 3-4 times at this point,
// so just writing a more generic version
UCLASS()
class TAT_API UTATEditorActorDependencySubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
#if WITH_EDITOR
   TConstArrayView<TWeakObjectPtr<AActor>> GetDependencies(FName groupKey, TWeakObjectPtr<AActor> actor) const;
   TConstArrayView<TWeakObjectPtr<AActor>> GetReverseDependencies(FName groupKey, TWeakObjectPtr<AActor> actor) const;
   void UpdateDependencies(FName groupKey, TWeakObjectPtr<AActor> actor, TConstArrayView<AActor*> dependencies);

   template <typename TActor>
   void UpdateDependencies(FName groupKey, TWeakObjectPtr<AActor> actor, TConstArrayView<TObjectPtr<TActor>> dependencies)
   {
      TConstArrayView<TActor*> rawView(dependencies);
      // NB: The built-in array-view conversion does not allow this, but th
      UpdateDependencies(groupKey, actor, TConstArrayView<AActor*>((AActor**)rawView.GetData(), rawView.Num()));
   }
#endif
   
protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;
private:
#if WITH_EDITORONLY_DATA
   struct FDependencyGroup
   {
      TMap<TWeakObjectPtr<AActor>, TArray<TWeakObjectPtr<AActor>>> Dependencies;
      TMap<TWeakObjectPtr<AActor>, TArray<TWeakObjectPtr<AActor>>> ReverseDependencies;

      template<typename TContainer>
      void UpdateDependencies(TWeakObjectPtr<AActor>, const TContainer& newDependencies);
   };

   TMap<FName, FDependencyGroup> _dependencyGroups;;
#endif

};


