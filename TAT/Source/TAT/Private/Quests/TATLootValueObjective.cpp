// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/TATLootValueObjective.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Loot/TATLootInventory.h"
#include "Quests/TATPlayerObjective.h"
#include "Loot/TATLootSubsystem.h"
#include "Player/TATPlayerState.h"
#include "Character/TATTeams.h"
#include "Loot/TATLootInventorySubsystem.h"

// ose
#include "Character/OSETeamInterface.h"

// ue
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootValueObjective)

struct FTATStealLootValueTrackerParams : public FTATObjectiveTrackerParams
{
   TAT_DEFINE_TRACKER_PARAMS(FTATStealLootValueTrackerParams)
   
   int32 Amount = 0;
};

FText FTATLootValueObjectiveInfo::GetObjectiveText(const UObject* worldContext) const
{
   const FText& templateText = OverrideObjectiveText
      ? ObjectiveText
      : UTATProjectSettings::Get().DefaultLootValueObjectiveText;

   return FText::FormatNamed(templateText,
      TEXT("Amount"), Amount);
}

FGameplayTag FTATLootValueObjectiveInfo::GetRelatedLootTag() const
{
   return FGameplayTag::EmptyTag;
}

FTATObjectiveTrackerPayload FTATLootValueObjectiveInfo::CreateTracker() const
{
   TSharedPtr<FTATStealLootValueTrackerParams> trackerParams = MakeShared<FTATStealLootValueTrackerParams>();
   trackerParams->Amount = Amount;
   return FTATObjectiveTrackerPayload{ UTATLootValueObjectiveTracker::StaticClass(), MoveTemp(trackerParams) };
}

FString FTATLootValueObjectiveInfo::GetDebugDescription() const
{
   return FString::Printf(TEXT("Steal loot worth %i"), Amount);
}

#if WITH_EDITOR
void FTATLootValueObjectiveInfo::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if (Amount <= 0)
   {
      reportError(INVTEXT("Loot value objective has zero or negative amount requirement"));
   }

   if (OverrideObjectiveText && ObjectiveText.IsEmpty())
   {
      reportError(INVTEXT("Objective text is overridden but empty"));
   }
}
#endif

UTATLootValueObjectiveTracker::UTATLootValueObjectiveTracker()
   : _owningPlayerTeam(IOSETeamInterface::kInvalidTeam)
{
}

void UTATLootValueObjectiveTracker::Initialize(const FTATObjectiveTrackerContext& context)
{
   const FTATStealLootValueTrackerParams* params = context.GetParams<FTATStealLootValueTrackerParams>();
   check(params != nullptr);
   _amount = params->Amount;
   
   auto getTeam = [](UTATLootInventoryComponent* comp) -> uint8
   {
      const IOSETeamInterface* teamInterface = Cast<IOSETeamInterface>(comp ? comp->GetOwner() : nullptr);
      return (teamInterface != nullptr) ? teamInterface->GetTeam() : IOSETeamInterface::kInvalidTeam;
   };

   _inventory = UTATLootInventoryComponent::GetForActor(context.PlayerState);
   if (ensure(_inventory))
   {
      // Get the owning player's team. This may not have been assigned yet, so we'll register for team change events to handle it when it does.
      if (ATATPlayerState* ps = Cast<ATATPlayerState>(_inventory->GetOwner()))
      {
         ps->OnPlayerTeamChanged.AddUObject(this, &UTATLootValueObjectiveTracker::_OnOwningPlayerTeamChanged);
      }
      const uint8 playerTeam = getTeam(_inventory);
      if (playerTeam != IOSETeamInterface::kInvalidTeam)
      {
         _OnOwningPlayerTeamChanged(playerTeam);
      }
   }
}

bool UTATLootValueObjectiveTracker::IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const
{
   int32 totalValue = 0;
   totalValue += matchData.KeptCarriedLootValue;
   totalValue += matchData.StashedLootValue;
   totalValue += matchData.AllyCarriedLootValue;
   return totalValue >= _amount;
}

