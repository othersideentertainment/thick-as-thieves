// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "OSESense.h"
#include "OSESenseConfig.h"
#include "OSESenseConfig_Touch.generated.h"

UCLASS(meta = (DisplayName = "OSE Touch config"))
class OSEPERCEPTION_API UOSESenseConfig_Touch : public UOSESenseConfig
{
	GENERATED_UCLASS_BODY()
public:	
	virtual TSubclassOf<UOSESense> GetSenseImplementation() const override;
};
