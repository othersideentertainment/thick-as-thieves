// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose

// ue4
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATHighlightStateMgrComponent.generated.h"

UCLASS(BlueprintType)
class TAT_API UTATHighlightStateMgrComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATHighlightStateMgrComponent();

   // statics
   // Set highlight for all primitive components in the actor with the specified tag
   static void HighlightMeshesWithTag(AActor* actor, FName tag, bool isHighlighted);
   // Set highlight for a specific component
   static void HighlightComponent(UPrimitiveComponent* component, bool isHighlighted);
   static UTATHighlightStateMgrComponent* FindOrAdd(AActor* actor);

   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Meta = (DefaultToSelf = Actor))
   static void RequestActorHighlight(FName requestingSystemName, AActor* actor, bool isHighlighted);

   void RequestHighlightChange(FName requestingSystemName, bool isHighlighted);

private:
   void _UpdateHighlights();

private:
   TMap<FName, bool> _highlightRequests;
   bool _wasHighlighted = false;
};
