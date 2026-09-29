// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATScreenMgr.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "Player/TATPlayerController.h"
#include "UI/TATUIDeveloperSettings.h"
#include "UI/TATScreenWidget.h"

// ue4
#include "AkGameplayStatics.h"
#include "AkStateValue.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/AssetManager.h"
#include "MoviePlayer.h"

// This is more about documentation than conditional compilation to indicate what parts of this file are touched by the CommonUI/ScreenMgr input hack.
#define TAT_INPUT_HACK 1

#if TAT_INPUT_HACK
#include "CommonActivatableWidget.h"
#include "Input/CommonUIActionRouterBase.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATScreenMgr)

DEFINE_LOG_CATEGORY(LogTATScreenMgr);

#if TAT_INPUT_HACK
/// [TVT-8705] 4/15/2026 JC: This hack is needed because we're still using a mix of the old screen manager widgets and CommonUI.
/// If there's a CommonUI widget on the screen using a menu input mode, we don't want to stomp on that state.
static bool IsAnyCommonUIWidgetUsingMenuInputMode(ATATPlayerController& pc)
{
   const ULocalPlayer* localPlayer = pc.GetLocalPlayer();
   if (localPlayer == nullptr)
   {
      return false;
   }
   UCommonUIActionRouterBase* actionRouter = localPlayer->GetSubsystem<UCommonUIActionRouterBase>();
   if (actionRouter == nullptr)
   {
      return false;
   }
   return actionRouter->GetActiveInputMode() == ECommonInputMode::Menu;
}
#endif

UTATScreenMgr::UTATScreenMgr()
{
#if TAT_INPUT_HACK
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = true;
#else
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
#endif
   SetIsReplicatedByDefault(false);
}

void UTATScreenMgr::BeginPlay()
{
   Super::BeginPlay();

   _RecalculateInputState();
   _LoadAkStates();
   _RefreshAkState();

   // Movie playback will temporarily reset focus to the loading screen
   // This can happen after we push a UI screen to the stack (e.g. if we did so during map load)
   // Once the loading screen finishes, make sure that we are focusing the expected screen widget
   if (IGameMoviePlayer* moviePlayer = GetMoviePlayer())
   {
      moviePlayer->OnMoviePlaybackFinished().AddUObject(this, &ThisClass::_OnMoviePlaybackFinished);
   }
}

void UTATScreenMgr::EndPlay(EEndPlayReason::Type reason)
{
   if (IGameMoviePlayer* moviePlayer = GetMoviePlayer())
   {
      moviePlayer->OnMoviePlaybackFinished().RemoveAll(this);
   }

   Super::EndPlay(reason);
}

void UTATScreenMgr::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

#if TAT_INPUT_HACK
   // _recalcInputStatePending is set to true when _RecalculateInputState() is called while a CommonUI widget is in menu mode.
   // This polls the CommonUI input mode until it's no longer in menu mode and then recalculates again.
   if (_recalcInputStatePending && !IsAnyCommonUIWidgetUsingMenuInputMode(_GetOwnerPC()))
   {
      UE_LOG(LogTATScreenMgr, Warning, TEXT("_RecalculateInputState: CommonUI is now out of menu mode; calling _RecalculateInputState again"));
      _recalcInputStatePending = false;
      _RecalculateInputState();
   }
#endif
}

/* static */
UTATScreenMgr* UTATScreenMgr::TryGetScreenManager(const UObject* contextObj)
{
   if (ATATPlayerController* pc = ATATPlayerController::GetLocalTATPlayerController(contextObj))
   {
      if (UTATScreenMgr* screenMgr = pc->GetTATScreenMgr())
      {
         return screenMgr;
      }
   }
   return nullptr;
}

/* static */
void UTATScreenMgr::AddScreenToViewport(const UObject* contextObj, UTATScreenWidget* widget)
{
   if (UTATScreenMgr* screenMgr = TryGetScreenManager(contextObj))
   {
      screenMgr->AddScreen(widget);
   }
}

/* static */
void UTATScreenMgr::RemoveScreenFromViewport(const UObject* contextObj, UTATScreenWidget* widget)
{
   if (UTATScreenMgr* screenMgr = TryGetScreenManager(contextObj))
   {
      screenMgr->RemoveScreen(widget);
   }
}

