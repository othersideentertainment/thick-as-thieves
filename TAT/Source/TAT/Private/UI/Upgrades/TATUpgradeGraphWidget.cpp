// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Upgrades/TATUpgradeGraphWidget.h"

// tat
#include "Upgrades/TATUpgradeGraph.h"
#include "UI/Upgrades/TATUpgradeGraphNodeWidget.h"

// ose
#include "UI/OSERadialPaintLibrary.h"

// ue
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#include "Misc/UObjectToken.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUpgradeGraphWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATUpgradeGraphWidget, Log, All);

void UTATUpgradeGraphWidget::NativeConstruct()
{
   Super::NativeConstruct();

   _nodeWidgetMap.Reset();

   if (UpgradeGraph == nullptr)
   {
      UE_LOG(LogTATUpgradeGraphWidget, Error, TEXT("Skipping upgrade graph widget creation: no upgrade graph assigned"));
      return;
   }

   _nodeWidgetMap.Reserve(UpgradeGraph->AllNodes.Num());

   // Track all existing nodes that were added manually
   ForEachGraphNodeWidget([this](UTATUpgradeGraphNodeWidget* nodeWidget)
   {
      check(nodeWidget != nullptr);
      if (!nodeWidget->UpgradeNode.IsValid())
      {
         return;
      }

      nodeWidget->OnUpgradeNodeWidgetFocused.AddUniqueDynamic(this, &UTATUpgradeGraphWidget::_OnUpgradeNodeWidgetFocused);

      UTATUpgradeGraphNode* node = nodeWidget->UpgradeNode.GetNode<UTATUpgradeGraphNode>();
      if (node != nullptr && !_nodeWidgetMap.Contains(node))
      {
         _nodeWidgetMap.Add(node, nodeWidget);
      }
   });

   // Everything after this point is related to auto-creating node widgets
   if (!AutoCreateWidgetsForMissingNodes)
   {
      return;
   }

   if (UpgradeEntryContainer == nullptr)
   {
      UE_LOG(LogTATUpgradeGraphWidget, Error, TEXT("AutoCreateWidgetsForMissingNodes is enabled, but UpgradeEntryContainer isn't valid - can't auto-create widgets without a valid parent"));
      return;
   }

   if (UpgradeNodeWidgetClass == nullptr)
   {
      UE_LOG(LogTATUpgradeGraphWidget, Error, TEXT("AutoCreateWidgetsForMissingNodes is enabled, but UpgradeNodeWidgetClass isn't valid - can't auto-create widgets without a widget class"));
      return;
   }

   // Automatically create widgets for all missing nodes.
   // I'm not sure how useful this is for shipping, but it's very handy for development and iteration because newly added nodes will appear in the UI automatically.
   UpgradeGraph->TraverseNodes(AutoCreateTraverseOrder, [this](UOSEGenericGraphNode* node) -> bool
   {
      UTATUpgradeGraphNode* upgradeNode = Cast<UTATUpgradeGraphNode>(node);
      if (upgradeNode != nullptr && !_nodeWidgetMap.Contains(upgradeNode) && ShouldAutoCreateWidgetForNode(upgradeNode))
      {
         UTATUpgradeGraphNodeWidget* nodeWidget = CreateWidget<UTATUpgradeGraphNodeWidget>(this, UpgradeNodeWidgetClass);
         check(nodeWidget != nullptr);
         nodeWidget->SetUpgradeNode(upgradeNode->AsHandle());
         UpgradeEntryContainer->AddChild(nodeWidget);
         _nodeWidgetMap.Add(upgradeNode, nodeWidget);
         nodeWidget->OnUpgradeNodeWidgetFocused.AddDynamic(this, &UTATUpgradeGraphWidget::_OnUpgradeNodeWidgetFocused);
         OnAutoCreatedWidgetForUpgradeNode(upgradeNode, nodeWidget);
      }
      return UOSEGenericGraph::TraverseContinue;
   });
}

