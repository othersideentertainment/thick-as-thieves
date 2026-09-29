// Copyright Epic Games, Inc. All Rights Reserved.
// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

//UE
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "OSESoundSourceComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class OSEPERCEPTION_API UOSESoundSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOSESoundSourceComponent();


   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   bool VelocityDriven = false;
   
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float MinimumSpeedTrigger = 0.0f;


   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   float BaseStimulusStrength = 1.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float StimulusStrengthScalar = 1.0f;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FGameplayTagContainer StimulusTag;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
   void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   void RegisterSense();
   void UnregisterSense();

   bool bSuccessfullyRegistered = false;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

 

};
