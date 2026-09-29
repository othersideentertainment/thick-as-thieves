// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tutorial/TATCommonTutorialActions.h"

// tat
#include "Analytics/TATAnalyticsManager.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "Quests/TATContractState.h"
#include "SaveGame/TATSaveGame.h"
#include "Tutorial/TATTutorialRespawnPoint.h"
#include "Tutorial/TATTutorialParams.h"
#include "Tutorial/TATTutorialValidationParams.h"
#include "UI/Tutorial/TATHUDHighlightWidget.h"
#include "UI/TATActivatableWidget.h"
#include "UI/TATHUD.h"
#include "UI/TATHUDExtraObjectiveInterface.h"
#include "UI/TATLayoutWidget.h"
#include "UI/TATScreenMgr.h"
#include "UI/TATScreenWidget.h"
#include "UI/TATUIZOrder.h"
#include "Variation/Clues/TATClueActorSpawner.h"
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/TATCharacterSpawner.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCommonTutorialActions)

namespace TutorialActionHelpers
{
   static FString GetNameForSoftActor(const TSoftObjectPtr<AActor>& softActor)
   {
      if(const AActor* actor = softActor.Get())
      {
         return actor->GetActorNameOrLabel();
      }
      else
      {
         return softActor.ToSoftObjectPath().GetSubPathString();
      }
   }
   
   static UTATAnalyticsManager* GetTATAnalyticsManager(const FTATTutorialParams& params)
   {
      UGameInstance* gameInstance = params.World->GetGameInstance();
      check(gameInstance);
      return gameInstance->GetSubsystem<UTATAnalyticsManager>();
   }
   static int GetZOrder(ETATTutorialUISort sort)
   {
      switch(sort)
      {
      case ETATTutorialUISort::Normal:
      default:
         return 0;
      case ETATTutorialUISort::AboveCommonUI:
         return TATUIZOrder::AboveCommonUI;
      }
   }
}

void UTATTutorialAction_InstantBlueprintBase::RunInstant(const FTATTutorialParams& params) const
{
   BP_Run(params);
}

FString UTATTutorialAction_InstantBlueprintBase::GetDebugName() const
{
   return BP_GetDebugName();
}

#if WITH_EDITOR
void UTATTutorialAction_InstantBlueprintBase::Validate(const FTATTutorialValidationParams& params) const
{
   BP_Validate(params);
}
#endif

FString UTATTutorialAction_InstantBlueprintBase::BP_GetDebugName_Implementation() const
{
   return GetClass()->GetName();
}

FTATTutorialActionHandle UTATTutorialAction_CleanupBlueprintBase::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   UObject* cleanupParam = nullptr;
   const bool shouldCleanup = BP_Run(params, cleanupParam);

   next();

   if (!shouldCleanup)
   {
      return {};
   }
   
   return FTATTutorialActionHandle {
      .CancelAction = [weakSelf = MakeWeakObjectPtr(this), weakParam = MakeWeakObjectPtr(cleanupParam)]()
      {
         if (const UTATTutorialAction_CleanupBlueprintBase* self = weakSelf.Get())
         {
            self->BP_Cleanup(weakParam.Get());
         }
      }
   };
}

FString UTATTutorialAction_CleanupBlueprintBase::GetDebugName() const
{
   return BP_GetDebugName();
}

#if WITH_EDITOR
void UTATTutorialAction_CleanupBlueprintBase::Validate(const FTATTutorialValidationParams& params) const
{
   BP_Validate(params);
}
#endif

FString UTATTutorialAction_CleanupBlueprintBase::BP_GetDebugName_Implementation() const
{
   return GetClass()->GetName();
}

void UTATTutorialAction_SpawnActor::RunInstant(const FTATTutorialParams& params) const
{
   check(params.World);

   UClass* actorClass = ActorClassToSpawn.Get();
   if (actorClass == nullptr)
   {
      // LOG error
      return;
   }

   const AActor* spawnMarkerActor = ActorToSpawnAt.Get();
   if (spawnMarkerActor == nullptr)
   {
      // LOG error
      return;
   }
   
   params.World->SpawnActor(actorClass, &spawnMarkerActor->GetActorTransform());
}

