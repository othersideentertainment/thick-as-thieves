// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATUIFunctionLibrary.h"

// TAT
#include "TATGameInstance.h"
#include "CharacterCustomization/TATCharacterLoadout.h"
#include "GameFramework/TATWorldSettings.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"
#include "UI/TATToastSubsystem.h"

// OSE
#include "UI/OSERadialPaintLibrary.h"

// Wwise
#include "AkAudioEvent.h"

// UE
#include <CommonUserWidget.h>
#include <Online.h>
#include <OnlineSessionSettings.h>
#include <Components/GridPanel.h>
#include <Components/GridSlot.h>
#include <Components/UniformGridPanel.h>
#include <Components/UniformGridSlot.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <Input/CommonUIInputTypes.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUIFunctionLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogTATUIFunctionLibrary, Log, All);

// static
bool UTATUIFunctionLibrary::IsCharacterLoadoutEmpty(const FTATCharacterLoadout& loadout)
{
   return loadout.IsEmpty();
}

// static
bool UTATUIFunctionLibrary::RequestToastIfLocallyControlled(AActor* playerActor, FGameplayTag toastId, FText message, TSoftObjectPtr<UPaperSprite> iconOverride, bool discardIfNotShownImmediately)
{
   APlayerController* playerController = nullptr;

   // Accept either a pawn or a controller. Makes for less boilerplate in blueprints.
   if (APlayerController* pc = Cast<APlayerController>(playerActor))
   {
      playerController = pc;
   }
   else if (APawn* pawn = Cast<APawn>(playerActor))
   {
      playerController = pawn->GetController<APlayerController>();
   }
   else if (APlayerState* ps = Cast<APlayerState>(playerActor))
   {
      playerController = ps->GetPlayerController();
   }

   if (playerController != nullptr)
   {
      if (ULocalPlayer* localPlayer = playerController->GetLocalPlayer())
      {
         if (UTATToastSubsystem* toastSubsystem = localPlayer->GetSubsystem<UTATToastSubsystem>())
         {
            return toastSubsystem->RequestToastMessage(toastId, message, iconOverride, discardIfNotShownImmediately);
         }
      }
   }

   return false;
}

int32 UTATUIFunctionLibrary::GetNextIndexWrapped(int32 currentIndex, int32 deltaIndex, int32 totalItems)
{
   if (totalItems <= 0)
   {
      UE_LOG(LogTATUIFunctionLibrary, Error, TEXT("GetNextIndexWrapped() : 'totalItems' was zero!"));
      return INDEX_NONE;
   }

   const int32 currentIndexClamped = FMath::Clamp(currentIndex, 0, totalItems);
   UE_CLOG(currentIndexClamped != currentIndex, LogTATUIFunctionLibrary, Warning
           , TEXT("GetNextIndexWrapped() : 'currentIndex' was outside the bounds of 0->'totalItems'! Clamping..."));

   const int32 deltaIndexClamped = FMath::Clamp(deltaIndex, -totalItems, +totalItems);
   UE_CLOG(deltaIndexClamped != deltaIndex, LogTATUIFunctionLibrary, Warning
           , TEXT("GetNextIndexWrapped() : 'deltaIndex' was outside the bounds of '-totalItems'->'+totalItems'! Clamping..."));

   const int32 newIndex = currentIndexClamped + deltaIndexClamped;
   return (newIndex + totalItems) % totalItems;
}

int32 UTATUIFunctionLibrary::MapFloatToIntegerRange(float value, int32 rangeStart, int32 rangeEnd)
{
   if (value < 0 || value > 1)
   {
      UE_LOG(LogTATUIFunctionLibrary, Error, TEXT("MapFloatToIntegerRange() called with value outside 0 <-> 1 range!"));
      return INDEX_NONE;
   }
   if (rangeEnd <= rangeStart)
   {
      UE_LOG(LogTATUIFunctionLibrary, Error, TEXT("MapFloatToIntegerRange() providerd invalid range (%d <-> %d)!"), rangeStart, rangeEnd);
      return INDEX_NONE;
   }

   // Map it to the length of the range
   const int32 rangeLength = rangeEnd - rangeStart;
   const int32 mappedToRangeLength = FMath::RoundToInt32(value * rangeLength);
   return rangeStart + mappedToRangeLength;
}

