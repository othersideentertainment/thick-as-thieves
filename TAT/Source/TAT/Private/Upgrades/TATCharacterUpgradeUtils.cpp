// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Upgrades/TATCharacterUpgradeUtils.h"

// tat
#include "Character/TATCharacterMetadata.h"
#include "Upgrades/TATUpgradeGraph.h"
#include "Upgrades/TATUpgradeCurrency.h"
#include "SaveGame/TATSaveGame.h"
#include "SaveGame/TATCharacterDataContext.h"
#include "Progression/TATProgressionSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterUpgradeUtils)

static constexpr int32 kFallbackCharacterUpgradeLevelValue = 0;

// static
int32 UTATCharacterUpgradeUtils::GetCharacterUpgradeLevelForTag(const FTATCharacterDataContext& context, FGameplayTag upgradeTag)
{
   if (!context.IsValid())
   {
      return kFallbackCharacterUpgradeLevelValue;
   }
   return context.SaveGame->GetCharacterProgression(context.SaveId).UnlockedUpgrades.GetValue(upgradeTag, kFallbackCharacterUpgradeLevelValue);
}

// static
int32 UTATCharacterUpgradeUtils::GetCharacterUpgradeLevelForNode(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle)
{
   if (!context.IsValid())
   {
      return kFallbackCharacterUpgradeLevelValue;
   }
   UTATUpgradeGraphNode* node = upgradeNodeHandle.GetNode<UTATUpgradeGraphNode>();
   if (node == nullptr)
   {
      return kFallbackCharacterUpgradeLevelValue;
   }
   return context.SaveGame->GetCharacterProgression(context.SaveId).UnlockedUpgrades.GetValue(node->GetUpgradeTag(), kFallbackCharacterUpgradeLevelValue);
}

// static
bool UTATCharacterUpgradeUtils::IsCharacterUpgradeNodeUnlocked(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle)
{
   UTATUpgradeGraphNode* upgradeNode = upgradeNodeHandle.GetNode<UTATUpgradeGraphNode>();
   if (!upgradeNode)
   {
      return false;
   }
   return GetCharacterUpgradeLevelForTag(context, upgradeNode->GetUpgradeTag()) >= upgradeNode->GetUpgradeLevel();
}

// static
int32 UTATCharacterUpgradeUtils::GetNumUnlockedUpgradesWithProgressionType(const FTATCharacterDataContext& context, UTATUpgradeGraph* upgradeGraph, ETATUpgradeProgressionType progressionType)
{
   int32 result = 0;
   if (upgradeGraph == nullptr)
   {
      return result;
   }
   for (UOSEGenericGraphNode* node : upgradeGraph->AllNodes)
   {
      UTATUpgradeGraphNode* upgradeNode = Cast<UTATUpgradeGraphNode>(node);
      if (upgradeNode != nullptr
         && upgradeNode->GetUpgradeProgressionType() == progressionType
         && GetCharacterUpgradeLevelForTag(context, upgradeNode->GetUpgradeTag()) >= upgradeNode->GetUpgradeLevel())
      {
         ++result;
      }
   }
   return result;
}

