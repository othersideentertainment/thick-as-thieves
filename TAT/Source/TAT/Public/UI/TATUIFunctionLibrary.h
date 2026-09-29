// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <GameplayTagContainer.h>
#include <Input/UIActionBindingHandle.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "TATUIFunctionLibrary.generated.h"

class UAkAudioEvent;
class UCommonUserWidget;
class UPanelWidget;
class UWidget;
struct FTATCharacterLoadout;
class UTATMatchSettingsBase;
class UToolComponent;
class UUserWidget;
class UWidgetAnimation;
struct FGameplayAbilityActorInfo;
class UPaperSprite;
class UGridPanel;
class UUniformGridPanel;

enum class ETATDifficulty : uint8;

USTRUCT(BlueprintType)
struct FTATPressAndHoldAnimationSet
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite)
   UUserWidget* AnimationTarget = nullptr;

   UPROPERTY(BlueprintReadWrite)
   UWidgetAnimation* Animation = nullptr;

   bool operator==(const FTATPressAndHoldAnimationSet& Other) const
   {
      return Other.AnimationTarget == AnimationTarget && Other.Animation == Animation;
   }
};

DECLARE_DYNAMIC_DELEGATE(FTATUIActionDelegate);

USTRUCT(BlueprintType)
struct FTATUIActionBindingArgs
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FDataTableRowHandle InputAction;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   TEnumAsByte<EInputEvent> InputEvent = IE_Pressed;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FTATUIActionDelegate OnExecute;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool bDisplayInActionBar = true;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FText DisplayNameOverride;
};

