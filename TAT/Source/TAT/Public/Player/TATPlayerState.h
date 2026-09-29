// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Items/TATItemInventorySystemInterface.h"
#include "Loot/TATLootInterface.h"
#include "Quests/TATPlayerQuestSlot.h"
#include "SaveGame/TATCharacterDataContext.h"
#include "AI/Perception/TATHearingStimSourceReactor.h"

// ose
#include "Player/OSEPlayerState.h"
#include "Identity/OSESaveGameSystem.h"

// ue
#include "GameplayAbilitySpecHandle.h"

#include "TATPlayerState.generated.h"

// tat
class ATATPlayerController;
class UTATAttributeSet;
class UTATKnownCluesComponent;
class UTATStaminaAttributeSet;
class UTATItemInventoryComponent;
class UTATRootPlayerObjective;
class UTATPlayerQuestComponent;
class UTATTokenInventoryComponent;
class UTATToolComponent;
class UTATMatchSettingsBase;
struct FMatchPersistentData;
struct FTATInventorySlot;
struct FTATItemLoadout;

// ue4
struct FLinearColor;

UENUM(BlueprintType)
enum class ETATMapIntroState : uint8
{
   Loading,
   TitleCard,
   Cutscene,
   MissionIntroduction,
   Complete,

   MAX                  UMETA(Hidden)
};

UCLASS()
class TAT_API ATATPlayerState
   : public AOSEPlayerState
   , public ITATItemInventorySystemInterface
   , public ITATLootInventoryInterface
   , public ITATHearingStimSourceReactor
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerColorChanged, const FLinearColor, color);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsInATeamChanged, bool, isOnTeam);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTATCharacterChanged, ATATPlayerState*, ps, ETATCharacter, character);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTATOutfitChanged, ATATPlayerState*, ps, const TArray<FTATCharacterLoadoutEntry>&, outfitLoadout);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerLevelChanged, ATATPlayerState*, ps, int32, level);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterSaveIdChanged, ATATPlayerState*, ps, FTATCharacterSaveId, character);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchCompletionStateChanged, ATATPlayerState*, ps, EMatchCompletionState, matchCompletionState);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUISelectedTATCharacterChanged, ETATCharacter, character);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTotemRespawnCountUpdated, int32, newTotemRespawnCount);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsReadyCheckedChanged, bool, isReadyChecked);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsReadyForCutsceneSkipChanged, bool, isReadyToSkipCutscene);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapIntroStateChanged, ETATMapIntroState, newState);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveCompleteChanged, bool, isObjectiveComplete, ETATPlayerQuestSlot, questSlot);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNumberOfTimesKnockedOutByOtherPlayersChanged);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnconsciousTagChanged, FGameplayTag, UnconsciousTag, int32, tagCount);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNameChanged);

