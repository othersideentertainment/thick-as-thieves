// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Player/TATPlayerState.h"

// tat
#include "Abilities/TATAttributeSet.h"
#include "Developer/TATEditorSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Abilities/TATStaminaAttributeSet.h"
#include "Developer/TATToolSettings.h"
#include "Developer/TATAbilitySettings.h"
#include "GameFramework/TATGameMode.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Items/TATItemInventoryComponent.h"
#include "Items/Tokens/TATTokenInventoryComponent.h"
#include "Loot/TATLootInventory.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Net/TATIrisGroupSubsystem.h"
#include "Online/TATGameState.h"
#include "SaveGame/TATCharacterDataContext.h"
#include "Player/TATPlayerController.h"
#include "Quests/TATPlayerQuestComponent.h"
#include "Quests/TATSharedObjectiveSubsystem.h"
#include "SaveGame/TATSaveGame.h"
#include "Tools/TATToolComponent.h"
#include "Tools/TATToolSetComponent.h"
#include "Settings/TATMatchSettings.h"
#include "TATGameInstance.h"
#include "UI/TATToastBroadcaster.h"
#include "UI/TATHUDIndicatorSubsystem.h"
#include "Player/TATCharacter.h"
#include "GameFramework/TATDifficulty.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Identity/OSESaveGameSystem.h"
#include "Items/ToolSetInterface.h"
#include "OSEProjectSettings.h"

// ue
#include "Engine/AssetManager.h"
#include "GameFramework/GameModeBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerState)

DEFINE_LOG_CATEGORY_STATIC(LogTATPlayerState, Log, All);

ATATPlayerState::ATATPlayerState(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   // Don't replicate gameplay effects to non-owning players
   GetAbilitySystemComponent()->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

   // Create the attribute set, this replicates by default. Adding it as a subobject of the owning actor
   // of an AbilitySystemComponent automatically registers the AttributeSet with the AbilitySystemComponent
   _tatAttributes = CreateDefaultSubobject<UTATAttributeSet>(TEXT("TATAttributeSet"));
   _tatStaminaAttributes = CreateDefaultSubobject<UTATStaminaAttributeSet>(TEXT("TATStaminaAttributeSet"));
   _tatLootInventory = CreateDefaultSubobject<UTATLootInventoryComponent>(TEXT("TATLootInventoryComponent"));
   _knownCluesComponent = CreateDefaultSubobject<UTATKnownCluesComponent>(TEXT("KnownClues"));
   _tatItemInventory = CreateDefaultSubobject<UTATItemInventoryComponent>(TEXT("ItemInventoryComponent"));
   _tokenInventory = CreateDefaultSubobject<UTATTokenInventoryComponent>(TEXT("TokenInventory"));
   _questComponent = CreateDefaultSubobject<UTATPlayerQuestComponent>(TEXT("QuestComponent"));
}

void ATATPlayerState::OnNetConnectionSet()
{
   // Set initial iris replication groups, since the connectionId may not have existed when initially set
   if(UTATIrisGroupSubsystem* groupSubsystem = GetWorld()->GetSubsystem<UTATIrisGroupSubsystem>())
   {
      _questComponent->AuthorityAddConnectionToContractGroups();
      if(GetTeam() != IOSETeamInterface::kInvalidTeam)
      {
         groupSubsystem->ForTeam(GetTeam()).AllowGroupForPlayer(this);
      }
   }
}

#if WITH_EDITOR
EDataValidationResult ATATPlayerState::IsDataValid(FDataValidationContext& context) const
{
   // Validate match settings as part of the player state.
   // UTATMatchSettingsBase::IsDataValid validates the settings metadata, not the values, so we don't need to care if we currently have a valid instance.
   TSubclassOf<UTATMatchSettingsBase> matchSettingsClass = UTATProjectSettings::Get().GetMatchSettingsClass();
   check(matchSettingsClass != nullptr);
   EDataValidationResult matchSettingsResult = EDataValidationResult::NotValidated;
   if (const UTATMatchSettingsBase* matchSettings = matchSettingsClass.GetDefaultObject())
   {
      matchSettingsResult = matchSettings->IsDataValid(context);
   }
   return CombineDataValidationResults(Super::IsDataValid(context), matchSettingsResult);
}
#endif

/* static */
ATATPlayerState* ATATPlayerState::GetTATPlayerState(const UObject* contextObj, int index)
{
   return Cast<ATATPlayerState>(AOSEPlayerState::GetOSEPlayerState(contextObj, index));
}

/* static */
ATATPlayerState* ATATPlayerState::GetLocalTATPlayerState(const UObject* contextObj)
{
   return Cast<ATATPlayerState>(AOSEPlayerState::GetLocalOSEPlayerState(contextObj));
}

void ATATPlayerState::_SetPlayerColorFromTeam()
{
   // main menu and test maps may not subclass from TATGameMode
   if (ATATGameMode* gameMode = GetWorld()->GetAuthGameMode<ATATGameMode>())
   {
      // Select player color from pool
      _playerColor = gameMode->GetColorForTeamID(GetTeam());
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _playerColor, this);

      _BroadcastPlayerColorChanged();
   }
}

void ATATPlayerState::OnRep_PlayerName()
{
   Super::OnRep_PlayerName();
   OnPlayerNameChanged.Broadcast();
}

