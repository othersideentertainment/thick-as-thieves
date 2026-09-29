// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATCheatManager.h"

// tat
#include "Breakables/TATBreakableComponent.h"
#include "Character/TATCharacterAIBase.h"
#include "Damage/TATDamageFunctionLibrary.h"
#include "Damage/TATDamageTypes.h"
#include "Developer/TATEditorSettings.h"
#include "Developer/TATLootSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATGameMode.h"
#include "GameFramework/TATWorldSettings.h"
#include "Interactables/TATEscapePoint.h"
#include "Items/TATItemActor.h"
#include "Items/TATItemInfo.h"
#include "Items/Tokens/TATInventoryToken.h"
#include "Items/Tokens/TATTokenInventoryComponent.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Lockpicking/LockpickableInterface.h"
#include "Lockpicking/TATCombinationLockComponent.h"
#include "Loot/TATLootActor.h"
#include "Loot/TATLootInventory.h"
#include "Loot/TATStashedLootSubsystem.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/DemoHubMissionMgr.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerController.h"
#include "Player/TATPlayerState.h"
#include "Quests/TATQuestHandle.h"
#include "Quests/TATQuestHandleUtils.h"
#include "Quests/TATContractState.h"
#include "Quests/TATContractSelectionComponent.h"
#include "SaveGame/TATSaveGame.h"
#include "Tools/TATToolComponent.h"
#include "UI/TATToastSubsystem.h"
#include "UI/TATToastBroadcaster.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Indicators/TATGlyphComponent.h"
#include "Settings/TATGameUserSettings.h"
#include "Settings/TATMatchSettings.h"
#include "Settings/TATMatchSettingsBase.h"
#include "Developer/TATWeatherSettings.h"
#include "Environment/TATWeatherTypeInfo.h"
#include "Environment/TATWeatherManager.h"
#include "Environment/TATWeatherPreset.h"
#include "Tutorial/TATTutorialRunnerComponent.h"
#include "UI/TATHUD.h"
#include "Developer/TATDevToolTypes.h"
#include "Developer/TATDevToolSubsystem.h"

// ose
#include "OSECoreCheats.h"
#include "OSEProjectSettings.h"
#include "AI/OSEAISettings.h"
#include "Identity/OSEIdentityMgr.h"
#include "Identity/OSESaveGameSystem.h"
#include "Interactables/Electrical/TATPowerNetworkComponent.h"
#include "Interactables/Electrical/TATPowerNetworkSubsystem.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Online/TATPvPGameMode.h"
#include "Quests/TATActiveQuestSubsystem.h"
#include "Quests/TATPlayerObjective.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/Modules/TATQuestGraph.h"
#include "Quests/Modules/TATQuestGraphRootNode.h"
#include "Quests/Modules/TATQuestGraphSubgraphNode.h"
#include "Upgrades/TATCharacterUpgradeUtils.h"
#include "Upgrades/TATUpgradeGraph.h"

// ue4
#include "EngineUtils.h"
#include "TATGameInstance.h"
#include "Engine/AssetManager.h"
#include "Engine/Console.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Interactables/TATInteractionTargeterComponent.h"
#include "Kismet/GameplayStatics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCheatManager)

const FString UTATCheatManager::kTATCheatPrefix = TEXT("tat.");

static constexpr uint64 kFastForwardDebugMessageKey = 563826294;

DEFINE_LOG_CATEGORY_STATIC(LogTATCheatManager, Log, All);

namespace CheatHelpers
{
   static FGameplayTag TryLookupTagFromString(const FString& string, const TCHAR* prefix)
   {
      FGameplayTag tag = FGameplayTag::RequestGameplayTag(FName(string, FNAME_Find), false);
      if (!tag.IsValid())
      {
         tag = FGameplayTag::RequestGameplayTag(FName(FString::Printf(TEXT("%s.%s"), prefix, *string), FNAME_Find), false);
      }
      return tag;
   }
}

void UTATCheatManager::InitCheatManager()
{
   Super::InitCheatManager();
}

bool UTATCheatManager::ProcessConsoleExec(const TCHAR* cmd, FOutputDevice& ar, UObject* executor)
{
   FString tatCmd(cmd);
   if (tatCmd.Contains(kTATCheatPrefix))
   {
      tatCmd.RemoveFromStart(kTATCheatPrefix);
      return Super::ProcessConsoleExec(*tatCmd, ar, executor);
   }
   // I suppose we also allow these to be called w/o the "tat" prefix?
   return Super::ProcessConsoleExec(*tatCmd, ar, executor);
}

void UTATCheatManager::SetStaminaConsumptionEnabled(int value)
{
#if OSE_CHEATS_ENABLED
   if (AGameModeBase* gm = GetWorld()->GetAuthGameMode())
   {
      for (int playerNumber = 0; playerNumber < gm->GetNumPlayers(); playerNumber++)
      {
         SetPlayerStaminaConsumptionEnabled(playerNumber, value);
      }
   }
#endif //OSE_CHEATS_ENABLED
}

void UTATCheatManager::MissionSetRandomSeed(int32 randomSeed)
{
#if OSE_CHEATS_ENABLED
   UTATEditorSettings& editorSettings = *GetMutableDefault<UTATEditorSettings>();
   editorSettings.WorldRandomizationSeed = randomSeed;
#endif
}

void UTATCheatManager::PrintKnownClues()
{
#if OSE_CHEATS_ENABLED
   if (UTATKnownCluesComponent* knownClues = UTATKnownCluesComponent::Get(_GetOwningPlayerState()))
   {
      FString message;
      message.Appendf(TEXT("%d known clues:\n"), knownClues->GetClueCount());
      for(int i = 0; i < knownClues->GetClueCount(); ++i)
      {
         message.Append(knownClues->GetClueText(i).ToString());
         message.Append(TEXT("\n"));
      }
      _PrintToConsole(message);
   }
#endif
}

void UTATCheatManager::PrintPlayerStats()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      FString message;
      message.Append(TEXT("Player Stats:\n"));
      for(const FOSEPlayerStat& stat : ps->GetPlayerStats().Stats)
      {
         message.Appendf(TEXT("%s: %d\n"), *stat.Tag.ToString(), stat.IntValue);
      }
      _PrintToConsole(message);
   }
#endif
}

void UTATCheatManager::PrintSessionStats()
{
#if OSE_CHEATS_ENABLED
   if (AOSEGameState* gs = GetWorld()->GetGameState<AOSEGameState>())
   {
      FString message;
      message.Append(TEXT("Session Stats:\n"));
      for(const FOSEPlayerStat& stat : gs->GetSessionStats().Stats)
      {
         message.Appendf(TEXT("%s: %d\n"), *stat.Tag.ToString(), stat.IntValue);
      }
      _PrintToConsole(message);
   }
#endif
}

void UTATCheatManager::DeleteSaveAndQuit(bool force)
{
#if OSE_CHEATS_ENABLED
   if (!force)
   {
      _PrintToConsole(TEXT("Use 'DeleteSaveAndQuit true' if you actually want to delete your save"));
      return;
   }

   if (APlayerController* pc = GetOwnerPlayerController())
   {
      UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(this);
      saveGameSystem->Delete();
      pc->ConsoleCommand(TEXT("quit force"));
   }
#endif
}

void UTATCheatManager::UnlockUpgrade(FString upgradeTagName, int level, bool autoEquip)
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FString tagError;
         TOptional<FGameplayTag> upgradeTag = _FindGameplayTag(upgradeTagName, TEXT("Upgrade"), FGameplayTag::RequestGameplayTag("Upgrade"), &tagError);
         if (!upgradeTag)
         {
            _PrintToConsole(FString::Printf(TEXT("UnlockUpgrade failed: %s"), *tagError));
            return;
         }

         save->SetUpgradeLevel(ps->GetCharacterSaveId(), *upgradeTag, level, autoEquip);
         save->DispatchProgressionUnexpectedlyChanged();

         ps->ForceRespawnCharacter();
      }
   }
#endif
}

void UTATCheatManager::EquipChildUpgrade(FString upgradeTagName)
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FString tagError;
         TOptional<FGameplayTag> upgradeTag = _FindGameplayTag(upgradeTagName, TEXT("Upgrade"), FGameplayTag::RequestGameplayTag("Upgrade"), &tagError);
         if (!upgradeTag)
         {
            _PrintToConsole(FString::Printf(TEXT("EquipChildUpgrade failed: %s"), *tagError));
            return;
         }

         FGameplayTag parentTag = upgradeTag->RequestDirectParent();
         if(parentTag.IsValid() && upgradeTag->IsValid() && save->GetCharacterProgression(ps->GetCharacterSaveId()).UnlockedUpgrades.HasTag(*upgradeTag))
         {
            save->EquipChildUpgrade(ps->GetCharacterSaveId(), *upgradeTag, parentTag);
            save->DispatchProgressionUnexpectedlyChanged();

            ps->ForceRespawnCharacter();
         }
      }
   }
#endif
}

void UTATCheatManager::EquipLoadoutUpgrade(const FGameplayTag& upgradeTag)
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         if (save->GetCharacterProgression(ps->GetCharacterSaveId()).UnlockedUpgrades.HasTag(upgradeTag))
         {
            save->EquipLoadoutSkill(ps->GetCharacterSaveId(), upgradeTag);
            save->DispatchProgressionUnexpectedlyChanged();

            ps->ForceRespawnCharacter();
         }
      }
   }
#endif
}

void UTATCheatManager::RemoveUpgrades()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         save->ResetUpgradeProgressForCharacter(ps->GetCharacterSaveId());
         save->DispatchProgressionUnexpectedlyChanged();

         ps->ForceRespawnCharacter();
      }
   }
#endif
}

void UTATCheatManager::RemoveUpgradesWithRefund()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      const FTATCharacterDataContext context = ps->GetCharacterDataContext();
      if (!context.IsValid())
      {
         _PrintToConsole(TEXT("Failed to remove upgrades: invalid character data"));
         return;
      }

      TSoftObjectPtr<UTATUpgradeGraph> upgradeGraphSoft = UTATCharacterUpgradeUtils::GetCharacterUpgradeGraph(context);
      if (upgradeGraphSoft.IsNull())
      {
         _PrintToConsole(TEXT("Failed to remove upgrades: character has no upgrade graph"));
         return;
      }

      constexpr bool refundUpgradeCosts = true;
      UTATCharacterUpgradeUtils::ResetAllCharacterUpgradeProgress(context, upgradeGraphSoft.LoadSynchronous(), refundUpgradeCosts);
      context.SaveGame->DispatchProgressionUnexpectedlyChanged();
      ps->ForceRespawnCharacter();
   }
#endif
}

void UTATCheatManager::RemoveUpgrade(FString upgradeTagName)
{
#if OSE_CHEATS_ENABLED
   FString tagError;
   TOptional<FGameplayTag> upgradeTag = _FindGameplayTag(upgradeTagName, TEXT("Upgrade"), FGameplayTag::RequestGameplayTag("Upgrade"), &tagError);
   if (!upgradeTag)
   {
      _PrintToConsole(FString::Printf(TEXT("RemoveUpgrade failed: %s"), *tagError));
      return;
   }

   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (!ps->GetAllUpgradeState().HasTag(*upgradeTag))
      {
         // nothing to remove
         return;
      }

      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         save->ResetUpgradeProgressForTag(ps->GetCharacterSaveId(), *upgradeTag);
         save->DispatchProgressionUnexpectedlyChanged();

         ps->ForceRespawnCharacter();
      }
   }
#endif
}

void UTATCheatManager::ListUnlockedUpgrades()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      const FUpgradeState& upgradeState = ps->GetAllUpgradeState();
      TArray<FGameplayTag, TInlineAllocator<64>> unlockedUpgrades;
      for (const auto& pair : upgradeState.Values)
      {
         if (pair.Value >= 1)
         {
            unlockedUpgrades.Add(pair.Key);
         }
      }

      if (unlockedUpgrades.Num() == 0)
      {
         _PrintToConsole(TEXT("No unlocked upgrades"));
      }
      else
      {
         // Sort the upgrade tags to keep the listing deterministic
         unlockedUpgrades.Sort();
         for (const FGameplayTag& tag : unlockedUpgrades)
         {
            _PrintToConsole(FString::Printf(TEXT("%s (level %d)"), *tag.ToString(), upgradeState.GetValue(tag)));
         }
      }
   }
#endif
}

void UTATCheatManager::ListAllUpgrades()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      const FTATCharacterDataContext ctx = ps->GetCharacterDataContext();
      if (ctx.IsValid())
      {
         UTATCharactersMetadata* charactersMetadata = UTATCharacterMetadataFunctionLibrary::GetCharactersMetadataAsset();
         check(charactersMetadata != nullptr);
         const FTATCharacterMetadata* charMeta = charactersMetadata->Characters.Find(ctx.GetCharacterDataChecked().Character);
         if (charMeta != nullptr && !charMeta->UpgradeGraph.IsNull())
         {
            if (UTATUpgradeGraph* upgradeGraph = charMeta->UpgradeGraph.LoadSynchronous())
            {
               const FUpgradeState& upgradeState = ps->GetAllUpgradeState();
               upgradeGraph->TraverseNodes(EOSEGenericGraphSearchMode::DepthFirstSearch, [this, &upgradeState](UOSEGenericGraphNode* genericNode)
               {
                  UTATUpgradeGraphNode* node = Cast<UTATUpgradeGraphNode>(genericNode);
                  if (node == nullptr)
                  {
                     return UOSEGenericGraph::TraverseContinue;
                  }

                  const FGameplayTag upgradeTag = node->GetUpgradeTag();
                  const int32 upgradeLevel = node->GetUpgradeLevel();
                  if (upgradeState.GetValue(upgradeTag) >= upgradeLevel)
                  {
                     _PrintToConsole(FString::Printf(TEXT("%s (level %d): UNLOCKED"), *upgradeTag.ToString(), upgradeLevel));
                  }
                  else
                  {
                     _PrintToConsole(FString::Printf(TEXT("%s (level %d): locked"), *upgradeTag.ToString(), upgradeLevel));
                  }

                  return UOSEGenericGraph::TraverseContinue;
               });
            }
         }
      }
   }
