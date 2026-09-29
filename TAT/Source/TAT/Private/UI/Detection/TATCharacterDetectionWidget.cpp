// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Detection/TATCharacterDetectionWidget.h"

// tat
#include "AI/Detection/TATDetectionSettingsAsset.h"
#include "Developer/TATProjectSettings.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Tools/TATToolFunctionLibrary.h"

// ose
#include "OSELightDetectionInterface.h"
#include "Character/OSECharacterBase.h"
#include "Items/ToolSetComponent.h"
#include "Traversal/TraversalInterface.h"

// ue
#include "GameplayTagAssetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterDetectionWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATCharacterDetectionWidget, Log, All);

void UTATCharacterDetectionWidget::NativeTick(const FGeometry& myGeometry, float inDeltaTime)
{
   Super::NativeTick(myGeometry, inDeltaTime);

   const bool lightValueChanged = _TryUpdateLightDetectionValue();
   const bool settingsValueChanged = _TryUpdateDetectionSettingsValue();
   if (lightValueChanged || settingsValueChanged)
   {
      _TryUpdateTotalDetectionValue();
   }
}

void UTATCharacterDetectionWidget::_OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character)
{
   Super::_OnLocalCharacterIsReady_Implementation(character);

   if (UTATProjectSettings::ShouldUseLightDetection())
   {
      _lightDetectionInterface = TScriptInterface<IOSELightDetectionInterface>(character);
   }

   if (IsValid(DetectionAsset))
   {
      _traversalInterface = TScriptInterface<ITraversalInterface>(character);
      _gameplayTagInterface = TScriptInterface<IGameplayTagAssetInterface>(character);

      _toolSetComponent = UTATToolFunctionLibrary::GetToolSetComponentFromActor(character);
      if (_toolSetComponent != nullptr)
      {
         _toolSetComponent->OnEquippedToolChanged.AddUniqueDynamic(this, &UTATCharacterDetectionWidget::_OnEquippedToolChanged);
         _OnEquippedToolChanged();
      }
      else
      {
         UE_LOG(LogTATCharacterDetectionWidget, Warning, TEXT("Could not find toolset component on %s!")
            , (character != nullptr) ? *character->GetName() : TEXT("null character"));
      }
   }

   if (!(ShouldUpdateLightValue() || ShouldUpdateDetectionSettingsValue()))
   {
      // if we have no checks to perform, hide the widget
      SetVisibility(ESlateVisibility::Hidden);
   }
}

void UTATCharacterDetectionWidget::_OnEquippedToolChanged()
{
   if (_TryUpdateWeaponDetectionValue())
   {
      _TryUpdateTotalDetectionValue();
   }
}

bool UTATCharacterDetectionWidget::_TryUpdateLightDetectionValue()
{
   if (ShouldUpdateLightValue())
   {
      const float newActualLightIntensity = _lightDetectionInterface->GetActualLightIntensityFromLightSources();
      const float newLightDetectionValuePlusMinimum = _lightDetectionInterface->GetCurrentLightIntensityPlusMinimumValue();

      if (newActualLightIntensity != _actualLightDetectionValue || newLightDetectionValuePlusMinimum != _lightDetectionValuePlusMinimum)
      {
         _actualLightDetectionValue = newActualLightIntensity;
         _lightDetectionValuePlusMinimum = newLightDetectionValuePlusMinimum;
         OnLightDetectionValueChanged(GetLightDetectionValue());
         return true;
      }
   }

   return false;
}

bool UTATCharacterDetectionWidget::_TryUpdateDetectionSettingsValue()
{
   if (ShouldUpdateDetectionSettingsValue())
   {
      float newValue = 1.0f;
      if (_traversalInterface->IsCrouching())
      {
         if (const FTATDetectionRampSettings* rampSettings = DetectionAsset->DetectionSettings.RampSettings.Find(DetectionAssetAlertness))
         {
            newValue *= rampSettings->CrouchSettings.DetectionMultiplier;
         }
      }
      for (const FTATTagDetectionRampModifier& tagModifier : DetectionAsset->DetectionSettings.GameplayTagRampModifiers)
      {
         if (_gameplayTagInterface->HasMatchingGameplayTag(tagModifier.Tag))
         {
            newValue *= tagModifier.Multipliers.Get(DetectionAssetAlertness);
         }
      }
      if (newValue != _detectionSettingsValue)
      {
         _detectionSettingsValue = newValue;
         OnSettingsValueChanged(GetDetectionSettingsValue());
         return true;
      }
   }
   return false;
}

bool UTATCharacterDetectionWidget::_TryUpdateWeaponDetectionValue()
{
   if (ShouldUpdateDetectionSettingsWeaponValue())
   {
      if (const FTATDetectionRampSettings* rampSettings = DetectionAsset->DetectionSettings.RampSettings.Find(DetectionAssetAlertness))
      {
         float newValue = 1.0f;
         if (UTATItemFunctionLibrary::IsWeaponTool(_toolSetComponent->GetCurrentTool()))
         {
            newValue = rampSettings->WeaponEquippedDetectionMultiplier;
         }
         if (newValue != _detectionSettingsWeaponValue)
         {
            _detectionSettingsWeaponValue = newValue;

            const float fullValue = GetDetectionSettingsValue();
            OnSettingsValueChanged(fullValue);

            // this method isn't called on tick, so let's call
            // OnTotalDetectionValueChanged ourselves if the weapon value changes
            OnTotalDetectionValueChanged(GetTotalDetectionValue());
            return true;
         }
      }
   }
   return false;
}

bool UTATCharacterDetectionWidget::_TryUpdateTotalDetectionValue()
{
   const float newValue = GetTotalDetectionValue();
   if (newValue != _totalDetectionValue)
   {
      _totalDetectionValue = newValue;
      OnTotalDetectionValueChanged(_totalDetectionValue);
      return true;
   }
   return false;
}
