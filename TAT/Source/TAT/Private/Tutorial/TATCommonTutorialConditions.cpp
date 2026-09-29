// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tutorial/TATCommonTutorialConditions.h"

// tat
#include "Interactables/TATLockableToggle.h"
#include "Interactables/TATSwingingDoor.h"
#include "Items/TATItemInfo.h"
#include "Loot/TATLootInventory.h"
#include "Player/TATPlayerState.h"
#include "Tutorial/TATTutorialParams.h"
#include "Tutorial/TATTutorialValidationParams.h"
#include "UI/TATLayoutSubsystem.h"
#include "UI/TATLayoutWidget.h"
#include "UI/TATScreenMgr.h"
#include "UI/TATScreenWidget.h"
#include "UI/Queue/TATUIQueue.h"
#include "Variation/Clues/TATLocalClueFactSubsystem.h"
#include "Variation/TATCharacterSpawner.h"

// ose
#include "AI/Alertness/OSEAlertnessInterface.h"
#include "Character/OSECharacterBase.h"
#include "Interactables/InteractorInterface.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"

// ue
#include "AbilitySystemComponent.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/Character.h"
#include "SaveGame/TATSaveGame.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCommonTutorialConditions)

namespace TutorialConditionHelpers
{
   template<typename T>
   static FString GetNameForClass(const TSoftClassPtr<T> actorClass)
   {
      return actorClass.ToSoftObjectPath().GetAssetName();
   }

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

   static FString FormatConditions(const TCHAR* label, TConstArrayView<TObjectPtr<UTATTutorialCondition>> conditions)
   {
      FString debugName = label;
      debugName.Append(TEXT(": "));
      for(const UTATTutorialCondition* condition : conditions)
      {
         if(condition)
         {
            debugName.Append(condition->GetDebugName());
            debugName.Append(TEXT(" "));
         }
      }
      return debugName;
   }

#if WITH_EDITOR
   static void ValidateConditions(TConstArrayView<TObjectPtr<UTATTutorialCondition>> conditions, const FTATTutorialValidationParams& params)
   {
      for(int i = 0; i < conditions.Num(); i++)
      {
         auto reportScope = [&params, i](const FText& message)
         {
            params.ReportError(FText::FormatOrdered(INVTEXT("[{0}] {1}"), i, message));
         };

         const UTATTutorialCondition* condition = conditions[i];
         if(condition == nullptr)
         {
            reportScope(INVTEXT("Condition is null"));
         }
         else
         {
            condition->Validate(FTATTutorialValidationParams {
               .World = params.World,
               .ErrorReporter = FTATTutorialValidationParams::FReportErrorRef(reportScope),
            });
         }
      }
   }
#endif
}

namespace TutorialCvars
{
   static TAutoConsoleVariable<bool> CVarDrawVolumes(
      TEXT("tat.Tutorial.DrawVolumes"),
      false,
      TEXT("If set to true, draws boxes of tutorial volumes when checking their condition")
      );
}

bool UTATTutorialCondition_BlueprintBase::IsMet(const FTATTutorialParams& params) const
{
   return BP_IsMet(params);
}

FString UTATTutorialCondition_BlueprintBase::GetDebugName() const
{
   FString debugName = BP_GetDebugName();
   return debugName.IsEmpty() ? Super::GetDebugName() : debugName;
}

#if WITH_EDITOR
void UTATTutorialCondition_BlueprintBase::Validate(const FTATTutorialValidationParams& params) const
{
   BP_Validate(params);
}
#endif

bool UTATTutorialCondition_HasTool::IsMet(const FTATTutorialParams& params) const
{
   // If tool is not loaded, then definitely don't have it
   UClass* toolClass = ToolClass.Get();
   if(toolClass == nullptr)
   {
      return false;
   }

   TScriptInterface<IToolSetInterface> toolSet = IToolSetInterface::GetToolSetFromActor(params.Character);
   return toolSet != nullptr && toolSet->HasToolClass(toolClass);
}

