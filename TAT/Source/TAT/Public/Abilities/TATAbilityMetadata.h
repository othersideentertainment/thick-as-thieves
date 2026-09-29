// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityInfo.h"

// tat

#include "TATAbilityMetadata.generated.h"

class UAkAudioEvent;


//---------------------------------------------------------------------------------------------------
/// Designated place to store generic ability SFX (so abilities can assign their own custom SFX 
/// for stuff like cooldowns).
//---------------------------------------------------------------------------------------------------
UCLASS(BlueprintType, EditInlineNew)
class TAT_API UTATAbilitySFXMetadata : public UOSEAbilityMetadata
{
   GENERATED_BODY()

public:
   // SFX played when ability cooldown is finished
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   UAkAudioEvent* CooldownFinishedSFX = nullptr;
};