int32 UTATUpgradeGraphWidget::NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements,
   int32 layerId, const FWidgetStyle& inWidgetStyle, bool parentEnabled) const
{
   if (!DrawArrowsForDependentNodes)
   {
      return layerId;
   }

   FOSERadialPaintContext ctx{ layerId, allottedGeometry, myCullingRect, outDrawElements, inWidgetStyle, parentEnabled };

   const FSlateRect baseRect = FSlateRect(FVector2f::Zero(), allottedGeometry.GetLocalSize());
   const float maxArrowLengthSquared = FVector2f::DistSquared(baseRect.GetTopLeft2f(), baseRect.GetBottomRight2f());

   const FVector2f transformOffset = IsDesignTime() ? FVector2f::Zero() : FVector2f(Inverse(args.GetWindowToDesktopTransform()));
   auto transformRect = [&allottedGeometry, transformOffset](const FSlateRect& rect) -> FSlateRect
   {
      return FSlateRect(
         allottedGeometry.AbsoluteToLocal(rect.GetTopLeft2f() + transformOffset),
         allottedGeometry.AbsoluteToLocal(rect.GetBottomRight2f() + transformOffset)
      );
   };

   auto connectWidgetsWithArrow = [&](UTATUpgradeGraphNodeWidget* srcWidget, UTATUpgradeGraphNodeWidget* destWidget)
   {
      check(srcWidget != nullptr);
      check(destWidget != nullptr);
      const FSlateRect srcRect = transformRect(srcWidget->GetSlateRect());
      const FSlateRect destRect = transformRect(destWidget->GetSlateRect());
      const FVector2f baseLineStart = srcRect.GetCenter2f();
      const FVector2f baseLineEnd = destRect.GetCenter2f();

      if ((!baseRect.ContainsPoint(baseLineStart) && !baseRect.ContainsPoint(baseLineEnd)) || FVector2f::DistSquared(baseLineStart, baseLineEnd) > maxArrowLengthSquared)
      {
         return;
      }

      UOSERadialPaintLibrary::FIntersectionArray2f srcIntersections;
      if (!UOSERadialPaintLibrary::FindLineSegmentRectEdgeIntersections(baseLineStart, baseLineEnd, srcRect.InsetBy(FMargin(ArrowLineOffsetFromNodeWidget)), srcIntersections))
      {
         return;
      }
      check(srcIntersections.Num() > 0);

      UOSERadialPaintLibrary::FIntersectionArray2f destIntersections;
      if (!UOSERadialPaintLibrary::FindLineSegmentRectEdgeIntersections(baseLineStart, baseLineEnd, destRect.InsetBy(FMargin(ArrowLineOffsetFromNodeWidget)), destIntersections))
      {
         return;
      }
      check(destIntersections.Num() > 0);

      ctx.DrawArrow(srcIntersections[0], destIntersections[0], ArrowStyle, baseRect);
   };

#if WITH_EDITOR
   if (IsDesignTime())
   {
      // At design time in the editor, attempt to draw arrows between dependent nodes for preview purposes.
      // This looks very different from the runtime version because it can't assume anything is constant.
      UTATUpgradeGraph* graph = nullptr;
      TMap<UOSEGenericGraphNode*, UTATUpgradeGraphNodeWidget*, TInlineSetAllocator<64>> nodeMap;
      ForEachGraphNodeWidget([&graph, &nodeMap](UTATUpgradeGraphNodeWidget* nodeWidget)
      {
         UTATUpgradeGraph* thisGraph = Cast<UTATUpgradeGraph>(nodeWidget->UpgradeNode.Graph);
         if (thisGraph == nullptr)
         {
            return;
         }

         if (graph == nullptr)
         {
            // UpgradeGraph may not be valid yet - just use whatever graph the nodes are assigned to (we can assume only one graph is being used though)
            graph = thisGraph;
         }
         else if (graph != thisGraph)
         {
            // node belongs to some other graph, ignore it
            return;
         }

         if (UTATUpgradeGraphNode* node = nodeWidget->UpgradeNode.GetNode<UTATUpgradeGraphNode>())
         {
            nodeMap.Add(node, nodeWidget);
         }
      });

      if (graph != nullptr)
      {
         graph->TraverseNodeEdgePairs(EOSEGenericGraphSearchMode::DepthFirstSearch,
            [&nodeMap, &connectWidgetsWithArrow](UOSEGenericGraphNode* node, UOSEGenericGraphEdge* edge) -> bool
         {
            check(node != nullptr);
            if (edge != nullptr && nodeMap.Contains(node))
            {
               UTATUpgradeGraphNode* srcNode = Cast<UTATUpgradeGraphNode>(edge->GetOtherNode(node));
               if (srcNode != nullptr && nodeMap.Contains(srcNode))
               {
                  UTATUpgradeGraphNodeWidget* srcEntryWidget = nodeMap[srcNode];
                  UTATUpgradeGraphNodeWidget* destEntryWidget = nodeMap[node];
                  check(srcEntryWidget != nullptr);
                  check(destEntryWidget != nullptr);
                  connectWidgetsWithArrow(srcEntryWidget, destEntryWidget);
               }
            }
            return UOSEGenericGraph::TraverseContinue;
         });
      }
   }
   else
#endif // WITH_EDITOR
   {
      // Draw arrows between dependent widgets (runtime version)
      for (const auto& widgetPair : _nodeWidgetMap)
      {
         UTATUpgradeGraphNode* node = widgetPair.Key.Get();
         UTATUpgradeGraphNodeWidget* widget = widgetPair.Value.Get();
         if (node == nullptr || widget == nullptr)
         {
            continue;
         }

         // Find all nodes this node is connected to
         for (const auto& edgePair : node->Edges)
         {
            if (edgePair.Key == node)
            {
               continue;
            }

            if (UTATUpgradeGraphNode* otherNode = Cast<UTATUpgradeGraphNode>(edgePair.Value->GetOtherNode(node)))
            {
               const TWeakObjectPtr<UTATUpgradeGraphNodeWidget>* otherWidget = _nodeWidgetMap.Find(otherNode);
               if (otherWidget != nullptr && otherWidget->IsValid())
               {
                  connectWidgetsWithArrow(widget, otherWidget->Get());
               }
            }
         }
      }
   }

   return layerId;
}

