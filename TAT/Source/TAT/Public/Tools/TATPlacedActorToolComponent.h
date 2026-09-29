// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATToolComponent.h"
#include "Tools/TATToolWorldActorConstraint.h"

#include "TATPlacedActorToolComponent.generated.h"

class UAnimMontage;

UCLASS()
class TAT_API UTATPlacedActorToolComponent : public UTATToolComponent
{
   GENERATED_BODY()
public:
   // From UTATToolComponent
   virtual bool AuthorityGetParametersForWorldActor(FGameplayTag usageTag, FTATGearWorldActorParameters& worldActorParams) const override;

   /// The actor to spawn in on placement, based on the usage type, that performs the effect
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Tool", Meta = (Categories = "Tool.Usage"))
   TMap<FGameplayTag, TSoftClassPtr<AActor>> WorldActorClasses;

   /// What animation to play when we place the actor
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Placement")
   UAnimMontage* PlaceMontage = nullptr;

   /// What is the maximum range we can place these actors at
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Placement")
   float MaxRange = 500.0f;

   /// What profile to trace the placement with
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Placement")
   FCollisionProfileName PlacementCollisionProfile;

   /// What constraints are there on where we can place our world actor
   /// If null, place anywhere within range
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Placement")
   TSubclassOf<UTATToolWorldActorConstraint> WorldActorConstraint;

protected:
   virtual bool OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool) override;
};