// static
ETATUpgradePurchaseState UTATCharacterUpgradeUtils::GetCharacterUpgradePurchaseState(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeHandle)
{
   if (!context.IsValid())
   {
      return ETATUpgradePurchaseState::Unknown;
   }

   UTATUpgradeGraphNode* upgradeNode = upgradeHandle.GetNode<UTATUpgradeGraphNode>();
   if (upgradeNode == nullptr)
   {
      return ETATUpgradePurchaseState::Unknown;
   }

   const FTATCharacterProgression& characterData = context.GetCharacterDataChecked();

   // check if the upgrade is already unlocked
   if (characterData.UnlockedUpgrades.GetValue(upgradeNode->GetUpgradeTag()) >= upgradeNode->GetUpgradeLevel())
   {
      return ETATUpgradePurchaseState::Unlocked;
   }

   // traverse the graph backwards from this node to see if they have all prereqs unlocked
   UTATUpgradeGraph* graph = Cast<UTATUpgradeGraph>(upgradeNode->GetGraph());
   check(graph != nullptr);
   bool allPrereqsUnlocked = true;
   graph->TraverseNodesBackward(EOSEGenericGraphSearchMode::BreadthFirstSearch, upgradeNode,
      [&characterData, &allPrereqsUnlocked](UOSEGenericGraphNode* node) -> bool
      {
         if (UTATUpgradeGraphNode* parentNode = Cast<UTATUpgradeGraphNode>(node))
         {
            if (characterData.UnlockedUpgrades.GetValue(parentNode->GetUpgradeTag()) < parentNode->GetUpgradeLevel())
            {
               allPrereqsUnlocked = false;
               return UOSEGenericGraph::TraverseBreak;
            }
         }
         return UOSEGenericGraph::TraverseContinue;
      });
   if (!allPrereqsUnlocked)
   {
      return ETATUpgradePurchaseState::Locked;
   }

   // make sure they have all required costs
   if (upgradeNode->MoneyCost > 0 && context.SaveGame->GetMoney() < upgradeNode->MoneyCost)
   {
      return ETATUpgradePurchaseState::Available;
   }
   for (const FTATUpgradeCurrencyCost& cost : upgradeNode->Cost)
   {
      if (context.SaveGame->GetUpgradeCurrency(context.SaveId, cost.CurrencyTag) < cost.Amount)
      {
         return ETATUpgradePurchaseState::Available;
      }
   }

   return ETATUpgradePurchaseState::Purchaseable;
}

// static
bool UTATCharacterUpgradeUtils::RequestUnlockCharacterUpgrade(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle, bool autoEquip)
{
   if (!context.IsValid())
   {
      return false;
   }

   if (GetCharacterUpgradePurchaseState(context, upgradeNodeHandle) != ETATUpgradePurchaseState::Purchaseable)
   {
      return false;
   }

   UTATUpgradeGraphNode* upgradeNode = upgradeNodeHandle.GetNode<UTATUpgradeGraphNode>();

   // The call above to GetUpgradePurchaseState could not have returned Purchasable if the node was null
   check(upgradeNode != nullptr);

   const FGameplayTag upgradeTag = upgradeNode->GetUpgradeTag();
   if (!ensure(upgradeTag.IsValid()))
   {
      return false;
   }

   const int32 upgradeLevel = upgradeNode->GetUpgradeLevel();
   if (!ensure(upgradeLevel > 0))
   {
      return false;
   }

   // Verify we don't already have this upgrade before subtracting costs
   if (context.SaveGame->GetCharacterProgression(context.SaveId).UnlockedUpgrades.GetValue(upgradeTag) >= upgradeLevel)
   {
      return false;
   }

   // subtract required costs
   if (upgradeNode->MoneyCost > 0)
   {
      context.SaveGame->UpdateMoney(-upgradeNode->MoneyCost);
   }
   for (const FTATUpgradeCurrencyCost& cost : upgradeNode->Cost)
   {
      if (cost.Amount > 0)
      {
         context.SaveGame->RemoveUpgradeCurrency(context.SaveId, cost.CurrencyTag, cost.Amount);
      }
   }

   // do the actual upgrade
   context.SaveGame->SetUpgradeLevel(context.SaveId, upgradeTag, upgradeLevel, autoEquip);
   return true;
}

// static
bool UTATCharacterUpgradeUtils::RefundUpgradeNode(const FTATCharacterDataContext& context, UTATUpgradeGraphNode* upgradeNode)
{
   if (!context.IsValid() || upgradeNode == nullptr)
   {
      return false;
   }
   bool didRefund = false;
   if (upgradeNode->MoneyCost > 0)
   {
      context.SaveGame->UpdateMoney(upgradeNode->MoneyCost);
      didRefund = true;
   }
   for (const FTATUpgradeCurrencyCost& cost : upgradeNode->Cost)
   {
      if (cost.Amount > 0)
      {
         context.SaveGame->AddUpgradeCurrency(context.SaveId, cost.CurrencyTag, cost.Amount);
         didRefund = true;
      }
   }
   return didRefund;
}

