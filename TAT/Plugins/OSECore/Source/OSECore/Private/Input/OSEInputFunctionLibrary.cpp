// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/OSEInputFunctionLibrary.h"

// ose
#include "OSECommon.h"

// ue4
#include "EnhancedPlayerInput.h"
#include "InputMappingContext.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEInputFunctionLibrary)

namespace
{
   bool TEMP_KeysToDisplayName(const FKey& key, FText& outText)
   {
      // HACK HACK HACK: This is useful for PC development but these need to become icons for real platform / gamepad development

      if (key == EKeys::Gamepad_FaceButton_Bottom)
      {
         outText = FText::FromString(TEXT("A"));
         return true;
      }
      else if (key == EKeys::Gamepad_FaceButton_Right)
      {
         outText = FText::FromString(TEXT("B"));
         return true;
      }
      else if (key == EKeys::Gamepad_FaceButton_Left)
      {
         outText = FText::FromString(TEXT("X"));
         return true;
      }
      else if (key == EKeys::Gamepad_FaceButton_Top)
      {
         outText = FText::FromString(TEXT("Y"));
         return true;
      }
      else if (key == EKeys::Gamepad_LeftShoulder)
      {
         outText = FText::FromString(TEXT("LB"));
         return true;
      }
      else if (key == EKeys::Gamepad_RightShoulder)
      {
         outText = FText::FromString(TEXT("RB"));
         return true;
      }
      else if (key == EKeys::Gamepad_LeftTrigger)
      {
         outText = FText::FromString(TEXT("LT"));
         return true;
      }
      else if (key == EKeys::Gamepad_RightTrigger)
      {
         outText = FText::FromString(TEXT("RT"));
         return true;
      }
      else if (key == EKeys::Gamepad_DPad_Up)
      {
         outText = FText::FromString(TEXT("D-Up"));
         return true;
      }
      else if (key == EKeys::Gamepad_DPad_Down)
      {
         outText = FText::FromString(TEXT("D-Down"));
         return true;
      }
      else if (key == EKeys::Gamepad_DPad_Right)
      {
         outText = FText::FromString(TEXT("D-Right"));
         return true;
      }
      else if (key == EKeys::Gamepad_DPad_Left)
      {
         outText = FText::FromString(TEXT("D-Left"));
         return true;
      }
      else if (key == EKeys::LeftMouseButton)
      {
         outText = FText::FromString(TEXT("LMB"));
         return true;
      }
      else if (key == EKeys::RightMouseButton)
      {
         outText = FText::FromString(TEXT("RMB"));
         return true;
      }
      else if (key == EKeys::MiddleMouseButton)
      {
         outText = FText::FromString(TEXT("MMB"));
         return true;
      }
      else if (key == EKeys::MouseWheelAxis)
      {
         outText = FText::FromString(TEXT("Mouse Wheel"));
         return true;
      }
      return false;
   }
}

FText UOSEInputFunctionLibrary::GetDisplayNameForKey(const FKey& key, bool longDisplayName)
{
   FText tempKeyDisplayName;
   if (TEMP_KeysToDisplayName(key, tempKeyDisplayName))
   {
      return tempKeyDisplayName;
   }
   else if (key.IsValid())
   {
      return key.GetDisplayName(longDisplayName);
   }
   return FText::GetEmpty();
}

FKey UOSEInputFunctionLibrary::GetKeyForActionMappingName(const FString& actionMappingName, EOSEInputHardwareType inputHardwareType)
{
   UInputSettings* settings = UInputSettings::GetInputSettings();
   check(settings);

   TArray<FInputActionKeyMapping> mappings;
   settings->GetActionMappingByName(*actionMappingName, mappings);

   // TODO: We may have multiple bindings for a given action, how can we make one of these the "default" that the UI calling this wants to show?
   for (const FInputActionKeyMapping& actionMapping : mappings)
   {
      const FKey& key = actionMapping.Key;
      if (key.IsGamepadKey() && inputHardwareType == EOSEInputHardwareType::Gamepad)
         return key;
      else if (!key.IsGamepadKey() && inputHardwareType == EOSEInputHardwareType::KeyboardMouse)
         return key;
   }

   return FKey();
}

FText UOSEInputFunctionLibrary::GetKeyTextForActionMappingName(const FString& actionMappingName, EOSEInputHardwareType inputHardwareType, bool longDisplayName)
{
   FKey key = GetKeyForActionMappingName(actionMappingName, inputHardwareType);
   return GetDisplayNameForKey(key, longDisplayName);
}

