// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"

#include "TATInstigatorEffectComponent.generated.h"


class UGameplayEffect;

// A component that applies a gameplay effect to the instigator with the owner in the effect context
// removing it on destroy
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UTATInstigatorEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTATInstigatorEffectComponent();

protected:
	// Called when the game starts
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:

   UPROPERTY(EditDefaultsOnly, Category = Effect)
   TSubclassOf<UGameplayEffect> _instigatorEffect;

   FActiveGameplayEffectHandle _authorityInstigatorEffect;
		
};
