// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "TATEnhancedInputUserSettings.generated.h"

UCLASS()
class TAT_API UTATEnhancedInputUserSettings : public UEnhancedInputUserSettings
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintCallable, BlueprintPure)
   bool IsDirty() const;
   
   virtual void ApplySettings() override;
   
private:
   UFUNCTION(BlueprintCallable)
   void StoreInitialSettings();
   UFUNCTION(BlueprintCallable)
   void RestoreInitialSettings();
   
   
   TMap<FName, TMap<EPlayerMappableKeySlot, FKey>> _InitialKeyMappings;
};