#endif
}

void UTATCheatManager::ForceRefreshUpgradeState()
{
#if OSE_CHEATS_ENABLED
   ATATPlayerState* ps = _GetOwningPlayerState();
   if (ps != nullptr && ps->IsLocalPlayerState())
   {
      ps->ServerRefreshCharacterUpgradeAndLoadoutState();
   }
#endif
}

void UTATCheatManager::AddMoney(int32 amount)
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->UpdateMoney(amount);
      save->DispatchProgressionUnexpectedlyChanged();
   }
#endif
}

void UTATCheatManager::ClearMoney()
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      const int32 moneyToClear = -save->GetMoney();
      save->UpdateMoney(moneyToClear);
      save->DispatchProgressionUnexpectedlyChanged();
   }
#endif
}

void UTATCheatManager::SetXP(int32 totalXP)
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      const int32 xpToAdd = totalXP - save->GetXP().XP;
      save->AddXP(xpToAdd);
      save->DispatchProgressionUnexpectedlyChanged();
   }
#endif
}

void UTATCheatManager::AddXP(int32 amountXP)
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->AddXP(amountXP);
      save->DispatchProgressionUnexpectedlyChanged();
   }
#endif
}

void UTATCheatManager::PrintCurrentXP()
{
#if OSE_CHEATS_ENABLED
   if (const UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      const FTATPlayerExperience& xpData = save->GetXP();
      _PrintToConsole(FString::Format(TEXT("Current Level: {0} Total XP: {1} Level XP: {2}"), { xpData.Level, xpData.XP, xpData.CurrentLevelXP}));
   }
#endif
}

void UTATCheatManager::AddUpgradeCurrency(FString currency, int32 amount)
{
#if OSE_CHEATS_ENABLED
   ATATPlayerState* ps = _GetOwningPlayerState();
   const FTATCharacterSaveId character = (ps != nullptr) ? ps->GetCharacterSaveId() : FTATCharacterSaveId{};
   if (!character.IsValid())
   {
      return;
   }
   FString tagError;
   TOptional<FGameplayTag> currencyTag = _FindGameplayTag(currency, TEXT("UpgradeCurrency"), FGameplayTag::RequestGameplayTag("UpgradeCurrency"), &tagError);
   if (!currencyTag)
   {
      _PrintToConsole(FString::Format(TEXT("Invalid upgrade currency: {0}"), { tagError }));
      return;
   }
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->AddUpgradeCurrency(character, *currencyTag, amount);
      save->DispatchProgressionUnexpectedlyChanged();
   }
#endif
}

void UTATCheatManager::ClearAllUpgradeCurrency()
{
#if OSE_CHEATS_ENABLED
   ATATPlayerState* ps = _GetOwningPlayerState();
   const FTATCharacterSaveId character = (ps != nullptr) ? ps->GetCharacterSaveId() : FTATCharacterSaveId{};
   if (!character.IsValid())
   {
      return;
   }
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->ClearUpgradeCurrencies(character);
      save->DispatchProgressionUnexpectedlyChanged();
   }
#endif
}

void UTATCheatManager::AddTool(const FString& toolName)
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (pc->HasAuthority())
      {
         if (IToolSetSystemInterface* toolSetSystem = pc->GetPawn<IToolSetSystemInterface>())
         {
            if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystem->GetToolSetInterface())
            {
               TSubclassOf<UToolComponent> toolClass = _LoadToolByName(toolName);
               if (toolClass.Get() != nullptr)
               {
                  toolSetInterface->AuthorityAddToolClass(toolClass);
               }
            }
         }
      }
   }
#endif
}

void UTATCheatManager::RemoveTool(const FString& toolName)
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (pc->HasAuthority())
      {
         if (IToolSetSystemInterface* toolSetSystem = pc->GetPawn<IToolSetSystemInterface>())
         {
            if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystem->GetToolSetInterface())
            {
               TSubclassOf<UToolComponent> toolClass = _LoadToolByName(toolName);
               if (toolClass.Get() != nullptr)
               {
                  toolSetInterface->AuthorityRemoveToolsOfClass(toolClass);
               }
            }
         }
      }
   }
#endif
}

void UTATCheatManager::RefillAmmoForCurrentTool()
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (pc->HasAuthority())
      {
         if (IToolSetSystemInterface* toolSetSystem = pc->GetPawn<IToolSetSystemInterface>())
         {
            if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystem->GetToolSetInterface())
            {
               if (UTATToolComponent* currentTool = Cast<UTATToolComponent>(toolSetInterface->GetCurrentTool()))
               {
                  currentTool->AuthorityRefillAmmo();
               }
            }
         }
      }
   }
#endif
}

void UTATCheatManager::RefillAllAmmo()
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (pc->HasAuthority())
      {
         if (IToolSetSystemInterface* toolSetSystem = pc->GetPawn<IToolSetSystemInterface>())
         {
            if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystem->GetToolSetInterface())
            {
               int32 numTools = toolSetInterface->GetNumTools();

               for (int32 i = 0; i < numTools; i++)
               {
                  if (UTATToolComponent* tool = Cast<UTATToolComponent>(toolSetInterface->GetToolAtIndex(i)))
                  {
                     tool->AuthorityRefillAmmo_CHEAT();
                  }
               }
            }
         }
      }
   }
#endif
}

void UTATCheatManager::AddItem(FName itemName, int32 quantity)
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (auto ps = pc->GetPlayerState<ATATPlayerState>())
      {
         if (UTATItemInventoryComponent* inventory = ps->GetTATItemInventory())
         {
            TSubclassOf<UTATItemInfo> itemInfo = _LoadItemForName(itemName);
            if (itemInfo)
            {
               inventory->AuthorityAddItemMultiple(itemInfo, quantity);
            }
         }
      }
   }
#endif
}

void UTATCheatManager::AddSavedLoot(FName lootTag, int32 quantity)
{
#if OSE_CHEATS_ENABLED
   const FTATLootInfo* lootInfo = _FindLootInfoByTag(lootTag);
   if (lootInfo == nullptr)
   {
      _PrintToConsole(FString::Printf(TEXT("Failed to add loot: could not find loot tag '%s'."),
         *lootTag.ToString()));
      return;
   }

   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FTATCharacterSaveId character = ps->GetCharacterSaveId();

         FTATSavedLootAddRequest addRequest;
         addRequest.AddCount(lootInfo->LootIdentifier, quantity, save);

         save->AddLoot(character, addRequest);
         save->DispatchProgressionUnexpectedlyChanged();
      }
   }
#endif
}

void UTATCheatManager::AddStashedLoot(FName lootTag, int32 quantity)
{
#if OSE_CHEATS_ENABLED
   if (quantity > 0)
   {
      const FTATLootInfo* lootInfo = _FindLootInfoByTag(lootTag);
      if (lootInfo == nullptr)
      {
         _PrintToConsole(FString::Printf(TEXT("Failed to stash loot: could not find loot tag '%s'."),
            *lootTag.ToString()));
         return;
      }

      TArray<FTATLootIdentifier> lootToDeposit;
      for (int i = 0; i < quantity; ++i)
      {
         lootToDeposit.Add(lootInfo->LootIdentifier);
      }

      if (ATATPlayerState* ps = _GetOwningPlayerState())
      {
         if (UTATStashedLootSubsystem* stashSubsystem = GetWorld()->GetSubsystem<UTATStashedLootSubsystem>())
         {
            stashSubsystem->AuthorityAddStashedLoot(ps->GetTeam(), lootToDeposit);
         }
      }
   }
#endif
}

void UTATCheatManager::ClearCurrentCharacterProgression()
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         save->ResetCharacter(ps->GetCharacterSaveId());
         save->DispatchProgressionUnexpectedlyChanged();
      }
   }
#endif
}

void UTATCheatManager::PrintCurrentCharacterProgression()
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (const UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FString debugString;
         save->GetCharacterProgression(ps->GetCharacterSaveId()).AppendToDebugString(debugString);
         save->GetPlayerProgression().AppendToDebugString(debugString);
         _PrintToConsole(debugString);
      }
   }
#endif
}

void UTATCheatManager::ForceVariant(const FString& variantName)
{
#if OSE_CHEATS_ENABLED
   UTATEditorSettings& editorSettings = UTATEditorSettings::GetMutable();
   TSoftObjectPtr<UTATSceneVariantConfig> variant = _FindVariantForName(variantName);

   if (variant.IsNull())
   {
      _PrintToConsole(TEXT("No such variant"));
      return;
   }

   // TODO: remove variants for the same scene? (using asset registry deps)
   editorSettings.SceneVariantOverrides.Add(variant);
   _PrintToConsole(FString::Printf(TEXT("Forcing variant %s"), *variantName));
#endif
}

void UTATCheatManager::ClearVariantOverrides()
{
#if OSE_CHEATS_ENABLED
   UTATEditorSettings& editorSettings = UTATEditorSettings::GetMutable();
   editorSettings.SceneVariantOverrides.Reset();
   _PrintToConsole(TEXT("Variant overrides cleared"));
#endif
}

void UTATCheatManager::SpawnNPC(const FString& className)
{
#if OSE_CHEATS_ENABLED
   TSubclassOf<APawn> pawnClass = _FindBlueprintClassByName<APawn>(className);
   if (!pawnClass && !className.StartsWith(TEXT("BP_")))
   {
      // If we didn't find a valid asset, try again with a BP_ prefix
      pawnClass = _FindBlueprintClassByName<APawn>(TEXT("BP_") + className);
   }
   if (!pawnClass)
   {
      _PrintToConsole(FString::Printf(TEXT("Failed to spawn NPC: unable to find class named '%s'"), *className));
      return;
   }

   // If this pawn class defines an AI controller, spawn it first
   AController* controller = nullptr;
   APawn* pawnCDO = pawnClass.GetDefaultObject();
   check(pawnCDO != nullptr);
   if (pawnCDO->AIControllerClass)
   {
      FActorSpawnParameters params{};
      params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
      controller = GetWorld()->SpawnActor<AController>(pawnCDO->AIControllerClass, params);

      // Only bail if we failed to spawn a controller (but not if we didn't have a controller to begin with)
      if (controller == nullptr)
      {
         _PrintToConsole(FString::Printf(TEXT("Failed to spawn NPC: SpawnActor failed for the NPC's AIController (%s)"), *pawnCDO->AIControllerClass->GetName()));
         return;
      }
   }

   // Spawn the NPC's pawn
   APawn* spawnedPawn = CastChecked<APawn>(_SpawnActorForPlayer(pawnClass));
   if (spawnedPawn == nullptr)
   {
      _PrintToConsole(TEXT("Failed to spawn NPC: SpawnActor failed"));
      if (controller != nullptr)
      {
         controller->Destroy();
      }
      return;
   }

   // If we have a valid controller, have it possess the newly spawned pawn
   if (controller != nullptr)
   {
      controller->Possess(spawnedPawn);
   }
   else
   {
      _PrintToConsole(FString::Printf(TEXT("Warning: NPC of type %s was spawned without a controller (AIControllerClass was null)."), *className));
   }
#endif
}

void UTATCheatManager::PretendWasMatchmade()
{
#if OSE_CHEATS_ENABLED
   if (UTATGameInstance* gameInstance = GetWorld()->GetGameInstance<UTATGameInstance>())
   {
      gameInstance->SetPartyWasMatchmade(true);
   }
#endif
}

void UTATCheatManager::FindNPCTypes(const FString& query)
{
#if OSE_CHEATS_ENABLED
   static constexpr bool continueIterating = true;
   _ForEachBlueprintClassMatchingQuery<ATATCharacterAIBase>(query,
      [this](TSubclassOf<ATATCharacterAIBase> pawnClass) -> bool
      {
         check(pawnClass != nullptr);

         const FString name = pawnClass->GetName();

         // Strip the "_C" suffix since it's not needed for the cheats these names are intended to be used with
         FStringView shortName = name;
         if (shortName.EndsWith(TEXT("_C")))
         {
            shortName.RemoveSuffix(2);
         }

         // Skip any class ending with _Base, as those are base classes and not intended to be spawned on their own
         if (shortName.EndsWith(TEXT("_Base"), ESearchCase::IgnoreCase))
         {
            return continueIterating;
         }

         _PrintToConsole(FString(shortName));

         return continueIterating;
      });
#endif
}

