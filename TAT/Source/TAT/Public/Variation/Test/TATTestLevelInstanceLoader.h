// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TATTestLevelInstanceLoader.generated.h"

class ULevelStreamingDynamic;


// A test proof of concept of dynamically loading a randomly selected) level instance
//
// As _a_ possible approach for coarse map variation
//
// Not how this would actually be structured workflow-wise
UCLASS()
class TAT_API ATATTestLevelInstanceLoader : public AActor
{
   GENERATED_BODY()
   
public:	
   // Sets default values for this actor's properties
   ATATTestLevelInstanceLoader();

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

public:
   // A unique string to disambiguate levels. Not how this would be done for real
   UPROPERTY(EditAnywhere)
   FString LevelSuffix;

   UPROPERTY(EditAnywhere)
   TArray<TSoftObjectPtr<UWorld>> PossibleLevels;

private:
   UFUNCTION()
   void _OnRep_LevelToLoad();

   void _TryLoadInstance();

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_LevelToLoad)
   TSoftObjectPtr<UWorld> _levelToLoad;

   UPROPERTY(Transient)
   TObjectPtr<ULevelStreamingDynamic> _streamingLevel;
};
