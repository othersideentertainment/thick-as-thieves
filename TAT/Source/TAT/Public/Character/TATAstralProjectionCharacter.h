// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Character/OSECharacterBase.h"

// ue5
#include "GameplayTagContainer.h"

#include "TATAstralProjectionCharacter.generated.h"


UCLASS()
class ATATAstralProjectionCharacter : public AOSECharacterBase
{
   GENERATED_BODY()
public:
   ATATAstralProjectionCharacter();

   // from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;
private:
   UFUNCTION()
   void _OnCharacterReady(AOSECharacterBase* character);

   // tag for ability to activate when the character is ready
   UPROPERTY(EditDefaultsOnly, Category=Respawn)
   FGameplayTag _readyAbilityTag;
};