void UTATCheatManager::SpawnLoot(FName lootTag)
{
#if OSE_CHEATS_ENABLED
   const FTATLootInfo* lootInfo = _FindLootInfoByTag(lootTag);
   if (lootInfo == nullptr)
   {
      _PrintToConsole(FString::Printf(TEXT("Failed to spawn loot: could not find loot tag '%s'."),
         *lootTag.ToString(), *lootTag.ToString(), *lootTag.ToString()));
      return;
   }

   UClass* lootActorClass = lootInfo->ActorClass.LoadSynchronous();
   if (lootActorClass == nullptr)
   {
      UE_LOG(LogTATCheatManager, Error, TEXT("Failed to spawn loot: loot info for '%s' does not have a valid loot actor class"),
         *lootInfo->LootIdentifier.LootTag.ToString());
      return;
   }

   AActor* spawnedLootActor = _SpawnActorForPlayer(lootActorClass);
   if (spawnedLootActor == nullptr)
   {
      UE_LOG(LogTATCheatManager, Error, TEXT("Failed to spawn loot: SpawnActor failed"));
   }
#endif
}

void UTATCheatManager::DropMajorLoot()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      UTATLootInventoryComponent* invComp = ps->GetLootInventoryComponent();
      if (invComp == nullptr)
      {
         return;
      }

      if (invComp->HasMajorLoot())
      {
         invComp->AuthorityDropAllMajorLootInvoluntarily();
      }
      else
      {
         _PrintToConsole(TEXT("Not holding major loot!"));
      }
   }
#endif
}

void UTATCheatManager::LootPinata()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATLootInventoryComponent* invComp = ps->GetLootInventoryComponent())
      {
         invComp->AuthorityDropAllLootInvoluntary();
      }
   }
#endif
}

void UTATCheatManager::TutorialJump(int stepNumber)
{
#if OSE_CHEATS_ENABLED
   if(UTATTutorialRunnerComponent* runner = UTATTutorialRunnerComponent::Find(GetWorld()))
   {
      runner->StartRunningAtSnapshot(FTATTutorialResumeSnapshot{
         .State = FTATTutorialResumeSnapshot::EState::WaitToEnter,
         .StepIndex = stepNumber,
      });
   }
#endif
}

void UTATCheatManager::AddToken(const FString& tokenClassName)
{
#if OSE_CHEATS_ENABLED
   const UTATInventoryToken* token = _FindAssetByName<UTATInventoryToken>(tokenClassName);
   if (token == nullptr)
   {
      TStringBuilder<128> builder;
      builder.Append(TEXT("TK_"));
      builder.Append(tokenClassName);
      token = _FindAssetByName<UTATInventoryToken>(builder);
   }

   if (token == nullptr)
   {
      _PrintToConsole(FString::Printf(TEXT("Could not find token '%s'"), *tokenClassName));
      return;
   }

   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATTokenInventoryComponent* invComp = UTATTokenInventoryComponent::FromActor(ps))
      {
         invComp->AuthorityAddToken(token);
         _PrintToConsole(FString::Printf(TEXT("Added token '%s'"), *token->DisplayName.ToString()));
      }
   }
#endif
}

void UTATCheatManager::ClearTokens()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATTokenInventoryComponent* invComp = UTATTokenInventoryComponent::FromActor(ps))
      {
         while(invComp->GetTokenCount() > 0)
         {
            // might as well remove from the back
            invComp->AuthorityRemoveTokenAt(invComp->GetTokenCount() - 1);
         }
      }
   }
#endif
}

void UTATCheatManager::FreezeInteractTargeting()
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (APawn* pawn = ps->GetPawn())
      {
         if (UTATInteractionTargeterComponent* targeter = pawn->GetComponentByClass<UTATInteractionTargeterComponent>())
         {
            targeter->RequestTargetingFreeze(FName("CheatManager"));
         }
      }
   }
#endif
}

void UTATCheatManager::ClearInteractTargetingFreeze()
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (APawn* pawn = ps->GetPawn())
      {
         if (UTATInteractionTargeterComponent* targeter = pawn->GetComponentByClass<UTATInteractionTargeterComponent>())
         {
            targeter->ClearTargetingFreeze();
         }
      }
   }
#endif
}

void UTATCheatManager::ContractSelect(const FString& questTagString)
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      UTATContractSelectionComponent* questSelection = ps->FindComponentByClass< UTATContractSelectionComponent>();
      if (questSelection == nullptr)
      {
         _PrintToConsole(TEXT("Could not find QuestSelectionComponent, are you in the lobby?"));
         return;
      }

      FGameplayTag questTag = FGameplayTag::RequestGameplayTag(FName(questTagString), false);
      if (questTag.IsValid())
      {
         questSelection->SetSelectedContract(questTag);
      }
      else
      {
         _PrintToConsole(FString::Format(TEXT("Invalid quest tag: '{0};"), { questTagString }));
      }

      _PrintToConsole(FString::Format(TEXT("SelectedQuest: {0}"), { questSelection->GetSelectedContract().ToString() }));
   }
#endif
}

void UTATCheatManager::MissionSelect(const FString& questTagString)
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      UTATMatchSettings* matchSettings = Cast<UTATMatchSettings>(ps->GetLocalUIMatchSettings());
      if (matchSettings == nullptr)
      {
         _PrintToConsole(TEXT("Could not find match settings, are you the mission owner?"));
         return;
      }

      FGameplayTag missionTag = FGameplayTag::RequestGameplayTag(FName(questTagString), false);
      if (missionTag.IsValid())
      {
         matchSettings->Mission = missionTag;
         ps->ServerUpdateMatchSettings();
      }
      else
      {
         _PrintToConsole(FString::Format(TEXT("Invalid quest tag: '{0};"), { questTagString }));
      }

      _PrintToConsole(FString::Format(TEXT("SelectedMission: {0}"), { matchSettings->Mission.ToString() }));
   }
#endif
}

void UTATCheatManager::ContractIntro(const FString& questTagString)
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FGameplayTag questTag = FGameplayTag::RequestGameplayTag(FName(questTagString), false);
         if (questTag.IsValid())
         {
            save->SetContractState(questTag, ETATContractState::Intro);
            save->DispatchProgressionUnexpectedlyChanged();

            _PrintToConsole(FString::Format(TEXT("Started quest intro: {0}"), { questTag.ToString() }));
         }
         else
         {
            _PrintToConsole(FString::Format(TEXT("Invalid quest tag: '{0};"), { questTagString }));
         }
      }
   }
#endif
}

void UTATCheatManager::ContractTrigger(const FString& questTagString)
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FGameplayTag questTag = FGameplayTag::RequestGameplayTag(FName(questTagString), false);
         if (questTag.IsValid())
         {
            save->SetContractState(questTag, ETATContractState::Objective);
            save->DispatchProgressionUnexpectedlyChanged();

            _PrintToConsole(FString::Format(TEXT("Started quest: {0}"), { questTag.ToString() }));
         }
         else
         {
            _PrintToConsole(FString::Format(TEXT("Invalid quest tag: '{0};"), { questTagString }));
         }
      }
   }
#endif
}

void UTATCheatManager::ContractOutro(const FString& questTagString)
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FGameplayTag questTag = FGameplayTag::RequestGameplayTag(FName(questTagString), false);
         if (!questTag.IsValid())
         {
            _PrintToConsole(FString::Format(TEXT("Invalid quest tag: '{0};"), { questTagString }));
            return;
         }

         const FTATQuestHandle handle = UTATQuestHandleUtils::CreateQuestHandle(questTag, this);
         const FTATContractInfo* quest = handle.GetContract();
         if(quest == nullptr || quest->OutroFlow == ETATContractOutroFlow::Automatic)
         {
            _PrintToConsole(FString::Format(TEXT("Invalid contract tag: '{0};"), { questTagString }));
            return;
         }
         
         save->SetContractState(questTag, ETATContractState::Outro);
         save->DispatchProgressionUnexpectedlyChanged();

         _PrintToConsole(FString::Format(TEXT("Quest in outro: {0}"), { questTag.ToString() }));
      }
   }
#endif
}

void UTATCheatManager::ContractForget(const FString& questTagString)
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FGameplayTag questTag = FGameplayTag::RequestGameplayTag(FName(questTagString), false);
         if (questTag.IsValid())
         {
            save->SetContractState(questTag, ETATContractState::Unstarted);
            save->DispatchProgressionUnexpectedlyChanged();

            _PrintToConsole(FString::Format(TEXT("Started quest: {0}"), { questTag.ToString() }));
         }
         else
         {
            _PrintToConsole(FString::Format(TEXT("Invalid quest tag: '{0};"), { questTagString }));
         }
      }
   }
#endif
}

void UTATCheatManager::ContractComplete(const FString& questTagString)
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         FGameplayTag questTag = FGameplayTag::RequestGameplayTag(FName(questTagString), false);
         if (questTag.IsValid())
         {
            save->SetContractState(questTag, ETATContractState::Complete);
            save->DispatchProgressionUnexpectedlyChanged();

            _PrintToConsole(FString::Format(TEXT("Started quest: {0}"), { questTag.ToString() }));
         }
         else
         {
            _PrintToConsole(FString::Format(TEXT("Invalid quest tag: '{0};"), { questTagString }));
         }
      }
   }
#endif
}

void UTATCheatManager::PrintObjective()
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      for (const UTATRootPlayerObjective* objective : ps->GetObjectives())
      {
         _PrintToConsole(FString::Format(TEXT("[{3}] Objective: {0}, Complete: {1}\nDescription: {2}"), {objective->QuestTag.ToString(),
            objective->IsComplete(), objective->ObjectiveText.ToString(),
            StaticEnum<ETATPlayerQuestSlot>()->GetNameStringByValue(static_cast<int64>(objective->Slot))}));
         for (const UTATPlayerObjective* child : objective->GetChildObjectives())
         {
            _PrintToConsole(FString::Format(TEXT("  --- Complete: {0}\nDescription: {1}"), {objective->IsComplete(), objective->ObjectiveText.ToString()}));
         }
      }
   }
#endif
}

void UTATCheatManager::ContractClearAll()
{
#if OSE_CHEATS_ENABLED
   if (const ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
      {
         save->ClearContracts();
         save->DispatchProgressionUnexpectedlyChanged();

         _PrintToConsole(TEXT("Quests cleared"));
      }
   }
#endif
}

void UTATCheatManager::CompleteObjective()
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      // TODO: add parameter for type?
      for (UTATRootPlayerObjective* objective : ps->GetObjectives())
      {
         objective->AuthorityCheatComplete();
         _PrintToConsole(FString::Format(TEXT("[{2}] Objective: {0}, Complete: {1}"), { objective->QuestTag.ToString(), objective->IsComplete(),
            StaticEnum<ETATPlayerQuestSlot>()->GetNameStringByValue(static_cast<int64>(objective->Slot)) }));
      }
   }
#endif
}

namespace QuestHelpers
{
   struct FGraphTraversal
   {
      TArray<UTATQuestGraphRootNode*> Stack;
      TFunction<void(UOSEGenericGraphNode*, UOSEGenericGraphNode*, int32)> Callback;

      void Traverse(UTATQuestGraphRootNode* startNode)
      {
         check(startNode != nullptr);
         UTATQuestGraphBase* questGraph = Cast<UTATQuestGraphBase>(startNode->GetGraph());
         check(questGraph != nullptr);

         const int32 depth = Stack.Num();

         Stack.Add(startNode);

         questGraph->TraverseNodeEdgePairs(EOSEGenericGraphSearchMode::DepthFirstSearch,
            [this, depth](UOSEGenericGraphNode* node, UOSEGenericGraphEdge* fromEdge) -> bool
            {
               UOSEGenericGraphNode* parent = (fromEdge != nullptr) ? fromEdge->GetOtherNode(node) : nullptr;

               Callback(node, parent, depth);

               if (!ensure(node))
               {
                  return UOSEGenericGraph::TraverseBreak;
               }

               // If this is a subgraph node, recurse into it
               UTATQuestGraphSubgraphNode* subgraphNode = Cast<UTATQuestGraphSubgraphNode>(node);
               if (subgraphNode && subgraphNode->Module)
               {
                  if (UTATQuestGraphRootNode* subgraphRootNode = subgraphNode->Module->SelectRootNode())
                  {
                     Traverse(subgraphRootNode);
                  }
               }
               return UOSEGenericGraph::TraverseContinue;
            }, startNode);

         UTATQuestGraphNode* node = Stack.Pop();
         check(node == startNode);
      }
   };
}

