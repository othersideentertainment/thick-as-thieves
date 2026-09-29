// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "UObject/Interface.h"

#include "TATKnockoutHandlerInterface.generated.h"

class AOSECharacterBase;

UENUM(BlueprintType)
enum class ETATKnockoutType : uint8
{
   /// Knocked out by another character (NPC or player)
   KnockedOutByOtherCharacter,

   /// Full knockout without a character source. Generally this happens to an unconscious player that takes additional damage.
   NonCharacterKnockout,

   /// Not fully knocked out, just unconscious (eg. from environmental damage)
   TemporarilyUnconscious,
};

UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools")
class UTATKnockoutHandlerInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface for any object that wants to provide knockout handling behavior
class TAT_API ITATKnockoutHandlerInterface
{
   GENERATED_BODY()

public:

   /// Handles a knockout event.
   /// If this returns true, prevent normal knockout behavior (eg. death) to allow this object to provide custom logic.
   /// If this returns false, normal knockout behavior will occur.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintNativeEvent, Category = "Knockout Handler Interface")
   bool OnCharacterKnockedOut(ETATKnockoutType knockoutType, ATATCharacter* knockedOutCharacter, AOSECharacterBase* otherCharacter);
   virtual bool OnCharacterKnockedOut_Implementation(ETATKnockoutType knockoutType, ATATCharacter* knockedOutCharacter, AOSECharacterBase* otherCharacter) { return false; }

};


