// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATGameplayBehavior_AnimationBased.h"
#include "Character/TATCharacterAIBase.h"
#include "Loot/TATLootActor.h"

// ue
#include "CoreMinimal.h"

#include "TATGameplayBehavior_CollectMajorLoot.generated.h"

UCLASS()
class TAT_API UTATGameplayBehavior_CollectMajorLoot : public UTATGameplayBehavior_AnimationBased
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   void CollectMajorLoot(ATATLootActor* lootActor, TScriptInterface<ITATLootInventoryInterface> lootInventoryInterface);
};
