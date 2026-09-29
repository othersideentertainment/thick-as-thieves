// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "SaveGame/TATCharacterSaveId.h"

// ue
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATSaveProxySubsystem.generated.h"

class UTATCharacterProgressionViewModel;

// Could also be game instance subsystem, but accessors currently require world (but only to get instance)
UCLASS()
class TAT_API UTATSaveProxySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
   // from USubsystem
   virtual void Deinitialize() override;

   // TODO: should this expose the specified character, or simply the active one?
   UFUNCTION(BlueprintPure, BlueprintCosmetic)
   UTATCharacterProgressionViewModel* GetCharacterProgressionProxy(FTATCharacterSaveId character);

private:
   UPROPERTY(Transient)
   TMap<FTATCharacterSaveId, TObjectPtr<UTATCharacterProgressionViewModel>> _characterProgressionProxies;
};