bool UTATUpgradeGraphWidget::ShouldAutoCreateWidgetForNode_Implementation(UTATUpgradeGraphNode* node) const
{
   return AutoCreateWidgetsForMissingNodes;
}

void UTATUpgradeGraphWidget::OnAutoCreatedWidgetForUpgradeNode_Implementation(UTATUpgradeGraphNode* node, UTATUpgradeGraphNodeWidget* nodeWidget)
{

}

void UTATUpgradeGraphWidget::SelectFirstUpgradeNodeWidget()
{
   // This isn't ideal because we always walk the whole tree, but WidgetTree->ForEachWidget doesn't give us a way to stop iterating, so it is what it is.
   bool focusedWidget = false;
   ForEachGraphNodeWidget([&focusedWidget](UTATUpgradeGraphNodeWidget* nodeWidget)
   {
      check(nodeWidget != nullptr);
      if (focusedWidget)
      {
         return;
      }
      nodeWidget->SetFocus();
      focusedWidget = true;
   });
}

void UTATUpgradeGraphWidget::RefreshAllUpgradeNodeWidgets()
{
   ForEachGraphNodeWidget([](UTATUpgradeGraphNodeWidget* nodeWidget)
   {
      check(nodeWidget != nullptr);
      nodeWidget->RefreshUpgradeState();
   });
}

UTATUpgradeGraphNodeWidget* UTATUpgradeGraphWidget::GetWidgetForNode(UTATUpgradeGraphNode* node) const
{
   if (node != nullptr)
   {
      if (const TWeakObjectPtr<UTATUpgradeGraphNodeWidget>* weakResult = _nodeWidgetMap.Find(node))
      {
         return weakResult->Get();
      }
   }
   return nullptr;
}