void ATATPlayerState::BeginPlay()
{
   Super::BeginPlay();

   _questComponent->OnQuestObjectiveCompleteChanged.AddUniqueDynamic(this, &ThisClass::_BroadcastQuestObjectiveCompleteChanged);

   if (HasAuthority())
   {
      ATATPlayerController* pc = _GetOwnerPlayerController();
      if (pc && pc->IsLocalController())
      {
         // Init player state with save-data (or wait for 'on loaded')
         _InitLocalSaveData();
      }

      // Update attribute(s) based on match settings      
      if(const UTATMatchSettings* TATMatchSettings = UTATMatchSettingsBase::GetTATMatchSettings<UTATMatchSettings>(GetWorld()))
      {
         float RespawnCount;
         if(TATMatchSettings->RespawnCount == ERespawnCount::Unlimited)
         {
            RespawnCount = -1.0f;
         }
         else
         {
            RespawnCount = static_cast<float>(TATMatchSettings->RespawnCount);
         }
         GetAbilitySystemComponent()->SetNumericAttributeBase(UTATAttributeSet::GetNumRespawnsRemainingAttribute(), RespawnCount);
      }

      if (ensure(_tatLootInventory))
      {
         _tatLootInventory->OnAuthorityQuestLootChanged.AddDynamic(this, &ATATPlayerState::_OnAuthorityQuestLootChanged);
      }
   }

   if (!UTATProjectSettings::Get().GetMapTypeSettingsForCurrentWorldChecked(this).UseMapIntroStates)
   {
      _SetMapIntroState(ETATMapIntroState::Complete);
   }

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      const UOSEProjectSettings& oseProjectSettings = UOSEProjectSettings::Get();
      asc->RegisterGameplayTagEvent(oseProjectSettings.ConditionUnconsciousTag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ATATPlayerState::_OnUnconsciousTagChanged);
   }

#if WITH_EDITOR
   if (GEngine->IsEditor())
   {
      if (HasAuthority())
      {
         AuthorityApplyCheatsFromEditorSettings();
      }
      else
      {
         ClientApplyCheatsFromEditorSettings();
      }
   }
#endif
}

void ATATPlayerState::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);

   // Unbind save system 'on has save data'...
   UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(this);
   check(saveGameSystem);
   saveGameSystem->OnSaveDataStateChanged.RemoveDynamic(this, &ATATPlayerState::_OnSaveDataStateChanged);

   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      const UOSEProjectSettings& oseProjectSettings = UOSEProjectSettings::Get();
      asc->RegisterGameplayTagEvent(oseProjectSettings.ConditionUnconsciousTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
   }
}

void ATATPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // Damage log is not currently in use
   DISABLE_REPLICATED_PROPERTY_FAST(AOSEPlayerState, _damageLog);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _character, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _uiSelectedCharacter, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _currentOutfitLoadout, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _playerLevel, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _isReadyChecked, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _isReadyToSkipCutscene, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _mapIntroState, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _numTotemRespawnsThisMatch, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _playerColor, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _isInATeam, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _matchCompletionState, params);

   params.Condition = COND_SkipOwner;
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _analyticsId, params);
}

void ATATPlayerState::ClientInitialize(AController* controller)
{
   Super::ClientInitialize(controller);

   // only ever called on the client
   check(controller);

   // when the local client gets it's player controller we can check for local control
   if (controller->IsLocalController())
   {
      // Init player state with save-data (or wait for 'on loaded')
      _InitLocalSaveData();
   }
}

void ATATPlayerState::CopyProperties(APlayerState* playerState)
{
   if (ATATPlayerState* tatPS = Cast<ATATPlayerState>(playerState))
   {
      tatPS->_character = _character;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _character, tatPS);

      tatPS->_uiSelectedCharacter = _uiSelectedCharacter;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _uiSelectedCharacter, tatPS);

      tatPS->_currentOutfitLoadout = _currentOutfitLoadout;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _currentOutfitLoadout, tatPS);

      tatPS->_playerLevel = _playerLevel;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _playerLevel, tatPS);

      // the upgrade state is stuffed into the ASC, so copy it as well
      if (const auto asc = Cast<UOSEAbilitySystemComponent>(AbilitySystemComponent))
      {
         tatPS->_ApplyUpgradeState(asc->GetAllUpgradeState());
      }
   }
}

void ATATPlayerState::AuthorityApplyCheatsFromEditorSettings_Implementation()
{
}

void ATATPlayerState::ClientApplyCheatsFromEditorSettings_Implementation()
{
}

FTATCharacterDataContext ATATPlayerState::GetCharacterDataContext() const
{
   return FTATCharacterDataContext{ UTATSaveGame::GetTATSaveGame(this), _characterSave };
}

void ATATPlayerState::ServerRefreshCharacterUpgradeAndLoadoutState()
{
   if (!IsLocalPlayerState() || _ignoreSave)
   {
      return;
   }

   const FTATCharacterDataContext context = GetCharacterDataContext();
   if (!context.IsValid())
   {
      return;
   }

   if (!context.SaveGame)
   {
      return;
   }

   const FTATCharacterProgression& characterData = context.GetCharacterDataChecked();
   FUpgradeState upgradeState;
   characterData.GetUpgradeState(upgradeState);

   TArray<FTATCharacterLoadoutEntry> toolLoadout;
   _GetCharacterLoadoutTags(context, ETATLoadoutType::Tool, toolLoadout);

   TArray<FTATCharacterLoadoutEntry> outfitLoadout;
   _GetCharacterLoadoutTags(context, ETATLoadoutType::Outfit, outfitLoadout);

   ServerSetTATCharacterAndUpgradeState(context.SaveId, context.SaveId.Character, upgradeState, context.SaveGame->GetXP().Level, toolLoadout, outfitLoadout);

   // apply upgrade state immediately on the client
   if (!HasAuthority())
   {
      _ApplyUpgradeState(upgradeState);
   }
}

