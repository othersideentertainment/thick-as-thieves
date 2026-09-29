// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

#include "TATWorldAudio.generated.h"

UCLASS(Blueprintable, BlueprintType, Within=WorldSubsystem)
class TAT_API UTATWorldAudio : public UObject
{
   GENERATED_BODY()

public:
   void OnNativeWorldBeginPlay(UWorld* world);

   
   UFUNCTION(BlueprintImplementableEvent, Category = "Audio|TAT")
   void CleanUp();

protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "Audio|TAT")
   void OnWorldBeginPlay(UWorld* world);


   virtual UWorld* GetWorld() const override { return _world.Get(); }

private:
   TWeakObjectPtr<UWorld> _world;
};
