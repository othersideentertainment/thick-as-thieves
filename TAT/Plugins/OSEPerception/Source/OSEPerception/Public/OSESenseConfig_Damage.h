// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "OSESense.h"
#include "OSESenseConfig.h"
#include "OSESense_Damage.h"
#include "OSESenseConfig_Damage.generated.h"

UCLASS(meta = (DisplayName = "OSE Damage sense config"))
class OSEPERCEPTION_API UOSESenseConfig_Damage : public UOSESenseConfig
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
	TSubclassOf<UOSESense_Damage> Implementation;

	virtual TSubclassOf<UOSESense> GetSenseImplementation() const override;
};