FString UTATTutorialCondition_HasTool::GetDebugName() const
{
   return FString::Printf(TEXT("HasTool %s"), *TutorialConditionHelpers::GetNameForClass(ToolClass));
}

#if WITH_EDITOR
void UTATTutorialCondition_HasTool::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftClass(ToolClass, TEXTVIEW("ToolClass"));
}
#endif



bool UTATTutorialCondition_AllOf::IsMet(const FTATTutorialParams& params) const
{
   for(const UTATTutorialCondition* condition : Conditions)
   {
      if(condition && !condition->IsMet(params))
      {
         return false;
      }
   }
   return true;
}

FString UTATTutorialCondition_AllOf::GetDebugName() const
{
   return TutorialConditionHelpers::FormatConditions(TEXT("AllOf"), Conditions);
}

#if WITH_EDITOR
void UTATTutorialCondition_AllOf::Validate(const FTATTutorialValidationParams& params) const
{
   TutorialConditionHelpers::ValidateConditions(Conditions, params);
}
#endif


bool UTATTutorialCondition_AnyOf::IsMet(const FTATTutorialParams& params) const
{
   for(const UTATTutorialCondition* condition : Conditions)
   {
      if(condition && condition->IsMet(params))
      {
         return true;
      }
   }
   return false;
}

FString UTATTutorialCondition_AnyOf::GetDebugName() const
{
   return TutorialConditionHelpers::FormatConditions(TEXT("AnyOf"), Conditions);
}

#if WITH_EDITOR
void UTATTutorialCondition_AnyOf::Validate(const FTATTutorialValidationParams& params) const
{
   TutorialConditionHelpers::ValidateConditions(Conditions, params);
}
#endif

bool UTATTutorialCondition_ToolEquipped::IsMet(const FTATTutorialParams& params) const
{
   TScriptInterface<IToolSetInterface> toolSet = IToolSetInterface::GetToolSetFromActor(params.Character);
   if (toolSet == nullptr)
   {
      return false;
   }

   // If tool is not loaded, then definitely don't have it
   UClass* toolClass = ToolClass.Get();
   if(toolClass == nullptr)
   {
      return !ShouldBeEquipped;
   }

   const UToolComponent* currentTool = toolSet->GetCurrentTool();
   const bool toolIsEquipped = currentTool && currentTool->IsA(toolClass);
   return toolIsEquipped == ShouldBeEquipped;
}

FString UTATTutorialCondition_ToolEquipped::GetDebugName() const
{
   return FString::Printf(TEXT("%s %s"),
      ShouldBeEquipped ? TEXT("ToolEquipped") : TEXT("ToolUnequipped"),
      *TutorialConditionHelpers::GetNameForClass(ToolClass));
}

#if WITH_EDITOR
void UTATTutorialCondition_ToolEquipped::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftClass(ToolClass, TEXTVIEW("ToolClass"));
}
#endif

bool UTATTutorialCondition_HasKey::IsMet(const FTATTutorialParams& params) const
{
   // If not loaded, then definitely don't have it
   UClass* itemClass = KeyType.Get();
   if (itemClass == nullptr)
   {
      return false;
   }

   if (params.PlayerState)
   {
      return params.PlayerState->GetTATItemInventory()->HasItemOfClass(itemClass);
   }

   return false;
}

FString UTATTutorialCondition_HasKey::GetDebugName() const
{
   return FString::Printf(TEXT("HasKey %s"), *TutorialConditionHelpers::GetNameForClass(KeyType));
}

#if WITH_EDITOR
void UTATTutorialCondition_HasKey::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftClass(KeyType, TEXTVIEW("KeyType"));
}
#endif

bool UTATTutorialCondition_HasLoot::IsMet(const FTATTutorialParams& params) const
{
   if (params.PlayerState == nullptr)
   {
      return false;
   }

   return params.PlayerState->GetLootInventoryComponent()->AuthorityHasLoot(Loot);
}

FString UTATTutorialCondition_HasLoot::GetDebugName() const
{
   return FString::Printf(TEXT("HasLoot %s"), *Loot.ToString());
}