namespace UIHelpers
{
   // Given the index and span (number of cells covered) of a slot in a GridPanel, returns the end index (row or column).
   inline int32 GetSpanEndIndex(int32 startIndex, int32 span)
   {
      return startIndex + FMath::Max(0, span - 1);
   }

   // Calls the callback for each slot and index in a panel, casting the slot to the specified type.
   // Example: ForEachChildWidgetSlot<UGridSlot>(gridPanel, [](UGridSlot* slot, int32 index) {})
   template<typename SlotType, typename PanelType, typename Lambda>
   void ForEachChildWidgetSlot(PanelType* panel, Lambda&& callback)
   {
      if (panel == nullptr)
      {
         return;
      }

      for (int32 i = 0; i < panel->GetChildrenCount(); i++)
      {
         UWidget* childWidget = panel->GetChildAt(i);
         if (childWidget == nullptr || childWidget->Slot == nullptr)
         {
            continue;
         }
         callback(CastChecked<SlotType>(childWidget->Slot), i);
      }
   }

   template<typename SlotType>
   inline FVector2f SlotToCoord(SlotType* slot)
   {
      static_assert(std::is_same_v<SlotType, UGridSlot> || std::is_same_v<SlotType, UUniformGridSlot>,
                    "SlotToCoord only works with grid slots and uniform grid slots");
      check(slot != nullptr);
      return { static_cast<float>(slot->GetColumn()), static_cast<float>(slot->GetRow()) };
   }

   inline FSlateRect GridSlotToBox(UGridSlot* slot)
   {
      check(slot != nullptr);
      return FSlateRect{
         static_cast<float>(slot->GetColumn()) - 0.5f,
         static_cast<float>(slot->GetRow()) - 0.5f,
         static_cast<float>(GetSpanEndIndex(slot->GetColumn(), slot->GetColumnSpan())) + 1.0f,
         static_cast<float>(GetSpanEndIndex(slot->GetRow(), slot->GetRowSpan())) + 1.0f,
      };
   }
}

// static
int32 UTATUIFunctionLibrary::GetMaxRowInGridPanel(UGridPanel* gridPanel, int32 columnIndex)
{
   if (gridPanel == nullptr)
   {
      return 0;
   }
   int32 maxRow = 0;
   UIHelpers::ForEachChildWidgetSlot<UGridSlot>(gridPanel, [&](UGridSlot* slot, int32 index)
   {
      // If we're only checking for a particular column, make sure this cell's column range matches
      if (columnIndex >= 0)
      {
         const int32 columnStartIndex = slot->GetColumn();
         if (columnIndex < columnStartIndex || columnIndex > UIHelpers::GetSpanEndIndex(columnStartIndex, slot->GetColumnSpan()))
         {
            return;
         }
      }

      const int32 row = slot->GetRow();
      if (row > maxRow)
      {
         maxRow = row;
      }
   });
   return maxRow;
}

// static
int32 UTATUIFunctionLibrary::GetMaxColumnInGridPanel(UGridPanel* gridPanel, int32 rowIndex)
{
   if (gridPanel == nullptr)
   {
      return 0;
   }
   int32 maxColumn = 0;
   UIHelpers::ForEachChildWidgetSlot<UGridSlot>(gridPanel, [&](UGridSlot* slot, int32 index)
   {
      // If we're only checking for a particular row, make sure this cell's row range matches
      if (rowIndex >= 0)
      {
         const int32 rowStartIndex = slot->GetRow();
         if (rowIndex < rowStartIndex || rowIndex > UIHelpers::GetSpanEndIndex(rowStartIndex, slot->GetRowSpan()))
         {
            return;
         }
      }

      const int32 colEndIndex = UIHelpers::GetSpanEndIndex(slot->GetColumn(), slot->GetColumnSpan());
      if (colEndIndex > maxColumn)
      {
         maxColumn = colEndIndex;
      }
   });
   return maxColumn;
}