FString UTATTutorialAction_SpawnActor::GetDebugName() const
{
   return FString::Printf(TEXT("Spawn %s at %s"),
      *GetNameSafe(ActorClassToSpawn),
      *TutorialActionHelpers::GetNameForSoftActor(ActorToSpawnAt));
}

#if WITH_EDITOR
void UTATTutorialAction_SpawnActor::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(ActorToSpawnAt, TEXTVIEW("ActorSpawnAt"));
   params.RequireClass(ActorClassToSpawn, TEXTVIEW("ActorClassToSpawn"));
}
#endif

FTATTutorialActionHandle UTATTutorialAction_SpawnActorDuringStep::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   check(params.World);

   // always immediate
   next();

   UClass* actorClass = ActorClassToSpawn.Get();
   if (actorClass == nullptr)
   {
      // LOG error
      return {};
   }

   const AActor* spawnMarkerActor = ActorToSpawnAt.Get();
   if (spawnMarkerActor == nullptr)
   {
      // LOG error
      return {};
   }
   
   AActor* spawnedActor = params.World->SpawnActor(actorClass, &spawnMarkerActor->GetActorTransform());

   return FTATTutorialActionHandle {
      .CancelAction = [weakActor = MakeWeakObjectPtr(spawnedActor)]
      {
         if(AActor* actor = weakActor.Get())
         {
            actor->Destroy();
         }
      }
   };
   
}

FString UTATTutorialAction_SpawnActorDuringStep::GetDebugName() const
{
   return FString::Printf(TEXT("Spawn (temp) %s at %s"),
      *GetNameSafe(ActorClassToSpawn),
      *TutorialActionHelpers::GetNameForSoftActor(ActorToSpawnAt));
}

#if WITH_EDITOR
void UTATTutorialAction_SpawnActorDuringStep::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(ActorToSpawnAt, TEXTVIEW("ActorSpawnAt"));
   params.RequireClass(ActorClassToSpawn, TEXTVIEW("ActorClassToSpawn"));
}
#endif

FTATTutorialActionHandle UTATTutorialAction_Wait::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   check(params.World);

   FTimerHandle timerHandle;
   params.World->GetTimerManager().SetTimer(timerHandle, MoveTemp(next), DelaySeconds, false);

   // Just in case the set timer fails
   if(!timerHandle.IsValid())
   {
      // LOG error
      next();
   }

   // NB: This cancel action should be redundant, as the system will ignore calls to next
   //     if no longer waiting for that action. But might as well, I guess
   return FTATTutorialActionHandle {
      .CancelAction = [timerHandle, weakWorld = MakeWeakObjectPtr(params.World)] ()
         {
            if(UWorld* world = weakWorld.Get())
            {
               FTimerHandle timerHandleCopy = timerHandle;
               world->GetTimerManager().ClearTimer(timerHandleCopy);
            }
         }
   };
}

FString UTATTutorialAction_Wait::GetDebugName() const
{
   return FString::Printf(TEXT("Wait %.1fs"), DelaySeconds);
}

FTATTutorialActionHandle UTATTutorialAction_ProgressionAnalyticsEvent::Run(const FTATTutorialParams& params,
   TFunction<void()>&& next) const
{
   TWeakObjectPtr<UTATAnalyticsManager> analyticsManager = TutorialActionHelpers::GetTATAnalyticsManager(params);
   if (analyticsManager.IsValid())
   {
      analyticsManager->OnFTUEProgressionEventStarted(_AnalyticsSectionName, _AnalyticsEventName);
   }
   next();  
   return {
      .CancelAction = [analyticsManager, eventName = _AnalyticsEventName, eventSection = _AnalyticsSectionName] ()
      {
         if (analyticsManager.IsValid())
         {
            analyticsManager->OnFTUEProgressionEventCompleted(eventSection, eventName);
         }
      }
   };
}

FString UTATTutorialAction_ProgressionAnalyticsEvent::GetDebugName() const
{
   return FString::Printf(TEXT("Analytics Progression event Section [%s] Event [%s] during stage"), *_AnalyticsSectionName, *_AnalyticsEventName);
}

