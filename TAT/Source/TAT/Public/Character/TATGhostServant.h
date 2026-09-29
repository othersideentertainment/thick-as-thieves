// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATCharacterAIBase.h"

// ue
#include "CoreMinimal.h"

#include "TATGhostServant.generated.h"

UCLASS(Blueprintable)
class TAT_API ATATGhostServant : public ATATCharacterAIBase
{
   GENERATED_BODY()

public:
   ATATGhostServant();

protected:
   virtual void BeginPlay() override;

   virtual void Tick(float DeltaSeconds) override;
   
   UFUNCTION(BlueprintNativeEvent)
   void _OnCreatorDied();

   UPROPERTY(EditAnywhere, Category="TAT|Behavior", meta=(Units="cm"))
   float _MaxDistanceFromCreator { 1000.f };
   
   bool bTriggeredLeftRangeOfCreator { false };
   UFUNCTION(BlueprintNativeEvent)
   void _OnLeftRangeOfCreator();
};
