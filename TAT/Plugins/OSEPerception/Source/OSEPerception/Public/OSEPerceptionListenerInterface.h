// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"
#include "OSEPerceptionListenerInterface.generated.h"

class UOSEPerceptionComponent;

UINTERFACE()
class OSEPERCEPTION_API UOSEPerceptionListenerInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class OSEPERCEPTION_API IOSEPerceptionListenerInterface
{
	GENERATED_IINTERFACE_BODY()

	virtual UOSEPerceptionComponent* GetOSEPerceptionComponent() { return nullptr; }
};