#if WITH_EDITOR
void UTATTutorialCondition_HasLoot::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireTag(Loot.LootTag, TEXTVIEW("Loot"));
}
#endif

bool UTATTutorialCondition_IsInVolume::IsMet(const FTATTutorialParams& params) const
{
   const AActor* volumeActor = TriggerVolume.Get();
   if (volumeActor == nullptr)
   {
      // LOG error (unless expect to use during loading)
      return false;
   }

   if (params.Character == nullptr)
   {
      return false;
   }

#if ENABLE_DRAW_DEBUG
   if (TutorialCvars::CVarDrawVolumes.GetValueOnGameThread())
   {
      if (const UBoxComponent* box = volumeActor->GetComponentByClass<UBoxComponent>())
      {
         DrawDebugBox(params.World, box->GetComponentLocation(), box->GetScaledBoxExtent(), box->GetComponentQuat(), FColor::Blue,
            false, -1, SDPG_Foreground);
      }
   }
#endif

   return volumeActor->IsOverlappingActor(params.Character);
}

FString UTATTutorialCondition_IsInVolume::GetDebugName() const
{
   return FString::Printf(TEXT("InsideVolume %s"), *TutorialConditionHelpers::GetNameForSoftActor(TriggerVolume));
}

#if WITH_EDITOR
void UTATTutorialCondition_IsInVolume::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(TriggerVolume, TEXTVIEW("TriggerVolume"));
}
#endif

bool UTATTutorialCondition_HasClueFact::IsMet(const FTATTutorialParams& params) const
{
   const UTATLocalClueFactSubsystem* localClueFacts = params.World->GetSubsystem<UTATLocalClueFactSubsystem>();
   return localClueFacts && localClueFacts->IsFactKnown(FactTag);
}

FString UTATTutorialCondition_HasClueFact::GetDebugName() const
{
   return FString::Printf(TEXT("HasClueFact %s"), *FactTag.ToString());
}

#if WITH_EDITOR
void UTATTutorialCondition_HasClueFact::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireTag(FactTag, TEXTVIEW("FactTag"));
}
#endif

bool UTATTutorialCondition_NoOpenScreens::IsMet(const FTATTutorialParams& params) const
{
   const UTATScreenMgr* screenMgr = UTATScreenMgr::TryGetScreenManager(params.World);
   const bool tatScreensOkay = screenMgr && screenMgr->GetNumScreensInStack() == 0 && !UTATUIQueue::AreAnyRunning(params.World);
   if(!tatScreensOkay)
   {
      return false;
   }

   // also check common UI layouts
   // NOTE: this assumes that any active widget is modal/screen-like, which is true currently,
   //       but may not definitely remain so
   const UTATLayoutWidget* layoutWidget = UTATLayoutSubsystem::GetPlayerLayout(params.Controller->GetLocalPlayer());
   return layoutWidget && !layoutWidget->IsAnyWidgetActive();
}

FString UTATTutorialCondition_NoOpenScreens::GetDebugName() const
{
   return TEXT("NoOpenScreens");
}

bool UTATTutorialCondition_IsWidgetActive::IsMet(const FTATTutorialParams& params) const
{
   TSubclassOf<UCommonActivatableWidget> loadedWidgetClass = WidgetClass.Get();
   // If not loaded, not active
   if(loadedWidgetClass == nullptr)
   {
      return false;
   }

   for (FThreadSafeObjectIterator iter(loadedWidgetClass, false, RF_ClassDefaultObject); iter; ++iter)
   {
      const UCommonActivatableWidget* widget = Cast<UCommonActivatableWidget>(*iter);
      if(widget && widget->GetWorld() == params.World && widget->IsActivated())
      {
         return true;
      }
   }

   return false;
}

FString UTATTutorialCondition_IsWidgetActive::GetDebugName() const
{
   return FString::Printf(TEXT("WidgetIsActive %s"), *TutorialConditionHelpers::GetNameForClass(WidgetClass));
}

