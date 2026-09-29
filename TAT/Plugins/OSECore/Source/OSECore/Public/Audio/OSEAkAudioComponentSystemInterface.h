// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AudioEnums.h"

#include "OSEAkAudioComponentSystemInterface.generated.h"

class UAkComponent;

//---------------------------------------------------------------------------------------
/// UOSEAkAudioComponentSystemInterface
/// 
/// This interface is to allow easy access to AkComponents on an actor. Supplying
/// GetAkComponent() provides instant access instead of using GetComponentByClass<>
/// which has to search for the component.
//---------------------------------------------------------------------------------------

UINTERFACE(BlueprintType, MinimalAPI, Category = "Audio|OSE")
class UOSEAkAudioComponentSystemInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IOSEAkAudioComponentSystemInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Audio|OSE")
   UAkComponent* GetAkComponent(EAkComponentType akComponentType) const;
};
