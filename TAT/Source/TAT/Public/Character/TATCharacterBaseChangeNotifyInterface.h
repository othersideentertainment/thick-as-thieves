// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "UObject/Interface.h"

#include "TATCharacterBaseChangeNotifyInterface.generated.h"

UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools")
class UTATCharacterBaseChangeNotifyInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface for any actor that wants to get a notify when a character starts basing on it
class TAT_API ITATCharacterBaseChangeNotifyInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintNativeEvent, Category = "Character Base Change Notify Interface")
   void OnCharacterBeginBasing(ACharacter* character);
   virtual void OnCharacterBeginBasing_Implementation(ACharacter* character) {}

};
