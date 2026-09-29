// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

// tat
#include "AI/UnifiedStealthSystem/TATStealthScoreComponent.h"
#include "AI/UnifiedStealthSystem/TATStealthScoreInterface.h"
#include "UI/TATUserWidget.h"

#include "TATUnifiedStealthWidget.generated.h"

enum class EStimSeverity : uint8;
class UProgressBar;
class UDynamicEntryBox;
UCLASS(Abstract)
class TAT_API UTATUnifiedStealthWidget : public UTATUserWidget
{
   GENERATED_BODY()
public:
   UTATUnifiedStealthWidget(const FObjectInitializer& objectInitializer);
protected:
   virtual void _OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character) override;
   virtual void NativeConstruct() override;
   
   UFUNCTION(BlueprintNativeEvent)
   void _UpdateStealthValue(float val);

   UFUNCTION(BlueprintNativeEvent)
   void _OnShowFirefliesToggled(bool bNewEnabled);
   
   UFUNCTION(BlueprintNativeEvent)
   void _HandleOwnStim(EStimSeverity stimSeverity, float range);
   
   virtual void NativeTick(const FGeometry& myGeometry, float inDeltaTime) override;
private:
   
   UPROPERTY(Transient)
   TWeakObjectPtr<UTATStealthScoreComponent> _TrackedComponent { nullptr };
   
   int _MaxTotalStealthScore { 0 };
   bool _shouldShowFireflies = false;
};
