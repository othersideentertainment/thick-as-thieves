// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Common/TATGameplayTagTableRow.h"
#include "GameFramework/TATDifficulty.h"
#include "Quests/Rewards/TATQuestReward.h"

// ue
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "TATQuestInfo.generated.h"

class UAkAudioEvent;
class UTATQuestGraph;
class UTATClueSetBase;
class UTexture2D;
struct FTATQuestObjectiveInfo;
struct FTATQuestRewardContext;
struct FTATQuestRewardRequirementContext;
enum class ETATCharacter : uint8;
class UTATScreenWidget;

UENUM()
enum class ETATContractOutroFlow : uint8
{
   // Automatically completes the contract after completing the match
   Automatic,
   // requires interaction with an interactable in the thieves den to complete the contract
   Interactable,
   // Requires interaction with the Tristan fairy in the thieves den to complete the contract
   Fairy
};

// Text for journal entry, with per-character overrides
USTRUCT()
struct FTATQuestJournalEntry
{
   GENERATED_BODY()

   // default journal text if there is no override for that character
   UPROPERTY(EditAnywhere, meta = (MultiLine = true))
   FText DefaultJournalText;

   // character-specific journal text
   UPROPERTY(EditAnywhere, meta = (MultiLine = true))
   TMap<ETATCharacter, FText> CharacterOverrides;

   const FText& ForCharacter(ETATCharacter character) const;
};

// A minimal base class for QuestInfo so that Redirects can also be them
USTRUCT(NotBlueprintType)
struct TAT_API FTATMinimalQuestInfo : public FTATGameplayTagTableRow
{
   GENERATED_BODY()

public:

   virtual FGameplayTag GetQuestTag() const { return FGameplayTag(); }
   virtual const FText& GetTitle() const { return FText::GetEmpty(); }
   virtual FText GetObjectiveText(const UObject* worldContext) const { return FText::GetEmpty(); }

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const {}
#endif
};

// Struct with basic info about a quest
// Vacillating between data table and data asset, so exposure kept small for now
USTRUCT()
struct TAT_API FTATQuestInfo : public FTATMinimalQuestInfo
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, Category = Quest)
   FText Title;

   /// This text will support the following field(s): {MatchDurationInMinutes}, {EndgameDurationInMinutes}
   UPROPERTY(EditAnywhere, Category = Quest)
   FText Description;

   UPROPERTY(EditAnywhere, Category = Quest)
   FText PostMatchSuccessText;

   UPROPERTY(EditAnywhere, Category = Quest)
   FText PostMatchFailureText;

   // Displayed in pre/post-match UI to visualize the objective
   UPROPERTY(EditAnywhere, Category = Quest)
   TSoftObjectPtr<UTexture2D> ObjectiveDisplayImage;

   UPROPERTY(EditAnywhere, Category = Quest, meta = (TitleProperty = "{Quantity} {RewardType}"))
   TArray<FTATQuestReward> Rewards;

   void GrantRewards(const FTATQuestRewardContext& context) const;

   virtual const FText& GetTitle() const override final { return Title; }
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};

// A stub for now, but will be moving stuff here
USTRUCT(meta=(RowNameTag=QuestTag))
struct TAT_API FTATMissionInfo : public FTATQuestInfo
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Quest, meta = (Categories="Mission", HideFromDataTableEditorColumn))
   FGameplayTag QuestTag;

   /// Length of match (in seconds) based on difficulty
   UPROPERTY(Config, EditAnywhere, Category = Quest, meta = (ArraySizeEnum="/Script/TAT.ETATDifficulty", Units="s", ClampMin="0"))
   float MatchDurationsByDifficulty[static_cast<int>(ETATDifficulty::MAX)] = {5400.0f, 3600.0f, 2700.0f };

   // Controls the major map variation for that mission
   UPROPERTY(EditAnywhere, Category = Quest, meta = (DisplayAfter=Description, DataTableHeaderPriority = 1))
   TSoftObjectPtr<UTATQuestGraph> QuestGraph;

   /// Collection of reward "tiers" with support for locking them behind requirements which must be met to receive the rewards.
   /// Order is not significant; each reward is individually evaluated w.r.t its requirements.
   /// TODO: move up into FTATQuestInfo when support for personal quests is in reach
   UPROPERTY(EditAnywhere, Category = Quest, meta = (DisplayAfter=Rewards))
   TArray<FTATQuestConditionalReward> BonusRewards;

   // Returns a bitmask indicating the bonus reward levels unlocked.
   // i.e. a bitmask of: ...0010
   // would grant the 2nd bonus reward (but not the first)
   uint32 GetUnlockedBonusRewardsBitmask(const FTATQuestRewardRequirementContext& context) const;

   void GrantBonusRewards(const FTATQuestRewardContext& context, uint32 unlockedBonusRewardsBitmask) const;

   static bool IsBonusRewardUnlocked(uint32 unlockedBonusRewardsBitmask, int32 rewardIndex);
   
   virtual FGameplayTag GetQuestTag() const override final { return QuestTag; }

   float GetMatchDurationForDifficulty(ETATDifficulty difficulty) const;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};