FTATTutorialActionHandle UTATTutorialAction_ConditionalDesignAnalyticsEvent_GameplayEvent::Run(
   const FTATTutorialParams& params,
   TFunction<void()>&& next) const
{
   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(params.Character);
   if (asc == nullptr)
   {
      // LOG error
      next();
      return {};
   }

   TWeakObjectPtr<UTATAnalyticsManager> analyticsManager = TutorialActionHelpers::GetTATAnalyticsManager(params);
   
   FDelegateHandle handle = asc->GenericGameplayEventCallbacks.FindOrAdd(_EventTagToMatch).AddLambda([analyticsManager, analyticsTag = _AnalyticsEventName](const FGameplayEventData* eventData)
   {
      if (analyticsManager.IsValid())
      {
            analyticsManager->OnDesignEvent(analyticsTag);
      }
   });
   next();
   
   return FTATTutorialActionHandle {
      .CancelAction = [tagToMatch = _EventTagToMatch, handle, asc = MakeWeakObjectPtr(asc)]()
      {
         if (asc.IsValid())
         {
            asc->GenericGameplayEventCallbacks.FindOrAdd(tagToMatch).Remove(handle);
         }
      }
   };
}

FString UTATTutorialAction_ConditionalDesignAnalyticsEvent_GameplayEvent::GetDebugName() const
{
   return FString::Printf(TEXT("Trigger Analytics if event %s is triggered"), *_EventTagToMatch.ToString());
}

FTATTutorialActionHandle UTATTutorialAction_ConditionalDesignAnalyticsEvent_GameplayTag::Run(const FTATTutorialParams& params,
   TFunction<void()>&& next) const
{
   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(params.Character);
   if (asc == nullptr)
   {
      // LOG error
      next();
      return {};
   }

   TWeakObjectPtr<UTATAnalyticsManager> analyticsManager = TutorialActionHelpers::GetTATAnalyticsManager(params);
   FDelegateHandle handle = asc->RegisterGameplayTagEvent(_TagToMatch).AddLambda(
      [analyticsManager, analyticsTag = _AnalyticsEventName, triggerOnAdd = _TriggerOnAdd](
      const FGameplayTag tag,
      const int32 newTagCount)
   {
      if (analyticsManager.IsValid())
      {
         if (triggerOnAdd && newTagCount > 0)
         {
            analyticsManager->OnDesignEvent(analyticsTag);
         }
         else if (triggerOnAdd == false && newTagCount == 0)
         {
            analyticsManager->OnDesignEvent(analyticsTag);
         }
      }
   });
   next();
   
   return FTATTutorialActionHandle {
      .CancelAction = [tagToMatch = _TagToMatch, handle, asc = MakeWeakObjectPtr(asc)]()
      {
         if (asc.IsValid())
         {
            asc->UnregisterGameplayTagEvent(handle, tagToMatch);
         }
      }
   };
}

FString UTATTutorialAction_ConditionalDesignAnalyticsEvent_GameplayTag::GetDebugName() const
{   
   return FString::Printf(TEXT("Trigger Analytics if %s is %s"), *_TagToMatch.ToString(), _TriggerOnAdd ? TEXT("added") : TEXT("removed"));
}


FTATTutorialActionHandle UTATTutorialAction_AddOverlayUIBase::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   check(params.Controller);

   UUserWidget* widget = CreateWidget(params.Controller);
   if(widget == nullptr)
   {
      // LOG error
      next();
      return {};
   }

   // May ultimately need more than this depending on if it plays nice with ordering,
   // but starting here for simplicity
   widget->AddToViewport(TutorialActionHelpers::GetZOrder(SortOrder));
   
   next();

   return FTATTutorialActionHandle {
      .CancelAction = [weakWidget = MakeWeakObjectPtr(widget)] ()
      {
         if(UUserWidget* widget = weakWidget.Get())
         {
            widget->RemoveFromParent();
            widget->MarkAsGarbage();
         }
      }
   };
}

FString UTATTutorialAction_AddOverlayUIBase::GetDebugName() const
{
   return _GetDebugName();
}

