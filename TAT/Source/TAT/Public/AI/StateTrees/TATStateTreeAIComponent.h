// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Components/StateTreeAIComponent.h"

#include "TATStateTreeAIComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATStateTreeAIComponent : public UStateTreeAIComponent
{
   GENERATED_BODY()

public:
   UTATStateTreeAIComponent(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   virtual bool SetContextRequirements(FStateTreeExecutionContext& context, bool bLogErrors) override;
private:
   
   UPROPERTY(EditDefaultsOnly)
   bool _shouldStartStateTreeIfPawnSet { true };
   
   UFUNCTION()
   void _OnPawnChanged(APawn* oldPawn, APawn* newPawn);

};