// A stub for now, but will be moving stuff here
USTRUCT(meta=(RowNameTag=QuestTag))
struct TAT_API FTATContractInfo : public FTATQuestInfo
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Quest, meta = (Categories="Contract", HideFromDataTableEditorColumn))
   FGameplayTag QuestTag;


   UPROPERTY(EditAnywhere, Category = Quest, meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATQuestObjectiveInfo", HideFromDataTableEditorColumn, DisplayAfter=Description))
   FInstancedStruct Objective;

   // The map that this contract is for
   // Allowed to be missing for contracts
   // CONSIDER: Make this cook-only-untracked, so it doesn't pull maps that it references into the build if otherwise unused
   UPROPERTY(EditAnywhere, Category = Quest, meta = (DisplayAfter=Objective, DataTableHeaderPriority = 1))
   TSoftObjectPtr<UWorld> Map;

   // Dynamic sublevels to load when this contract is active
   UPROPERTY(EditAnywhere, Category = Quest, meta = (DisplayAfter=Map, DataTableHeaderPriority = 2))
   TArray<TSoftObjectPtr<UWorld>> SublevelsToLoad;

   // The contract order in the contract chain.
   // Should be 1:1 with the contract number in the tag
   // Used to prioritize contracts when multiple players have an active one
   UPROPERTY(EditAnywhere, Category = Quest, meta = (DisplayAfter=Objective, DataTableHeaderPriority = 1))
   int ContractOrder = 0;

   // The clues associated with this quest
   // CLUE-WIP: the structure of this clue data is subject to change
   // CLUE-WIP: Add validation that it is set once more settled
   UPROPERTY(EditAnywhere, Category = Quest, meta = (DisplayAfter=ObjectiveDisplayImage, DataTableHeaderPriority = 2))
   TSoftObjectPtr<UTATClueSetBase> Clues;

   // What the flow is for completing the quest once back from the match
   UPROPERTY(EditAnywhere, Category = Narrative, meta = (DisplayAfter=Rewards, DataTableHeaderPriority = 4))
   ETATContractOutroFlow OutroFlow = ETATContractOutroFlow::Automatic;

   // NOTE: Just plain text is *not* the (sole) final shape of the intro/outro narrative
   //       encounters. There will definitely be other shapes that this takes. I
   //       am just erring on the side of keeping it bare in order to not make
   //       assumptions about the shape of future containers for that narrative
   //       content.

   // Note shown when accepting the quest
   UPROPERTY(EditAnywhere, Category = Narrative, meta = (MultiLine = true, DisplayAfter = OutroFlow, DataTableHeaderPriority = 4))
   FText IntroEncounterText;

   // Note shown when completing the quest
   UPROPERTY(EditAnywhere, Category = Narrative, meta = (DisplayAfter=IntroEncounterText, MultiLine = true, DataTableHeaderPriority = 5))
   FText OutroEncounterText;

   // VO Audio played during the contract outro
   // (i.e. from Tristan)
   UPROPERTY(EditAnywhere, Category = Narrative, meta = (EditCondition = "OutroFlow == ETATContractOutroFlow::Fairy", EditConditionHides, HideFromDataTableEditorColumn, DisplayAfter=OutroEncounterText))
   TSoftObjectPtr<UAkAudioEvent> OutroEncounterVO;

   // [Optional] Specifies a screen widget that will be displayed after the contract outro.
   // This can be used to show any full screen widget - for example, showing the credits after completing the last contract.
   UPROPERTY(EditAnywhere, Category = Narrative)
   TSoftClassPtr<UTATScreenWidget> PostOutroCustomScreenWidget;

   // [Optional] shown after failing the objective in a match
   UPROPERTY(EditAnywhere, Category = Narrative, meta = (DisplayAfter=OutroVO, MultiLine = true, DataTableHeaderPriority = 5))
   FText ObjectiveFailedEncounterText;

   // The journal entry added when the quest is started
   UPROPERTY(EditAnywhere, Category=Narrative, meta=(HideFromDataTableEditorColumn, DisplayAfter = ObjectiveFailedEncounterText))
   FTATQuestJournalEntry IntroJournalEntry;
   
   // The journal entry added when the quest is complete
   UPROPERTY(EditAnywhere, Category = Narrative, meta = (HideFromDataTableEditorColumn, DisplayAfter = IntroJournalEntry))
   FTATQuestJournalEntry CompleteJournalEntry;

   // If set, completing this contract will start this contract immediately, skipping its intro
   UPROPERTY(EditAnywhere, Category = Chain, meta=(Categories="Contract", HideFromDataTableEditorColumn, DisplayAfter = CompleteJournalEntry))
   FGameplayTag NextContractInChain;

   
   const FTATQuestObjectiveInfo* GetObjective() const;
   const FTATQuestObjectiveInfo& GetObjectiveChecked() const;

   virtual FGameplayTag GetQuestTag() const override final { return QuestTag; }
   virtual FText GetObjectiveText(const UObject* worldContext) const override final;
#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void(const FText&)> reportError) const override;
#endif
};