// static
int32 UTATUIFunctionLibrary::GetMaxRowInUniformGridPanel(UUniformGridPanel* gridPanel, int32 columnIndex)
{
   if (gridPanel == nullptr)
   {
      return 0;
   }
   int32 maxRow = 0;
   UIHelpers::ForEachChildWidgetSlot<UUniformGridSlot>(gridPanel, [&](UUniformGridSlot* slot, int32 index)
   {
      if (columnIndex >= 0 && slot->GetColumn() != columnIndex)
      {
         return;
      }
      const int32 row = slot->GetRow();
      if (row > maxRow)
      {
         maxRow = row;
      }
   });
   return maxRow;
}

// static
int32 UTATUIFunctionLibrary::GetMaxColumnInUniformGridPanel(UUniformGridPanel* gridPanel, int32 rowIndex)
{
   if (gridPanel == nullptr)
   {
      return 0;
   }
   int32 maxColumn = 0;
   UIHelpers::ForEachChildWidgetSlot<UUniformGridSlot>(gridPanel, [&](UUniformGridSlot* slot, int32 index)
   {
      if (rowIndex >= 0 && slot->GetRow() != rowIndex)
      {
         return;
      }
      const int32 col = slot->GetColumn();
      if (col > maxColumn)
      {
         maxColumn = col;
      }
   });
   return maxColumn;
}

// static
void UTATUIFunctionLibrary::GetGridPanelIndexWithDirectionalOffset(UGridPanel* gridPanel, int32 currentIndex, int32 offsetX, int32 offsetY, int32& outNewIndex, TSubclassOf<UWidget> requiredWidgetClass)
{
   if (gridPanel == nullptr || currentIndex < 0 || currentIndex >= gridPanel->GetChildrenCount() || (offsetX == 0 && offsetY == 0))
   {
      outNewIndex = currentIndex;
      return;
   }

   UWidget* currentChild = gridPanel->GetChildAt(currentIndex);
   if (currentChild == nullptr || currentChild->Slot == nullptr)
   {
      outNewIndex = currentIndex;
      return;
   }
   UGridSlot* currentSlot = CastChecked<UGridSlot>(currentChild->Slot);
   const FSlateRect currentSlotRect = UIHelpers::GridSlotToBox(currentSlot);
   const FVector2f currentSlotRectCenter = currentSlotRect.GetCenter2f();
   const FVector2f targetCoord = currentSlotRectCenter + (FVector2f(offsetX, offsetY) * 0.5f);
   const FVector2f searchDirection = FVector2f(offsetX, offsetY).GetSafeNormal();

   int32 bestIndex = INDEX_NONE;
   float bestDistSquaredToTarget = std::numeric_limits<float>::max();

   UIHelpers::ForEachChildWidgetSlot<UGridSlot>(gridPanel, [&](UGridSlot* slot, int32 index)
   {
      if (slot == currentSlot)
      {
         return;
      }

      // If we have a required widget class, only try this widget if it matches
      if (requiredWidgetClass != nullptr)
      {
         UWidget* child = gridPanel->GetChildAt(index);
         if (child == nullptr || !child->IsA(requiredWidgetClass))
         {
            return;
         }
      }

      const FSlateRect slotRect = UIHelpers::GridSlotToBox(slot);

      // If the angle from the current slot towards this one is more than 75 degrees, skip it
      // This seems like a very high angle to test for, but really we're just filtering out things that are backwards and at sharp angles.
      // The intersection test below will filter out widgets that aren't on the trace line.
      const FVector2f slotDirection = (slotRect.GetCenter2f() - currentSlotRectCenter).GetSafeNormal();
      const float slotAngleDeg = FMath::RadiansToDegrees(FMath::Acos(slotDirection.Dot(searchDirection)));
      if (slotAngleDeg >= 75.0f)
      {
         return;
      }

      // Shoot a ray from our start point in the direction we're looking and find all intersections with this slot
      UOSERadialPaintLibrary::FIntersectionArray2f intersections;
      if (!UOSERadialPaintLibrary::FindLineSegmentRectEdgeIntersections(currentSlotRectCenter, currentSlotRectCenter + (searchDirection * 10000.0f), slotRect, intersections))
      {
         return;
      }

      // If we had any hits, find the closest hit to our target
      TOptional<FVector2f> bestPt;
      float bestDistSquared = std::numeric_limits<float>::max();
      for (const FVector2f& pt : intersections)
      {
         const float distSq = FVector2f::DistSquared(targetCoord, pt);
         if (distSq < bestDistSquared)
         {
            bestDistSquared = distSq;
            bestPt = pt;
         }
      }

      // Use this slot if it's better than any existing one we may have
      if (bestPt)
      {
         const float distSquared = FVector2f::DistSquared(*bestPt, targetCoord);
         if (distSquared < bestDistSquaredToTarget)
         {
            bestIndex = index;
            bestDistSquaredToTarget = distSquared;
         }
      }
   });

   outNewIndex = (bestIndex != INDEX_NONE) ? bestIndex : currentIndex;
}