void ATATPlayerState::ServerSetTATCharacterAndUpgradeState_Implementation(FTATCharacterSaveId saveId, ETATCharacter character, const FUpgradeState& upgradeState,
                                                                          int32 level, const TArray<FTATCharacterLoadoutEntry>& gearLoadout, const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   _characterSave = saveId;

   _character = character;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _character, this);

   ChangePlayerLevel(level);

   _ApplyUpgradeState(upgradeState);  // explicitly doing this before broadcast
   _BroadcastTATCharacterChanged();

   if (ATATPlayerController* pc = _GetOwnerPlayerController())
   {
      if (IsOnlyASpectator())
      {
         SetIsOnlyASpectator(false);
         pc->ServerSetSpectatorWaiting(true);
         pc->ServerRestartPlayer();
      }
      else if (APawn* pawn = GetPawn())
      {
         FTransform pawnXfm = pawn->GetActorTransform();
         pawn->Destroy();
         GetWorld()->GetAuthGameMode()->RestartPlayerAtTransform(pc, pawnXfm);
      }

      // NB: Apply our initial gear loadout after the new pawn has been spawned, since it will own the toolset component
      _ApplyGearLoadout(gearLoadout);

      _ApplyOutfitLoadout(outfitLoadout);
   }
}

bool ATATPlayerState::ServerSetTATCharacterAndUpgradeState_Validate(FTATCharacterSaveId saveId, ETATCharacter character, const FUpgradeState& upgradeState,
                                                                    int32 level, const TArray<FTATCharacterLoadoutEntry>& gearLoadout, const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   // no validation needed
   return true;
}

void ATATPlayerState::ServerChangeOutfitLoadout_Implementation(const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   _ApplyOutfitLoadout(outfitLoadout);
}

bool ATATPlayerState::ServerChangeOutfitLoadout_Validate(const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   // TODO check outfit choices have been unlocked by the player
   return true;
}

void ATATPlayerState::ChangeOutfitLoadout(const FTATCharacterLoadout& newLoadout)
{
   ServerChangeOutfitLoadout(newLoadout.Entries);

   if (!HasAuthority())
   {
      _ApplyOutfitLoadout(newLoadout.Entries);
   }

   if (!_ignoreSave)
   {
      const FTATCharacterDataContext context = GetCharacterDataContext();
      context.SaveGame->UpdateCharacterLoadout(_characterSave, newLoadout);
   }
}

void ATATPlayerState::ChangeTATCharacter(ETATCharacter character)
{
   // Shim for now
   ChangeSavedCharacter(FTATCharacterSaveId::FromCharacterType(character));
}

void ATATPlayerState::ChangeSavedCharacter(FTATCharacterSaveId character)
{
   if (_characterSave != character)
   {
      _ForceChangeSavedCharacter(character);
   }
}

void ATATPlayerState::ChangePlayerLevel(int32 level)
{
   if (_playerLevel != level)
   {
      _playerLevel = level;
      _BroadcastPlayerLevelChanged();
   }
}

void ATATPlayerState::ForceRespawnCharacter()
{
   _ForceChangeSavedCharacter(_characterSave);
}

void ATATPlayerState::_ForceChangeSavedCharacter(FTATCharacterSaveId character)
{
   _characterSave = character;

   FUpgradeState upgradeState;
   TArray<FTATCharacterLoadoutEntry> gearLoadout;
   TArray<FTATCharacterLoadoutEntry> outfitLoadout;
   int32 level = _playerLevel;

   if(!_ignoreSave)
   {
      const FTATCharacterDataContext context = GetCharacterDataContext();
      check(context.IsValid());
      context.GetCharacterDataChecked().GetUpgradeState(upgradeState);

      // Trusting the client until we care otherwise
      // explicitly sending both so that the ordering is consistent
      _GetCharacterLoadoutTags(context, ETATLoadoutType::Tool, gearLoadout);
      _GetCharacterLoadoutTags(context, ETATLoadoutType::Outfit, outfitLoadout);
      if (context.SaveGame)
      {
         level = context.SaveGame->GetXP().Level;
      }
   }

   ServerSetTATCharacterAndUpgradeState(character, character.Character, upgradeState, level, gearLoadout, outfitLoadout);

   // guess we can init now on the client, too, so we don't need to wait for the return trip...?
   if (!HasAuthority())
   {
      _character = character.Character; // todo
      _ApplyUpgradeState(upgradeState); // explicitly doing this before broadcast
      _BroadcastTATCharacterChanged();
   }

   // and let's save this into save data now too so we persist any changes via both ui and cheats
   if(!_ignoreSave)
   {
      const FTATCharacterDataContext context = GetCharacterDataContext();
      context.SaveGame->SetSelectedCharacter(_characterSave);
   }

   OnCharacterSaveIdChanged.Broadcast(this, _characterSave);
}

void ATATPlayerState::ServerSetUISelectedTATCharacter_Implementation(ETATCharacter character)
{
   _uiSelectedCharacter = character;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _uiSelectedCharacter, this);
   
   _BroadcastUISelectedTATCharacterChanged();
}

bool ATATPlayerState::ServerSetUISelectedTATCharacter_Validate(ETATCharacter character)
{
   // no validation needed...?
   return true;
}

void ATATPlayerState::ChangeUISelectedTATCharacter(ETATCharacter character)
{
   if (_uiSelectedCharacter != character)
   {
      ServerSetUISelectedTATCharacter(character);

      if (!HasAuthority())
      {
         _uiSelectedCharacter = character;
         _BroadcastUISelectedTATCharacterChanged();
      }
   }
}

void ATATPlayerState::ServerSetIsReadyChecked_Implementation(bool isReadyChecked)
{
   _isReadyChecked = isReadyChecked;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _isReadyChecked, this);

   _BroadcastIsReadyCheckedChanged();
}

bool ATATPlayerState::ServerSetIsReadyChecked_Validate(bool isReadyChecked)
{
   // no validation needed
   return true;
}

void ATATPlayerState::SetIsReadyChecked(bool isReadyChecked)
{
   if (_isReadyChecked != isReadyChecked)
   {
      ServerSetIsReadyChecked(isReadyChecked);
   }
}

void ATATPlayerState::ForceLocalIsReadyChecked(bool isReadyChecked)
{
   _isReadyChecked = isReadyChecked;
   _OnRep_IsReadyChecked();
}