void UTATCheatManager::DumpQuestGraph()
{
#if OSE_CHEATS_ENABLED
   if (UWorld* world = GetWorld())
   {
      if (const UTATActiveQuestSubsystem* activeQuestSubsystem = world->GetSubsystem<UTATActiveQuestSubsystem>())
      {
         if (const UTATQuestGraph* questGraph = activeQuestSubsystem->GetQuestGraph())
         {
            auto getNodeDebugNameSafe = [](UOSEGenericGraphNode* node) -> FString
            {
               return (node != nullptr) ? node->GetNodeDebugName() : TEXT("NULL");
            };


            FString message;
            message.Appendf(TEXT("============ Quest Graph: %s ============\n"), *GetNameSafe(questGraph));
            QuestHelpers::FGraphTraversal graphTraversal{};
            graphTraversal.Callback = [&](UOSEGenericGraphNode* node, UOSEGenericGraphNode* parent, int32 depth)
            {
               message.Appendf(TEXT("%s%s\n"), ActiveQuestHelpers::Indent(depth), *getNodeDebugNameSafe(node));
               if (node == nullptr)
               {
                  UOSEGenericGraph* questGraph = (parent != nullptr) ? parent->GetGraph() : nullptr;
                  _PrintErrorToConsole(FString::Printf(TEXT("Found NULL node in quest graph %s (parent: %s)"),
                     *GetNameSafe(questGraph), *getNodeDebugNameSafe(parent)));
               }
            };

            if (UTATQuestGraphRootNode* rootNode = questGraph->SelectRootNode())
            {
               graphTraversal.Traverse(rootNode);
               _PrintToConsole(message);
            }
            else
            {
               _PrintErrorToConsole(TEXT("Failed to dump quest graph: no root node found"));
            }
         }
         else
         {
            _PrintErrorToConsole(TEXT("Failed to dump quest graph: quest graph not found"));
         }
      }
      else
      {
         _PrintErrorToConsole(TEXT("Failed to dump quest graph: no active quest subsystem"));
      }
   }
   else
   {
      _PrintErrorToConsole(TEXT("Failed to dump quest graph: invalid world"));
   }
#endif
}

void UTATCheatManager::ChangeCharacter(int characterNum)
{
#if OSE_CHEATS_ENABLED
   if (ATATPlayerState* ps = _GetOwningPlayerState())
   {
      if (characterNum > int(ETATCharacter::None) && characterNum < int(ETATCharacter::MAX))
      {
         ps->ChangeTATCharacter(ETATCharacter(characterNum));
      }
   }
#endif
}

void UTATCheatManager::ToggleAIDebugHUD()
{
#if OSE_CHEATS_ENABLED
   UOSEAISettings& settings = *GetMutableDefault<UOSEAISettings>();
   settings.ShowingAIDebugHUD = !settings.ShowingAIDebugHUD;

   // everything can update itself.
   // Future work: Refactor to OSE level & create an interface for `_OnDebugHUDShowingChanged`
   for (TActorIterator<ATATCharacterAIBase> it(GetWorld()); it; ++it)
   {
      if (ATATCharacterAIBase* character = (*it))
      {
         character->OnDebugHUDShowingChanged(settings.ShowingAIDebugHUD);
      }
   }
#endif
}

void UTATCheatManager::ToggleDisableLockpickingMinigame()
{
#if OSE_CHEATS_ENABLED
   UTATEditorSettings& editorSettings = UTATEditorSettings::GetMutable();
   editorSettings.DisableLockpickingMinigame = !editorSettings.DisableLockpickingMinigame;
   UE_LOG(LogTATCheatManager, Log, TEXT("Lockpicking minigame is %s"), editorSettings.DisableLockpickingMinigame ? TEXT("Disabled") : TEXT("Enabled"));
#endif // OSE_CHEATS_ENABLED
}

void UTATCheatManager::ForceSoloEscape()
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (ATATSessionGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATSessionGameMode>())
      {
         gameMode->HandlePlayersEscaped({ pc }, nullptr);
      }
   }
#endif
}

void UTATCheatManager::ForceSoloCaught()
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if(ATATSessionGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATSessionGameMode>())
      {
         gameMode->PlayerCaught(pc);
      }
   }
#endif
}

void UTATCheatManager::ForceTeamEscape()
{
#if OSE_CHEATS_ENABLED
   if(ATATSessionGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATSessionGameMode>())
   {
      TArray<APlayerController*> teamControllers = _CollectControllersOnPlayerTeam();
      if (teamControllers.Num() > 0)
      {
         gameMode->HandlePlayersEscaped(teamControllers, nullptr);
      }
   }
#endif
}

void UTATCheatManager::ForceTeamCaught()
{
#if OSE_CHEATS_ENABLED
   if(ATATSessionGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATSessionGameMode>())
   {
      TArray<APlayerController*> teamControllers = _CollectControllersOnPlayerTeam();
      if (teamControllers.Num() > 0)
      {
         gameMode->HandlePlayersCaught(teamControllers);
      }
   }
#endif
}

void UTATCheatManager::ForceEndMatch()
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if(ATATSessionGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATSessionGameMode>())
      {
         gameMode->ForceRemainingPlayersCaught();
      }
   }
#endif
}

void UTATCheatManager::EscapeOpen(bool force)
{
#if OSE_CHEATS_ENABLED
   constexpr float traceRange = 2000.f;
   auto* escapePoint = Cast<ATATEscapePoint>(_LineTracePlayerViewTarget(traceRange));
   if (escapePoint)
   {
      if (force || escapePoint->GetState() == ETATEscapePointState::Pending)
      {
         escapePoint->CheatOpenEscape();
      }
      else
      {
         // I had force false for parity with All, but maybe it should just be the default for this one?
         _PrintToConsole(TEXT("Escape route not pending (use 'EscapeOpen true' to force)"));
      }
   }
   else
   {
      _PrintToConsole(TEXT("Not looking at an escape point"));
   }
#endif
}

void UTATCheatManager::EscapeOpenAll(bool force)
{
#if OSE_CHEATS_ENABLED
   for (TActorIterator<ATATEscapePoint> it(GetWorld()); it; ++it)
   {
      ATATEscapePoint* escapePoint = (*it);
      if (force || escapePoint->GetState() == ETATEscapePointState::Pending)
      {
         escapePoint->CheatOpenEscape();
      }
   }
#endif
}

void UTATCheatManager::SetPhaseTimer(const FString& time)
{
#if OSE_CHEATS_ENABLED
   if (ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
   {
      if (const TOptional<int32> timeSeconds = _ParseDurationToSeconds(time))
      {
         gameState->AuthoritySetPhaseTimeRemaining((float)timeSeconds.GetValue());
         PrintPhase();
      }
      else
      {
         _PrintToConsole(FString::Printf(TEXT("Failed to set match timer: invalid time value '%s'"), *time));
      }
   }
#endif
}

void UTATCheatManager::PrintPhase()
{
#if OSE_CHEATS_ENABLED
   if (ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
   {
      _PrintToConsole(FString::Printf(TEXT("Phase: %s, Duration: %.1f, Remaining: %.1f"),
         *UEnum::GetValueAsString(gameState->GetCurrentPhase()),
         gameState->GetCurrentPhaseDuration(),
         gameState->GetTimeLeftInPhase()));
   }
#endif
}

void UTATCheatManager::StartEndgame(const FString& possibleTime)
{
#if OSE_CHEATS_ENABLED
   if (ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
   {
      if (const TOptional<int32> timeSeconds = _ParseDurationToSeconds(possibleTime))
      {
         gameState->AuthorityStartEndgameWithDuration(ETATEndgameReason::Mission, (float)timeSeconds.GetValue());
      }
      else
      {
         gameState->AuthorityStartEndgame(ETATEndgameReason::Mission);
      }
      
      PrintPhase();
   }
#endif
}

void UTATCheatManager::StartEndgameTimer(const FString& possibleTime)
{
#if OSE_CHEATS_ENABLED
   if (ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
   {
      if (const TOptional<int32> timeSeconds = _ParseDurationToSeconds(possibleTime))
      {
         gameState->AuthorityStartEndgameWithDuration(ETATEndgameReason::Timer, (float)timeSeconds.GetValue());
      }
      else
      {
         gameState->AuthorityStartEndgame(ETATEndgameReason::Timer);
      }
      
      PrintPhase();
   }
#endif
}

void UTATCheatManager::DamageSelf(float damageAmount, FName damageType /*= NAME_None*/)
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = GetOwnerPlayerController();
   APawn* pawn = pc->GetPawn();

   // Default to physical damage if we don't specify a type
   if (damageType == NAME_None)
   {
      damageType = "DamageType.Physical";
   }

   FGameplayTag damageTypeTag = FGameplayTag::RequestGameplayTag(damageType, false);
   if (damageTypeTag.IsValid())
   {
      UTATDamageFunctionLibrary::DealDamage(pawn, pawn, FTATDamageWithType(damageAmount, damageTypeTag), FVector::ZeroVector, FHitResult());
   }
   else
   {
      UE_LOG(LogTATCheatManager, Error, TEXT("DamageSelf: Damage type tag '%s' not recognised"), *damageType.ToString());
   }
#endif
}

void UTATCheatManager::RepairTarget()
{
#if OSE_CHEATS_ENABLED
   APlayerController* const pc = GetOwnerPlayerController();
   FHitResult hit;
   AActor* targetActor = GetTarget(pc, hit);
   if (targetActor)
   {
      if (UTATBreakableComponent* breakable = targetActor->GetComponentByClass<UTATBreakableComponent>())
      {
         breakable->ForceRepair();
      }
   }
#endif
}

void UTATCheatManager::RevealCombination()
{
#if OSE_CHEATS_ENABLED
   APlayerController* const pc = GetOwnerPlayerController();
   FHitResult hit;
   AActor* targetActor = GetTarget(pc, hit);
   bool found = false;
   if (targetActor)
   {
      if (UTATCombinationLockComponent* comboLock = targetActor->GetComponentByClass<UTATCombinationLockComponent>())
      {
         _PrintToConsole(FString::Printf(TEXT("Combination: %s"), *comboLock->GetCombinationText().ToString()));
         found = true;
      }
   }

   if(!found)
   {
      _PrintToConsole(TEXT("No combination lock found"));
   }
#endif
}

void UTATCheatManager::ForceUnlock()
{
#if OSE_CHEATS_ENABLED
   APlayerController* const pc = GetOwnerPlayerController();
   FHitResult hit;
   AActor* targetActor = GetTarget(pc, hit);

   // may not work for locked thing that are components (or weirder stuff), but this should work for 95%
   if (targetActor)
   {
      if (ILockpickableInterface* lockpickable = Cast<ILockpickableInterface>(targetActor))
      {
         lockpickable->Unlock();
      }
   }
#endif
}

void UTATCheatManager::DamageTarget(float damageAmount)
{
#if OSE_CHEATS_ENABLED
   APlayerController* const pc = GetOwnerPlayerController();
   FHitResult hit;
   AActor* targetActor = GetTarget(pc, hit);
   if (targetActor)
   {
      UTATDamageFunctionLibrary::DealDamage(pc->GetPawn(), targetActor, FTATDamageWithType(damageAmount, TAG_DamageType_Physical), hit.Location, hit);
   }
#endif
}

namespace CheatHelpers
{
   inline UTATToastSubsystem* GetToastSubsystem(APlayerController* pc)
   {
      if (pc != nullptr)
      {
         ULocalPlayer* player = pc->GetLocalPlayer();
         return (player != nullptr) ? player->GetSubsystem<UTATToastSubsystem>() : nullptr;
      }
      return nullptr;
   }

   FGameplayTag NormalizeToastId(APlayerController* pc, FName toastId)
   {
      if (toastId == NAME_None)
      {
         toastId = "Toast.Type.Gameplay.Default";
      }

      const FGameplayTag toastTag = FGameplayTag::RequestGameplayTag(toastId, false);

      UTATToastSubsystem* toastSubsystem = GetToastSubsystem(pc);
      if (toastSubsystem != nullptr && !toastSubsystem->IsValidToastType(toastTag))
      {
         // Try prefixing with "Toast.Type." to allow more concise cheats
         const FGameplayTag prefixedToastTag = FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("Toast.Type.%s"), *toastId.ToString())), false);
         if (prefixedToastTag.IsValid())
         {
            return prefixedToastTag;
         }
      }

      return toastTag;
   }
}

void UTATCheatManager::Toast(const FString& text, FName toastId)
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   UTATToastSubsystem* toastSubsystem = CheatHelpers::GetToastSubsystem(pc);
   if (toastSubsystem == nullptr)
   {
      return;
   }

   const FGameplayTag toastTag = CheatHelpers::NormalizeToastId(pc, toastId);
   if (!toastTag.IsValid())
   {
      // This specifically checks if toastId is a valid gameplay tag, not if it's a valid toast id.
      _PrintToConsole(FString::Printf(TEXT("Toast not sent: toast id '%s' is not a valid gameplay tag"), *toastId.ToString()));
      return;
   }
   else if (!toastSubsystem->IsValidToastType(toastTag))
   {
      _PrintToConsole(FString::Printf(TEXT("Toast not sent: toast id '%s' is not registered with the toast subsystem"), *toastTag.ToString()));
      return;
   }

   const bool onCooldown = toastSubsystem->IsToastOnCooldown(toastTag);
   static constexpr bool discardMessageIfOnCooldown = false;
   const bool toastSentOrQueued = toastSubsystem->RequestToastMessage(toastTag, FText::AsCultureInvariant(text), nullptr, discardMessageIfOnCooldown);
   if (!toastSentOrQueued)
   {
      // This shouldn't happen because we're already verifying that the toast id is valid, and we're not discarding messages on cooldown,
      // but if any of these assumptions change in the future this is very handy for debugging.
      _PrintToConsole(FString::Printf(TEXT("Toast not sent: unknown reason")));
      return;
   }

   if (toastSentOrQueued && onCooldown)
   {
      _PrintToConsole(FString::Printf(TEXT("Toast queued: toast id '%s' was on cooldown"), *toastTag.ToString()));
   }
#endif
}