void UTATLootValueObjectiveTracker::CheatComplete()
{
   if (IsComplete() || _inventory == nullptr)
   {
      return;
   }

   int32 totalValue = _GetCurrentLootValue();
   if (totalValue >= _amount)
   {
      return;
   }

   const UDataTable* dataTable = nullptr;
   if (const UTATLootSubsystem* lootSubsystem = UTATLootSubsystem::Get(_inventory))
   {
      dataTable = lootSubsystem->GetLootDataTable();
   }
   if (dataTable == nullptr)
   {
      return;
   }

   // Just give random loot items until the player has the required total value of loot
   // That way any later parts of the flow still work, and there isn't a different book-keeping
   dataTable->ForeachRow<FTATLootInfo>(TEXT("UTATLootValueObjectiveTracker::CheatComplete"),
      [&](const FName& key, const FTATLootInfo& lootInfo)
      {
         if (totalValue >= _amount)
         {
            // we're done, but ForeachRow doesn't have a "break" mechanism
            return;
         }

         // Skip any loot that doesn't look normal-ish
         if (!lootInfo.LootIdentifier.IsValid()
            || lootInfo.DisplaySprite.IsNull()
            || lootInfo.LootStaticMesh.StaticMesh.IsNull()
            || lootInfo.ActorClass.IsNull()
            || lootInfo.IsLargeCarry
            || lootInfo.IsQuestRelated
            || lootInfo.Value <= 0
            || lootInfo.AutoConvertToMoney)
         {
            return;
         }

         // Just hopes there is enough inventory space, but the player should hopefully be able to resolve that themselves
         FTATLootItemVariant lootItem(lootInfo.LootIdentifier);
         if (lootInfo.RequiresInstanceStorage())
         {
            lootItem.Set(lootInfo.CreateDefaultInstance(this));
         }
         if (_inventory->AuthorityTryPickupLootItem(lootItem))
         {
            totalValue += lootInfo.Value;
         }
      });
}

void UTATLootValueObjectiveTracker::_OnOwningPlayerTeamChanged(uint8 newTeam)
{
   if (!ensure(_inventory))
   {
      return;
   }
   
   UWorld* world = _inventory->GetWorld();
   check(world != nullptr);
   UTATLootInventorySubsystem* lootInventorySubsystem = world->GetSubsystem<UTATLootInventorySubsystem>();

   if (lootInventorySubsystem != nullptr && _owningPlayerTeam != IOSETeamInterface::kInvalidTeam)
   {
      lootInventorySubsystem->GetLootValueChangedDelegateForTeam(_owningPlayerTeam).RemoveAll(this);
   }

   _owningPlayerTeam = newTeam;

   if (lootInventorySubsystem != nullptr)
   {
      lootInventorySubsystem->GetLootValueChangedDelegateForTeam(_owningPlayerTeam).AddUObject(this, &UTATLootValueObjectiveTracker::_OnLootInventoryComponentLootValueChanged);
   }

   _RefreshObjectiveState();
}

void UTATLootValueObjectiveTracker::_OnLootInventoryComponentLootValueChanged(uint8 team, UTATLootInventoryComponent* inventoryComponent)
{
   _RefreshObjectiveState();
}

int32 UTATLootValueObjectiveTracker::_GetCurrentLootValue() const
{
   if (!ensure(_inventory))
   {
      return 0;
   }

   int32 result = 0;
   result += _inventory->GetTotalHeldLootValue();

   // this includes allied stashed loot - no need to add this for each allied coop player
   result += _inventory->GetTotalStashedLootValue();

   UWorld* world = _inventory->GetWorld();
   if (ensure(world != nullptr))
   {
      if (UTATLootInventorySubsystem* inventorySubsystem = world->GetSubsystem<UTATLootInventorySubsystem>())
      {
         const TOptional<uint8> teamFilter = (_owningPlayerTeam != IOSETeamInterface::kInvalidTeam) ? TOptional{_owningPlayerTeam} : TOptional<uint8>{};
         inventorySubsystem->ForEachPlayerOwnedLootInventoryComponent(
            [this, &result](UTATLootInventoryComponent* inventoryComponent)
            {
               if (_IsAlliedCoopPlayerInventory(inventoryComponent))
               {
                  result += inventoryComponent->GetTotalHeldLootValue();
               }
            }, teamFilter);
      }
   }

   return result;
}

void UTATLootValueObjectiveTracker::_RefreshObjectiveState()
{
   const int32 totalValue = _GetCurrentLootValue();
   _SetProgress(FTATQuestObjectiveState { .IsComplete = totalValue >= _amount, .Progress = totalValue });
}

bool UTATLootValueObjectiveTracker::_IsAlliedCoopPlayerInventory(UTATLootInventoryComponent* inventoryComponent) const
{
   if (!IsValid(inventoryComponent) || inventoryComponent == _inventory)
   {
      return false;
   }
   
   auto getPlayerState = [](UTATLootInventoryComponent* inv) -> ATATPlayerState*
   {
      return Cast<ATATPlayerState>(inv ? inv->GetOwner() : nullptr);
   };

   const EOSETeamAttitude attitude1 = UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(
      getPlayerState(_inventory),
      getPlayerState(inventoryComponent),
      ETATTeamDisguiseHandling::UseOriginalTeam);
   return attitude1 == EOSETeamAttitude::Friendly;
}