FTATTutorialActionHandle UTATTutorialAction_ShowScreenBase::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   // taking a similar approach to UTATScreenQueueAction::Run
   // except don't bother loading from a soft reference, since the tutorial
   // can just keep it loaded (but don't do this for more one-off stuff like
   // infographics)
   //
   // Can revisit, it just makes the cancellation a bit more complicated
   check(params.Controller);
  
   UTATScreenMgr* screenMgr = UTATScreenMgr::TryGetScreenManager(params.Controller);
   if (screenMgr == nullptr)
   {
      next();
      return {};
   }

   UTATScreenWidget* screen = CreateScreenWidget(params.Controller);
   if (screen == nullptr)
   {
      next();
      return {};
   }
   
   screenMgr->AddScreen(screen, [next = MoveTemp(next)](UTATScreenWidget* screen) { next(); });
    
   TWeakObjectPtr<UTATAnalyticsManager> analyticsManager = _ShouldTriggerAnalyticsEvent && _AnalyticsEventName.IsEmpty() == false
                                                              ? TutorialActionHelpers::GetTATAnalyticsManager(params)
                                                              : nullptr;
   if (analyticsManager.IsValid())
   {
      analyticsManager->OnFTUEProgressionEventStarted(_AnalyticsSectionName, _AnalyticsEventName);
   }
   
   return {
      .CancelAction = [
         weakScreen = MakeWeakObjectPtr(screen),
         analyticsManager,
         eventName = _AnalyticsEventName,
         eventSection = _AnalyticsSectionName] ()
      {
         if (analyticsManager.IsValid())
         {
            analyticsManager->OnFTUEProgressionEventCompleted(eventSection, eventName);
         }
         if(UTATScreenWidget* screen = weakScreen.Get())
         {
            screen->RemoveScreen();
         }
      }
   };
}

FString UTATTutorialAction_ShowScreenBase::GetDebugName() const
{
   return _GetDebugName();
}

FTATTutorialActionHandle UTATTutorialAction_SwallowBackAction::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   TSubclassOf<UTATActivatableWidget> loadedWidgetClass = WidgetClass.Get();
   // If not loaded, not active
   if(loadedWidgetClass == nullptr)
   {
      next();
      return {};
   }
   
   UTATActivatableWidget* widget = [this, &params, &loadedWidgetClass] () -> UTATActivatableWidget*
   {
      for (FThreadSafeObjectIterator iter(loadedWidgetClass, false, RF_ClassDefaultObject); iter; ++iter)
      {
         UTATActivatableWidget* widget = Cast<UTATActivatableWidget>(*iter);
         if(widget && widget->GetWorld() == params.World)
         {
            return widget;
         }
      }
      return nullptr;
   }();
   if(widget == nullptr)
   {
      next();
      return {};
   }

   const bool previouslySuppressed = widget->SwallowBackAction;
   widget->SwallowBackAction = true;
   next();
   return {
      .CancelAction = [weakWidget = MakeWeakObjectPtr(widget), previouslySuppressed] ()
      {
         if(UTATActivatableWidget* widget = weakWidget.Get())
         {
            widget->SwallowBackAction = previouslySuppressed;
         }
      }
   };
}

FString UTATTutorialAction_SwallowBackAction::GetDebugName() const
{
   return FString::Printf(TEXT("SwallowBackAction %s"), *WidgetClass.ToSoftObjectPath().GetAssetName());
}

#if WITH_EDITOR
void UTATTutorialAction_SwallowBackAction::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftClass(WidgetClass, TEXTVIEW("WidgetClass"));
}
#endif

UUserWidget* UTATTutorialAction_ShowHUDHighlight::CreateWidget_Implementation(APlayerController* controller) const
{
   const ATATHUD* hud = controller->GetHUD<ATATHUD>();
   if(hud == nullptr)
   {
      return nullptr;
   }

   UWidget* targetWidget = hud->GetHudElementByTag(HudElement);
   if(targetWidget == nullptr)
   {
      return nullptr;
   }

   UTATHUDHighlightWidget* highlightWidget = ::CreateWidget<UTATHUDHighlightWidget>(controller, HighlightWidgetClass);
   if(highlightWidget == nullptr)
   {
      return nullptr;
   }

   highlightWidget->SetTarget(targetWidget);
   return highlightWidget;
}

