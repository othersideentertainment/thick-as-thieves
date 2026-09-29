// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "TATStateTreeDistractionComponent.generated.h"


class ATATAIController;
class UTATStateTreeAIComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATStateTreeDistractionComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATStateTreeDistractionComponent();
   void StartDelayTimer();

   UFUNCTION(BlueprintCallable)
   void SetDistractableState(bool allowDistractions);

protected:
   
   virtual void BeginPlay() override;
   void StopDelayTimer();
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   
   void OnDistractionPossible();
   void OnDistractionEQSResult(TSharedPtr<FEnvQueryResult> envQueryResult);

   UPROPERTY(EditDefaultsOnly, Category="Tuning")
   float _DistractionDelayVariance { 1.f };
   UPROPERTY(EditDefaultsOnly, Category="Tuning")
   float _DistractionDelay { 10.f };

   FTimerHandle _DistractionHandle;

   UPROPERTY(EditDefaultsOnly, Category="Tuning")
   TObjectPtr<UEnvQuery> _QueryTemplate { nullptr };
   
   UPROPERTY(EditAnywhere, Category="Tuning")
   TEnumAsByte<EEnvQueryRunMode::Type> _EQSRunMode = EEnvQueryRunMode::SingleResult;
   
   int32 _InProgressRequestID { INDEX_NONE };
   bool _IsDistractionAllowed { false };

private:
   
   UPROPERTY(Transient)
   UTATStateTreeAIComponent* _StateTreeAIComponent { nullptr };
   UPROPERTY(Transient)
   ATATAIController* _TATAIController { nullptr };
};
