// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "UObject/SoftObjectPtr.h"

// tat
#include "UI/TATNavigationConfig.h"

#include "TATUIDeveloperSettings.generated.h"

class UInputAction;
class UTATLayoutWidget;
class UTATUIColorsData;
class UAkStateValue;

UENUM(BlueprintType)
enum class EPressAndHoldContext : uint8
{
   HoldDurationDestructive = 0,
   HoldDurationConfirmation,
   HoldDurationShort,
   HoldDurationInteract
};

USTRUCT()
struct FExtendedNavigationActionBindings
{
   GENERATED_BODY()

   UPROPERTY(Config, EditAnywhere)
   TSoftObjectPtr<UInputAction> InputAction;
   UPROPERTY(Config, EditAnywhere)
   TArray<FKey> KeyBindings;
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] UI Settings"))
class TAT_API UTATUIDeveloperSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UTATUIDeveloperSettings() {}

   UFUNCTION(BlueprintPure, Category = "TAT UI Settings")
   static UTATUIDeveloperSettings* GetTATUISettings() { return GetMutableDefault<UTATUIDeveloperSettings>(); }

   UPROPERTY(Config, EditAnywhere, Category = "TAT UI Settings")
   TSoftClassPtr<UTATLayoutWidget> LayoutWidgetClass;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "TAT UI Settings|Colors")
   TSoftObjectPtr<UTATUIColorsData> UIColorsData;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, EditFixedSize, Category = "TAT UI Settings")
   TMap<EPressAndHoldContext, float> PressAndHoldDurationSettings;

   UPROPERTY(Config, EditAnywhere)
   TMap<EUIExtendedNavigationAction, struct FExtendedNavigationActionBindings> ExtendedNavigationBindings;

   // Default state to apply while at least 1 screen is on the stack (can be overridden via UTATScreenWidget::AkStateWhileOnTopOfStackOverride)
   UPROPERTY(Config, EditAnywhere, Category = "TAT UI Settings|Audio")
   TSoftObjectPtr<UAkStateValue> AkStateWhileStackPopulated;

   // Default state when no screens are on the stack
   UPROPERTY(Config, EditAnywhere, Category = "TAT UI Settings|Audio")
   TSoftObjectPtr<UAkStateValue> AkStateWhileStackEmpty;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "TAT UI Settings|Audio")
   float numberLerpSingleSpeedMaxValue = 1.0f;
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "TAT UI Settings|Audio")
   float numberLerpSlowSpeedMaxValue = 20.0f;
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "TAT UI Settings|Audio")
   float numberLerpMediumSpeedMaxValue = 100.0f;
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "TAT UI Settings|Audio")
   float numberLerpFastSpeedMaxValue = 1000.0f;
};