void ATATPlayerState::ServerSetIsReadyForCutsceneSkip_Implementation(bool isReadyToSkipCutscene)
{
   _isReadyToSkipCutscene = isReadyToSkipCutscene;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _isReadyToSkipCutscene, this);

   _BroadcastIsReadyToSkipCutscene();
}

bool ATATPlayerState::ServerSetIsReadyForCutsceneSkip_Validate(bool isReadyToSkipCutscene)
{
   // no validation needed
   return true;
}

void ATATPlayerState::SetIsReadyForCutsceneSkip(bool isReadyToSkipCutscene)
{
   if (_isReadyToSkipCutscene != isReadyToSkipCutscene)
   {
      ServerSetIsReadyForCutsceneSkip(isReadyToSkipCutscene);
   }
}

void ATATPlayerState::ServerSetMapIntroState_Implementation(ETATMapIntroState newState)
{
   if (static_cast<int>(newState) > static_cast<int>(_mapIntroState))
   {
      _SetMapIntroState(newState);
   }
}

bool ATATPlayerState::ServerSetMapIntroState_Validate(ETATMapIntroState introState)
{
   return true;
}

void ATATPlayerState::LocalSetMapIntroStateComplete(ETATMapIntroState completedState)
{
   if (IsLocalPlayerState())
   {
      int nextStateInt = static_cast<int>(completedState) + 1;
      if (nextStateInt > static_cast<int>(_mapIntroState) &&
          nextStateInt < static_cast<int>(ETATMapIntroState::MAX))
      {
         ETATMapIntroState nextState = static_cast<ETATMapIntroState>(nextStateInt);

#if WITH_EDITOR
         const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
         if (editorSettings.SkipMapIntroStates && completedState == ETATMapIntroState::Loading)
         {
            nextState = ETATMapIntroState::Complete;
         }
#endif // WITH_EDITOR

         // change it locally so that we can progress without waiting for the rpc to replicate back to us
         _SetMapIntroState(nextState);

         // tell the server so it can keep track of all the various states
         ServerSetMapIntroState(nextState);
      }
   }
   else
   {
      UE_LOG(LogTATPlayerState, Error, TEXT("LocalSetMapIntroStateComplete called on %s player state, but that is not the local one!"), *GetPlayerName());
   }
}

void ATATPlayerState::_GetCharacterLoadoutTags(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FTATCharacterLoadoutEntry>& outLoadout) const
{
   constexpr bool getDefaultLoadoutIfEmpty = true;
   const FTATCharacterLoadout characterLoadout = context.SaveGame->GetCharacterLoadout(context.SaveId, loadoutType, getDefaultLoadoutIfEmpty);

   outLoadout.Reset();
   outLoadout.Reserve(characterLoadout.Entries.Num());
   for (const FTATCharacterLoadoutEntry& entry : characterLoadout.Entries)
   {
      if (entry.LoadoutTag.IsValid())
      {
         outLoadout.Add(entry);
      }
   }

#if WITH_EDITOR
   // in editor allow override w/ developer settings
   if (loadoutType == ETATLoadoutType::Tool && GEngine->IsEditor() && !GIsAutomationTesting)
   {
      const UTATEditorSettings& tatEditorSettings = *UTATEditorSettings::GetTATEditorSettings();
      TArray<FGameplayTag> tags;
      if (tatEditorSettings.GetOverrideGearLoadout(GetWorld(), tags))
      {
         outLoadout.SetNum(tags.Num());
         for (int32 i = 0; i < tags.Num(); i++)
         {
            outLoadout[i] = FTATCharacterLoadoutEntry{ tags[i], ETATCharacterInputActionType::None };
         }
      }
   }
#endif
}

void ATATPlayerState::_OnSaveDataStateChanged(EOSESaveDataState newState)
{
   if (newState == EOSESaveDataState::HasSaveData)
   {
      _InitLocalPlayerState();
   }
}

void ATATPlayerState::_InitLocalSaveData()
{
   // Bind save system 'on has save data'
   // On 'HasSaveData', load character/progression
   UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(this);
   check(saveGameSystem);
   if (saveGameSystem->HasLoadedSaveData())
   {
      _InitLocalPlayerState();
   }
   else
   {
      saveGameSystem->OnSaveDataStateChanged.AddDynamic(this, &ATATPlayerState::_OnSaveDataStateChanged);
   }
}

void ATATPlayerState::_InitLocalPlayerState()
{
   _InitAnalyticsId();
   _InitTATCharacter();
   _InitPlayerLevel();

   // bp init
   OnInitLocalPlayerState();

   // Let the HUD indicator subsystem know this is a good time to init
   if (APlayerController* pc = GetPlayerController())
   {
      if (ULocalPlayer* localPlayer = pc->GetLocalPlayer())
      {
         if (UTATHUDIndicatorSubsystem* hudIndicatorSubsystem = localPlayer->GetSubsystem<UTATHUDIndicatorSubsystem>())
         {
            hudIndicatorSubsystem->OnLocalPlayerStateReady(this);
         }
      }
   }
}