void UTATCheatManager::ToastBroadcast(const FString& text, FName toastId, bool reliable)
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   const FGameplayTag toastTag = CheatHelpers::NormalizeToastId(pc, toastId);

   ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(pc);
   if (!toastBroadcaster)
   {
      _PrintToConsole(TEXT("Failed to send toast broadcast: Toast broadcast actor not found!"));
      return;
   }

   if (!toastTag.IsValid())
   {
      // This specifically checks if toastId is a valid gameplay tag, not if it's a valid toast id.
      _PrintToConsole(FString::Printf(TEXT("Toast not sent: toast id '%s' is not a valid gameplay tag"), *toastId.ToString()));
      return;
   }

   static constexpr bool discardMessageIfOnCooldown = false;
   toastBroadcaster->AuthoritySendUnformattedToastToAllPlayers(toastTag, FText::AsCultureInvariant(text), nullptr, discardMessageIfOnCooldown, reliable);
#endif
}

void UTATCheatManager::SpawnThiefVisionIndicator(FName indicatorId, float deduplicateDistance, int32 count)
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   if (thiefVisionSubsystem == nullptr)
   {
      return;
   }

   FGameplayTag indicatorType = FGameplayTag::RequestGameplayTag(indicatorId, false);

   // If the type isn't valid, first try adding the thief vision indicator prefix to the tag
   if (!indicatorType.IsValid() || !thiefVisionSubsystem->IsValidThiefVisionIndicatorType(indicatorType))
   {
      const FString prefixedName = FString::Format(TEXT("Indicator.ThiefVision.{0}"), { indicatorId.ToString() });
      indicatorType = FGameplayTag::RequestGameplayTag(FName(prefixedName), false);
   }

   // The type still isn't valid, so we can't spawn it
   if (!indicatorType.IsValid() || !thiefVisionSubsystem->IsValidThiefVisionIndicatorType(indicatorType))
   {
      _PrintToConsole(FString::Format(TEXT("Invalid thief vision indicator tag '{0}'"), { indicatorId.ToString() }));
      return;
   }

   AActor* instigator = pc->GetPawn();

   int32 numSpawned = 0;
   const float spreadDistance = FMath::Max(50.0f, deduplicateDistance + 1.0f);
   const int32 gridSize = FMath::Max(2, static_cast<int32>(FMath::Floor(FMath::Sqrt(static_cast<float>(count)))));

   constexpr float maxDistance = 2000.0f;
   FHitResult hitResult;
   _LineTracePlayerViewTarget(maxDistance, ECC_WorldDynamic, &hitResult);
   for (int32 i = 0; i < count; i++)
   {
      FVector loc = hitResult.Location;
      loc.X += spreadDistance * (i % gridSize);
      loc.Y += spreadDistance * (i / gridSize);
      if (thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(indicatorType, FTransform(loc), deduplicateDistance, instigator))
      {
         ++numSpawned;
      }
      else
      {
         _PrintToConsole(FString::Format(TEXT("FAILED spawning thief vision indicator (#{0}) of type '{1}'"), { i, indicatorType.ToString() }));
         break;
      }
   }

   if (numSpawned > 0)
   {
      _PrintToConsole(FString::Format(TEXT("Spawned {0} thief vision indicator(s) of type '{1}'"), { numSpawned, indicatorType.ToString() }));
   }
#endif
}

void UTATCheatManager::RemoveMyThiefVisionIndicators()
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   if (thiefVisionSubsystem == nullptr)
   {
      return;
   }

   int32 numRemoved = 0;
   AActor* instigator = pc->GetPawn();
   if (instigator != nullptr)
   {
      FTATThiefVisionIndicatorQuery query{};
      query.FilterByInstigator = true;
      query.Instigator = instigator;
      numRemoved = thiefVisionSubsystem->AuthorityRemoveThiefVisionIndicatorsMatchingQuery(query);
   }
   _PrintToConsole(FString::Printf(TEXT("Removed %i Thief Vision indicators owned by %s"), numRemoved, *GetNameSafe(instigator)));
#endif
}

void UTATCheatManager::RemoveAllThiefVisionIndicators()
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   if (thiefVisionSubsystem == nullptr)
   {
      return;
   }

   // An empty query will match ALL indicators
   FTATThiefVisionIndicatorQuery query{};
   const int32 numRemoved = thiefVisionSubsystem->AuthorityRemoveThiefVisionIndicatorsMatchingQuery(query);
   _PrintToConsole(FString::Printf(TEXT("Removed %i Thief Vision indicators"), numRemoved));
#endif
}

void UTATCheatManager::GlyphDebug()
{
#if OSE_CHEATS_ENABLED
   const bool newEnabled = !UTATGlyphComponent::IsGlyphDebugModeEnabled();
   constexpr bool setFromCheat = true;
   UTATGlyphComponent::SetGlyphDebugMode(newEnabled, setFromCheat);
   _PrintToConsole(newEnabled ? TEXT("Enabled glyph debugging") : TEXT("Disabled glyph debugging"));
#endif
}

void UTATCheatManager::WeatherType(const FString& weatherTypeTag)
{
#if OSE_CHEATS_ENABLED
   UDataTable* weatherDataTable = UTATWeatherSettings::Get().WeatherTypeDataTable.LoadSynchronous();
   if (weatherDataTable == nullptr)
   {
      return;
   }

   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   UWorld* world = pc->GetWorld();
   check(world != nullptr);

   constexpr bool errorIfNotFound = false;
   const FTATWeatherTypeInfo* weatherTypeInfo = UTATWeatherSettings::Get().FindWeatherTypeInfo(
      FGameplayTag::RequestGameplayTag(FName(weatherTypeTag), errorIfNotFound), weatherDataTable);

   if (weatherTypeInfo == nullptr && !weatherTypeTag.StartsWith(TEXT("Weather.Type."), ESearchCase::IgnoreCase))
   {
      weatherTypeInfo = UTATWeatherSettings::Get().FindWeatherTypeInfo(
         FGameplayTag::RequestGameplayTag(FName(TEXT("Weather.Type.") + weatherTypeTag), errorIfNotFound), weatherDataTable);
   }

   if (weatherTypeInfo == nullptr)
   {
      _PrintToConsole(FString::Printf(TEXT("Weather type '%s' not found"), *weatherTypeTag));
      return;
   }

   TSoftClassPtr<ATATWeatherPreset> presetClass = UTATWeatherSettings::GetWeatherPresetClassForWeatherType(world, weatherTypeInfo->WeatherType);

   // This shouldn't happen - we probably resolved a preset that someone left as a null reference somewhere.
   // If we have a "canonical" preset, set that instead
   if (presetClass.IsNull() && !weatherTypeInfo->DefaultPreset.IsNull())
   {
      presetClass = weatherTypeInfo->DefaultPreset;
   }

   if (!presetClass.IsNull())
   {
      ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
      if (worldSettings != nullptr && worldSettings->WeatherManager != nullptr)
      {
         _PrintToConsole(FString::Printf(TEXT("Setting weather type to '%s' (preset: %s)"), *weatherTypeTag, *presetClass.ToString()));
         worldSettings->WeatherManager->SetWeatherPresetType(presetClass.LoadSynchronous());
      }
      else
      {
         _PrintToConsole(TEXT("Failed to set weather type - level does not have a configured weather manager"));
      }
   }
   else
   {
      _PrintToConsole(FString::Printf(TEXT("Weather type '%s' does not have a valid preset class assigned"), *weatherTypeTag));
   }
#endif
}

void UTATCheatManager::WeatherPreset(const FString& presetClassName)
{
#if OSE_CHEATS_ENABLED
   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   UWorld* world = pc->GetWorld();
   check(world != nullptr);
   ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
   ATATWeatherManager* weatherManager = (worldSettings != nullptr) ? worldSettings->WeatherManager : nullptr;

   if (weatherManager == nullptr)
   {
      _PrintToConsole(TEXT("Failed to set weather preset - level does not have a configured weather manager"));
      return;
   }

   TSubclassOf<ATATWeatherPreset> presetClass = _FindBlueprintClassByName<ATATWeatherPreset>(presetClassName);
   if (!presetClass)
   {
      presetClass = _FindBlueprintClassByName<ATATWeatherPreset>(TEXT("BP_") + presetClassName);
   }
   if (!presetClass)
   {
      presetClass = _FindBlueprintClassByName<ATATWeatherPreset>(TEXT("BP_WeatherPreset_") + presetClassName);
   }
   if (!presetClass)
   {
      _PrintToConsole(FString::Printf(TEXT("Invalid weather preset class '%s'"), *presetClassName));
      return;
   }

   _PrintToConsole(FString::Printf(TEXT("Setting weather preset to '%s'"), *FSoftClassPath(presetClass).ToString()));
   weatherManager->SetWeatherPresetType(presetClass);
#endif
}

void UTATCheatManager::RefreshLevelDepthmap()
{
#if OSE_CHEATS_ENABLED
   ATATWeatherManager* weatherManager = ATATWeatherManager::Get(this);
   if (weatherManager == nullptr)
   {
      _PrintToConsole(TEXT("Level does not have a configured weather manager"));
      return;
   }

   constexpr bool forceUpdate = true;
   const bool success = weatherManager->UpdateSceneDepthTexture(forceUpdate);
   _PrintToConsole(success ? TEXT("Level scene depth texture updated") : TEXT("Failed to update scene depth texture"));
#endif
}

void UTATCheatManager::DebugTemporalTraces()
{
#if OSE_CHEATS_ENABLED
   ATATWeatherManager* weatherManager = ATATWeatherManager::Get(this);
   if (weatherManager == nullptr)
   {
      _PrintToConsole(TEXT("Level does not have a configured weather manager"));
      return;
   }
   const bool newEnabled = !weatherManager->GetTemporalTraceDebuggingEnabled();
   weatherManager->SetTemporalTraceDebuggingEnabled(newEnabled);
   _PrintToConsole(newEnabled ? TEXT("Temporal trace debugging enabled") : TEXT("Temporal trace debugging disabled"));
#endif
}

void UTATCheatManager::MatchSetting(FName propName, const FString& newValue)
{
#if OSE_CHEATS_ENABLED
   UTATGameInstance& gameInstance = UTATGameInstance::Get(this);
   UTATMatchSettingsBase& matchSettings = gameInstance.GetMatchSettings();

   if (newValue.Len() == 0)
   {
      // just get and print the value to the console
      FString valueStr;
      if (matchSettings.GetMatchSettingsValueAsString(propName, valueStr))
      {
         _PrintToConsole(FString::Printf(TEXT("%s = %s"), *propName.ToString(), *valueStr));
      }
      else
      {
         _PrintToConsole(FString::Printf(TEXT("Failed getting match settings property '%s': property does not exist"), *propName.ToString()));
      }
   }
   else
   {
      const ATATDemoHubMissionMgr* missionManager = ATATDemoHubMissionMgr::GetDemoHubMissionManager(this);
      const TSoftObjectPtr<UWorld> selectedMap = missionManager ? missionManager->GetSelectedMap() : nullptr;

      FTATMatchSettingsQueryContext queryContext;
      if (!selectedMap.IsNull())
      {
         queryContext = FTATMatchSettingsQueryContext(selectedMap, this);
      }
      else
      {
         queryContext = FTATMatchSettingsQueryContext::MakeFromWorldContext(this);
      }

      // set a new value
      FString errorMsg;
      if (matchSettings.SetMatchSettingsValueAsString(propName, newValue, queryContext, &errorMsg))
      {
         _PrintToConsole(FString::Printf(TEXT("Set match settings property %s to '%s'"), *propName.ToString(), *newValue));

         // We just set the local copy of match settings, now we need to replicate that change down to all clients.
         // This doesn't happen automatically because this isn't supported outside of this cheat anyway.
         gameInstance.AuthorityNotifyCheatUpdatedMatchSettings();
      }
      else
      {
         _PrintToConsole(FString::Printf(TEXT("Failed setting match settings property '%s': %s"), *propName.ToString(), *errorMsg));
      }
   }
#endif
}