#if WITH_EDITOR
void UTATTutorialCondition_IsWidgetActive::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftClass(WidgetClass, TEXTVIEW("WidgetClass"));
}
#endif

bool UTATTutorialCondition_IsScreenOpen::IsMet(const FTATTutorialParams& params) const
{
   // If not loaded, not open
   const TSubclassOf<UTATScreenWidget> loadedScreenClass = ScreenClass.Get();
   if(loadedScreenClass == nullptr)
   {
      return false;
   }
   
   const UTATScreenMgr* screenMgr = UTATScreenMgr::TryGetScreenManager(params.World);
   if(!screenMgr)
   {
      return false;
   }

   // Currently check if the screen is the top screen, not sure if there is a usecase for caring if it is below a screen
   UTATScreenWidget* topScreen = screenMgr->GetTopScreen();
   return topScreen && topScreen->IsA(loadedScreenClass);
}

FString UTATTutorialCondition_IsScreenOpen::GetDebugName() const
{
   return FString::Printf(TEXT("IsScreenOpen %s"), *TutorialConditionHelpers::GetNameForClass(ScreenClass));
}

#if WITH_EDITOR
void UTATTutorialCondition_IsScreenOpen::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftClass(ScreenClass, TEXTVIEW("ScreenClass"));
}
#endif

bool UTATTutorialCondition_IsContentUnlocked::IsMet(const FTATTutorialParams& params) const
{
   if(const UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(params.World))
   {
      for(const FGameplayTag& tag : RequiredContent)
      {
         if(!save->HasUnlockedContent(tag))
         {
            return false;
         }
      }
      return true;
   }

   return false;
}

FString UTATTutorialCondition_IsContentUnlocked::GetDebugName() const
{
   return FString::Printf(TEXT("IsContentUnlocked %s"), *RequiredContent.ToString());
}

bool UTATTutorialCondition_IsCharacterReady::IsMet(const FTATTutorialParams& params) const
{
   const AOSECharacterBase* character = Cast<AOSECharacterBase>(params.Character);
   return character && character->IsCharacterReady();
}

FString UTATTutorialCondition_IsCharacterReady::GetDebugName() const
{
   return TEXT("IsCharacterReady");
}

bool UTATTutorialCondition_IsTargetingInteractable::IsMet(const FTATTutorialParams& params) const
{
   const AActor* interactableToTarget = Interactable.Get();
   if (interactableToTarget == nullptr)
   {
      // LOG error (unless expect to use during loading)
      return false;
   }

   if (!(params.Character && params.Character->Implements<UInteractorInterface>()))
   {
      return false;
   }

   return IInteractorInterface::Execute_GetCurrentInteractable(params.Character).GetObject() == interactableToTarget;
}

FString UTATTutorialCondition_IsTargetingInteractable::GetDebugName() const
{
   return FString::Printf(TEXT("IsTargetingInteractable %s"), *TutorialConditionHelpers::GetNameForSoftActor(Interactable));
}

#if WITH_EDITOR
void UTATTutorialCondition_IsTargetingInteractable::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(Interactable, TEXTVIEW("Interactable"));
}
#endif

bool UTATTutorialCondition_IsTargetingInteractableClass::IsMet(const FTATTutorialParams& params) const
{
   const UClass* targetClass = InteractableClass.Get();
   if (targetClass == nullptr)
   {
      return false;
   }

   if (!(params.Character && params.Character->Implements<UInteractorInterface>()))
   {
      return false;
   }

   const UObject* targetedObject = IInteractorInterface::Execute_GetCurrentInteractable(params.Character).GetObject();
   return targetedObject && targetedObject->IsA(targetClass);
}

FString UTATTutorialCondition_IsTargetingInteractableClass::GetDebugName() const
{
   return FString::Printf(TEXT("IsTargetingInteractableClass %s"), *TutorialConditionHelpers::GetNameForClass(InteractableClass));
}

#if WITH_EDITOR
void UTATTutorialCondition_IsTargetingInteractableClass::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftClass(InteractableClass, TEXTVIEW("InteractableClass"));
}
#endif