public:
   ATATPlayerState(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   void OnNetConnectionSet();

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // statics
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT", meta = (WorldContext = "contextObj"))
   static ATATPlayerState* GetTATPlayerState(const UObject* contextObj, int index);
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT", meta = (WorldContext = "contextObj"))
   static ATATPlayerState* GetLocalTATPlayerState(const UObject* contextObj);

   // attributes
   virtual UTATAttributeSet* GetTATAttributeSet() const { return _tatAttributes; }

   // from ITATItemInventorySystemInterface
   virtual UTATItemInventoryComponent* GetTATItemInventory() const override { return _tatItemInventory; }

   // from ITATLootInventoryInterface
   virtual UTATLootInventoryComponent* GetLootInventoryComponent() const override { return _tatLootInventory; }

   // what color should we use to identify this player?
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   FLinearColor GetPlanningPhaseColor() const { return _playerColor; }
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnPlayerColorChanged OnPlayerColorChanged;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnNameChanged OnPlayerNameChanged;

   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   bool GetIsInATeam() const { return _isInATeam; }
   
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnIsInATeamChanged OnIsInATeamChanged;

   const FGuid& GetAnalyticsId() const { return _analyticsId; }

   // which character are we?
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   ETATCharacter GetTATCharacter() const { return _character; }

   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   FTATCharacterSaveId GetCharacterSaveId() const { return _characterSave; }

   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   int32 GetPlayerLevel() const { return _playerLevel; }

   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   const TArray<FTATCharacterLoadoutEntry>& GetCurrentOutfitLoadout() const { return _currentOutfitLoadout; }

   /// No need to expose this to blueprints because there's already UTATCharacterDataContextUtils::GetCharacterDataContextFromPlayerState
   FTATCharacterDataContext GetCharacterDataContext() const;

   /// Sends the server the current upgrade and loadout state from the save game.
   /// Only useful to call on the local player state.
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ServerRefreshCharacterUpgradeAndLoadoutState();

   UFUNCTION(Reliable, Server, WithValidation)
   void ServerSetTATCharacterAndUpgradeState(FTATCharacterSaveId saveId, ETATCharacter character, const FUpgradeState& upgradeState,
                                             int32 level, const TArray<FTATCharacterLoadoutEntry>& gearLoadout, const TArray<FTATCharacterLoadoutEntry>& outfitLoadout);
   UFUNCTION(Reliable, Server, WithValidation)
   void ServerChangeOutfitLoadout(const TArray<FTATCharacterLoadoutEntry>& outfitLoadout);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ChangeOutfitLoadout(const FTATCharacterLoadout& newLoadout);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ChangeTATCharacter(ETATCharacter character);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ChangeSavedCharacter(FTATCharacterSaveId character);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ChangePlayerLevel(int32 level);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ForceRespawnCharacter();
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnTATCharacterChanged OnTATCharacterChanged;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnCharacterSaveIdChanged OnCharacterSaveIdChanged;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnTATOutfitChanged OnCharacterOutfitChanged;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnPlayerLevelChanged OnPlayerLevelChanged;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnUnconsciousTagChanged OnUnconsciousTagChanged;

   // which character is selected in the ui?
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   ETATCharacter GetUISelectedTATCharacter() const { return _uiSelectedCharacter; }
   UFUNCTION(Reliable, Server, WithValidation)
   void ServerSetUISelectedTATCharacter(ETATCharacter character);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ChangeUISelectedTATCharacter(ETATCharacter character);
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnUISelectedTATCharacterChanged OnUISelectedTATCharacterChanged;

   // readycheck -- can be used to confirm whether everyone is ready to play a selected mission, or leave a results screen, etc
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   bool IsReadyChecked() const { return _isReadyChecked; }
   UFUNCTION(Reliable, Server, WithValidation)
   void ServerSetIsReadyChecked(bool isReadyChecked);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void SetIsReadyChecked(bool isReadyChecked);
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnIsReadyCheckedChanged OnIsReadyCheckedChanged;
   void ForceLocalIsReadyChecked(bool isReadyChecked);

   // ready for cutscene skip -- used to confirm whether everyone wants to skip the cutscene
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   bool IsReadyForCutsceneSkip() const { return _isReadyToSkipCutscene; }
   UFUNCTION(Reliable, Server, WithValidation)
   void ServerSetIsReadyForCutsceneSkip(bool isReadyToSkipCutscene);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void SetIsReadyForCutsceneSkip(bool isReadyToSkipCutscene);
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnIsReadyCheckedChanged OnIsReadyForCutsceneSkipChanged;
   
   // map intro state
   UFUNCTION(Reliable, Server, WithValidation)
   void ServerSetMapIntroState(ETATMapIntroState newState);
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void LocalSetMapIntroStateComplete(ETATMapIntroState completedState);
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   ETATMapIntroState GetMapIntroState() const { return _mapIntroState; }
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnMapIntroStateChanged OnMapIntroStateChanged;

   void AuthorityInitializeQuest();
   void AuthorityForceObjectiveComplete(ETATPlayerQuestSlot questSlot);
   bool IsQuestCompleteForMatchEnd(ETATPlayerQuestSlot questSlot, bool escaped, const FMatchPersistentData& matchData) const;
   UFUNCTION(BlueprintPure, Category= "PlayerState|TAT|Quest")
   bool IsQuestObjectiveComplete(ETATPlayerQuestSlot questSlot) const;
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT|Quest")
   FGameplayTag GetActiveQuestTag(ETATPlayerQuestSlot questSlot) const;
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT|Quest")
   bool HasActiveQuest(ETATPlayerQuestSlot questSlot) const;
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT|Quest")
   UTATRootPlayerObjective* GetObjectiveForSlot(ETATPlayerQuestSlot questSlot) const;
   TConstArrayView<TObjectPtr<UTATRootPlayerObjective>> GetObjectives() const;

   // may apply to mission or contracts
   TConstArrayView<FGameplayTag> GetActiveQuestTags() const;
   bool HasQuestRelatedLoot(FTATLootIdentifier lootId) const;
   // Returns true if the player has any loot in their inventory that matches the loot from one of their quests
   bool HasRelevantQuestLootInInventory() const;

   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnQuestObjectiveCompleteChanged OnQuestObjectiveCompleteChanged;

   /// Local match settings (mission owner only, for UI purposes)
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   UTATMatchSettingsBase* GetLocalUIMatchSettings() const;
   /// Upload our current copy of the local ui match settings to the server (mission owner only)
   UFUNCTION(BlueprintCallable, Category = "PlayerState|TAT")
   void ServerUpdateMatchSettings();
   UFUNCTION(Reliable, Server, WithValidation)
   void ServerUpdateMatchSettingsBytes(const TArray<uint8>& newMatchSettingsData);

   // match completion state
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnMatchCompletionStateChanged OnMatchCompletionStateChanged;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PlayerState|TAT")
   void AuthorityOnRespawned();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PlayerState|TAT")
   int32 AuthorityIncrementTotemRespawnCount(bool broadcastTotemRespawnToast);

   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   int32 GetTotemRespawnCount() const { return _numTotemRespawnsThisMatch; }

   /// Intended for use by cheats
   void AuthoritySetTotemRespawnCountDirect(int32 newTotemRespawnCount);

   // match completion state
   UPROPERTY(BlueprintAssignable, Category = "PlayerState|TAT")
   FOnTotemRespawnCountUpdated OnTotemRespawnCountUpdated;

   // begin ITATHearingStimSourceReactor
   virtual void HandleReactToOwnStim(const FGameplayTag& stimTag, float loudness) override;
   // end ITATHearingStimSourceReactor
 
   // XP that the player collects when playing a match that is different from the default XP EndMatch gain, 
   // this XP will be granted at the end of the match together with the rest of EndMatch XP
   // for example: discovered secret area of a map, killed special enemy, etc.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PlayerState|TAT|XP")
   void AuthorityGrantXPWhenMatchEnds(const FTATFinishedMatchXPGained& xpGained);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "PlayerState|TAT|XP")
   const TArray<FTATFinishedMatchXPGained>& AuthorityGetAdditionalXPWhenMatchEnds() const;