void UTATUpgradeGraphWidget::SetUpgradeNodeWidgetSelected(UTATUpgradeGraphNodeWidget* nodeWidget)
{
   if (nodeWidget == nullptr)
   {
      return;
   }
   _SetSelectedNodeWidget(nodeWidget);
}

void UTATUpgradeGraphWidget::ForEachGraphNodeWidget(TFunctionRef<void(UTATUpgradeGraphNodeWidget*)> callback) const
{
   if (WidgetTree != nullptr)
   {
      WidgetTree->ForEachWidget([callback](UWidget* childWidget)
      {
         if (UTATUpgradeGraphNodeWidget* nodeWidget = Cast<UTATUpgradeGraphNodeWidget>(childWidget))
         {
            callback(nodeWidget);
         }
      });
   }
}

#if WITH_EDITOR
void UTATUpgradeGraphWidget::ValidateCompiledWidgetTree(const UWidgetTree& blueprintWidgetTree, IWidgetCompilerLog& compileLog) const
{
   Super::ValidateCompiledWidgetTree(blueprintWidgetTree, compileLog);

   // Make sure all nodes widgets reference the same graph - either the one specified in the UpgradeGraph field,
   // or the first valid graph we find while checking all of the node widgets.
   UTATUpgradeGraph* selectedGraph = UpgradeGraph;

   blueprintWidgetTree.ForEachWidget([&selectedGraph, &compileLog](UWidget* childWidget)
   {
      UTATUpgradeGraphNodeWidget* nodeWidget = Cast<UTATUpgradeGraphNodeWidget>(childWidget);
      if (nodeWidget == nullptr || nodeWidget->UpgradeNode.Graph == nullptr)
      {
         return;
      }

      UTATUpgradeGraph* upgradeNodeGraph = Cast<UTATUpgradeGraph>(nodeWidget->UpgradeNode.Graph);
      if (upgradeNodeGraph == nullptr)
      {
         TSharedRef<FTokenizedMessage> err = compileLog.Error(FText::Format(INVTEXT("Node {0} UpgradeNode is set to an unsupported graph type '{1}'. Nodes must point to a graph derived from TATUpgradeGraph."),
            FText::FromString(nodeWidget->GetName()), FText::FromString(nodeWidget->UpgradeNode.Graph->GetName())));
         err->SetMessageLink(FUObjectToken::Create(nodeWidget));
      }
      else if (selectedGraph == nullptr)
      {
         // If we haven't found what graph we're using yet, pick this one
         selectedGraph = upgradeNodeGraph;
      }
      else if (upgradeNodeGraph != selectedGraph)
      {
         TSharedRef<FTokenizedMessage> err = compileLog.Error(FText::Format(INVTEXT("Found multiple upgrade graphs referenced. Node '{0}' references graph '{1}', but expected graph '{2}'."),
            FText::FromString(nodeWidget->GetName()), FText::FromString(upgradeNodeGraph->GetName()), FText::FromString(selectedGraph->GetName())));
         err->SetMessageLink(FUObjectToken::Create(nodeWidget));
      }
   });
}
#endif

void UTATUpgradeGraphWidget::_OnUpgradeNodeWidgetFocused(UTATUpgradeGraphNodeWidget* nodeWidget)
{
   _SetSelectedNodeWidget(nodeWidget);
}

void UTATUpgradeGraphWidget::_SetSelectedNodeWidget(UTATUpgradeGraphNodeWidget* nodeWidget)
{
   check(nodeWidget != nullptr);

   // deselect all entries that aren't this one
   for (const auto& item : _nodeWidgetMap)
   {
      UTATUpgradeGraphNodeWidget* widget = item.Value.Get();
      if (widget != nullptr && widget != nodeWidget)
      {
         widget->SetNodeSelected(false);
      }
   }

   nodeWidget->SetNodeSelected(true);
   OnUpgradeNodeWidgetSelected(nodeWidget);
   OnUpgradeNodeSelected.Broadcast(this, nodeWidget);
}