void ATATPlayerState::_ApplyUpgradeState(const FUpgradeState& state)
{
   // Check if we have any upgrades we want to inject from match settings
   TArray<TPair<FGameplayTag, int32>, TInlineAllocator<8>> matchSettingsEnabledUpgrades;
   const UTATMatchSettingsBase& matchSettings = UTATGameInstance::Get(this).GetMatchSettings();
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
   for (const FTATMatchSettingsUpgradeConfig& upgrade : projectSettings.MatchSettingsUpgrades)
   {
      if (!upgrade.IsValid())
      {
         continue;
      }

      bool isEnabled = false;
      if (!matchSettings.GetMatchSettingsValueAsBool(upgrade.MatchSettingsBoolPropertyName, isEnabled))
      {
         const ETATMatchSettingsPropertyType propType = matchSettings.GetMatchSettingPropertyType(upgrade.MatchSettingsBoolPropertyName);
         UE_LOG(LogTATPlayerState, Error, TEXT("Match settings property '%s' is configured as an upgrade toggle, so it should have type bool, but it's of type %s"),
            *upgrade.MatchSettingsBoolPropertyName.ToString(),
            *StaticEnum<ETATMatchSettingsPropertyType>()->GetNameStringByValue(static_cast<int64>(propType)));
         continue;
      }

      if (isEnabled)
      {
         matchSettingsEnabledUpgrades.Emplace(upgrade.UpgradeTag, upgrade.UpgradeLevel);
      }
   }

   const ETATDifficulty difficulty = TATDifficulty::GetDifficultyForMatch(GetWorld());
   uint8 difficultyAsInt = static_cast<uint8>(difficulty);

   // the difficulty enum starts at 0, but the min upgrade is 1, so offset by 1
   difficultyAsInt += 1;
   
   FUpgradeState newUpgradeState = state;
   newUpgradeState.Values.Add(TAG_MatchSetting_Difficulty, difficultyAsInt);
   if(matchSettingsEnabledUpgrades.IsEmpty() == false)
   {
      for (const TPair<FGameplayTag, int32>& upgrade : matchSettingsEnabledUpgrades)
      {
         newUpgradeState.Values.Add(upgrade.Key, upgrade.Value);
      }
   }

   UOSEAbilitySystemComponent* abilityComponent = CastChecked<UOSEAbilitySystemComponent>(AbilitySystemComponent);
   abilityComponent->SetUpgradeState(newUpgradeState);
}

void ATATPlayerState::_BroadcastQuestObjectiveCompleteChanged(bool isComplete, ETATPlayerQuestSlot slot)
{
   OnQuestObjectiveCompleteChanged.Broadcast(isComplete, slot);
}

void ATATPlayerState::_OnRep_TATCharacter()
{
   _BroadcastTATCharacterChanged();
}

void ATATPlayerState::_BroadcastTATCharacterChanged()
{
   UE_LOG(LogTATPlayerState, Verbose, TEXT("%s is now character %s"), *GetPlayerName(), *UEnum::GetValueAsString(_character));
   OnTATCharacterChanged.Broadcast(this, _character);

#if WITH_EDITOR
   if (!_ranAutoExecCheats)
   {
      GetWorldTimerManager().SetTimerForNextTick(this, &ATATPlayerState::_OnAutoExecCheats);
      _ranAutoExecCheats = true;
   }
#endif
}

void ATATPlayerState::_OnRep_UISelectedTATCharacter()
{
   _BroadcastUISelectedTATCharacterChanged();
}

void ATATPlayerState::_BroadcastUISelectedTATCharacterChanged()
{
   UE_LOG(LogTATPlayerState, Verbose, TEXT("%s is now selecting character %s in the ui"), *GetPlayerName(), *UEnum::GetValueAsString(_character));
   OnUISelectedTATCharacterChanged.Broadcast(_uiSelectedCharacter);
}

void ATATPlayerState::_InitTATCharacter()
{
   FTATCharacterSaveId myCharacter = FTATCharacterSaveId();

   if(!_ignoreSave)
   {
      UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
      check(saveGame);
      myCharacter = saveGame->GetSelectedCharacter();
   }

#if WITH_EDITOR
   // in editor allow override w/ developer settings
   if (GEngine->IsEditor())
   {
      const UTATEditorSettings& tatEditorSettings = *UTATEditorSettings::GetTATEditorSettings();
      ETATCharacter overrideCharacter = tatEditorSettings.GetOverrideCharacter(GetWorld());
      if (overrideCharacter != ETATCharacter::None)
      {
         // TODO: switch from FTATCharacterSaveId::FromCharacterType
         myCharacter = FTATCharacterSaveId::FromCharacterType(overrideCharacter);
      }
   }
#endif

   if(_overrideCharacter != ETATCharacter::None)
   {
      myCharacter = FTATCharacterSaveId::FromCharacterType(_overrideCharacter);
   }
   
   ChangeSavedCharacter(myCharacter);
}

void ATATPlayerState::_OnRep_CurrentOutfitLoadout()
{
   _BroadcastCurrentOutfitLoadoutChanged();
}

void ATATPlayerState::_BroadcastCurrentOutfitLoadoutChanged()
{
   OnCharacterOutfitChanged.Broadcast(this, _currentOutfitLoadout);
}

void ATATPlayerState::_OnRep_PlayerLevel()
{
   _BroadcastPlayerLevelChanged();
}

void ATATPlayerState::_BroadcastPlayerLevelChanged()
{
   OnPlayerLevelChanged.Broadcast(this, _playerLevel);
}

void ATATPlayerState::_InitPlayerLevel()
{
   if(!_ignoreSave)
   {
      UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
      check(saveGame);
      ChangePlayerLevel(saveGame->GetXP().Level);
      saveGame->OnXPProgressionChanged.AddUniqueDynamic(this, &ATATPlayerState::_OnXPProgressionChanged);
   }
}

void ATATPlayerState::_OnXPProgressionChanged()
{
   if(!_ignoreSave)
   {
      UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
      check(saveGame);
      ChangePlayerLevel(saveGame->GetXP().Level);
   }
}

void ATATPlayerState::_OnRep_MatchCompletionState()
{
   _BroadcastMatchCompletionStateChanged();
}

void ATATPlayerState::_BroadcastMatchCompletionStateChanged()
{
   UE_LOG(LogTATPlayerState, Verbose, TEXT("%s match completion state is now %s"), *GetPlayerName(), *UEnum::GetValueAsString(_matchCompletionState));
   OnMatchCompletionStateChanged.Broadcast(this, _matchCompletionState);
}

