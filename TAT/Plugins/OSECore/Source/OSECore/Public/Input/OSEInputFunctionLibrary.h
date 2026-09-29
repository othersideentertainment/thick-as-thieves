// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEInputFunctionLibrary.generated.h"

class UInputAction;

UENUM(BlueprintType)
enum class EOSEInputHardwareType : uint8
{
   KeyboardMouse,
   Gamepad
};

USTRUCT(BlueprintType)
struct FOSEKeyAndChordPair
{
   GENERATED_BODY()
   
   UPROPERTY(BlueprintReadOnly)
   FKey Key;
   UPROPERTY(BlueprintReadOnly)
   FKey Chord;
};

UCLASS()
class OSECORE_API UOSEInputFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   //---------------------------------------------------------------------------------------
   /// Original Input
   //---------------------------------------------------------------------------------------

   /// Get a key from an original input action mapping name, useful for UI mappings that don't use enhanced input
   UFUNCTION(BlueprintPure, Category = "Input")
   static FKey GetKeyForActionMappingName(const FString& actionMappingName, EOSEInputHardwareType inputHardwareType);
   /// Get key FText from an original input action mapping name, useful for UI mappings that don't use enhanced input
   UFUNCTION(BlueprintPure, Category = "Input")
   static FText GetKeyTextForActionMappingName(const FString& actionMappingName, EOSEInputHardwareType inputHardwareType, bool longDisplayName = false);

   //---------------------------------------------------------------------------------------
   /// Enhanced Input
   //---------------------------------------------------------------------------------------

   /// Returns the input context priority blueprints should use to apply a new input context
   UFUNCTION(BlueprintPure, Category = "Input|Enhanced")
   static int32 GetInputContextPriority(EOSEEnhancedInputContextPriority priority) { return static_cast<int32>(priority); }

   /// Get a key from an enhanced input action mapping asset, useful for showing the key associated with an action in UI
   UFUNCTION(BlueprintPure, Category = "Input|Enhanced", meta = (WorldContext = "worldContextObject"))
   static FKey GetKeyForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType);
   UFUNCTION(BlueprintPure, Category = "Input|Enhanced", meta = (WorldContext = "worldContextObject"))
   static FKey GetKeyAndChordForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType, FKey& outChordedKey);
   
   UFUNCTION(BlueprintPure, Category = "Input|Enhanced", meta = (WorldContext = "worldContextObject"))
   static TArray<FOSEKeyAndChordPair> GetKeysForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType);   
   
   /// Get key text from an enhanced input action mapping asset, useful for showing the key associated with an action in UI
   UFUNCTION(BlueprintPure, Category = "Input|Enhanced", meta = (WorldContext = "worldContextObject"))
   static FText GetKeyTextForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType, bool longDisplayName = false);

   /// Get the chorded key from a chorded input action, useful for showing the key associated with an action in UI   
   // TODO: deprecate and replace with GetKeyAndChordForInputAction
   UFUNCTION(BlueprintPure, Category = "Input|Enhanced", meta = (WorldContext = "worldContextObject"))
   static FKey GetChordedKeyForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType);
   /// Get key text from an enhanced input action mapping asset, useful for showing the key associated with an action in UI
   UFUNCTION(BlueprintPure, Category = "Input|Enhanced", meta = (WorldContext = "worldContextObject"))
   static FText GetChordedKeyTextForInputAction(UObject* worldContextObject, const UInputAction* inputAction, EOSEInputHardwareType inputHardwareType, bool longDisplayName = false);

   //---------------------------------------------------------------------------------------
   /// C++ Utility
   //---------------------------------------------------------------------------------------

   /// C++-only utility to get an FText for a key, with a bunch of hardcoded TEMP constants for development
   static FText GetDisplayNameForKey(const FKey& key, bool longDisplayName);
};
