// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"

#include "TATFXFunctionLibrary.generated.h"


UCLASS()
class TAT_API UTATFXFunctionLibrary : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()

public:
   // TODO: replace with engine version, since realms are gone
   UFUNCTION(BlueprintCallable, Category = Niagara, meta = (DisplayName= "Spawn System at Location (TAT)", Keywords = "niagara System", WorldContext = "worldContextObject", UnsafeDuringActorConstruction = "true"))
   static UNiagaraComponent* SpawnSystemAtLocationInRealm(const UObject* worldContextObject, class UNiagaraSystem* systemTemplate, FVector location, FRotator rotation = FRotator::ZeroRotator, FVector scale = FVector(1.f), bool autoDestroy = true, bool autoActivate = true, ENCPoolMethod poolingMethod = ENCPoolMethod::None, bool preCullCheck = true);

};