UCLASS()
class TAT_API UTATUIFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure)
   static bool IsCharacterLoadoutEmpty(const FTATCharacterLoadout& loadout);

   /// If the given actor is a locally controlled player, request the given toast message
   /// If it's not a locally controlled player, do nothing
   UFUNCTION(BlueprintCallable, Category = "TAT|UI")
   static bool RequestToastIfLocallyControlled(
      UPARAM(Meta = (DisplayName = "Player Actor or Controller")) AActor* playerActor,
      UPARAM(Meta = (Categories = "Toast.Type")) FGameplayTag toastId,
      FText message,
      TSoftObjectPtr<UPaperSprite> iconOverride = nullptr,
      bool discardIfNotShownImmediately = false);

   // Finds the next index within the bounds of the total items
   // Will wrap around if outside the bounds
   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static int32 GetNextIndexWrapped(int32 currentIndex, int32 deltaIndex, int32 totalItems);

   /// Given a float from 0 <-> 1 and an integer range, interpolates the float to the range and rounds to an integer.
   /// Eg. the integer range 0 <-> 5 (inclusive)
   /// 0.0 would return 0
   /// 1.0 would return 5
   /// 0.2 would return 1 
   /// 0.15 would also return 1
   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static int32 MapFloatToIntegerRange(float value, int32 rangeStart, int32 rangeEnd);

   /// Gets the max row index in a grid panel. If columnIndex is >= 0, it will return the max row for that column, otherwise the max row for the whole panel.
   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static int32 GetMaxRowInGridPanel(UGridPanel* gridPanel, int32 columnIndex = -1);

   /// Gets the max column index in a grid panel. If rowIndex is >= 0, it will return the max column for that row, otherwise the max column for the whole panel.
   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static int32 GetMaxColumnInGridPanel(UGridPanel* gridPanel, int32 rowIndex = -1);

   /// Gets the max row index in a uniform grid panel. If columnIndex is >= 0, it will return the max row for that column, otherwise the max row for the whole panel.
   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static int32 GetMaxRowInUniformGridPanel(UUniformGridPanel* gridPanel, int32 columnIndex = -1);

   /// Gets the max column index in a uniform grid panel. If rowIndex is >= 0, it will return the max column for that row, otherwise the max column for the whole panel.
   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static int32 GetMaxColumnInUniformGridPanel(UUniformGridPanel* gridPanel, int32 rowIndex = -1);

   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static void GetGridPanelIndexWithDirectionalOffset(UGridPanel* gridPanel, int32 currentIndex, int32 offsetX, int32 offsetY, int32& outNewIndex, TSubclassOf<UWidget> requiredWidgetClass = nullptr);

   UFUNCTION(BlueprintPure, Category = "TAT|UI")
   static void GetUniformGridPanelIndexWithDirectionalOffset(UUniformGridPanel* gridPanel, int32 currentIndex, int32 offsetX, int32 offsetY, int32& outNewIndex, TSubclassOf<UWidget> requiredWidgetClass = nullptr);

   // Don't use it for real stuff
   // 1. Assumes input does not have line breaks
   // 2. Erases FText provenance
   // 3. Not the most performant
   // 4. ...
   UFUNCTION(BlueprintCallable, Category = "TAT|UI")
   static FText BadLineWrap(const FText& input, int maxCharsPerLine);

   // Returns the match settings instance that should be used in the match lobby UI. 
   // NOTE: requires the local player state to be present, and the mission owner to have been selected
   UFUNCTION(BlueprintPure, Category = "TAT|UI", meta = (DefaultToSelf = "contextObject"))
   static UTATMatchSettingsBase* GetMatchSettingsForLobbyUI(const UObject* contextObject);

   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "TAT|UI")
   static ULocalPlayer* GetLocalPlayerFromController(APlayerController* playerController);

   UFUNCTION(BlueprintCallable, Category = "TAT|UI")
   static UWidget* GetFirstVisibleChild(const UPanelWidget* panelWidget);

   UFUNCTION(BlueprintCallable, Category = "TAT|UI", meta=(DefaultToSelf = "widget", AutoCreateRefTerm = "bindingArgs", ReturnDisplayName = "BindingHandle"))
   static FUIActionBindingHandle RegisterActionBinding(UCommonUserWidget* widget, const FTATUIActionBindingArgs& bindingArgs);

   UFUNCTION(BlueprintCallable, Category = "TAT|UI", meta=(AutoCreateRefTerm = "displayName"))
   static void SetActionBindingDisplayName(FUIActionBindingHandle bindingHandle, const FText& displayName);

   UFUNCTION(BlueprintCallable, Category = "TAT|UI")
   static void SetActionBindingDisplayInActionBar(FUIActionBindingHandle bindingHandle, bool bDisplayInActionBar);

   UFUNCTION(BlueprintCallable, Category = "TAT|UI")
   static void UnregisterActionBinding(FUIActionBindingHandle bindingHandle);

   UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "TAT|UI", meta=(DefaultToSelf = "widget"))
   static int32 PlaySound(UAkAudioEvent* soundEvent);

   UFUNCTION(BlueprintCallable, Category = "TAT|UI", meta = (DefaultToSelf = "widget"))
   static APlayerState* GetOwningPlayerState(const UUserWidget* widget);

   UFUNCTION(BlueprintCallable, Category = "TAT|UI")
   static FText GetSessionInfo();

   UFUNCTION(BlueprintCallable, BlueprintPure, meta = (WorldContext = "contextObject"))
   static bool CanInviteFriendsToLobby(const UObject* contextObject);

   UFUNCTION(BlueprintCallable, meta = (WorldContext = "contextObject"))
   static bool ShowInviteUI(const UObject* contextObject);

   // Displays the profile on the platform external UI, returns true if successful.
   UFUNCTION(BlueprintCallable)
   static bool ShowPlayerProfileOnExternalUI(const APlayerState* requestor, const APlayerState* requestee);

   // Returns true if the External UI Interface is retrieved from the Online Subsystem
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static bool IsPlatformExternalUIAvailable();
   
   UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DefaultToSelf = "contextObject"))
   static bool CheckWorldURLHasOption(const UObject* contextObject, FString option);

   // Gets the difficulty for the active match
   UFUNCTION(BlueprintCallable, meta = (WorldContext = "contextObject"))
   static bool GetMatchDifficulty(const UObject* contextObject, ETATDifficulty& outDifficulty);
};