FString UTATTutorialAction_ShowHUDHighlight::GetDebugName() const
{
   return FString::Printf(TEXT("Show HUD Highlight: %s"), *HudElement.ToString());
}

#if WITH_EDITOR
void UTATTutorialAction_ShowHUDHighlight::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireClass(HighlightWidgetClass, TEXTVIEW("HighlightWidgetClass"));
}
#endif

FString UTATTutorialAction_ShowScreenBase::_GetDebugName_Implementation() const
{
   return GetClass()->GetName();
}

UTATScreenWidget* UTATTutorialAction_ShowScreenBase::CreateScreenWidget_Implementation(APlayerController* controller) const
{
   return nullptr;
}

FTATTutorialActionHandle UTATTutorialAction_ApplyGameplayEffect::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   check(params.PlayerState);

   // continuing regardless
   next();

   if (!IsValid(Effect))
   {
      // LOG error
      return {};
   }

   UAbilitySystemComponent* asc = params.PlayerState->GetAbilitySystemComponent();
   check(asc);

   constexpr float level = 0;
   FActiveGameplayEffectHandle effectHandle = asc->ApplyGameplayEffectToSelf(Effect.GetDefaultObject(), level, asc->MakeEffectContext());

   if (effectHandle.IsValid() && RemoveOnStepEnd)
   {
      return FTATTutorialActionHandle {
         .CancelAction = [effectHandle] ()
         {
            if (UAbilitySystemComponent* asc = effectHandle.GetOwningAbilitySystemComponent())
            {
               asc->RemoveActiveGameplayEffect(effectHandle, 1);
            }
         }
      };
   }
   return {};
}

FString UTATTutorialAction_ApplyGameplayEffect::GetDebugName() const
{
   return FString::Printf(TEXT("Apply %s%s"), *GetNameSafe(Effect), RemoveOnStepEnd ? TEXT(" (RemovedAtStepEnd)") : TEXT(""));
}

#if WITH_EDITOR
void UTATTutorialAction_ApplyGameplayEffect::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireClass(Effect, TEXTVIEW("Effect"));
}
#endif

FTATTutorialActionHandle UTATTutorialAction_RemoveGameplayEffect::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   check(params.PlayerState);

   // continuing regardless
   next();

   UAbilitySystemComponent* asc = params.PlayerState->GetAbilitySystemComponent();
   check(asc);
   asc->RemoveActiveGameplayEffectBySourceEffect(Effect, nullptr, StacksToRemove);

   return {};
}

FString UTATTutorialAction_RemoveGameplayEffect::GetDebugName() const
{
   return FString::Printf(TEXT("Remove %s"), *GetNameSafe(Effect));
}

#if WITH_EDITOR
void UTATTutorialAction_RemoveGameplayEffect::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireClass(Effect, TEXTVIEW("Effect"));
}
#endif

void UTATTutorialAction_SpawnReadableClue::RunInstant(const FTATTutorialParams& params) const
{
   const AActor* clueSpawnerActor = ClueSpawner.Get();
   if (clueSpawnerActor == nullptr)
   {
      // LOG error
      return;
   }

   UTATClueActorSpawnerComponent* spawnerComponent = clueSpawnerActor->GetComponentByClass<UTATClueActorSpawnerComponent>();
   if (spawnerComponent == nullptr)
   {
      // LOG error
      return;
   }

   FTATClueContext context;
   Clue.ApplyToSpawner(spawnerComponent, context);
}

FString UTATTutorialAction_SpawnReadableClue::GetDebugName() const
{
   return FString::Printf(TEXT("Spawn Clue at %s"), *TutorialActionHelpers::GetNameForSoftActor(ClueSpawner));
}

#if WITH_EDITOR
void UTATTutorialAction_SpawnReadableClue::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(ClueSpawner, TEXTVIEW("ClueSpawner"));
}
#endif

FString UTATTutorialAction_AddOverlayUIBase::_GetDebugName_Implementation() const
{
   return TEXT("UI Overlay");
}

