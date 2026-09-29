// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// wwise
#include "AkComponent.h"

#include "TATAutoDetachAkComponent.generated.h"


// A subclass of AkComponent that can automatically detach from its parent when not playing audio
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATAutoDetachAkComponent : public UAkComponent
{
   GENERATED_BODY()

public:
   UTATAutoDetachAkComponent();

   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

protected:
   void _UpdateAutoDetach();
   
   // If true, will automatically detach from its parent if not playing audio
   // This can reduce cost of moving the component it is attached to
   UPROPERTY(EditDefaultsOnly, Category=AutoDetach)
   bool _autoDetachWhenNotPlaying = false;

   TWeakObjectPtr<USceneComponent> _previousAttachComponent;
   FName _previousSocket;
};
