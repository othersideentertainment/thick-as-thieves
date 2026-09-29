// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATDevToolTypes.generated.h"

// This should get defined in TAT.Build.cs, but just in case, make sure it always has a value
#if !defined(TAT_ENABLE_DEV_TOOLS)
#define TAT_ENABLE_DEV_TOOLS 0
#endif

struct FTATDevToolState;

/// Dev tool menu screen anchor point
UENUM()
enum class ETATDevMenuPos : uint8
{
   Custom = 0,
   TopLeft,
   TopCenter,
   TopRight,
   BottomLeft,
   BottomCenter,
   BottomRight,
};

/// General dev tool UI settings
USTRUCT()
struct FTATDevToolSubsystemSettings
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   ETATDevMenuPos MenuPos = ETATDevMenuPos::TopCenter;

   UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "MenuPos == ETATDevMenuPos::Custom"))
   FVector2D CustomMenuPos = FVector2D::ZeroVector;

   UPROPERTY(EditDefaultsOnly)
   TArray<FString> DefaultVisibleWindows;

   UPROPERTY(EditDefaultsOnly)
   TArray<TSoftClassPtr<UObject>> DefaultVisibleClasses;

   UPROPERTY(EditDefaultsOnly)
   bool ShowImGuiDemoTool = false;

};

