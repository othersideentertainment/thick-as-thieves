// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TATSignificanceWorldSubsystem.generated.h"

UCLASS()
class TAT_API UTATSignificanceWorldSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override;
};