private:
   UFUNCTION()
   void _OnRep_NumTotemRespawnsThisMatch();

public:

   void AuthorityWasKnockedOutByOtherPlayer(const ATATPlayerState* otherPlayer);

   UPROPERTY()
   FOnNumberOfTimesKnockedOutByOtherPlayersChanged OnNumberOfTimesKnockedOutByOtherPlayersChanged;

   const FUpgradeState& GetAllUpgradeState() const;

   void AuthorityOnEarlyDisconnect();

   void AuthoritySetMatchCompletionState(EMatchCompletionState newMatchCompletionState);

   bool HasEscapedOrBeenCaptured() const { return _matchCompletionState != EMatchCompletionState::Unset; }
   UFUNCTION(BlueprintPure, Category = "PlayerState|TAT")
   EMatchCompletionState GetMatchCompletionState() const { return _matchCompletionState; }

   const FTATCachedPlayerInfo& GetPlayerKnockedOutByInfo() const { return _playerKnockedOutByInfo; }

   // from AActor
   virtual void BeginPlay();
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void ClientInitialize(AController* controller) override;

   // from APlayerState
   virtual void CopyProperties(APlayerState* playerState) override;

   UFUNCTION(BlueprintPure, Category="PlayerState|TAT")
   bool IsMissionOwner() const;

   void AuthoritySetPlayerIsInATeam(bool val);

   void ForceLocalChangeCharacter(ETATCharacter character);

   void ForceLocalChangeOutfit(TArray<FTATCharacterLoadoutEntry> outfitLoadout);