namespace CheatHelpers
{
void ListMatchSettingsInternal(const UTATMatchSettingsBase& matchSettings, TFunctionRef<void(const FString&)> printFunc)
{
   static const TCHAR* indent = TEXT("        ");

   TStringBuilder<127> line;
   auto printAndResetLine = [&line, &printFunc]()
   {
      if (line.Len() > 0)
      {
         printFunc(line.ToString());
         line.Reset();
      }
   };

   TArray<FTATMatchSettingsPropertyDef> allProps;
   matchSettings.GetAllMatchSettingsProperties(allProps);
   FString propValue;
   for (const FTATMatchSettingsPropertyDef& prop : allProps)
   {
      // name
      line << prop.Name.ToString() << TEXT(" = ");
      // value
      matchSettings.GetMatchSettingsValueAsString(prop.Name, propValue);
      line << TEXT("'") << propValue << TEXT("'");
      // type
      UEnum* propEnumType = matchSettings.GetMatchSettingsEnumType(prop.Name);
      if (prop.Type == ETATMatchSettingsPropertyType::Enum && propEnumType != nullptr)
      {
         line << TEXT(" [Enum(") << propEnumType->GetName() << TEXT(")]");
      }
      else if (prop.Type == ETATMatchSettingsPropertyType::GameplayTag)
      {
         line << TEXT(" [GameplayTagGroup(") << prop.GameplayTagGroup.ToDebugString() << TEXT(")]");
      }
      else
      {
         if (UEnum* enumType = StaticEnum<ETATMatchSettingsPropertyType>())
         {
            line << TEXT(" [") << enumType->GetNameStringByValue(static_cast<int64>(prop.Type)) << TEXT("]");
         }
      }
      printAndResetLine();

      // description
      if (!prop.Description.IsEmpty())
      {
         line << indent << prop.Description.ToString();
         printAndResetLine();
      }

      // value bounds
      if (prop.UseMinValue || prop.UseMaxValue)
      {
         line << indent;

         auto boundsToString = [&prop](double val)
         {
            return (prop.Type == ETATMatchSettingsPropertyType::Integer) ? FString::FromInt(FMath::RoundToInt32(val)) : FString::SanitizeFloat(val);
         };

         if (prop.UseMinValue && prop.UseMaxValue)
         {
            line << TEXT("Min = ") << boundsToString(prop.MinValue) << TEXT(", Max = ") << boundsToString(prop.MaxValue);
         }
         else if (prop.UseMinValue && !prop.UseMaxValue)
         {
            line << TEXT("Min = ") << boundsToString(prop.MinValue);
         }
         else if (!prop.UseMinValue && prop.UseMaxValue)
         {
            line << TEXT("Max = ") << boundsToString(prop.MaxValue);
         }
         printAndResetLine();
      }

      // If this is an enum or gameplay tag type, dump all possible values to the console to make it self-documenting
      if (propEnumType != nullptr)
      {
         line << indent << TEXT("AllEnumValues=(");
         const int64 maxValue = propEnumType->GetMaxEnumValue();
         for (int32 i = 0; i < propEnumType->NumEnums(); i++)
         {
            if (propEnumType->GetValueByIndex(i) >= maxValue)
            {
               break;
            }
            if (i > 0)
            {
               line << TEXT(", ");
            }
            line << propEnumType->GetNameStringByValue(i);
         }
         line << TEXT(")");
         printAndResetLine();
      }
      else if (prop.Type == ETATMatchSettingsPropertyType::GameplayTag)
      {
         FTATMatchSettingsGameplayTagGroup gameplayTagGroup;
         if (matchSettings.GetMatchSettingsGameplayTagGroup(prop.Name, gameplayTagGroup))
         {
            line << indent << TEXT("ValidTags=(") << gameplayTagGroup.ToTagListDebugString() << TEXT(")");
            printAndResetLine();
         }
      }
   }
   printFunc(FString());
}
} // namespace CheatHelpers

void UTATCheatManager::ListMatchSettings()
{
#if OSE_CHEATS_ENABLED
   CheatHelpers::ListMatchSettingsInternal(
      UTATGameInstance::Get(this).GetMatchSettings(),
      [this](const FString& line) { _PrintToConsole(line); });
#endif
}

void UTATCheatManager::ListMatchSettingsLocal()
{
#if OSE_CHEATS_ENABLED
   CheatHelpers::ListMatchSettingsInternal(
      UTATGameInstance::Get(this).GetMatchSettings(),
      [this](const FString& line) { _PrintToConsole(line); });
#endif
}

void UTATCheatManager::PowerSurge(float durationSeconds)
{
#if OSE_CHEATS_ENABLED
   durationSeconds = FMath::Max(0.25f, durationSeconds);

   UTATPowerNetworkComponent* powerNetworkComponent = nullptr;

   constexpr float maxDistance = 4000.f;
   if (AActor* viewTarget = _LineTracePlayerViewTarget(maxDistance))
   {
      powerNetworkComponent = UTATPowerNetworkComponent::GetPowerNetworkComponent(viewTarget);
   }
   else if (AActor* actor = _FindNearbyActorMatchingPredicate(maxDistance, [](AActor* actor) { return UTATPowerNetworkComponent::GetPowerNetworkComponent(actor) != nullptr; }))
   {
      powerNetworkComponent = UTATPowerNetworkComponent::GetPowerNetworkComponent(actor);
   }

   if (powerNetworkComponent != nullptr)
   {
      if (powerNetworkComponent->AuthorityTriggerPowerSurge(durationSeconds))
      {
         _PrintToConsole(FString::Printf(TEXT("Triggered %.2f second power surge from actor %s"), durationSeconds, *GetNameSafe(powerNetworkComponent->GetOwner())));
      }
      else if (!powerNetworkComponent->IsPowered())
      {
         _PrintToConsole(FString::Printf(TEXT("Failed to trigger power surge from actor %s: actor is unpowered"), *GetNameSafe(powerNetworkComponent->GetOwner())));
      }
      else
      {
         _PrintToConsole(FString::Printf(TEXT("Failed to trigger power surge from actor %s"), *GetNameSafe(powerNetworkComponent->GetOwner())));
      }
   }
   else
   {
      _PrintToConsole(TEXT("Failed to trigger power surge: no nearby actor with a power network component found"));
   }
#endif
}

void UTATCheatManager::PowerNetworkDebugger()
{
#if OSE_CHEATS_ENABLED
   if (UTATPowerNetworkSubsystem* powerNetworkSubsystem = GetWorld()->GetSubsystem<UTATPowerNetworkSubsystem>())
   {
      powerNetworkSubsystem->TogglePowerNetworkDebugVis(GetPlayerController());
   }
#endif
}

void UTATCheatManager::DebugHUDIndicators()
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (ATATHUD* hud = pc->GetHUD<ATATHUD>())
      {
         hud->ToggleDebugDrawHUDIndicators();
      }
   }
#endif
}

void UTATCheatManager::FtueSetSavedState(ETATSavedFtueState state)
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->SetFtueState(state);
      save->DispatchProgressionUnexpectedlyChanged();
      FtuePrintSavedState();
   }
#endif
}

void UTATCheatManager::FtuePrintSavedState()
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      _PrintToConsole(FString::Printf(TEXT("FTUE state: '%s'"), *StaticEnum<ETATSavedFtueState>()->GetDisplayValueAsText(save->GetFtueState()).ToString()));
   }
#endif
}

void UTATCheatManager::FtueReset()
{
   FtueSetSavedState(ETATSavedFtueState::Unstarted);
}

void UTATCheatManager::FtueSkip()
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->SetFtueState(ETATSavedFtueState::Complete);
      for (const TPair<FGameplayTag, ETATContractState>& contractPair : UTATProjectSettings::Get().PostFtueContracts)
      {
         save->SetContractState(contractPair.Key, contractPair.Value);
      }
      save->AddUnlocks(UTATProjectSettings::Get().PostFtueUnlockedContent);
      save->DispatchProgressionUnexpectedlyChanged();
      FtuePrintSavedState();
   }
#endif
}

void UTATCheatManager::UnlockAdd(const FString& unlockableTagString)
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      const FGameplayTag unlockableTag = FGameplayTag::RequestGameplayTag(FName(unlockableTagString), false);
      if(!unlockableTag.IsValid())
      {
         _PrintToConsole(FString::Printf(TEXT("Unknown unlockable tag '%s'"), *unlockableTagString));
         return;
      }
      save->AddUnlock(unlockableTag);
   }
#endif
}

void UTATCheatManager::UnlockRemove(const FString& unlockableTagString)
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      const FGameplayTag unlockableTag = FGameplayTag::RequestGameplayTag(FName(unlockableTagString), false);
      if(!unlockableTag.IsValid())
      {
         _PrintToConsole(FString::Printf(TEXT("Unknown unlockable tag '%s'"), *unlockableTagString));
         return;
      }
      save->RemoveUnlock(unlockableTag);
   }
#endif
}

void UTATCheatManager::UnlockResetAll()
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->ResetUnlocks();
   }
#endif
}

void UTATCheatManager::UnlockResetSeen()
{
#if OSE_CHEATS_ENABLED
   if (UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this))
   {
      save->ClearSeenUnlocks();
   }
#endif
}

void UTATCheatManager::UserSettingSetFloat(const FString& settingTagName, float newValue)
{
#if OSE_CHEATS_ENABLED
   if (UTATGameUserSettings* settings = UTATGameUserSettings::Get())
   {
      FString tagError;
      TOptional<FGameplayTag> settingTag = _FindGameplayTag(settingTagName, TEXT("Settings"), FGameplayTag::RequestGameplayTag("Settings"), &tagError);
      if (!settingTag)
      {
         _PrintToConsole(FString::Printf(TEXT("UserSettingSetFloat failed: %s"), *tagError));
         return;
      }
      if (settings->SetSetting(*settingTag, newValue))
      {
         settings->ApplySetting(*settingTag);
         _PrintToConsole(FString::Printf(TEXT("%s -> %f"), *(settingTag.GetValue().ToString()), newValue));
      }
      else
      {
         _PrintToConsole(FString::Printf(TEXT("Error setting %s -> %f"), *(settingTag.GetValue().ToString()), newValue));
      }
   }
#endif
}

void UTATCheatManager::UserSettingSetBool(const FString& settingTagName, bool newValue)
{
#if OSE_CHEATS_ENABLED
   if (UTATGameUserSettings* settings = UTATGameUserSettings::Get())
   {
      FString tagError;
      TOptional<FGameplayTag> settingTag = _FindGameplayTag(settingTagName, TEXT("Settings"), FGameplayTag::RequestGameplayTag("Settings"), &tagError);
      if (!settingTag)
      {
         _PrintToConsole(FString::Printf(TEXT("UserSettingSetBool failed: %s"), *tagError));
         return;
      }
      if (settings->SetSetting(*settingTag, newValue))
      {
         settings->ApplySetting(*settingTag);
         _PrintToConsole(FString::Printf(TEXT("%s -> %s"), *(settingTag.GetValue().ToString()), newValue ? TEXT("True") : TEXT("False")));
      }
      else
      {
         _PrintToConsole(FString::Printf(TEXT("Error setting %s -> %s"), *(settingTag.GetValue().ToString()), newValue ? TEXT("True") : TEXT("False")));
      }
   }
#endif
}

void UTATCheatManager::ZZZ_CrashTheGame_NullPtr()
{
#if OSE_CHEATS_ENABLED
   AActor* actor = nullptr; // intentionally null to crash the game!
   actor->GetName();
#endif
}

void UTATCheatManager::ZZZ_CrashTheGame_Assert()
{
#if OSE_CHEATS_ENABLED
   // intentionally crashes!
   check(false);
#endif
}

void UTATCheatManager::FastForward(const FString& duration, float rate)
{
#if OSE_CHEATS_ENABLED
   const TOptional<int32> timeSeconds = _ParseDurationToSeconds(duration);
   if(!timeSeconds)
   {
      _PrintToConsole(FString::Printf(TEXT("FastForward: invalid time value '%s'"), *duration));
      return;
   }

   // Relay to server, also do it on client for pawn + progress
   APlayerController* pc = GetOwnerPlayerController();
   if (!pc->HasAuthority())
   {
      pc->ServerExec(FString::Printf(TEXT("FastForward %s %f"), *duration, rate));
   }

   GetWorld()->GetTimerManager().SetTimer(
      _fastForwardTimerHandle,
      FTimerDelegate::CreateUObject(this, &UTATCheatManager::_OnFastForwardComplete),
      *timeSeconds, false);

   GetWorld()->GetTimerManager().SetTimer(
      _fastForwardProgressHandle,
      FTimerDelegate::CreateUObject(this, &UTATCheatManager::_OnFastForwardProgress),
      1, true);

   const float clampedRate = pc->GetWorldSettings()->SetTimeDilation(rate);

   // counter balance the dilation on the pawn, just because it is annoying to move super fast, but not everything will be slowed
   if (APawn* pawn = pc->GetPawn())
   {
      pawn->CustomTimeDilation = 1 / clampedRate;
   }
   _PrintToConsole(TEXT("FastForward started"));
#endif
}

void UTATCheatManager::ResetRespawnTotem()
{
#if OSE_CHEATS_ENABLED
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (ATATPlayerState* playerState = pc->GetPlayerState<ATATPlayerState>())
      {
         playerState->AuthoritySetTotemRespawnCountDirect(0);
      }
   }
#endif
}

void UTATCheatManager::DevTools()
{
#if OSE_CHEATS_ENABLED
#if TAT_ENABLE_DEV_TOOLS
   UTATEditorSettings& editorSettings = UTATEditorSettings::GetMutable();
   editorSettings.EnableDevToolUI = !editorSettings.EnableDevToolUI;
#else
   _PrintErrorToConsole(TEXT("Dev tools are disabled in this build"));
#endif // TAT_ENABLE_DEV_TOOLS
#endif // OSE_CHEATS_ENABLED
}

void UTATCheatManager::DevTool(const FString& name)
{
#if OSE_CHEATS_ENABLED
#if TAT_ENABLE_DEV_TOOLS
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (UWorld* world = pc->GetWorld())
      {
         if (UTATDevToolSubsystem* devToolSubsystem = world->GetSubsystem<UTATDevToolSubsystem>())
         {
            const bool success = devToolSubsystem->ToggleDevToolByName(name);
            if (!success)
            {
               _PrintErrorToConsole(FString::Printf(TEXT("ToggleDevToolByName: No dev tools found matching the name '%s'"), *name));
            }
         }
      }
   }
#else
   _PrintErrorToConsole(TEXT("Dev tools are disabled in this build"));
