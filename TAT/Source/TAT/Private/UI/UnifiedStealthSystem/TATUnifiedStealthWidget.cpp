// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/UnifiedStealthSystem/TATUnifiedStealthWidget.h"

// tat
#include "AI/UnifiedStealthSystem/TATStealthScoreInterface.h"
#include "AI/UnifiedStealthSystem/TATUnifiedStealthSettings.h"

// ose
#include "Character/OSECharacterBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUnifiedStealthWidget)

UTATUnifiedStealthWidget::UTATUnifiedStealthWidget(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   ReceiveOnLocalCharacterIsReady = true;
}

void UTATUnifiedStealthWidget::_OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character)
{
   Super::_OnLocalCharacterIsReady_Implementation(character);

   if(const ITATStealthScoreInterface* stealthScoreInterface = Cast<ITATStealthScoreInterface>(character))
   {
      _TrackedComponent = stealthScoreInterface->GetStealthScoreComponent();
      _TrackedComponent->OnOwnHearingStimEvent.AddUObject(this, &UTATUnifiedStealthWidget::_HandleOwnStim);
      
      const float stealthScore = _TrackedComponent->GetStealthDetectionScore();
      const UTATUnifiedStealthSettings& settings = UTATUnifiedStealthSettings::Get();
      _shouldShowFireflies = stealthScore > settings.FireFlyThreshold;
      _OnShowFirefliesToggled(_shouldShowFireflies);
   }
}

void UTATUnifiedStealthWidget::NativeConstruct()
{
   Super::NativeConstruct();
   _MaxTotalStealthScore = 1.F;
}

void UTATUnifiedStealthWidget::_UpdateStealthValue_Implementation(float val)
{
}

void UTATUnifiedStealthWidget::_OnShowFirefliesToggled_Implementation(bool bNewEnabled)
{
}

void UTATUnifiedStealthWidget::_HandleOwnStim_Implementation(EStimSeverity stimSeverity, float range)
{
   
}

void UTATUnifiedStealthWidget::NativeTick(const FGeometry& myGeometry, const float inDeltaTime)
{
   Super::NativeTick(myGeometry, inDeltaTime);
   if(_TrackedComponent.IsValid())
   {
      const float stealthScore = _TrackedComponent->GetStealthDetectionScore();
      _UpdateStealthValue(stealthScore);

      const UTATUnifiedStealthSettings& settings = UTATUnifiedStealthSettings::Get();
      const bool newShouldShowFireflies = stealthScore > settings.FireFlyThreshold;
      if (newShouldShowFireflies != _shouldShowFireflies)
      {
         _shouldShowFireflies = newShouldShowFireflies;
         _OnShowFirefliesToggled(_shouldShowFireflies);
      }
   }
}