void ATATPlayerState::_InitAnalyticsId()
{
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   check(saveGame);
   _analyticsId = saveGame->GetPlayerAnalyticsId();
   _ServerSetAnalyticsId(_analyticsId);
}

void ATATPlayerState::_ServerSetAnalyticsId_Implementation(const FGuid& analyticsId)
{
   _analyticsId = analyticsId;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _analyticsId, this);
}

void ATATPlayerState::_ApplyGearLoadout(const TArray<FTATCharacterLoadoutEntry>& gearLoadout)
{
   check(HasAuthority());
   const FTATMapTypeSettings& mapTypeSettings = UTATProjectSettings::Get().GetMapTypeSettingsForCurrentWorldChecked(this);

   if (mapTypeSettings.AddDefaultPlayerGearLoadoutOnSpawn)
   {
      TSoftObjectPtr<UDataTable> gearMetadata = UTATToolSettings::Get().GearMetadataTable;
      TWeakObjectPtr<ATATPlayerState> weakThis(this);

      // Load in the gear metadata table so we can look up gear tag -> tool class
      UAssetManager::GetStreamableManager().RequestAsyncLoad(gearMetadata.ToSoftObjectPath(), [gearMetadata, weakThis, gearLoadout]
      {
         TArray<TSoftClassPtr<UTATToolComponent>, TInlineAllocator<8>> toolClassesInLoadout;

         ATATPlayerState* self = weakThis.Get();

         const FUpgradeState* upgradeState = nullptr;
         if (self != nullptr)
         {
            UOSEAbilitySystemComponent* abilityComponent = CastChecked<UOSEAbilitySystemComponent>(self->AbilitySystemComponent);
            upgradeState = &abilityComponent->GetAllUpgradeState();
         }

         check(gearMetadata.Get());
         for (const FTATCharacterLoadoutEntry& entry : gearLoadout)
         {
            TSoftClassPtr<UTATToolComponent> gearClassToLoad = UTATToolSettings::LookupToolClassByToolType(gearMetadata.Get(), entry.LoadoutTag, upgradeState);

            if (ensure(!gearClassToLoad.IsNull()))
            {
               toolClassesInLoadout.Add(gearClassToLoad);
            }
         }

         if (self != nullptr)
         {
            // Load in the gear class and add it to our toolset
            // NB: Load them as a group so we can preserve order when adding them
            self->_LoadAndAddGearToLoadout(toolClassesInLoadout);
         }
      });
   }
}

void ATATPlayerState::_LoadAndAddGearToLoadout(const TArray<TSoftClassPtr<UTATToolComponent>, TInlineAllocator<8>>& gearClasses)
{
   // NB: Unfortunately, cannot use TInlineAllocator since RequestAsyncLoad expects default TArray
   TArray<FSoftObjectPath> gearClassObjectPaths;
   gearClassObjectPaths.Reserve(gearClasses.Num());
   for (const auto& gearClass : gearClasses)
   {
      gearClassObjectPaths.Add(gearClass.ToSoftObjectPath());
   }

   TWeakObjectPtr<ATATPlayerState> weakThis(this);
   UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(gearClassObjectPaths), [gearClasses = gearClasses, weakThis]
   {
      if (ATATPlayerState* self = weakThis.Get())
      {
         if (APawn* pawn = self->GetPawn())
         {
            if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(pawn))
            {
               if (UTATToolSetComponent* tatToolset = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
               {
                  for (const auto& gearClass : gearClasses)
                  {
                     check(gearClass.Get());

                     tatToolset->AuthorityAddStartingToolClass(gearClass.Get());
                  }
               }
               else
               {
                  UE_LOG(LogTATPlayerState, Error, TEXT("Expected a UTATToolSetComponent on '%s'/'%s', found '%s'"),
                     *self->GetName(),
                     *pawn->GetName(),
                     *GetNameSafe(toolSetInterface.GetObject()));
               }
            }
         }
      }
   });
}

void ATATPlayerState::_ApplyOutfitLoadout(const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   _currentOutfitLoadout = outfitLoadout;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _currentOutfitLoadout, this);

   _BroadcastCurrentOutfitLoadoutChanged();
}

void ATATPlayerState::AuthorityInitializeQuest()
{
   check(HasAuthority());

   // done as a separate step because PlayerId isn't initialize yet in BeginPlay
   _questComponent->AuthorityInitializeQuest(GetPlayerId());
}

void ATATPlayerState::AuthorityForceObjectiveComplete(ETATPlayerQuestSlot questSlot)
{
   check(HasAuthority());
   _questComponent->AuthorityForceObjectiveComplete(questSlot);
}

bool ATATPlayerState::IsQuestObjectiveComplete(ETATPlayerQuestSlot questSlot) const
{
   return _questComponent->IsQuestObjectiveComplete(questSlot);
}

FGameplayTag ATATPlayerState::GetActiveQuestTag(ETATPlayerQuestSlot questSlot) const
{
   return _questComponent->GetActiveQuestTag(questSlot);
}

bool ATATPlayerState::HasActiveQuest(ETATPlayerQuestSlot questSlot) const
{
   return _questComponent->HasActiveQuest(questSlot);
}

UTATRootPlayerObjective* ATATPlayerState::GetObjectiveForSlot(ETATPlayerQuestSlot questSlot) const
{
   return _questComponent->GetObjectiveForSlot(questSlot);
}

TConstArrayView<TObjectPtr<UTATRootPlayerObjective>> ATATPlayerState::GetObjectives() const
{
   return _questComponent->GetObjectives();
}

TConstArrayView<FGameplayTag> ATATPlayerState::GetActiveQuestTags() const
{
   return _questComponent->GetActiveQuestTags();
}

bool ATATPlayerState::HasQuestRelatedLoot(FTATLootIdentifier lootId) const
{
   return _questComponent->HasQuestRelatedLoot(lootId);
}