FKey UOSEInputFunctionLibrary::GetKeyForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType)
{
   APlayerController* localController = UOSECommon::GetLocalPlayerController<APlayerController>(worldContextObject);

   if (localController && inputAction)
   {
      if (UEnhancedPlayerInput* playerInput = Cast<UEnhancedPlayerInput>(localController->PlayerInput))
      {
         for (const FEnhancedActionKeyMapping& mapping : playerInput->GetEnhancedActionMappings())
         {
            if (mapping.Action == inputAction && (mapping.Key.IsGamepadKey() == (EOSEInputHardwareType::Gamepad == inputHardwareType)))
            {
               return mapping.Key;
            }
         }
      }
   }
   return FKey();
}

FKey UOSEInputFunctionLibrary::GetKeyAndChordForInputAction(UObject* worldContextObject, const UInputAction* inputAction,
   EOSEInputHardwareType inputHardwareType, FKey& outChordedKey)
{
   APlayerController* localController = UOSECommon::GetLocalPlayerController<APlayerController>(worldContextObject);

   outChordedKey = FKey();
   if (localController && inputAction)
   {
      if (UEnhancedPlayerInput* playerInput = Cast<UEnhancedPlayerInput>(localController->PlayerInput))
      {
         for (const FEnhancedActionKeyMapping& mapping : playerInput->GetEnhancedActionMappings())
         {
            if (mapping.Action == inputAction && (mapping.Key.IsGamepadKey() == (EOSEInputHardwareType::Gamepad == inputHardwareType)))
            {
               for (UInputTrigger* inputTrigger : mapping.Triggers)
               {
                  // NB: exact cast, to avoid including chord blocker subclass
                  if (UInputTriggerChordAction* chord = ExactCast<UInputTriggerChordAction>(inputTrigger))
                  {
                     if (chord->ChordAction)
                     {
                        outChordedKey = GetKeyForInputAction(worldContextObject, chord->ChordAction, inputHardwareType);
                     }
                  }
               }
               
               return mapping.Key;
            }
         }
      }
   }
   return FKey();
}

TArray<FOSEKeyAndChordPair> UOSEInputFunctionLibrary::GetKeysForInputAction(
   UObject* worldContextObject,
   const UInputAction* inputAction,
   const EOSEInputHardwareType inputHardwareType)
{
   TArray<FOSEKeyAndChordPair> outKeys;
   const APlayerController* localController = UOSECommon::GetLocalPlayerController<APlayerController>(worldContextObject);
   if (localController && inputAction)
   {
      if (const UEnhancedPlayerInput* playerInput = Cast<UEnhancedPlayerInput>(localController->PlayerInput))
      {
         for (const FEnhancedActionKeyMapping& mapping : playerInput->GetEnhancedActionMappings())
         {
            if (mapping.Action == inputAction && (mapping.Key.IsGamepadKey() == (EOSEInputHardwareType::Gamepad == inputHardwareType)))
            {
               FOSEKeyAndChordPair data;
               data.Key = mapping.Key;
               data.Chord = FKey();
               for (UInputTrigger* inputTrigger : mapping.Triggers)
               {
                  // NB: exact cast, to avoid including chord blocker subclass
                  if (const UInputTriggerChordAction* chord = ExactCast<UInputTriggerChordAction>(inputTrigger))
                  {
                     if (chord->ChordAction)
                     {
                        data.Chord = GetKeyForInputAction(worldContextObject, chord->ChordAction, inputHardwareType);
                     }
                  }
               }
               outKeys.Add(data);
            }
         }
      }
   }
   return outKeys;
}

FText UOSEInputFunctionLibrary::GetKeyTextForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType, bool longDisplayName /* false */)
{
   FKey key = GetKeyForInputAction(worldContextObject, inputAction, inputHardwareType);
   return GetDisplayNameForKey(key, longDisplayName);
}

FKey UOSEInputFunctionLibrary::GetChordedKeyForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType)
{
   FKey choordedKey;
   GetKeyAndChordForInputAction(worldContextObject, inputAction, inputHardwareType, choordedKey);
   return choordedKey;
}

FText UOSEInputFunctionLibrary::GetChordedKeyTextForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType, bool longDisplayName /* false */)
{
   FKey key = GetChordedKeyForInputAction(worldContextObject, inputAction, inputHardwareType);
   return GetDisplayNameForKey(key, longDisplayName);
}

