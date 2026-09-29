// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATUserWidget.h"

// ue5
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "TATCustomReticleWidget.generated.h"

// A stub base class for custom tools reticle widgets
UCLASS(BlueprintType, Abstract, Blueprintable, meta = (DisableNativeTick))
class TAT_API UTATCustomReticleWidgetBase : public UTATUserWidget
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TAT|Tools")
   void OnToolStateChanged(FGameplayTag State); // "State" named to match bp in reparent
};