// static
void UTATUIFunctionLibrary::GetUniformGridPanelIndexWithDirectionalOffset(UUniformGridPanel* gridPanel, int32 currentIndex, int32 offsetX, int32 offsetY, int32& outNewIndex, TSubclassOf<UWidget> requiredWidgetClass)
{
   if (gridPanel == nullptr || currentIndex < 0 || currentIndex >= gridPanel->GetChildrenCount() || (offsetX == 0 && offsetY == 0))
   {
      outNewIndex = currentIndex;
      return;
   }

   UWidget* currentChild = gridPanel->GetChildAt(currentIndex);
   if (currentChild == nullptr || currentChild->Slot == nullptr)
   {
      outNewIndex = currentIndex;
      return;
   }

   UUniformGridSlot* currentSlot = CastChecked<UUniformGridSlot>(currentChild->Slot);
   const FVector2f currentCoord = UIHelpers::SlotToCoord(currentSlot);
   const FVector2f targetCoord = currentCoord + FVector2f(offsetX, offsetY);
   const FVector2f searchDirection = (targetCoord - currentCoord).GetSafeNormal();

   int32 bestIndex = INDEX_NONE;
   float bestDistSquaredToTarget = std::numeric_limits<float>::max();

   // Find the child index closest to the target coord
   UIHelpers::ForEachChildWidgetSlot<UUniformGridSlot>(gridPanel, [&](UUniformGridSlot* slot, int32 index)
   {
      if (slot == currentSlot)
      {
         return;
      }

      // If we have a required widget class, only try this widget if it matches
      if (requiredWidgetClass != nullptr)
      {
         UWidget* child = gridPanel->GetChildAt(index);
         if (child == nullptr || !child->IsA(requiredWidgetClass))
         {
            return;
         }
      }

      const FVector2f pos = UIHelpers::SlotToCoord(slot);

      // If the angle from the current slot towards this one is more than 35 degrees, skip it.
      // This ensures we're only looking at widgets in the correct general direction.
      const FVector2f slotDirection = (pos - currentCoord).GetSafeNormal();
      const float slotAngleDeg = FMath::RadiansToDegrees(FMath::Acos(slotDirection.Dot(searchDirection)));
      if (slotAngleDeg > 35.0f)
      {
         return;
      }

      // Use this slot if it's better than any existing slot we may have
      const float distSquared = FVector2f::DistSquared(pos, targetCoord);
      if (distSquared < bestDistSquaredToTarget)
      {
         bestIndex = index;
         bestDistSquaredToTarget = distSquared;
      }
   });

   outNewIndex = (bestIndex != INDEX_NONE) ? bestIndex : currentIndex;
}

FText UTATUIFunctionLibrary::BadLineWrap(const FText& input, int maxCharsPerLine)
{
   FString stringToWrap = input.ToString();
   bool didReplacement = false;

   FStringView view = stringToWrap;
   for (int i = 0; i + maxCharsPerLine < stringToWrap.Len();)
   {
      FStringView slice = view.SubStr(i, maxCharsPerLine);
      int index = -1;
      if (slice.FindLastChar((TCHAR)' ', index))
      {
         // Not efficient, but this is bad line wrapping
         stringToWrap.RemoveAt(i + index, EAllowShrinking::No);
         stringToWrap.InsertAt(i + index, (TCHAR)'\n');
         i += index;
         didReplacement = true;
      }
      else
      {
         i += maxCharsPerLine;
      }
   }

   return didReplacement ? FText::FromString(MoveTemp(stringToWrap)) : input;
}