UUserWidget* UTATTutorialAction_AddOverlayUIBase::CreateWidget_Implementation(APlayerController* controller) const
{
   return nullptr;
}

FTATTutorialActionHandle UTATTutorialAction_SetObjectiveText::Run(const FTATTutorialParams& params,
   TFunction<void()>&& next) const
{
   // always continue
   next();

   AHUD* hud = params.Controller->GetHUD();
   if(hud == nullptr || !hud->Implements<UTATHUDExtraObjectiveInterface>())
   {
      return {};
   }

   ITATHUDExtraObjectiveInterface::Execute_SetExtraHudObjective(hud, ObjectiveText);

   if(RemoveOnStepEnd)
   {
      return FTATTutorialActionHandle {
         .CancelAction = [weakHud = MakeWeakObjectPtr(hud)] ()
         {
            if (AHUD* hud = weakHud.Get())
            {
               check(hud->Implements<UTATHUDExtraObjectiveInterface>());
               ITATHUDExtraObjectiveInterface::Execute_SetExtraHudObjective(hud, FText::GetEmpty());
            }
         }
      };
   }
   else
   {
      return {};
   }
}

FString UTATTutorialAction_SetObjectiveText::GetDebugName() const
{
   return FString::Printf(TEXT("%s: %s%s"),
      ObjectiveText.IsEmpty() ? TEXT("ClearObjectiveText") : TEXT("SetObjectiveText"),
      *ObjectiveText.ToString(),
      RemoveOnStepEnd ? TEXT(" (RemovedAtStepEnd)") : TEXT(""));
}

FTATTutorialActionHandle UTATTutorialAction_SetActorHidden::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   // always continue
   next();

   AActor* actorToChange = Actor.Get();
   if (actorToChange == nullptr)
   {
      return {};
   }

   const bool originallyHidden = actorToChange->IsHidden();
   actorToChange->SetActorHiddenInGame(Visibility == ETATTutorialActorVisibility::Hidden);

   if (RevertOnStepEnd)
   {
      return FTATTutorialActionHandle {
         .CancelAction = [weakActor = MakeWeakObjectPtr(actorToChange), originallyHidden] ()
         {
            if (AActor* actor = weakActor.Get())
            {
               actor->SetActorHiddenInGame(originallyHidden);
            }
         }
      };
   }

   return {};
}

FString UTATTutorialAction_SetActorHidden::GetDebugName() const
{
   return FString::Printf(TEXT("%s: %s%s"),
      Visibility == ETATTutorialActorVisibility::Visible ? TEXT("ShowActor") : TEXT("HideActor"),
      *TutorialActionHelpers::GetNameForSoftActor(Actor),
      RevertOnStepEnd ? TEXT(" (RevertOnStepEnd)") : TEXT(""));
}

#if WITH_EDITOR
void UTATTutorialAction_SetActorHidden::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(Actor, TEXTVIEW("Actor"));
}
#endif



void UTATTutorialAction_SetRespawnPoint::RunInstant(const FTATTutorialParams& params) const
{
   check(params.Controller != nullptr);

   ATATTutorialRespawnPoint* respawnPoint = RespawnPoint.Get();
   if (!ensure(respawnPoint))
   {
      return;
   }
   
   ATATPlayerController* controller = CastChecked<ATATPlayerController>(params.Controller);
   controller->AuthoritySetRespawnPoint(respawnPoint);
}

FString UTATTutorialAction_SetRespawnPoint::GetDebugName() const
{
   return FString::Printf(TEXT("Set Respawn Point: %s"), *TutorialActionHelpers::GetNameForSoftActor(RespawnPoint));
}

#if WITH_EDITOR
void UTATTutorialAction_SetRespawnPoint::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(RespawnPoint, TEXTVIEW("RespawnPoint"));
}
#endif

void UTATTutorialAction_DestroyNPC::RunInstant(const FTATTutorialParams& params) const
{
   ATATCharacterSpawner* spawner = GuardSpawner.Get();
   if (spawner == nullptr)
   {
      return;
   }

   if (AActor* spawnedActor = spawner->GetSpawnedActor())
   {
      spawnedActor->Destroy();
   }
}

