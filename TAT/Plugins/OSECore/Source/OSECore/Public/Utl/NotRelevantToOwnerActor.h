// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NotRelevantToOwnerActor.generated.h"

// A simple base class an actor that is never net-relevant to its owner
//   More useful for blueprints than c++, since it is trivial
UCLASS(Blueprintable)
class OSECORE_API ANotRelevantToOwnerActor : public AActor
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ANotRelevantToOwnerActor();

   virtual void BeginReplication() override;
   virtual bool IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const override;
   virtual void SetOwner(AActor* newOwner) override;

private:
   void _TryUpdateConnectionFilter();
};