bool UTATTutorialCondition_PlayerMatchesTags::IsMet(const FTATTutorialParams& params) const
{
   UAbilitySystemComponent* asc = params.PlayerState ? params.PlayerState->GetAbilitySystemComponent() : nullptr;
   return asc && asc->HasAllMatchingGameplayTags(RequiredTags) && !asc->HasAnyMatchingGameplayTags(BlockedTags);
}

FString UTATTutorialCondition_PlayerMatchesTags::GetDebugName() const
{
   return FString::Printf(TEXT("Player Matches Tags: All(%s) None(%s)"), *RequiredTags.ToStringSimple(), *BlockedTags.ToStringSimple());
}

bool UTATTutorialCondition_NPCAlertness::IsMet(const FTATTutorialParams& params) const
{
   const ATATCharacterSpawner* spawner = GuardSpawner.Get();
   if (spawner == nullptr)
   {
      return false;
   }

   const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(spawner->GetSpawnedActor());
   return alertnessInterface != nullptr && (alertnessInterface->GetAlertnessLevel() >= MinimumAlertness);
}

FString UTATTutorialCondition_NPCAlertness::GetDebugName() const
{
   return FString::Printf(TEXT("NPC Has Alertness: %s"), *StaticEnum<EAlertnessLevel>()->GetDisplayNameTextByValue((int64)MinimumAlertness).ToString());
}

#if WITH_EDITOR
void UTATTutorialCondition_NPCAlertness::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(GuardSpawner, TEXTVIEW("GuardSpawner"));
}
#endif

bool UTATTutorialCondition_NPCIsOnscreen::IsMet(const FTATTutorialParams& params) const
{
   const ATATCharacterSpawner* spawner = GuardSpawner.Get();
   if (spawner == nullptr || spawner->GetSpawnedActor() == nullptr)
   {
      return false;
   }

   if (params.Controller == nullptr)
   {
      return false;
   }

   ULocalPlayer* const localPlayer = params.Controller->GetLocalPlayer();
   if (localPlayer && localPlayer->ViewportClient)
   {
      // get the projection data
      FSceneViewProjectionData projectionData;
      if (localPlayer->GetProjectionData(localPlayer->ViewportClient->Viewport, /*out*/ projectionData))
      {
         const FMatrix viewProjectionMatrix = projectionData.ComputeViewProjectionMatrix();
         FVector2D screenPosition;
         const FVector targetLocation = spawner->GetSpawnedActor()->GetActorLocation() + TargetWorldOffset;
         const bool didProject = FSceneView::ProjectWorldToScreen(targetLocation, FInt32Rect(-1,-1,1,1), viewProjectionMatrix, screenPosition);
         return didProject && FMath::Abs(screenPosition.X) < ScreenPercentage && FMath::Abs(screenPosition.Y) < ScreenPercentage;
      }
   }

   return false;
}

FString UTATTutorialCondition_NPCIsOnscreen::GetDebugName() const
{
   return TEXT("NPC Is Onscreen");
}

#if WITH_EDITOR
void UTATTutorialCondition_NPCIsOnscreen::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(GuardSpawner, TEXTVIEW("GuardSpawner"));
}
#endif

bool UTATTutorialCondition_NPCMatchesTags::IsMet(const FTATTutorialParams& params) const
{
   const ATATCharacterSpawner* spawner = GuardSpawner.Get();
   if (spawner == nullptr)
   {
      return false;
   }

   const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(spawner->GetSpawnedActor());
   return tagInterface && tagInterface->HasAllMatchingGameplayTags(RequiredTags) && !tagInterface->HasAnyMatchingGameplayTags(BlockedTags);
}

FString UTATTutorialCondition_NPCMatchesTags::GetDebugName() const
{
   return FString::Printf(TEXT("NPC Matches Tags: All(%s) None(%s)"), *RequiredTags.ToStringSimple(), *BlockedTags.ToStringSimple());
}

