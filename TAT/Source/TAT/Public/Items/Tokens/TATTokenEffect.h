// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATTokenEffect.generated.h"


// Polymorphic struct that represents side effects of holding a given token
//
// TODO: Should we define _which_ actor this is called on? (Character vs PlayerState)
USTRUCT()
struct TAT_API FTATTokenEffect
{
   GENERATED_BODY()
   
   virtual ~FTATTokenEffect() {}
   virtual void OnAdded(AActor* holdingActor) const {}
   virtual void OnRemoved(AActor* holdingActor) const {}

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const {}
#endif
};

USTRUCT(DisplayName = "Allow in Private Space")
struct TAT_API FTATTokenEffect_PrivateSpace : public FTATTokenEffect
{
   GENERATED_BODY()
   
   virtual void OnAdded(AActor* holdingActor) const override;
   virtual void OnRemoved(AActor* holdingActor) const override;

   UPROPERTY(EditAnywhere, meta = (Categories = "PrivateSpaceCategory"))
   FGameplayTagContainer SpaceTags;
};
