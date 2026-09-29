// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once 

#include "CoreMinimal.h"

// ose

// ue4
#include "GameFramework/HUD.h"

#include "OSEHUD.generated.h"

UCLASS()
class OSECORE_API AOSEHUD : public AHUD
{
   GENERATED_BODY()

public:
   AOSEHUD();
   
   // Primary draw call for the HUD
   virtual void DrawHUD() override;
};