FString UTATTutorialAction_DestroyNPC::GetDebugName() const
{
   return FString::Printf(TEXT("Destroy NPC: %s"), *TutorialActionHelpers::GetNameForSoftActor(GuardSpawner));
}

#if WITH_EDITOR
void UTATTutorialAction_DestroyNPC::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(GuardSpawner, TEXTVIEW("GuardSpawner"));
}
#endif

FTATTutorialActionHandle UTATTutorialAction_Slomo::Run(const FTATTutorialParams& params, TFunction<void()>&& next) const
{
   // always continue
   next();

   params.World->GetWorldSettings()->SetTimeDilation(TimeDilation);
   return FTATTutorialActionHandle {
      .CancelAction = [weakWorld = MakeWeakObjectPtr(params.World)]()
      {
         if (UWorld* world = weakWorld.Get())
         {
            world->GetWorldSettings()->SetTimeDilation(1);
         }
      }
   };
}

FString UTATTutorialAction_Slomo::GetDebugName() const
{
   return FString::Printf(TEXT("Slomo %f"), TimeDilation);
}

void UTATTutorialAction_SetSavedFtueState::RunInstant(const FTATTutorialParams& params) const
{
   if (UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(params.World))
   {
      if (!OnlyIfLower || saveGame->GetFtueState() < NewState)
      {
         saveGame->SetFtueState(NewState);
      }
   }
}

FString UTATTutorialAction_SetSavedFtueState::GetDebugName() const
{
   return FString::Printf(TEXT("Save FTUE Checkpoint: %s"), *StaticEnum<ETATSavedFtueState>()->GetValueOrBitfieldAsString(NewState));
}

void UTATTutorialAction_SetContractState::RunInstant(const FTATTutorialParams& params) const
{
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(params.World))
   {
      return save->SetContractState(Contract, NewState);
   }
}

FString UTATTutorialAction_SetContractState::GetDebugName() const
{
   return FString::Printf(TEXT("SetConstractState %s -> %s"), *Contract.ToString(),
      *StaticEnum<ETATContractState>()->GetNameStringByValue(static_cast<int64>(NewState)));
}

void UTATTutorialAction_UnlockContent::RunInstant(const FTATTutorialParams& params) const
{
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(params.World))
   {
      return save->AddUnlock(UnlockableTag);
   }
}

FString UTATTutorialAction_UnlockContent::GetDebugName() const
{
   return FString::Printf(TEXT("UnlockContent %s"), *UnlockableTag.ToString());
}

void UTATTutorialAction_SetSavedCharacter::RunInstant(const FTATTutorialParams& params) const
{
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(params.World))
   {
      return save->SetSelectedCharacter(FTATCharacterSaveId::FromCharacterType(Character));
   }
}

FString UTATTutorialAction_SetSavedCharacter::GetDebugName() const
{
   return FString::Printf(TEXT("SetSavedCharacter %s"), *StaticEnum<ETATCharacter>()->GetNameStringByValue(static_cast<int64>(Character)));
}

void UTATTutorialAction_GAFTUEStarted::RunInstant(const FTATTutorialParams& params) const
{
#if UE_SERVER
#else
   UGameInstance* gameInstance = params.World->GetGameInstance();
   check(gameInstance);
   gameInstance->GetSubsystem<UTATAnalyticsManager>()->OnFTUEStarted();
#endif
}

FString UTATTutorialAction_GAFTUEStarted::GetDebugName() const
{
   return FString::Printf(TEXT("GAFTUEStarted"));
}

void UTATTutorialAction_GAFTUECompleted::RunInstant(const FTATTutorialParams& params) const
{
#if UE_SERVER
#else
   UGameInstance* gameInstance = params.World->GetGameInstance();
   check(gameInstance);
   gameInstance->GetSubsystem<UTATAnalyticsManager>()->OnFTUECompleted();
#endif
}

FString UTATTutorialAction_GAFTUECompleted::GetDebugName() const
{
   return FString::Printf(TEXT("GAFTUECompleted"));
}

