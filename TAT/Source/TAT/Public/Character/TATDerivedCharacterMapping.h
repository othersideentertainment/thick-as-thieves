// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Engine/DataAsset.h"

#include "TATDerivedCharacterMapping.generated.h"

class ATATCharacter;

// A data asset that maps from player character to actor subclass
//
// The intent is to use this to look up the subclass for the character being "doubled".
// 
// NOTE: There may, in future, be long-term maintenance improvements from having the tulpa
//       procedurally mirror the relevant qualities of the source character at runtime, but
//       in the intermediate term, this creates more complexity and testing burden as the
//       qualities to be copied are in flux. But this may be something to revisit in future,
//       especially as there is greater visual customization of players.
UCLASS(BlueprintType)
class TAT_API UTATDerivedCharacterMapping : public UDataAsset
{
	GENERATED_BODY()
	
   UFUNCTION(BlueprintPure)
   TSoftClassPtr<AActor> FindDerivedClassForActor(const AActor* actorToDouble) const;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

protected:
   UPROPERTY(EditDefaultsOnly)
   TMap<TSoftClassPtr<ATATCharacter>, TSoftClassPtr<AActor>> _classesByCharacterType;

   UPROPERTY(EditDefaultsOnly)
   TSoftClassPtr<AActor> _fallbackClass;

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<AActor> _baseClass;
#endif
};
