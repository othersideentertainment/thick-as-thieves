// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

#include "OSESearchNode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSearchNode, Log, All);

UCLASS(Blueprintable, HideCategories = (Lighting, LightColor, Force, Collision, Rendering, Replication, Input, LOD, Cooking, Actor, "Actor Tick", HLOD, Streaming), AutoExpandCategories = ("AI|OSE|SearchNode"))
class OSEAI_API AOSESearchNode : public AActor, public IUtilityAITokenOwnerInterface
{
   GENERATED_BODY()

public:
   AOSESearchNode();

   virtual void BeginPlay() override;

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|SearchNode")
   void BeginCooldown();

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|SearchNode")
   bool IsCoolingDown();

   // from IUtilityAITokenOwnerInterface
   virtual UUtilityAITokenOwner* AuthorityGetTokenOwner_Implementation() const override { check(HasAuthority()); return _tokenOwner; }

private:
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|SearchNode")
   FOSEAITokenInfo _searchNodeToken;

   UPROPERTY(EditAnywhere, Category = "AI|OSE|SearchNode")
   float _cooldownInSeconds = 3.0f;

   float _cooldownStartTime = 0.0f;

   UPROPERTY(Transient)
   UUtilityAITokenOwner* _tokenOwner = nullptr;
};