#endif // TAT_ENABLE_DEV_TOOLS
#endif // OSE_CHEATS_ENABLED
}

void UTATCheatManager::DevToolsList()
{
#if OSE_CHEATS_ENABLED
#if TAT_ENABLE_DEV_TOOLS
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      if (UWorld* world = pc->GetWorld())
      {
         if (UTATDevToolSubsystem* devToolSubsystem = world->GetSubsystem<UTATDevToolSubsystem>())
         {
            TArray<FString> names;
            devToolSubsystem->GetAllDevToolNames(names);
            _PrintToConsole(FString::Printf(TEXT("There are %i registered dev tools"), names.Num()));
            for (const FString& devToolName : names)
            {
               _PrintToConsole(FString::Printf(TEXT("    * %s"), *devToolName));
            }
         }
      }
   }
#else
   _PrintErrorToConsole(TEXT("Dev tools are disabled in this build"));
#endif // TAT_ENABLE_DEV_TOOLS
#endif // OSE_CHEATS_ENABLED
}

TSubclassOf<UTATItemInfo> UTATCheatManager::_LoadItemForName(FName itemName) const
{
   FPrimaryAssetId itemId(UItemInfo::PrimaryAssetType, itemName);
   TSoftClassPtr<UTATItemInfo> classPath(UAssetManager::Get().GetPrimaryAssetPath(itemId));
   TSubclassOf<UTATItemInfo> itemInfo = classPath.LoadSynchronous();
   if (!itemInfo.Get())
   {
      UE_LOG(LogTATCheatManager, Warning, TEXT("Could not find item with name '%s'"), *itemName.ToString());
   }
   return itemInfo;
}

namespace CheatHelpers
{
   /// Given a path like "/Stuff/And/Things", returns the first directory component (eg. "Stuff").
   FStringView GetTopLevelDirName(FStringView path)
   {
      if (path.IsEmpty())
      {
         return path;
      }

      // skip over leading slash
      if (path.StartsWith('/'))
      {
         path.RemovePrefix(1);
      }

      // return the substring between the current start and the next slash character
      int32 nextPathSep = INDEX_NONE;
      if (!path.FindChar('/', nextPathSep))
      {
         return path;
      }

      return path.Left(nextPathSep);
   }
}

// static
void UTATCheatManager::_ForEachBlueprintClassMatchingQuery(UClass* baseClass, const FString& query, TFunctionRef<bool(UClass*)> callback, TConstArrayView<FString> topLevelDirs)
{
   const bool baseClassIsNativeClass = baseClass != nullptr && baseClass->IsNative();

   // For some reason, EnumerateAssets can return the same asset multiple times, so use a set to deduplicate.
   TSet<UClass*, DefaultKeyFuncs<UClass*>, TInlineSetAllocator<32>> foundClasses;

   FARFilter assetFilter{};
   assetFilter.PackagePaths.Add(TEXT("/Game"));
   assetFilter.bRecursivePaths = true;
   assetFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
   assetFilter.bRecursiveClasses = true;

   static constexpr bool continueEnumerating = true;
   const FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
   assetRegistryModule.Get().EnumerateAssets(assetFilter, [&](const FAssetData& assetData) -> bool
   {
      // If we have a top-level dir name filter, only fire the callback if the top-level directory name is in the array
      if (topLevelDirs.Num() > 0)
      {
         const FString path = assetData.PackagePath.ToString();
         FStringView pathView = path;

         // Skip over "/Game" prefix
         if (ensure(pathView.StartsWith(TEXT("/Game"))))
         {
            pathView.RemovePrefix(5);
         }

         const FStringView topLevelDirName = CheatHelpers::GetTopLevelDirName(pathView);
         if (!topLevelDirs.ContainsByPredicate([topLevelDirName](const FString& str) { return FStringView(str).Equals(topLevelDirName, ESearchCase::IgnoreCase); }))
         {
            return continueEnumerating;
         }
      }

      // Check the asset name against our search query
      if (!query.IsEmpty() && !assetData.AssetName.ToString().Contains(query, ESearchCase::IgnoreCase))
      {
         return continueEnumerating;
      }

      // Before checking the class directly (which requires loading the asset), if we have a native base class we can try checking asset registry tags first
      if (baseClassIsNativeClass)
      {
         FString assetNativeParentClassPath;
         if (assetData.GetTagValue(FBlueprintTags::NativeParentClassPath, assetNativeParentClassPath) && !assetNativeParentClassPath.IsEmpty())
         {
            UObject* outer = nullptr;
            ResolveName(outer, assetNativeParentClassPath, false, false);
            UClass* assetNativeParentClass = FindObject<UClass>(outer, *assetNativeParentClassPath);
            if (!assetNativeParentClass || !assetNativeParentClass->IsChildOf(baseClass))
            {
               return continueEnumerating;
            }
         }
      }

      // In order to check the actual class, we unfortunately have to load it
      UBlueprint* bp = Cast<UBlueprint>(assetData.GetAsset());
      UClass* cls = bp ? bp->GeneratedClass.Get() : assetData.GetClass(EResolveClass::Yes);
      if (cls != nullptr && !foundClasses.Contains(cls) && (baseClass == nullptr || cls->IsChildOf(baseClass)))
      {
         foundClasses.Add(cls);
         return callback(cls);
      }
      return continueEnumerating;
   });
}

// static
UClass* UTATCheatManager::_FindBlueprintClassByName(const FString& name, UClass* baseClass)
{
   const FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

   // Get full path of package by its short name
   const FName packageName = assetRegistryModule.Get().GetFirstPackageByName(*name);
   if (!packageName.IsNone())
   {
      // Load the class's package
      if (UPackage* package = LoadPackage(nullptr, *packageName.ToString(), LOAD_None))
      {
         // Get the generated class from the package (requires the _C suffix on the short name)
         const FString className = name + TEXT("_C");
         UClass* generatedClass = FindObject<UClass>(package, *className);
         if (generatedClass != nullptr && (baseClass == nullptr || generatedClass->IsChildOf(baseClass)))
         {
            return generatedClass;
         }
      }
   }

   return nullptr;
}

UObject* UTATCheatManager::_FindAssetByName(FStringView nameString, UClass* assetClass)
{
   const FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
   const FName packageName = assetRegistryModule.Get().GetFirstPackageByName(nameString);
   if(!packageName.IsValid())
   {
      return nullptr;
   }
   
   FARFilter filter;
   filter.ClassPaths.Add(assetClass->GetClassPathName());
   filter.PackageNames.Add(packageName);

   UObject* result = nullptr;
   assetRegistryModule.Get().EnumerateAssets(filter, [&](const FAssetData& assetData)
   {
      result = assetData.GetAsset();
      return false;
   });

   return result;
}

TSubclassOf<UToolComponent> UTATCheatManager::_LoadToolByName(const FString& toolName) const
{
   // If the tool name argument is the exact name of a tool class, return it
   if (TSubclassOf<UToolComponent> toolClass = _FindBlueprintClassByName<UToolComponent>(toolName))
   {
      return toolClass;
   }

   // If the tool name would be valid if we added a "BP_" prefix and/or a "_Tool" suffix, return that
   static const TCHAR* kToolBlueprintPrefix = TEXT("BP_");
   static const TCHAR* kToolBlueprintSuffix = TEXT("_Tool");
   const bool missingPrefix = !toolName.StartsWith(kToolBlueprintPrefix);
   const bool missingSuffix = !toolName.EndsWith(kToolBlueprintSuffix);
   if (missingPrefix || missingSuffix)
   {
      TStringBuilder<64> toolNameBuilder;
      if (missingPrefix)
      {
         toolNameBuilder.Append(kToolBlueprintPrefix);
      }
      toolNameBuilder.Append(toolName);
      if (missingSuffix)
      {
         toolNameBuilder.Append(kToolBlueprintSuffix);
      }

      if (TSubclassOf<UToolComponent> toolClass = _FindBlueprintClassByName<UToolComponent>(toolNameBuilder.ToString()))
      {
         return toolClass;
      }
   }

   UE_LOG(LogTATCheatManager, Warning, TEXT("Could not find tool with name '%s'"), *toolName);

   return nullptr;
}

const FTATLootInfo* UTATCheatManager::_FindLootInfoByTag(FName lootTag)
{
   auto findLootInfoByTag = [this](FName tag) -> const FTATLootInfo*
   {
      FTATLootIdentifier identifier;
      identifier.LootTag = FGameplayTag::RequestGameplayTag(tag, false);
      if (identifier.IsValid())
      {
         return UTATLootSettings::Get().FindLootInfo(this, identifier);
      }
      return nullptr;
   };

   TStringBuilder<127> tagBuilder;

   const FTATLootInfo* lootInfo = findLootInfoByTag(lootTag);

   // If we didn't find a valid loot info, try prefixing the tag with "Loot.Major."
   if (lootInfo == nullptr)
   {
      tagBuilder.Reset();
      tagBuilder.Append(TEXT("Loot.Major."));
      tagBuilder.Append(lootTag.ToString());
      lootInfo = findLootInfoByTag(FName(*tagBuilder));
   }

   // If we still didn't find a valid loot info, try prefixing the tag with "Loot.Minor."
   if (lootInfo == nullptr)
   {
      tagBuilder.Reset();
      tagBuilder.Append(TEXT("Loot.Minor."));
      tagBuilder.Append(lootTag.ToString());
      lootInfo = findLootInfoByTag(FName(*tagBuilder));
   }

   // If we still didn't find a valid loot info, try prefixing the tag with "Loot.Quest."
   if (lootInfo == nullptr)
   {
      tagBuilder.Reset();
      tagBuilder.Append(TEXT("Loot.Quest."));
      tagBuilder.Append(lootTag.ToString());
      lootInfo = findLootInfoByTag(FName(*tagBuilder));
   }

   if (lootInfo == nullptr)
   {
      _PrintToConsole(FString::Printf(TEXT("Failed to find loot: loot tag '%s' is not a valid loot item type (also tried 'Loot.Major.%s', 'Loot.Minor.%s', and 'Loot.Quest.%s')"),
         *lootTag.ToString(), *lootTag.ToString(), *lootTag.ToString(), *lootTag.ToString()));
      return nullptr;
   }
   else
   {
      return lootInfo;
   }
}

TSoftObjectPtr<class UTATSceneVariantConfig> UTATCheatManager::_FindVariantForName(const FString& variantString) const
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
   FARFilter filter;
   filter.ClassPaths.Add(UTATSceneVariantConfig::StaticClass()->GetClassPathName());

   TSoftObjectPtr<class UTATSceneVariantConfig> result;
   FName variantName(variantString);
   assetRegistryModule.Get().EnumerateAssets(filter, [&result, variantName](const FAssetData& assetData) {
      if (assetData.AssetName == variantName)
      {
         result = assetData.GetSoftObjectPath();
         return false;
      }

      return true;
   });

   return result;
}

FString UTATCheatManager::_GetPlayerName(APlayerController* pc) const
{
   FString playerName;

   if (pc == nullptr)
   {
      pc = GetOwnerPlayerController();
   }

   if (pc != nullptr)
   {
      if (APlayerState* ps = pc->GetPlayerState<APlayerState>())
      {
         playerName = ps->GetPlayerName();
      }

      if (playerName.Len() == 0)
      {
         APawn* pawn = pc->GetPawnOrSpectator();
         playerName = (pawn != nullptr) ? pawn->GetName() : pc->GetName();
      }
   }

   return playerName;
}