// static
UTATMatchSettingsBase* UTATUIFunctionLibrary::GetMatchSettingsForLobbyUI(const UObject* contextObject)
{
   if (!contextObject)
   {
      UE_LOG(LogTATUIFunctionLibrary, Error, TEXT("GetMatchSettingsForUI() called with null contextObject!"));
      return nullptr;
   }

   const UWorld* world = contextObject->GetWorld();
   check(world);

   const UTATGameInstance* tatGameInstance = world->GetGameInstance<UTATGameInstance>();
   check(tatGameInstance);

   // Should only be using this function in the mission lobby screen (which should occupy a Hub world)
   const ATATWorldSettings* worldSettings = CastChecked<ATATWorldSettings>(world->GetWorldSettings());
   UE_CLOG(worldSettings->MapType != ETATMapType::Hub, LogTATUIFunctionLibrary, Warning, TEXT("GetMatchSettingsForUI() called from world of type '%s' (%s expected)")
           , *UEnum::GetValueAsString(worldSettings->MapType)
           , *UEnum::GetValueAsString(ETATMapType::Hub));

   // Make sure local player state is present
   const ATATPlayerState* tatPs = ATATPlayerState::GetLocalTATPlayerState(contextObject);
   if (!tatPs)
   {
      UE_LOG(LogTATUIFunctionLibrary, Error, TEXT("GetMatchSettingsForUI() called before local player state was found! Defaulting to TATGameInstance match settings..."));
      return &tatGameInstance->GetMatchSettings();
   }

   // Make sure game state is present and a mission owner is selected
   const ATATGameState* tatGs = world->GetGameState<ATATGameState>();
   if (!tatGs)
   {
      UE_LOG(LogTATUIFunctionLibrary, Error, TEXT("GetMatchSettingsForUI() called before game state found! Defaulting to TATGameInstance match settings..."));
      return &tatGameInstance->GetMatchSettings();
   }
   if (tatGs->GetMissionOwnerPlayerState() == nullptr)
   {
      UE_LOG(LogTATUIFunctionLibrary, Error, TEXT("GetMatchSettingsForUI() called before a mission-owner player has been selected! Defaulting to TATGameInstance match settings..."));
      return &tatGameInstance->GetMatchSettings();
   }

   // Mission owners use their locally-cached match settings instance
   if (tatPs->IsMissionOwner())
   {
      return tatPs->GetLocalUIMatchSettings();
   }
   // Other clients use the game instance match settings (to which changes are replicated)
   else
   {
      return &tatGameInstance->GetMatchSettings();
   }
}

ULocalPlayer* UTATUIFunctionLibrary::GetLocalPlayerFromController(APlayerController* playerController)
{
   return playerController ? playerController->GetLocalPlayer() : nullptr;
}

UWidget* UTATUIFunctionLibrary::GetFirstVisibleChild(const UPanelWidget* panelWidget)
{
   for (int32 childIdx = 0; childIdx < panelWidget->GetChildrenCount(); childIdx++)
   {
      UWidget* childWidget = panelWidget->GetChildAt(childIdx);
      if (childWidget && childWidget->IsVisible())
      {
         return childWidget;
      }
   }

   return nullptr;
}

FUIActionBindingHandle UTATUIFunctionLibrary::RegisterActionBinding(UCommonUserWidget* widget, const FTATUIActionBindingArgs& bindingArgs)
{
   FBindUIActionArgs args(bindingArgs.InputAction, FSimpleDelegate::CreateWeakLambda(widget, [callback = bindingArgs.OnExecute]
   {
      callback.ExecuteIfBound();
   }));

   args.KeyEvent = bindingArgs.InputEvent;
   args.bDisplayInActionBar = bindingArgs.bDisplayInActionBar;
   args.OverrideDisplayName = bindingArgs.DisplayNameOverride;

   return widget->RegisterUIActionBinding(args);
}

void UTATUIFunctionLibrary::SetActionBindingDisplayName(FUIActionBindingHandle bindingHandle, const FText& displayName)
{
   if (bindingHandle.IsValid())
   {
      bindingHandle.SetDisplayName(displayName);
   }
}