#if WITH_EDITOR
void UTATTutorialCondition_NPCMatchesTags::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(GuardSpawner, TEXTVIEW("GuardSpawner"));
}
#endif

bool UTATTutorialCondition_ActorIsUnlocked::IsMet(const FTATTutorialParams& params) const
{
   const AActor* actor = LockableActor.Get();

   // It is somewhat surprising that the lock-related interfaces don't cover this,
   // but interested in digging into that. The tutorial can do a specific thing, and
   // use an interface later if needed.
   if (const ATATLockableToggle* lockableToggle = Cast<ATATLockableToggle>(actor))
   {
      return lockableToggle->IsLocked() == ShouldBeLocked;
   }
   else if (const ATATSwingingDoor* door = Cast<ATATSwingingDoor>(actor))
   {
      return door->IsLocked() == ShouldBeLocked;
   }
   return false;
}

FString UTATTutorialCondition_ActorIsUnlocked::GetDebugName() const
{
   return FString::Printf(TEXT("%s: %s"),
      ShouldBeLocked ? TEXT("IsLocked") : TEXT("IsUnlocked"),
      *TutorialConditionHelpers::GetNameForSoftActor(LockableActor));
}

#if WITH_EDITOR
void UTATTutorialCondition_ActorIsUnlocked::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(LockableActor, TEXTVIEW("LockableActor"));
}
#endif


bool UTATTutorialCondition_ToggleIsOn::IsMet(const FTATTutorialParams& params) const
{
   const IOSEToggleInterface* toggle = Cast<IOSEToggleInterface>(ToggleableActor.Get());
   return toggle && toggle->IsToggleOn() == ShouldBeOn;
}

FString UTATTutorialCondition_ToggleIsOn::GetDebugName() const
{
   return FString::Printf(TEXT("%s: %s"),
      ShouldBeOn ? TEXT("Is On/Open") : TEXT("Is Off/Closed"),
      *TutorialConditionHelpers::GetNameForSoftActor(ToggleableActor));
}

#if WITH_EDITOR
void UTATTutorialCondition_ToggleIsOn::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(ToggleableActor, TEXTVIEW("ToggleableActor"));
}
#endif

bool UTATTutorialCondition_DoorIsOpen::IsMet(const FTATTutorialParams& params) const
{
   const ATATSwingingDoor* door = Door.Get();
   return door && door->IsOpen() == ShouldBeOpen;
}

FString UTATTutorialCondition_DoorIsOpen::GetDebugName() const
{
   return FString::Printf(TEXT("%s: %s"),
      ShouldBeOpen ? TEXT("Is Open") : TEXT("Is Closed"),
      *TutorialConditionHelpers::GetNameForSoftActor(Door));
}

#if WITH_EDITOR
void UTATTutorialCondition_DoorIsOpen::Validate(const FTATTutorialValidationParams& params) const
{
   params.RequireSoftActor(Door, TEXTVIEW("Door"));
}
#endif


bool UTATTutorialCondition_PlayerHasEscaped::IsMet(const FTATTutorialParams& params) const
{
   return params.PlayerState && params.PlayerState->GetMatchCompletionState() == EMatchCompletionState::Escaped;
}

FString UTATTutorialCondition_PlayerHasEscaped::GetDebugName() const
{
   return TEXT("Player Has Escaped");
}

bool UTATTutorialCondition_ContractInState::IsMet(const FTATTutorialParams& params) const
{
   if(const UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(params.World))
   {
      return save->GetContractState(Contract) == RequiredState;
   }

   return false;
}

FString UTATTutorialCondition_ContractInState::GetDebugName() const
{
   return FString::Printf(TEXT("Contract %s == %s"), *Contract.ToString(),
      *StaticEnum<ETATContractState>()->GetNameStringByValue(static_cast<int64>(RequiredState)));
}

bool UTATTutorialCondition_IsOffline::IsMet(const FTATTutorialParams& params) const
{
   return true;
}

FString UTATTutorialCondition_IsOffline::GetDebugName() const
{
   return TEXT("Is Offline");
}
