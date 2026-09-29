// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTags.h"
#include "UObject/Interface.h"

#include "TATAstralProjectionProximityTargetInterface.generated.h"

UINTERFACE(Blueprintable, MinimalAPI, Category = "Interactable")
class UTATAstralProjectionProximityTargetInterface : public UInterface
{
   GENERATED_BODY()
};

/// If an actor wants to be affected by proximity to astral projection
class TAT_API ITATAstralProjectionProximityTargetInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintNativeEvent, Category = "Astral Projection")
   void AuthorityOnEnterProximity(AActor* sourceActor);

   UFUNCTION(BlueprintNativeEvent, Category = "Astral Projection")
   void AuthorityOnExitProximity(AActor* sourceActor);

   UFUNCTION(BlueprintNativeEvent, Category = "Astral Projection")
   FGameplayTag GetProximityTargetType() const;
};