bool ATATPlayerState::HasRelevantQuestLootInInventory() const
{
   UTATPlayerQuestComponent::FQuestLootResult questLoot = _questComponent->GetQuestRelatedLoot();

   for (FTATLootIdentifier lootId : questLoot)
   {
      if (_tatLootInventory->AuthorityHasLoot(lootId))
      {
         return true;
      }
   }

   return false;
}

UTATMatchSettingsBase* ATATPlayerState::GetLocalUIMatchSettings() const
{
   if (!IsMissionOwner())
   {
      return nullptr;
   }

   // auto-create if missing
   if (_localMatchSettings == nullptr)
   {
      ATATPlayerState* nonConstThis = const_cast<ATATPlayerState*>(this);
      nonConstThis->_localMatchSettings = UTATGameInstance::Get(this).GetMatchSettings().CopyMatchSettingsIntoNewObject(nonConstThis, NAME_None, RF_Transient);
   }

   return _localMatchSettings;
}

void ATATPlayerState::ServerUpdateMatchSettings()
{
   if (IsMissionOwner() && _localMatchSettings)
   {
      TArray<uint8> serializedMatchSettings;
      _localMatchSettings->SerializeToByteArray(serializedMatchSettings);
      ServerUpdateMatchSettingsBytes(serializedMatchSettings);
   }
}

bool ATATPlayerState::ServerUpdateMatchSettingsBytes_Validate(const TArray<uint8>& newMatchSettingsData)
{
   return IsMissionOwner() && newMatchSettingsData.Num() > 0;
}

void ATATPlayerState::ServerUpdateMatchSettingsBytes_Implementation(const TArray<uint8>& newMatchSettingsData)
{
   check(HasAuthority());
   UTATGameInstance::Get(this).UpdateMatchSettings(newMatchSettingsData);
}

void ATATPlayerState::AuthorityOnRespawned()
{
   if (ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(this))
   {
      toastBroadcaster->ClientToastBroadcast_PlayerRespawned(this);
   }
}

int32 ATATPlayerState::AuthorityIncrementTotemRespawnCount(bool broadcastTotemRespawnToast)
{
   ensure(HasAuthority());

   _numTotemRespawnsThisMatch++;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _numTotemRespawnsThisMatch, this);

   _OnRep_NumTotemRespawnsThisMatch();

   if (broadcastTotemRespawnToast)
   {
      if (ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(this))
      {
         toastBroadcaster->ClientToastBroadcast_PlayerRespawnTotemActivated(this);
      }
   }

   return _numTotemRespawnsThisMatch;
}

void ATATPlayerState::AuthoritySetTotemRespawnCountDirect(int32 newTotemRespawnCount)
{
   ensure(HasAuthority());

   _numTotemRespawnsThisMatch = FMath::Max(0, newTotemRespawnCount);
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _numTotemRespawnsThisMatch, this);

   _OnRep_NumTotemRespawnsThisMatch();
}

void ATATPlayerState::HandleReactToOwnStim(const FGameplayTag& stimTag, float loudness)
{
   if(ITATHearingStimSourceReactor* reactor = Cast<ITATHearingStimSourceReactor>(GetPawn()))
   {
      reactor->HandleReactToOwnStim(stimTag, loudness);
   }
}

void ATATPlayerState::AuthorityGrantXPWhenMatchEnds(const FTATFinishedMatchXPGained& xpGained)
{
   ensure(HasAuthority());
   _additionalGainedXP.Add(xpGained);
}

const TArray<FTATFinishedMatchXPGained>& ATATPlayerState::AuthorityGetAdditionalXPWhenMatchEnds() const
{
   ensure(HasAuthority());
   return _additionalGainedXP;
}

void ATATPlayerState::_OnRep_NumTotemRespawnsThisMatch()
{
   OnTotemRespawnCountUpdated.Broadcast(_numTotemRespawnsThisMatch);
}

void ATATPlayerState::AuthorityWasKnockedOutByOtherPlayer(const ATATPlayerState* otherPlayer)
{
   ensure(HasAuthority());
   check(otherPlayer != nullptr);

   if (_playerKnockedOutByInfo.IsValid())
   {
      UE_LOG(LogTATPlayerState, Error, TEXT("AuthorityWasKnockedOutByOtherPlayer() called multiple times! (previously knocked out by %s)"), *otherPlayer->GetPlayerName());
      return;
   }
   _playerKnockedOutByInfo = FTATCachedPlayerInfo(otherPlayer);
}

const FUpgradeState& ATATPlayerState::GetAllUpgradeState() const
{
   UOSEAbilitySystemComponent* abilityComponent = CastChecked<UOSEAbilitySystemComponent>(AbilitySystemComponent);
   return abilityComponent->GetAllUpgradeState();
}

void ATATPlayerState::AuthorityOnEarlyDisconnect()
{
   check(HasAuthority());

   GetLootInventoryComponent()->AuthorityDropAllLootInvoluntary();
   UTATItemFunctionLibrary::DropItemsForKO(GetTATItemInventory());
   if (ATATCharacter* character = GetPawn<ATATCharacter>())
   {
      character->AuthorityOnEarlyDisconnect();
   }

   if (ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(this))
   {
      toastBroadcaster->ClientToastBroadcast_PlayerDisconnected(GetPlayerName());
   }
}

void ATATPlayerState::AuthoritySetMatchCompletionState(EMatchCompletionState newMatchCompletionState)
{
   check(HasAuthority());

   // Should never switch from Escaped <-> Caught
   if (newMatchCompletionState != EMatchCompletionState::Unset)
   {
      UE_CLOG(_matchCompletionState != EMatchCompletionState::Unset, LogTATPlayerState, Error, TEXT("[%s] AuthoritySetMatchCompletionState() | Unexpected transition from %s to %s!")
         , *GetName()
         , *UEnum::GetValueAsString(_matchCompletionState)
         , *UEnum::GetValueAsString(newMatchCompletionState));
   }

   _matchCompletionState = newMatchCompletionState;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _matchCompletionState, this);

   _BroadcastMatchCompletionStateChanged();
}