void UTATUIFunctionLibrary::SetActionBindingDisplayInActionBar(FUIActionBindingHandle bindingHandle, bool bDisplayInActionBar)
{
   if (bindingHandle.IsValid())
   {
      bindingHandle.SetDisplayInActionBar(bDisplayInActionBar);
   }
}

void UTATUIFunctionLibrary::UnregisterActionBinding(FUIActionBindingHandle bindingHandle)
{
   if (bindingHandle.IsValid())
   {
      bindingHandle.Unregister();
   }
}

int32 UTATUIFunctionLibrary::PlaySound(UAkAudioEvent* soundEvent)
{
   return soundEvent ? soundEvent->PostAmbient(nullptr, nullptr, nullptr, (AkCallbackType)0, nullptr) : AK_INVALID_PLAYING_ID;
}

APlayerState* UTATUIFunctionLibrary::GetOwningPlayerState(const UUserWidget* widget)
{
   return widget ? widget->GetOwningPlayerState() : nullptr;
}

FText UTATUIFunctionLibrary::GetSessionInfo()
{
   const IOnlineSessionPtr sessionInterface = Online::GetSessionInterface();
   if (sessionInterface.IsValid())
   {
      const FNamedOnlineSession* session = sessionInterface->GetNamedSession(NAME_GameSession);
      if (session)
      {
         return FText::FromString(session->OwningUserName);
      }
   }

   return FText::GetEmpty();
}

bool UTATUIFunctionLibrary::CanInviteFriendsToLobby(const UObject* contextObject)
{
   if (contextObject == nullptr)
   {
      return false;
   }

   // A rough heuristic for is-the-multiplayer-host
   const UWorld* world = contextObject->GetWorld();
   if (world == nullptr || world->GetNetMode() != NM_ListenServer)
   {
      return false;
   }

   // and then is the OSS one that would do this
   const IOnlineExternalUIPtr externalUIInterface = Online::GetExternalUIInterface();
   return externalUIInterface.IsValid();
}

bool UTATUIFunctionLibrary::ShowInviteUI(const UObject* contextObject)
{
   const IOnlineExternalUIPtr externalUIInterface = Online::GetExternalUIInterface();
   if (!externalUIInterface.IsValid())
   {
      return false;
   }

   // player index 0
   return externalUIInterface->ShowInviteUI(0);
}

bool UTATUIFunctionLibrary::ShowPlayerProfileOnExternalUI(const APlayerState* requestor, const APlayerState* requestee)
{
   if (!IsValid(requestor) && !IsValid(requestee))
   {
      return false;
   }

   const IOnlineIdentityPtr identityInterface = Online::GetIdentityInterface();
   if (!identityInterface.IsValid())
   {
      return false;
   }

   const IOnlineExternalUIPtr externalUIInterface = Online::GetExternalUIInterface();
   if (!externalUIInterface.IsValid())
   {
      return false;
   }

   FUniqueNetIdPtr requestorID = identityInterface->GetUniquePlayerId(requestor->GetPlayerId());
   FUniqueNetIdPtr requesteeID = identityInterface->GetUniquePlayerId(requestee->GetPlayerId());

   if (requestorID.IsValid() && requesteeID.IsValid())
   {
      return externalUIInterface->ShowProfileUI(*requestorID, *requesteeID);
   }

   return false;
}

bool UTATUIFunctionLibrary::IsPlatformExternalUIAvailable()
{
   const IOnlineExternalUIPtr externalUIInterface = Online::GetExternalUIInterface();

   return externalUIInterface.IsValid();
}

bool UTATUIFunctionLibrary::CheckWorldURLHasOption(const UObject* contextObject, const FString option)
{
	const UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::LogAndReturnNull);
   return world ? world->URL.HasOption(*option) : false;
}

bool UTATUIFunctionLibrary::GetMatchDifficulty(const UObject* contextObject, ETATDifficulty& outDifficulty)
{
   UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::LogAndReturnNull);
   if (world == nullptr)
   {
      return false;
   }

   if (ATATWorldSettings::Get(contextObject).MapType == ETATMapType::Mission)
   {
      outDifficulty = TATDifficulty::GetDifficultyForMatch(world);
      return true;
   }
   return false;
}
