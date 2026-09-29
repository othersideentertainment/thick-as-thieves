// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Components/ActorComponent.h"
#include "OSESense.h"

#include "OSEPerceptionStimuliSourceComponent.generated.h"

/** Gives owning actor a way to auto-register as perception system's sense stimuli source */
UCLASS(ClassGroup = AI, HideCategories = (Activation, Collision), meta = (BlueprintSpawnableComponent), config = Game)
class OSEPERCEPTION_API UOSEPerceptionStimuliSourceComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "OSE Perception", BlueprintReadOnly, config)
	uint32 bAutoRegisterAsSource : 1;

	uint32 bSuccessfullyRegistered : 1;

	UPROPERTY(EditAnywhere, Category = "OSE Perception", BlueprintReadOnly)
	TArray<TSubclassOf<UOSESense> > RegisterAsSourceForSenses;

	virtual void OnRegister() override;
public:

	UOSEPerceptionStimuliSourceComponent(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	/** Registers owning actor as source of stimuli for senses specified in RegisterAsSourceForSenses. 
	 *	Note that you don't have to do it if bAutoRegisterAsSource == true */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void RegisterWithPerceptionSystem();

	/** Registers owning actor as source for specified sense class */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void RegisterForSense(TSubclassOf<UOSESense> SenseClass);

	/** Unregister owning actor from being a source of sense stimuli */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void UnregisterFromPerceptionSystem();

	/** Unregisters owning actor from sources list of a specified sense class */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void UnregisterFromSense(TSubclassOf<UOSESense> SenseClass);
};
