// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphSubgraphNode.h"

// tat
#include "Quests/Modules/TATQuestGraph.h"
#include "Quests/Modules/TATQuestGraphRootNode.h"
#include "Quests/Modules/TATQuestGraphUtil.h"

// ue
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphSubgraphNode)

namespace TATQuestGraphUtil
{
   FText MakeSubgraphDisplayNameText(UTATQuestGraphModule* subgraph)
   {
      if (subgraph == nullptr)
      {
         return FText::FromString(TEXT("None"));
      }

#if WITH_EDITORONLY_DATA
      if (!subgraph->ModuleDisplayName.IsEmpty())
      {
         return subgraph->ModuleDisplayName;
      }
#endif

      static constexpr const TCHAR* prefixes[] = {
         TEXT("QuestGraph_"), TEXT("QG_"),
         TEXT("QuestModule_"), TEXT("QM_"),
         TEXT("Breadcrumb_"), TEXT("BC_"),
         TEXT("PointOfInterest_"), TEXT("POI_"),
         TEXT("Entry_"), TEXT("ENT_"),
      };

      FString name = subgraph->GetName();
      for (const TCHAR* prefix : prefixes)
      {
         if (name.StartsWith(prefix))
         {
            name.RemoveFromStart(prefix);
         }
      }
      return FText::FromString(name);
   }
}


UTATQuestGraphSubgraphNode::UTATQuestGraphSubgraphNode()
{
#if WITH_EDITORONLY_DATA
   ContextMenuName = FText::FromString(TEXT("Subgraph"));
#endif
}

bool UTATQuestGraphSubgraphNode::ExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   return Super::ExecuteNode(params, ctx);
}

bool UTATQuestGraphSubgraphNode::PostExecuteNode(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx) const
{
   if (!Super::PostExecuteNode(params, ctx))
   {
      return false;
   }

   TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("[SUBGRAPH] Executing subgraph %s"), *GetNameSafe(Module));

   if (Module == nullptr)
   {
      return false;
   }

   if (Module == GetGraph())
   {
      UE_LOG(LogTemp, Error, TEXT("Failed to execute quest subgraph that's identical to the current graph. This would be an infinite loop!"));
      return false;
   }

   //TODO: Detect cycles in the whole stack (eg. GraphA -> GraphB -> GraphA)

   const bool success = Module->EvalQuestGraph(params, ctx);

   TAT_QUESTGRAPH_LOG_MESSAGE(ctx, this, TEXT("[SUBGRAPH] Completed subgraph %s (returned %s)"), *GetNameSafe(Module), (success ? TEXT("true") : TEXT("false")));

   return success;
}

FTATQuestGraphCompatibility UTATQuestGraphSubgraphNode::GetSelfCompatibility(const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& ctx, FTATQuestGraphCompatibilityCache& cache) const
{
   if (Module == nullptr)
   {
      return FTATQuestGraphCompatibility{};
   }

   UTATQuestGraphRootNode* rootNode = Module->SelectRootNode();
   if (rootNode == nullptr)
   {
      return FTATQuestGraphCompatibility{};
   }

   return cache.Lookup(rootNode, params, ctx);
}

FTATQuestGraphSceneCache::MaskType UTATQuestGraphSubgraphNode::GetSceneMask(FTATQuestGraphSceneCache& cache) const
{
   if (Module == nullptr)
   {
      return 0;
   }

   UTATQuestGraphRootNode* rootNode = Module->SelectRootNode();
   if (rootNode == nullptr)
   {
      return 0;
   }

   return cache.GetReachableMaskForNode(rootNode);
}

FString UTATQuestGraphSubgraphNode::GetNodeDebugName() const
{
   if (Module == nullptr)
   {
      return TEXT("SubgraphNode(NULL)");
   }
   return FString::Printf(TEXT("SubgraphNode(%s)"), *GetNameSafe(Module));
}

#if WITH_EDITOR

EDataValidationResult UTATQuestGraphSubgraphNode::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);
   int32 numIssues = 0;
   if (Module == nullptr)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("%s: Invalid quest graph module"), *GetNodeDebugName())));
      ++numIssues;
   }
   return CombineDataValidationResults(baseResult, (numIssues == 0) ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
}

TSharedPtr<SWidget> UTATQuestGraphSubgraphNode::ConstructNodeBodyWidget()
{
   TATQuestGraphUtil::FNodeBodyBuilder builder = TATQuestGraphUtil::FNodeBodyBuilder::Construct();

   builder.AddContentSlot(
      SNew(STextBlock)
      .Text(TATQuestGraphUtil::MakeAttribute(this, +[](UTATQuestGraphSubgraphNode* self) { return TATQuestGraphUtil::MakeSubgraphDisplayNameText(self->Module); })),
      TATQuestGraphUtil::MakeAttributeVisibilityFromBool(this, +[](UTATQuestGraphSubgraphNode* self) { return self->Module != nullptr; }));

   return builder.BodyWidget;
}

FLinearColor UTATQuestGraphSubgraphNode::GetBackgroundColor() const
{
   return TATQuestGraphUtil::kGenericColor;
}

FText UTATQuestGraphSubgraphNode::GetNodeDisplayTitle() const
{
   static const FText moduleText = FText::FromString(TEXT("Module"));
   return moduleText;
}

void UTATQuestGraphSubgraphNode::OnNodeDoubleClicked()
{
   Super::OnNodeDoubleClicked();
   check(GEditor != nullptr);

   // Open the data class asset editor for the current upgrade type when the node is double-clicked
   if (Module != nullptr)
   {
      if (UAssetEditorSubsystem* assetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
      {
         assetEditorSubsystem->OpenEditorForAsset(Module);
      }
   }
}

void UTATQuestGraphSubgraphNode::ValidateAgainstWorld(FTATQuestGraphMapCheckContext& context) const
{
   Super::ValidateAgainstWorld(context);

   // recurse here, since may be running from map-check, where it only runs the outer ones
   if(Module)
   {
      Module->ValidateAgainstWorld(context);
   }
}

#endif // WITH_EDITOR