AActor* UTATCheatManager::_SpawnActorForPlayer(UClass* actorClass, float dropDistance) const
{
   if (actorClass == nullptr)
   {
      return nullptr;
   }

   APawn* pawn = nullptr;
   if (APlayerController* pc = GetOwnerPlayerController())
   {
      pawn = pc->GetPawn();
   }

   if (pawn == nullptr)
   {
      return nullptr;
   }

   AActor* owner = nullptr;
   APawn* instigator = nullptr;
   AActor* newActor = GetWorld()->SpawnActorDeferred<AActor>(actorClass, FTransform(pawn->GetActorForwardVector().GetSafeNormal().Rotation()),
      owner, instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
   if (newActor != nullptr)
   {
      ATATItemActor* itemActor = Cast<ATATItemActor>(newActor);
      if (dropDistance == 0)
      {
         dropDistance = (itemActor != nullptr) ? itemActor->GetDropDistance() : 100.0f;
      }

      UGameplayStatics::FinishSpawningActor(newActor, FTransform());

      // We spawn at origin and then move to the drop location because the spawned actor's
      // root component not initialized until after FinishSpawningActor, and we need it
      // to calculate the final position.
      const FCollisionProfileName dropTraceProfile = UTATProjectSettings::Get().ItemDropTraceProfile;
      const FVector dropLocation = UTATItemFunctionLibrary::FindSuggestedDropStartFromActorEyes(pawn, dropDistance, dropTraceProfile);

      FVector newDropLocation;
      if (!UTATItemFunctionLibrary::FindDropLocationFromSuggestedStart(newActor, pawn, dropTraceProfile, dropLocation, newDropLocation))
      {
         // Failed to find spawn location, just raise it vertically so the bounding box doesn't intersect the floor
         FVector _origin, extent;
         newActor->GetActorBounds(true, _origin, extent);
         newDropLocation.Z += extent.Z * 0.5f;
      }

      if (itemActor != nullptr)
      {
         itemActor->HandleDrop(dropLocation, newDropLocation, pawn);
      }
      else
      {
         newActor->SetActorLocation(newDropLocation);
      }
   }

   return newActor;
}

AActor* UTATCheatManager::_LineTracePlayerViewTarget(float maxDistance, ECollisionChannel collisionChannel, FHitResult* outHitResult, bool drawDebug, const FCollisionQueryParams& queryParams, const FCollisionResponseParams& responseParams) const
{
   UWorld* world = GetWorld();
   if (world == nullptr)
   {
      return nullptr;
   }

   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return nullptr;
   }

   APawn* playerPawn = pc->GetPawnOrSpectator();
   if (playerPawn == nullptr)
   {
      return nullptr;
   }

   // If we didn't get a hit result pointer passed in, use this stack-allocated one
   FHitResult hitResult;
   if (outHitResult == nullptr)
   {
      outHitResult = &hitResult;
   }

   // Need to copy query params so we can add the player's pawn to the list of ignored actors
   FCollisionQueryParams queryParamsCopy = queryParams;
   queryParamsCopy.AddIgnoredActor(playerPawn);

   // Trace out from the pawn's viewpoint
   FVector traceStart = FVector::ZeroVector;
   FVector traceEnd = FVector::ZeroVector;
   bool blockingHit = false;
   if (UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(playerPawn, FGameplayAbilityTargetingLocationInfo(), maxDistance, traceStart, traceEnd))
   {
      blockingHit = world->LineTraceSingleByChannel(*outHitResult, traceStart, traceEnd, collisionChannel, queryParamsCopy, responseParams);
   }
   else
   {
      // Simple fallback trace
      traceStart = playerPawn->GetActorLocation();
      FRotator viewRotation;
      playerPawn->GetActorEyesViewPoint(traceStart, viewRotation);
      traceEnd = traceStart + (viewRotation.Quaternion().GetAxisX() * maxDistance);
      blockingHit = world->LineTraceSingleByChannel(*outHitResult, traceStart, traceEnd, collisionChannel, queryParamsCopy, responseParams);
   }

#if ENABLE_DRAW_DEBUG
   if (drawDebug)
   {
      const float kDrawDuration = 5.0f;
      if (blockingHit)
      {
         DrawDebugLine(world, traceStart, outHitResult->Location, FColor::Green, false, kDrawDuration);
         DrawDebugPoint(world, outHitResult->Location, 16.0f, FColor::Cyan, false, kDrawDuration);
         DrawDebugLine(world, outHitResult->Location, traceEnd, FColor::Orange, false, kDrawDuration);
      }
      else
      {
         DrawDebugLine(world, traceStart, traceEnd, FColor::Red, false, kDrawDuration);
      }
   }
#endif // ENABLE_DRAW_DEBUG

   if (blockingHit)
   {
      return outHitResult->GetActor();
   }

   return nullptr;
}

AActor* UTATCheatManager::_FindNearbyActorMatchingPredicate(float maxDistance, TFunctionRef<bool(AActor*)> predicate, ECollisionChannel collisionChannel, ECollisionResponse collisionResponse) const
{
   UWorld* world = GetWorld();
   if (world == nullptr)
   {
      return nullptr;
   }

   APlayerController* pc = GetOwnerPlayerController();
   if (pc == nullptr)
   {
      return nullptr;
   }

   APawn* playerPawn = pc->GetPawnOrSpectator();
   if (playerPawn == nullptr)
   {
      return nullptr;
   }

   // // Trace out from the pawn's viewpoint
   // FVector traceStart = FVector::ZeroVector;
   // FVector traceEnd = FVector::ZeroVector;
   // if (!UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(playerPawn, FGameplayAbilityTargetingLocationInfo(), maxDistance, traceStart, traceEnd))
   // {
   //    // Simple fallback coords
   //    traceStart = playerPawn->GetActorLocation();
   //    FRotator viewRotation;
   //    playerPawn->GetActorEyesViewPoint(traceStart, viewRotation);
   //    traceEnd = traceStart + (viewRotation.Quaternion().GetAxisX() * maxDistance);
   // }

   static const FName tatSphereTraceName(TEXT("TATCheatManagerSphereTraceMulti"));
   FCollisionQueryParams params{};
   params.TraceTag = tatSphereTraceName;
   params.bTraceComplex = true;
   params.AddIgnoredActor(playerPawn);

   //FCollisionObjectQueryParams objectParams{ collisionChannel };
   FCollisionResponseParams responseParams{ collisionResponse };

   //bool OverlapMultiByChannel(TArray<struct FOverlapResult>& OutOverlaps, const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
   TArray<FOverlapResult> overlaps;
   GetWorld()->OverlapMultiByChannel(overlaps, playerPawn->GetActorLocation(), FQuat::Identity, collisionChannel, FCollisionShape::MakeSphere(maxDistance), params, responseParams);

   for (const FOverlapResult& overlapResult : overlaps)
   {
      AActor* actor = overlapResult.GetActor();
      if (actor != nullptr && predicate(actor))
      {
         return actor;
      }
   }

   // TArray<FHitResult> hitResults;
   // GetWorld()->SweepMultiByObjectType(hitResults, traceStart, traceEnd, FQuat::Identity, objectParams, FCollisionShape::MakeSphere(maxDistance), params);

   // for (const FHitResult& hit : hitResults)
   // {
   //    AActor* actor = hit.GetActor();
   //    if (actor != nullptr && predicate(actor))
   //    {
   //       return actor;
   //    }
   // }

   return nullptr;
}

// static
TOptional<int32> UTATCheatManager::_ParseDurationToSeconds(const FString& text)
{
   auto tryParseInt = [](FStringView str) -> TOptional<int32>
   {
      // Strip leading and trailing whitespace
      str.TrimStartAndEndInline();

      // Strip leading zeroes on the left, but only if there is at least 2 chars (the string "0" is perfectly valid)
      while (str.Len() >= 2 && str[0] == '0')
      {
         str.MidInline(1);
      }

      if (str.Len() == 0)
      {
         return NullOpt;
      }

      // Reject any string that doesn't match the pattern [0-9]+
      for (int32 i = 0; i < str.Len(); i++)
      {
         if (str[i] < '0' || str[i] > '9')
         {
            return NullOpt;
         }
      }

      // In theory this copy shouldn't be needed at all, but there doesn't seem to be an FStringView version of Atoi,
      // which means we need to add a null terminator somehow.
      static constexpr int32 kMaxNumDigits = 16;
      if (str.Len() >= kMaxNumDigits)
      {
         return NullOpt;
      }
      TStringBuilder<kMaxNumDigits> buf;
      buf.Append(str);
      return FCString::Atoi(*buf);
   };

   FStringView view = text;

   // If the string contains a colon character, parse as "00:00"
   int32 sepIdx = INDEX_NONE;
   if (view.FindChar(':', sepIdx))
   {
      const TOptional<int32> minutes = tryParseInt(view.Mid(0, sepIdx));
      const TOptional<int32> seconds = tryParseInt(view.Mid(sepIdx + 1));
      if (!minutes || !seconds)
      {
         return NullOpt;
      }
      return (minutes.GetValue() * 60) + FMath::Clamp<int32>(seconds.GetValue(), 0, 59);
   }

   // No colon, parse as a single duration in seconds
   return tryParseInt(view);
}

// static
TOptional<FGameplayTag> UTATCheatManager::_FindGameplayTag(const FString& tagName, FStringView tryWithPrefix, TOptional<FGameplayTag> requiredParentTag, FString* outError)
{
   constexpr bool errorIfNotFound = false;
   const FGameplayTag directTag = FGameplayTag::RequestGameplayTag(FName(tagName), errorIfNotFound);
   if (directTag.IsValid() && (!requiredParentTag || directTag.MatchesTag(*requiredParentTag)))
   {
      return directTag;
   }
   FString prefixedTagStr;
   if (tryWithPrefix.Len() > 0)
   {
      prefixedTagStr = FString::Format(TEXT("{0}.{1}"), { tryWithPrefix, tagName });
      const FGameplayTag prefixedTag = FGameplayTag::RequestGameplayTag(FName(prefixedTagStr), errorIfNotFound);
      if (prefixedTag.IsValid() && (!requiredParentTag || prefixedTag.MatchesTag(*requiredParentTag)))
      {
         return prefixedTag;
      }
   }
   if (outError != nullptr)
   {
      FString alsoTried;
      if (prefixedTagStr.Len() > 0)
      {
         alsoTried = FString::Format(TEXT(" (also tried {0})"), { prefixedTagStr });
      }
      FString expectedParent;
      if (requiredParentTag)
      {
         expectedParent = FString::Format(TEXT(" Expected tag to have parent '{0}'."), { requiredParentTag->ToString() });
      }
      *outError = FString::Format(TEXT("Failed to resolve valid tag from input '{0}'{1}.{2}"), { tagName, alsoTried, expectedParent });
   }
   return NullOpt;
}

void UTATCheatManager::_PrintToConsole(const FString& string, bool isError)
{
#if WITH_EDITOR
   if (isError)
   {
      UE_LOG(LogTATCheatManager, Error, TEXT("%s"), *string);
   }
   else
   {
      UE_LOG(LogTATCheatManager, Log, TEXT("%s"), *string);
   }
#endif

   APlayerController* pc = GetOwnerPlayerController();
   if (!pc) return;

   // The client who did the command probably wants to see this rather than the server (or just dropping it)
   pc->ClientMessage(string);
}

bool UTATCheatManager::DoGameSpecificBugItLog(FOutputDevice& outputFile)
{
   Super::DoGameSpecificBugItLog(outputFile);

#if ALLOW_DEBUG_FILES
   ATATGameState* gs = GetWorld()->GetGameState<ATATGameState>();
   if (gs)
   {
      outputFile.Logf(TEXT("Authority: %s"), gs->HasAuthority() ? TEXT("Server") : TEXT("Client"));

      if (ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(GetWorld()->GetWorldSettings()))
      {
         outputFile.Logf(TEXT("World settings map type: %s"), *UEnum::GetDisplayValueAsText(worldSettings->MapType).ToString());
      }

      if (ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
      {
         outputFile.Logf(TEXT("World seed: %d"), gameState->GetMapSeed());

         if (const UTATMapVariationMgrComponent* variationMgr = gameState->GetMapVariationMgr())
         {
            variationMgr->GetActiveVariants().WriteToOutput(outputFile);
         }
      }

      outputFile.Logf(TEXT("Players In Session: %d"), gs->PlayerArray.Num());
      for(ATATPlayerState* ps : gs->GetTATPlayerStates())
      {
         if (!ps)
            continue;
      
         outputFile.Logf(TEXT("Player %s | %s | %s"), *ps->GetPlayerName()
                                                    , ps->IsLocalPlayerState() ? TEXT("Local") : TEXT("Remote")
                                                    , *UEnum::GetDisplayValueAsText(ps->GetTATCharacter()).ToString());
      }
   }   
#endif

   return true;
}

void UTATCheatManager::_OnFastForwardProgress()
{
   const FTimerManager& timerManager = GetWorld()->GetTimerManager();

   const float duration = timerManager.GetTimerRate(_fastForwardTimerHandle);
   const float progress = timerManager.GetTimerElapsed(_fastForwardTimerHandle);
   const float rate = GetOuterAPlayerController()->GetWorldSettings()->TimeDilation;

   GEngine->AddOnScreenDebugMessage(kFastForwardDebugMessageKey, 2, FColor::Orange, FString::Printf(TEXT("Fast forwarding %.1f/%.1f at %.1fx"), progress, duration, rate));
}

void UTATCheatManager::_OnFastForwardComplete()
{
   GetOuterAPlayerController()->GetWorldSettings()->SetTimeDilation(1);
   if (APawn* pawn = GetOwnerPlayerController()->GetPawn())
   {
      pawn->CustomTimeDilation = 1;
   }
   GetWorld()->GetTimerManager().ClearTimer(_fastForwardProgressHandle);
   _PrintToConsole(TEXT("FastForward Complete"));
   GEngine->AddOnScreenDebugMessage(kFastForwardDebugMessageKey, 5, FColor::Blue, TEXT("Fast forwarding complete"));
}

TArray<APlayerController*> UTATCheatManager::_CollectControllersOnPlayerTeam() const
{
   TArray<APlayerController*> result;

   auto tryGetTeam = [](const APlayerController* pc)-> TOptional<uint8>
   {
      if (pc == nullptr) return NullOpt;
      
      const IOSETeamInterface* teamInterface = pc->GetPlayerState<IOSETeamInterface>();
      if (teamInterface == nullptr) return NullOpt;

      return teamInterface->GetTeam();
   };

   const APlayerController* pc = GetOwnerPlayerController();
   if (const TOptional<uint8> team = tryGetTeam(pc))
   {
      for(FConstPlayerControllerIterator iterator = GetWorld()->GetPlayerControllerIterator(); iterator; ++iterator)
      {
         APlayerController* controller = iterator->Get();
         if (tryGetTeam(controller) == team)
         {
            result.Add(controller);
         }
      }
   }
   
   return result;
}

ATATPlayerState* UTATCheatManager::_GetOwningPlayerState() const
{
   APlayerController* pc = GetOwnerPlayerController();
   if (!pc) return nullptr;

   return pc->GetPlayerState<ATATPlayerState>();
}
