// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "UI/TATNavigationConfig.h"
#include "UI/TATScreenMetadata.h"
#include "UI/TATScreenMgr.h"
#include "UI/TATScreenWidget.h"
#include "UI/TATUIDeveloperSettings.h"

// ue4
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"
#include "EnhancedInputSubsystems.h"

#include "Engine/Console.h"
#include "Engine/Engine.h"
#include "Widgets/SViewport.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNavigationConfig)

DEFINE_LOG_CATEGORY_STATIC(LogTATNavigationConfig, Log, All);

TSharedPtr<FTATNavigationConfig> FTATNavigationConfig::_sInstance = nullptr;

namespace TATNavigationConfigCVars
{
   static int AllowViewportFocus = 0;
   FAutoConsoleVariableRef CVarAllowViewportFocus(
      TEXT("TAT.UI.AllowViewportFocus"),
      AllowViewportFocus,
      TEXT("Does FTATNavigationConfig allow focusing the viewport?"),
      ECVF_Default);
}

FTATNavigationConfig::FTATNavigationConfig()
{
   // Disabling navigation with the tab key because we use that for other stuff (and it feels weird with buttons)
   bTabNavigation = false;

   // Load and initialize extended navigation bindings from developer settings
   UTATUIDeveloperSettings* tatUIDeveloperSettings = UTATUIDeveloperSettings::GetTATUISettings();
   for (const TPair<EUIExtendedNavigationAction, struct FExtendedNavigationActionBindings>& pair : tatUIDeveloperSettings->ExtendedNavigationBindings)
   {
      EUIExtendedNavigationAction extendedNavigationAction = pair.Key;
      if (pair.Value.InputAction.IsValid() == false)
      {
         const TArray<FKey>& keys = pair.Value.KeyBindings;
         for (const FKey& key : keys)
         {
            _extendedKeyEventRules.Emplace(key, extendedNavigationAction);
         }
      }
   }
}

// Notified when navigation has caused a widget change to occur
void FTATNavigationConfig::OnNavigationChangedFocus(TSharedPtr<SWidget> oldWidget, TSharedPtr<SWidget> newWidget, FFocusEvent focusEvent)
{
   // super (doesn't do anything unless we end up setting up a base class of ours below this)
   FNavigationConfig::OnNavigationChangedFocus(oldWidget, newWidget, focusEvent);

   UE_LOG(LogTATNavigationConfig, Verbose, TEXT("Changing focus from widget of type %s to widget of type %s")
      , oldWidget ? *oldWidget->GetTypeAsString() : TEXT("NULL")
      , newWidget ? *newWidget->GetTypeAsString() : TEXT("NULL"));

   // NOTE: Allowing this on all platforms because an xbox does allow you to hook up a mouse/keyboard to it, and, well, it
   // shouldn't hurt the other platforms anyway.  Could revisit or change the CVar on various platforms

   if (UTATScreenMgr* screenMgr = _TryGetTATScreenMgr())
   {
      if (screenMgr->GetNumScreensInStack() > 0 && oldWidget.IsValid())
      {
         const bool isGameViewport = newWidget == FSlateApplication::Get().GetGameViewport();

         if (isGameViewport)
         {
#if WITH_EDITOR
            check(GEngine != nullptr);
            // make sure the console isn't open before reverting viewport focus, since viewport needs focus for handling console input
            if (const UWorld* world = GEngine->GetCurrentPlayWorld())
            {
               const UGameViewportClient* gameViewport = world->GetGameViewport();
               check(IsValid(gameViewport));

               const UConsole* console = gameViewport->ViewportConsole;
               check(IsValid(console));

               // allow focus change to proceed if console is open
               // we can't rely on UConsole::ConsoleActive(), since UConsole::OnConsoleActivationStateChanged is called before the underlying state is actually updated
               if (console->bCaptureKeyInput)
               {
                  UE_LOG(LogTATNavigationConfig, Verbose, TEXT("* OnNavigationChangedFocus() Detected open console - allowing focus change to viewport"), focusEvent.GetUser());
                  return;
               }
            }
#endif
            // don't allow focusing the viewport on PC while we have screens in our screen stack, keep focus on screens/widgets for gamepad navigation
            if (!TATNavigationConfigCVars::AllowViewportFocus)
            {
               UE_LOG(LogTATNavigationConfig, Verbose, TEXT("New widget of type %s is a viewport, changing focus back to the old widget of type %s")
                  , newWidget ? *newWidget->GetTypeAsString() : TEXT("NULL")
                  , oldWidget ? *oldWidget->GetTypeAsString() : TEXT("NULL"));

               FSlateApplication::Get().SetUserFocus(focusEvent.GetUser(), oldWidget);
            }
         }
         else
         {
            // prevent focus from reaching widgets below the top screen
            UTATScreenWidget* newScreenWidget = _TryFindOwningScreenWidget(newWidget);
            if (IsValid(newScreenWidget))
            {
               UE_LOG(LogTATNavigationConfig, VeryVerbose, TEXT("New widget is owned by screen %s"), *newScreenWidget->GetName());
               if (newScreenWidget == screenMgr->GetTopScreen())
               {
                  // Cache focused widget on the screen, so focus can be restored after another screen is pushed/popped from the stack
                  newScreenWidget->OnScreenReceiveFocus(newWidget, focusEvent.GetUser());
               }
               else
               {
                  UE_LOG(LogTATNavigationConfig, Verbose, TEXT("Attempting to focus widget below top screen, restoring focus to previous widget"));
                  FSlateApplication::Get().SetUserFocus(focusEvent.GetUser(), oldWidget);
               }
            }
         }
      }
   }
}