protected:
   virtual void _HandleTeamChanged(uint8 team) override;
   UFUNCTION(BlueprintNativeEvent, Category = "PlayerState|TAT")
   void AuthorityApplyCheatsFromEditorSettings();
   UFUNCTION(BlueprintNativeEvent, Category = "PlayerState|TAT")
   void ClientApplyCheatsFromEditorSettings();

   UFUNCTION(BlueprintImplementableEvent, Category = "PlayerState|TAT")
   void OnInitLocalPlayerState();
   void _SetPlayerColorFromTeam();

   virtual void OnRep_PlayerName();

public:
   DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerTeamChanged, uint8);
   FOnPlayerTeamChanged OnPlayerTeamChanged;

private:
   void _GetCharacterLoadoutTags(const FTATCharacterDataContext& context, ETATLoadoutType loadoutType, TArray<FTATCharacterLoadoutEntry>& outLoadout) const;

   /// Callback for save system `OnSaveDataStateChanged`
   /// On save data loaded, initialize player state
   UFUNCTION()
   void _OnSaveDataStateChanged(EOSESaveDataState newState);
   void _InitLocalSaveData();
   void _InitLocalPlayerState();
   void _ApplyUpgradeState(const FUpgradeState& state);
   void _ForceChangeSavedCharacter(FTATCharacterSaveId character);

   UFUNCTION()
   void _BroadcastQuestObjectiveCompleteChanged(bool isComplete, ETATPlayerQuestSlot slot);

   UFUNCTION()
   void _OnRep_TATCharacter();
   void _BroadcastTATCharacterChanged();

   UFUNCTION()
   void _OnRep_UISelectedTATCharacter();
   void _BroadcastUISelectedTATCharacterChanged();
   void _InitTATCharacter();

   UFUNCTION()
   void _OnRep_CurrentOutfitLoadout();
   void _BroadcastCurrentOutfitLoadoutChanged();

   UFUNCTION()
   void _OnRep_PlayerLevel();
   void _BroadcastPlayerLevelChanged();
   void _InitPlayerLevel();
   UFUNCTION()
   void _OnXPProgressionChanged();

   UFUNCTION()
   void _OnRep_MatchCompletionState();
   void _BroadcastMatchCompletionStateChanged();

   void _InitAnalyticsId();
   UFUNCTION(Reliable, Server)
   void _ServerSetAnalyticsId(const FGuid& analyticsId);
   
   void _ApplyGearLoadout(const TArray<FTATCharacterLoadoutEntry>& gearLoadout);
   void _LoadAndAddGearToLoadout(const TArray<TSoftClassPtr<UTATToolComponent>, TInlineAllocator<8>>& gearClasses);
   void _ApplyOutfitLoadout(const TArray<FTATCharacterLoadoutEntry>& outfitLoadout);

   UFUNCTION()
   void _OnRep_IsReadyChecked();
   void _BroadcastIsReadyCheckedChanged();

   UFUNCTION()
   void _OnRep_IsReadyToSkipCutscene();
   void _BroadcastIsReadyToSkipCutscene();

   UFUNCTION()
   void _OnRep_MapIntroState();
   void _BroadcastMapIntroStateChanged();
   void _SetMapIntroState(ETATMapIntroState newState);

   UFUNCTION()
   void _OnRep_PlayerColor();
   void _BroadcastPlayerColorChanged();

   UFUNCTION()
   void _OnRep_IsInATeam();
   void _BroadcastIsInATeamChanged();

   UFUNCTION()
   void _OnUnconsciousTagChanged(const FGameplayTag tag, int32 newTagCount) const;

   UFUNCTION()
   void _OnAutoExecCheats();

   UFUNCTION()
   void _OnAuthorityQuestLootChanged(ETATInventoryUpdateEventType eventType);

   ATATPlayerController* _GetOwnerPlayerController() const;