void UTATScreenMgr::AddScreen(UTATScreenWidget* widget, TFunction<void(UTATScreenWidget*)> onRemoveCallback)
{
   check(widget);

   // added screens should be focusable (avoid infinite focus recursion crash)
   checkf(widget->IsFocusable(), TEXT("'%s' added to top-of-screen stack, but is not focusable (fix class defaults 'Is Focusable')"), *widget->GetName());

   if (!IsScreenInStack(widget))
   {
      UTATScreenWidget* oldTopWidget = GetTopScreen();

      _screenStack.Add(FTATScreenMgrEntry{ widget, MoveTemp(onRemoveCallback) });

      UE_LOG(LogTATScreenMgr, Log, TEXT("%s added to screen stack"), *widget->GetName());
      
      // fix up our input state before calling BP handlers, so they can reassign focus as needed
      _RecalculateInputState();
      
      // screen is now on the stack
      widget->HandleOnScreenAddedToStack(_GetOwnerPC());
      // screen is now on the top of the stack
      widget->HandleOnScreenOnTopOfStack(_GetOwnerPC());

      // tell the screen that was previously on the top of the stack, if we had one, that is no longer on top
      if (oldTopWidget)
      {
         oldTopWidget->HandleOnScreenRemovedFromTopOfStack(_GetOwnerPC());
      }

      _RefreshAkState();

      // broadcast
      OnScreenAddedToStack.Broadcast(widget);
   }
   else
   {
      UE_LOG(LogTATScreenMgr, Error, TEXT("%s is already in the stack, can't double-add it!"), *widget->GetName());
   }
}

void UTATScreenMgr::RemoveScreen(UTATScreenWidget* widget, int32 indexHint)
{
   check(widget);

   // Make sure indexHint is either the default value or correct for this widget
   check(indexHint == INDEX_NONE || (_screenStack.IsValidIndex(indexHint) && _screenStack[indexHint].Widget == widget));

   const int32 index = (indexHint != INDEX_NONE)
      ? indexHint
      : _screenStack.IndexOfByPredicate([widget](const FTATScreenMgrEntry& entry) { return entry.Widget == widget; });

   if (_screenStack.IsValidIndex(index))
   {
      UTATScreenWidget* oldTopWidget = GetTopScreen();

      // Save a copy of the entry so we can handle the on removed callback later on
      const FTATScreenMgrEntry entry = _screenStack[index];

      _screenStack.RemoveAt(index);

      UE_LOG(LogTATScreenMgr, Log, TEXT("%s removed from screen stack"), *widget->GetName());

      // fix up our input state before calling BP handlers, so they can reassign focus as needed
      _RecalculateInputState();

      // tell our widget, if it was previously on top, that it's no longer on top
      if (widget == oldTopWidget)
      {
         oldTopWidget->HandleOnScreenRemovedFromTopOfStack(_GetOwnerPC());
      }

      // our widget is being removed from the stack
      widget->HandleOnScreenRemovedFromStack(_GetOwnerPC());

      // tell our new top widget, if it changed, that it's now the top widget
      UTATScreenWidget* newTopWidget = GetTopScreen();
      if (newTopWidget && oldTopWidget != newTopWidget)
      {
         newTopWidget->HandleOnScreenOnTopOfStack(_GetOwnerPC());
      }
      
      _RefreshAkState();

      // broadcast
      if (entry.OnRemovedCallback)
      {
         entry.OnRemovedCallback(widget);
      }
      OnScreenRemovedFromStack.Broadcast(widget);
   }
   else
   {
      UE_LOG(LogTATScreenMgr, Error, TEXT("Cannot remove screen %s when it's not in the stack!"), *widget->GetName());
   }
}

void UTATScreenMgr::RemoveAllScreensFromStack()
{
   for (int idx = _screenStack.Num() - 1; idx >= 0; idx--)
   {
      RemoveScreen(_screenStack[idx].Widget, idx);
   }
}

UTATScreenWidget* UTATScreenMgr::GetTopScreen() const
{
   if (_screenStack.Num() > 0)
   {
      const FTATScreenMgrEntry& entry = _screenStack.Last();
      check(entry.Widget != nullptr);
      return entry.Widget;
   }
   return nullptr;
}

void UTATScreenMgr::RestoreFocusToTopScreen() const
{
   // WARNING - This function was only meant as a fix while transitioning to CommonUI to fix TVT-8607 and TVT-8677.
   // See definition for more details.
   UTATScreenWidget* topScreen = GetTopScreen();
   if (topScreen)
   {
      topScreen->HandleOnScreenOnTopOfStack(_GetOwnerPC());
   }
}

bool UTATScreenMgr::IsScreenInStack(const UTATScreenWidget* widget) const
{
   return _screenStack.ContainsByPredicate([widget](const FTATScreenMgrEntry& entry) { return entry.Widget == widget; });
}