// static
void UTATCharacterUpgradeUtils::ResetCharacterUpgradeProgressForNode(const FTATCharacterDataContext& context, const FOSEGenericGraphNodeHandle& upgradeNodeHandle)
{
   if (!context.IsValid())
   {
      return;
   }
   if (UTATUpgradeGraphNode* upgradeNode = upgradeNodeHandle.GetNode<UTATUpgradeGraphNode>())
   {
      context.SaveGame->ResetUpgradeProgressForTag(context.SaveId, upgradeNode->GetUpgradeTag());
   }
}

// static
void UTATCharacterUpgradeUtils::ResetAllCharacterUpgradeProgress(const FTATCharacterDataContext& context, UTATUpgradeGraph* upgradeGraph, bool refundUpgradeCosts)
{
   if (!context.IsValid() || upgradeGraph == nullptr)
   {
      return;
   }

   if (refundUpgradeCosts)
   {
      // The upgrade map does not contain a list of all nodes the player has unlocked, it only stores the current level of each upgrade.
      // To refund all nodes, we'll find each current-level node they have unlocked, then refund that node and all prereq nodes for it.
      const FTATCharacterProgression& characterData = context.GetCharacterDataChecked();
      TSet<UTATUpgradeGraphNode*, DefaultKeyFuncs<UTATUpgradeGraphNode*>, TInlineSetAllocator<32>> refundedNodes;
      for (const auto& pair : characterData.UnlockedUpgrades.Values)
      {
         UTATUpgradeGraphNode* baseUpgradeNode = upgradeGraph->FindUpgradeNode(pair.Key, pair.Value);
         if (baseUpgradeNode == nullptr)
         {
            continue;
         }

         // Need to visit baseUpgradeNode so that gets refunded in addition to all its prereqs
         constexpr bool visitBaseUpgradeNode = true;

         upgradeGraph->TraverseNodesBackward(EOSEGenericGraphSearchMode::BreadthFirstSearch, baseUpgradeNode,
            [&](UOSEGenericGraphNode* node) -> bool
            {
               UTATUpgradeGraphNode* upgradeNode = Cast<UTATUpgradeGraphNode>(node);
               if (upgradeNode != nullptr && !refundedNodes.Contains(upgradeNode))
               {
                  // We'll only add nodes to the refundedNodes map if they had anything refunded.
                  // This reduces the chances we'll end up needing to do a dynamic allocation (if refundedNodes ends up larger than its inline allocator
                  // capacity), at the cost of calling refundNode a few more times than needed. That said, the repeated calls to refundNode should be very
                  // cheap because they're only on nodes with no costs.
                  if (RefundUpgradeNode(context, upgradeNode))
                  {
                     refundedNodes.Add(upgradeNode);
                  }
               }
               return UOSEGenericGraph::TraverseContinue;
            },
            visitBaseUpgradeNode);
      }
   }

   // Remove all upgrade tags
   context.SaveGame->ResetUpgradeProgressForCharacter(context.SaveId);
}

// static
TSoftObjectPtr<UTATUpgradeGraph> UTATCharacterUpgradeUtils::GetCharacterUpgradeGraph(const FTATCharacterDataContext& context)
{
   if (!context.IsValid())
   {
      return {};
   }
   UTATCharactersMetadata* charactersMetadata = UTATCharacterMetadataFunctionLibrary::GetCharactersMetadataAsset();
   check(charactersMetadata != nullptr);
   const FTATCharacterMetadata* charMeta = charactersMetadata->Characters.Find(context.GetCharacterDataChecked().Character);
   if (charMeta == nullptr)
   {
      return {};
   }
   return charMeta->UpgradeGraph;
}