protected:
   // give some default items to players to start a map with
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
   TArray<FTATInventorySlot> DefaultItems;

   // If this is specified, it will always use this character, ignoring save
   // For use by tutorial
   UPROPERTY(EditDefaultsOnly, Category = "Overrides")
   ETATCharacter _overrideCharacter = ETATCharacter::None;

   // Skips reading to/from the save for selected character and loadout
   // For use by tutorial
   UPROPERTY(EditDefaultsOnly, Category = "Overrides")
   bool _ignoreSave = false;
   
private:
   // attributes
   UPROPERTY()
   UTATAttributeSet* _tatAttributes = nullptr;
   UPROPERTY()
   UTATStaminaAttributeSet* _tatStaminaAttributes = nullptr;

   // the character that the player has selected in the ui
   UPROPERTY(ReplicatedUsing = _OnRep_UISelectedTATCharacter)
   ETATCharacter _uiSelectedCharacter = ETATCharacter::None;

   // the character that the player has chosen to spawn as
   UPROPERTY(ReplicatedUsing = _OnRep_TATCharacter)
   ETATCharacter _character = ETATCharacter::None;

   UPROPERTY(ReplicatedUsing = _OnRep_CurrentOutfitLoadout)
   TArray<FTATCharacterLoadoutEntry> _currentOutfitLoadout;

   FTATCharacterSaveId _characterSave;

   UPROPERTY(ReplicatedUsing = _OnRep_PlayerLevel)
   int32 _playerLevel = 0;

   UPROPERTY(ReplicatedUsing = _OnRep_MatchCompletionState)
   EMatchCompletionState _matchCompletionState = EMatchCompletionState::Unset;

   // are we ready to go?
   UPROPERTY(ReplicatedUsing = _OnRep_IsReadyChecked)
   bool _isReadyChecked = false;

   // are we ready to skip this cutscene?
   UPROPERTY(ReplicatedUsing = _OnRep_IsReadyToSkipCutscene)
   bool _isReadyToSkipCutscene = false;

   UPROPERTY(ReplicatedUsing = _OnRep_MapIntroState)
   ETATMapIntroState _mapIntroState = ETATMapIntroState::Loading;

   // local match settings instance, used for editing in a UI before sending the values to the server (mission owner only)
   UPROPERTY(Transient)
   TObjectPtr<UTATMatchSettingsBase> _localMatchSettings;

   UPROPERTY(Transient)
   UTATItemInventoryComponent* _tatItemInventory = nullptr;

   UPROPERTY(EditAnywhere)
   UTATLootInventoryComponent* _tatLootInventory = nullptr;

   UPROPERTY()
   TObjectPtr<UTATTokenInventoryComponent> _tokenInventory = nullptr;

   UPROPERTY()
   TObjectPtr<UTATPlayerQuestComponent> _questComponent = nullptr;

   UPROPERTY()
   UTATKnownCluesComponent* _knownCluesComponent = nullptr;

   UPROPERTY(Transient, Replicated, ReplicatedUsing = _OnRep_NumTotemRespawnsThisMatch)
   int32 _numTotemRespawnsThisMatch = 0;

   // XP that the player collects when playing a match that is different from the default XP EndMatch gain, 
   // this XP will be granted at the end of the match together with the rest of EndMatch XP
   UPROPERTY(Transient)
   TArray<FTATFinishedMatchXPGained> _additionalGainedXP;

   // Color used to represent this player.
   // Assigned server-side in BeginPlay, selected from a pool of colors in TATGameMode
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_PlayerColor)
   FLinearColor _playerColor;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_IsInATeam)
   bool _isInATeam { false };

   UPROPERTY(Transient, Replicated)
   FGuid _analyticsId;

   UPROPERTY(Transient)
   FTATCachedPlayerInfo _playerKnockedOutByInfo;

#if WITH_EDITOR
   bool _ranAutoExecCheats = false;
#endif
};