void UTATScreenMgr::_RecalculateInputState()
{
   UTATScreenWidget* widget = GetTopScreen();
   bool showingScreen = widget != nullptr;
   ATATPlayerController& localPC = _GetOwnerPC();

#if TAT_INPUT_HACK
   // If there's a CommonUI widget on the screen right now and it's using the Menu input mode, don't touch the input mode state
   if (IsAnyCommonUIWidgetUsingMenuInputMode(_GetOwnerPC()))
   {
      if (!_recalcInputStatePending)
      {
         UE_LOG(LogTATScreenMgr, Warning, TEXT("_RecalculateInputState: Deferring input state change until CommonUI is out of menu mode"));
      }
      _recalcInputStatePending = true;
      return;
   }
#endif // TAT_INPUT_HACK

   // cursor is a singular global state so we'll rely on our current
   // widget to tell us if we should be showing it right now
   localPC.bShowMouseCursor = widget ? widget->ShowMouseCursor : false;

   // change input mode depending on whether or not there's ui showing
   if (showingScreen)
   {
      if (widget->AutoSetGameAndUIInputModeOnShow)
      {
         bool hideCursorDuringCapture = false;

         // Deliberately avoid passing the screen widget. Slate will cache an FReply event that will re-focus the screen widget after a delay, 
         // which stomps our attempts to dynamically restore focus in UTATScreenWidget::_ReassignFocus()
         UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(&localPC, nullptr, EMouseLockMode::DoNotLock, hideCursorDuringCapture);
         UE_LOG(LogTATScreenMgr, Warning, TEXT("_RecalculateInputState: Set input mode to GameAndUI"));
      }
   }
   else
   {
      // Reset the cursor to the center of the screen to prevent camera sweeps based on where the mouse was last on UI
      int32 sizeX, sizeY;
      localPC.GetViewportSize(sizeX, sizeY);
      const FVector2D viewportSize = FVector2D(sizeX, sizeY);
      const FVector2D viewportCenter = viewportSize / 2.0f;
      localPC.SetMouseLocation(viewportCenter.X, viewportCenter.Y);

      UWidgetBlueprintLibrary::SetInputMode_GameOnly(&localPC);
      UE_LOG(LogTATScreenMgr, Warning, TEXT("_RecalculateInputState: Set input mode to GameOnly"));
   }
}

ATATPlayerController& UTATScreenMgr::_GetOwnerPC() const
{
   ATATPlayerController* pc = Cast<ATATPlayerController>(GetOwner());
   check(pc);  // we only exist if our local player controller spawned us
   return *pc;
}

void UTATScreenMgr::_LoadAkStates()
{
   if (!_ShouldRefreshAkState())
   {
      return;
   }

   TArray<FSoftObjectPath> pathsToLoad;
   auto addPath = [&pathsToLoad](const TSoftObjectPtr<UAkStateValue>& state) {
      if (!state.IsNull())
      {
         pathsToLoad.AddUnique(state.ToSoftObjectPath());
      }
   };
   const UTATUIDeveloperSettings* tatUISettings = UTATUIDeveloperSettings::GetTATUISettings();
   addPath(tatUISettings->AkStateWhileStackEmpty);
   addPath(tatUISettings->AkStateWhileStackPopulated);

   UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), [weakThis = MakeWeakObjectPtr(this)] {
      if (UTATScreenMgr* self = weakThis.Get())
      {
         const UTATUIDeveloperSettings* tatUISettings = UTATUIDeveloperSettings::GetTATUISettings();
         self->_akStateWhileStackPopulated = tatUISettings->AkStateWhileStackPopulated.Get();
         self->_akStateWhileStackEmpty = tatUISettings->AkStateWhileStackEmpty.Get();
         self->_RefreshAkState();
      }
   });
}

void UTATScreenMgr::_RefreshAkState()
{
   if (!_ShouldRefreshAkState())
   {
      return;
   }

   const UTATUIDeveloperSettings* tatUISettings = UTATUIDeveloperSettings::GetTATUISettings();
   const UAkStateValue* akState = nullptr;
   if (_screenStack.Num() > 0)
   {
      const FTATScreenMgrEntry& topEntry = _screenStack.Last();
      check(topEntry.Widget != nullptr);
      akState = topEntry.Widget->OverrideAkState ? topEntry.Widget->AkStateWhileOnTopOfStackOverride : _akStateWhileStackPopulated;
   }
   else
   {
      akState = _akStateWhileStackEmpty;
   }
   
   if (akState)
   {
      UAkGameplayStatics::SetState(akState);
   }
}

bool UTATScreenMgr::_ShouldRefreshAkState() const
{
   // Skip state assignment outside of gameplay maps
   const ATATWorldSettings& tatWorldSettings = ATATWorldSettings::Get(this);
   switch (tatWorldSettings.MapType)
   {
   case ETATMapType::Developer:
   case ETATMapType::Transition:
      return false;
   default:
      return true;
   }
}

void UTATScreenMgr::_OnMoviePlaybackFinished()
{
   if (_screenStack.Num() > 0)
   {
      UE_LOG(LogTATScreenMgr, Verbose, TEXT("Movie playback finished, refreshing focus on the top of our stack"));
      const FTATScreenMgrEntry& lastEntry = _screenStack.Last();
      check(lastEntry.Widget != nullptr);
      lastEntry.Widget->HandleOnTopOfStackWhenFocusRefreshed();
   }
}