EUIExtendedNavigationAction FTATNavigationConfig::GetExtendedNavigationActionFromKey(const FKeyEvent& InKeyEvent, const UTATUserWidget* widget) const
{
   const ULocalPlayer* owningPlayer = widget->GetOwningLocalPlayer();
   if (const UCommonInputSubsystem* commonInputSubsystem = UCommonInputSubsystem::Get(owningPlayer))
   {
      const UTATUIDeveloperSettings* tatUIDeveloperSettings = UTATUIDeveloperSettings::GetTATUISettings();
      for (const TPair<EUIExtendedNavigationAction, struct FExtendedNavigationActionBindings>& pair : tatUIDeveloperSettings->ExtendedNavigationBindings)
      {
         if (pair.Value.InputAction.IsValid() == false)
            continue;
         
         const FKey boundKey = CommonUI::GetFirstKeyForInputType(owningPlayer, commonInputSubsystem->GetCurrentInputType(), pair.Value.InputAction.Get());
         if (InKeyEvent.GetKey() == boundKey)
            return pair.Key;
      }
   }
   // Fall back after checking input actions (which may have been rebound)
   if (const EUIExtendedNavigationAction* Rule = _extendedKeyEventRules.Find(InKeyEvent.GetKey()))
   {
      return *Rule;
   }
   return EUIExtendedNavigationAction::Invalid;
}

TSharedRef<FTATNavigationConfig> FTATNavigationConfig::GetInstance()
{
   if (!_sInstance)
   {
      _sInstance = MakeShared<FTATNavigationConfig>();
   }
   check(_sInstance)
   return _sInstance.ToSharedRef();
}

UTATScreenMgr* FTATNavigationConfig::_TryGetTATScreenMgr() const
{
   // Engine comment about this function:
   // Tries to find the currently active primary Game or Play in Editor world, returning null if it is ambiguous.
   // This should only be called if you do not have a reliable world context object to use.

   // Chooch comment about this function: seems to work in multiplayer pie pretty well?  We need to query the screen manager
   // for state and it exists per-window but this object only exists once for the whole slate app.
   if (UWorld* world = GEngine->GetCurrentPlayWorld())
   {
      return UTATScreenMgr::TryGetScreenManager(world);
   }
   return nullptr;
}

UTATScreenWidget* FTATNavigationConfig::_TryFindOwningScreenWidget(TSharedPtr<SWidget> widget) const
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_FTATNavigationConfig_TryFindOwningScreenWidget);

   if (!widget)
   {
      return nullptr;
   }

   TSharedPtr<SWidget> parent = widget->GetParentWidget();
   while (parent)
   {
      TSharedPtr<FTATScreenMetadata> metadata = parent->GetMetaData<FTATScreenMetadata>();
      if (metadata)
      {
         return metadata->ScreenObject.Get();
      }
      parent = parent->GetParentWidget();
   }
   return nullptr;
}
