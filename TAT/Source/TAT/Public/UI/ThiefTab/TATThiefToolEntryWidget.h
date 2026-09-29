// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATUserWidget.h"

#include "TATThiefToolEntryWidget.generated.h"

class UTATToolComponent;

UCLASS(Blueprintable, BlueprintType, meta = (DisableNativeTick))
class TAT_API UTATThiefToolEntryWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   void SetTool(const UToolComponent* toolComponent);

   UFUNCTION(BlueprintPure, Category = "TAT|Gear")
   const UToolComponent* GetTool() const { return _toolComponent; }

   // Returns true if the tool uses ammo (along with the current/max ammo counts), false otherwise
   UFUNCTION(BlueprintPure, Category = "TAT|Gear")
   bool GetAmmo(int& currentAmmo, int& maxAmmo) const;

private:
   UPROPERTY(Transient)
   const UToolComponent* _toolComponent = nullptr;
};