void ATATPlayerState::_OnRep_IsReadyChecked()
{
   _BroadcastIsReadyCheckedChanged();
}

void ATATPlayerState::_BroadcastIsReadyCheckedChanged()
{
   UE_LOG(LogTATPlayerState, Verbose, TEXT("%s is %s"), *GetPlayerName(), _isReadyChecked ? TEXT("ready") : TEXT("NOT ready"));
   OnIsReadyCheckedChanged.Broadcast(_isReadyChecked);
}

void ATATPlayerState::_OnRep_IsReadyToSkipCutscene()
{
   _BroadcastIsReadyToSkipCutscene();
}

void ATATPlayerState::_BroadcastIsReadyToSkipCutscene()
{
   UE_LOG(LogTATPlayerState, Verbose, TEXT("%s is %s to skip the cutscene"), *GetPlayerName(), _isReadyToSkipCutscene ? TEXT("ready") : TEXT("NOT ready"));
   OnIsReadyForCutsceneSkipChanged.Broadcast(_isReadyToSkipCutscene);
}

void ATATPlayerState::_OnRep_MapIntroState()
{
   _BroadcastMapIntroStateChanged();
}

void ATATPlayerState::_BroadcastMapIntroStateChanged()
{
   UE_LOG(LogTATPlayerState, Verbose, TEXT("%s's map intro state is now %s"), *GetPlayerName() , *UEnum::GetValueAsString(_mapIntroState));
   OnMapIntroStateChanged.Broadcast(_mapIntroState);
}

void ATATPlayerState::_SetMapIntroState(ETATMapIntroState newState)
{
   if (_mapIntroState != newState)
   {
      UE_LOG(LogTATPlayerState, Log, TEXT("Setting MapIntroState for player %d -> %s"),
         GetPlayerId(),
         *WriteToString<64>(StaticEnum<ETATMapIntroState>()->GetNameByValue(static_cast<int64>(newState))));
      _mapIntroState = newState;
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _mapIntroState, this);

      _BroadcastMapIntroStateChanged();
   }
}

void ATATPlayerState::_OnRep_PlayerColor()
{
   _BroadcastPlayerColorChanged();
}

void ATATPlayerState::_BroadcastPlayerColorChanged()
{
   UE_LOG(LogTATPlayerState, Verbose, TEXT("%s's player color is now %s"), *GetPlayerName(), *_playerColor.ToString());
   OnPlayerColorChanged.Broadcast(_playerColor);
}

void ATATPlayerState::_OnRep_IsInATeam()
{
   _BroadcastIsInATeamChanged();
}

void ATATPlayerState::_BroadcastIsInATeamChanged()
{
   OnIsInATeamChanged.Broadcast(_isInATeam);
}

void ATATPlayerState::_OnUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount) const
{
   OnUnconsciousTagChanged.Broadcast(tag, newTagCount);
}

ATATPlayerController* ATATPlayerState::_GetOwnerPlayerController() const
{
   return Cast<ATATPlayerController>(GetOwner());
}

void ATATPlayerState::_OnAutoExecCheats()
{
#if WITH_EDITOR && OSE_CHEATS_ENABLED
   APlayerController* pc = GetPlayerController();
   if (pc == nullptr)
   {
      return;
   }

   for (const FTATEditorSettingsCheatCommand& cmd : UTATEditorSettings::Get().AutoExecCheats)
   {
      if (cmd.Context == ETATEditorSettingsCheatCommandContext::Player)
      {
         cmd.RunCheat(GetWorld(), pc);
      }
   }
#endif
}

void ATATPlayerState::_OnAuthorityQuestLootChanged(ETATInventoryUpdateEventType eventType)
{
   if(ATATCharacter* character = GetPawn<ATATCharacter>())
   {
      character->MulticastOnQuestLootChanged(eventType);
   }
}

bool ATATPlayerState::IsMissionOwner() const
{
   if (UWorld* world = GetWorld())
   {
      if (auto* tatGameState = world->GetGameState<ATATGameState>())
      {
         return tatGameState->GetMissionOwnerPlayerState() == this;
      }
   }
   return false;
}

void ATATPlayerState::AuthoritySetPlayerIsInATeam(const bool val)
{
   check(HasAuthority());

   _isInATeam = val;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _isInATeam, this);

   _BroadcastIsInATeamChanged();
}

void ATATPlayerState::ForceLocalChangeCharacter(ETATCharacter character)
{
   _character = character;
   _OnRep_TATCharacter();
}

void ATATPlayerState::ForceLocalChangeOutfit(TArray<FTATCharacterLoadoutEntry> outfitLoadout)
{
   _ApplyOutfitLoadout(outfitLoadout);
}

void ATATPlayerState::_HandleTeamChanged(uint8 team)
{
   Super::_HandleTeamChanged(team);
   if(ATATCharacter* character = GetPawn<ATATCharacter>())
   {
      character->HandleTeamChanged();
   }
   _SetPlayerColorFromTeam();
   _tatLootInventory->SetTeam(team);

   if (HasAuthority())
   {
      if (UTATSharedObjectiveSubsystem* sharedObjectiveSubsystem = GetWorld()->GetSubsystem<UTATSharedObjectiveSubsystem>())
      {
         sharedObjectiveSubsystem->SetPlayerTeam(GetPlayerId(), team);
      }
      if(UTATIrisGroupSubsystem* groupSubsystem = GetWorld()->GetSubsystem<UTATIrisGroupSubsystem>())
      {
         // NOTE: also initialized in OnNetConnectionSet
         // TODO: remove from prior team group, if any?
         groupSubsystem->ForTeam(team).AllowGroupForPlayer(this);
      }
   }

   OnPlayerTeamChanged.Broadcast(team);
}
