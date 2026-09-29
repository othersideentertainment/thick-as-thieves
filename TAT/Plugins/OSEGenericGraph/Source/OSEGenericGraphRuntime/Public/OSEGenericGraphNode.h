// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "OSEGenericGraphNodeHandle.h"
#include "Templates/SubclassOf.h"
#include "OSEGenericGraphNode.generated.h"

class UOSEGenericGraph;
class UOSEGenericGraphEdge;

UENUM(BlueprintType)
enum class EOSEGenericGraphNodeLimit : uint8
{
   Unlimited,
   Limited
};

UENUM(BlueprintType)
enum class EOSEGenericGraphNodeStyle : uint8
{
   BorderDark,   // BehaviorTree, BTEditor.Graph.BTNode
   BorderLight,  // StateMachine, Graph.StateNode
   Flat,         // Default, Graph.Node
   Glossy,       // Variable, Graph.VarNode
};

struct OSEGENERICGRAPHRUNTIME_API FOSEGenericGraphNodeCustomAction
{
   FText Category;
   FText Label;
   FText TooltipText;
   TFunction<UOSEGenericGraphNode*(UObject*)> CreateNode;

   FORCEINLINE UOSEGenericGraphNode* Create(UObject* Outer) const { return CreateNode ? CreateNode(Outer) : nullptr; }

   FORCEINLINE bool operator==(const FOSEGenericGraphNodeCustomAction& rhs) const { return Category.EqualTo(rhs.Category) && Label.EqualTo(rhs.Label); }
   FORCEINLINE bool operator!=(const FOSEGenericGraphNodeCustomAction& rhs) const { return !operator==(rhs); }

#if WITH_EDITOR
   /// Helper for implementing UOSEGenericGraphNode::GetCustomGraphContextMenuActions by generating one action per enum value
   template<typename NodeType, typename EnumType>
   static void MakeFromEnumValues(
      TArray<FOSEGenericGraphNodeCustomAction>& OutActions,
      const TFunction<void(NodeType*, EnumType)>& NodeSetupCallback,
      const FText& ActionCategory = FText::GetEmpty(),
      const FText& LabelFormat = FText::GetEmpty(),
      const FText& TooltipFormat = FText::GetEmpty());
#endif
};

inline uint32 GetTypeHash(const FOSEGenericGraphNodeCustomAction& CustomAction)
{
   return HashCombine(GetTypeHash(CustomAction.Category.BuildSourceString()), GetTypeHash(CustomAction.Label.BuildSourceString()));
}

UCLASS(Blueprintable)
class OSEGENERICGRAPHRUNTIME_API UOSEGenericGraphNode : public UObject
{
   GENERATED_BODY()

public:
   UOSEGenericGraphNode();

   UPROPERTY(VisibleDefaultsOnly, Category = "Generic Graph Node")
   TObjectPtr<UOSEGenericGraph> Graph;

   UPROPERTY(BlueprintReadOnly, Category = "Generic Graph Node")
   TArray<UOSEGenericGraphNode*> ParentNodes;

   UPROPERTY(BlueprintReadOnly, Category = "Generic Graph Node")
   TArray<UOSEGenericGraphNode*> ChildrenNodes;

   UPROPERTY(BlueprintReadOnly, Category = "Generic Graph Node")
   TMap<UOSEGenericGraphNode*, UOSEGenericGraphEdge*> Edges;

   UPROPERTY(BlueprintReadOnly, DuplicateTransient, Category = "Generic Graph Node")
   FGuid NodeId;

   UFUNCTION(BlueprintCallable, Category = "Generic Graph Node")
   virtual UOSEGenericGraphEdge* GetEdge(UOSEGenericGraphNode* ChildNode) const;

   UFUNCTION(BlueprintCallable, Category = "Generic Graph Node")
   bool IsLeafNode() const;

   UFUNCTION(BlueprintCallable, Category = "Generic Graph Node")
   UOSEGenericGraph* GetGraph() const;

   /// Gets the debug name of this node. Only useful in editor builds.
   UFUNCTION(BlueprintPure, Category = "Generic Graph Node")
   virtual FString GetNodeDebugName() const;

   /// Returns a handle that can represent this node.
   /// (No need to expose this to blueprints because there's an autocast function for that - UOSEGenericGraphNodeHandleFunctionLibrary::MakeGenericGraphNodeHandle)
   FOSEGenericGraphNodeHandle AsHandle() const { return FOSEGenericGraphNodeHandle{ Graph, NodeId }; }

   //////////////////////////////////////////////////////////////////////////
#if WITH_EDITORONLY_DATA
   UPROPERTY(VisibleDefaultsOnly, Category = "Generic Graph Node|Editor")
   TSubclassOf<UOSEGenericGraph> CompatibleGraphType;

   UPROPERTY(EditDefaultsOnly, Category = "Generic Graph Node|Editor")
   FText ContextMenuName;

   UPROPERTY(EditDefaultsOnly, Category = "Generic Graph Node|Editor")
   EOSEGenericGraphNodeLimit ParentLimitType = EOSEGenericGraphNodeLimit::Unlimited;

   UPROPERTY(EditDefaultsOnly, Category = "Generic Graph Node|Editor", meta = (ClampMin = "0", EditCondition = "ParentLimitType == EOSEGenericGraphNodeLimit::Limited", EditConditionHides))
   int32 ParentLimit = 0;

   UPROPERTY(EditDefaultsOnly, Category = "Generic Graph Node|Editor")
   EOSEGenericGraphNodeLimit ChildrenLimitType = EOSEGenericGraphNodeLimit::Unlimited;

   UPROPERTY(EditDefaultsOnly, Category = "Generic Graph Node|Editor", meta = (ClampMin = "0", EditCondition = "ChildrenLimitType == EOSEGenericGraphNodeLimit::Limited", EditConditionHides))
   int32 ChildrenLimit = 0;

#endif

#if WITH_EDITOR
   // These functions allow customizing the behavior and appearance of the node in the editor.

   /// Subclasses can override this to provide extra actions on the graph's context menu for adding graph nodes.
   /// This can be used to add several variations on one node as separate node "types" (eg. an "ability" node could have one menu entry per ability).
   virtual void GetCustomGraphContextMenuActions(TArray<FOSEGenericGraphNodeCustomAction>& OutActions) const {}

   /// Gets the node's visual style
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const { return EOSEGenericGraphNodeStyle::BorderDark; }

   /// Gets the node's background color
   virtual FLinearColor GetBackgroundColor() const;

   /// Gets the node's icon
   virtual const FSlateBrush* GetNodeIcon() const;

   /// Gets the node's tooltip text
   virtual FText GetNodeTooltipText() const { return FText::GetEmpty(); }

   /// Gets the primary title text displayed at the top of the node
   virtual FText GetNodeDisplayTitle() const { return FText::FromString(TEXT("Generic")); }

   /// Gets the secondary title text displayed under the title at the top of the node
   virtual FText GetNodeDisplaySubtitle() const { return FText::GetEmpty(); }

   /// Gets the title of this node as it is displayed in menus (eg. in the editor customization for FOSEGenericGraphNodeHandle when selecting a node)
   virtual FText GetNodeListViewTitle() const { return GetNodeDisplayTitle(); }

   /// Returns true if the node should be editable by pressing F2 or clicking the node title (also requires that the graph has bCanRenameNode=true)
   virtual bool IsTitleEditable() const { return false; }

   /// If the node is editable, return the string that will initially populate the text box when the user edits the title.
   virtual FText GetEditableTitle() const { return FText::GetEmpty(); }

   /// When the user finishes editing the title, this will be called to assign the value to the node.
   virtual void SetEditableTitle(const FText& NewTitle) {}

   /// Perform a custom action when double-clicking a node (eg. opening an asset editor)
   virtual void OnNodeDoubleClicked() {}

   /// Checks if the node indicator should be visible (a small text block on the top-left of the node)
   virtual bool GetNodeIndicatorVisible() const { return false; }

   /// Gets the size of the node indicator (if visible)
   virtual FVector2D GetNodeIndicatorSize() const { return FVector2D(40, 40); }

   /// Gets the text that appears in the node indicator (if visible)
   virtual FText GetNodeIndicatorText() const { return FText::GetEmpty(); }

   /// Gets the background color of the node indicator (if visible)
   virtual FLinearColor GetNodeIndicatorBackgroundColor() const { return FLinearColor::White; }

   /// Allow creating a custom slate widget for this node.
   virtual TSharedPtr<SWidget> ConstructNodeBodyWidget() { return nullptr; }

   /// Can override this to add custom logic to determine if this node can create a connection with another node.
   virtual bool CanCreateConnection(UOSEGenericGraphNode* Other, FText& ErrorMessage);

   /// Can override this to add custom logic to determine if this node can create a connection TO another node (with this node as the parent)
   virtual bool CanCreateConnectionTo(UOSEGenericGraphNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage);

   /// Can override this to add custom logic to determine if this node can create a connection FROM another node (with this node as the child)
   virtual bool CanCreateConnectionFrom(UOSEGenericGraphNode* Other, int32 NumberOfParentNodes, FText& ErrorMessage);
#endif
};

#if WITH_EDITOR
template<typename NodeType, typename EnumType>
void FOSEGenericGraphNodeCustomAction::MakeFromEnumValues(
   TArray<FOSEGenericGraphNodeCustomAction>& OutActions,
   const TFunction<void(NodeType*, EnumType)>& NodeSetupCallback,
   const FText& ActionCategory,
   const FText& LabelFormat,
   const FText& TooltipFormat)
{
   UClass* NodeClass = NodeType::StaticClass();
   check(NodeClass != nullptr);
   const FText NodeTypeDisplayName = NodeClass->GetDisplayNameText();

   const UEnum* EnumMetadata = StaticEnum<EnumType>();
   check(EnumMetadata != nullptr);
   for (int32 i = 0; i < EnumMetadata->NumEnums(); i++)
   {
      const int64 ValueAsInt = EnumMetadata->GetValueByIndex(i);
      const int64 MaxEnumValue = EnumMetadata->GetMaxEnumValue();
      if ((MaxEnumValue != 0 && MaxEnumValue == ValueAsInt) || EnumMetadata->HasMetaData(TEXT("Hidden"), i))
      {
         continue;
      }

      FOSEGenericGraphNodeCustomAction& NewAction = OutActions.Emplace_GetRef();
      NewAction.Category = !ActionCategory.IsEmpty() ? ActionCategory : NodeTypeDisplayName;

      const FText ValueName = EnumMetadata->GetDisplayNameTextByIndex(i);
      NewAction.Label = !LabelFormat.IsEmpty() ? FText::FormatOrdered(LabelFormat, ValueName) : ValueName;

      const FText BaseTooltip = !TooltipFormat.IsEmpty()
         ? FText::FormatOrdered(TooltipFormat, ValueName)
         : FText::FormatOrdered(FText::FromString(TEXT("Create a {0} node set to {1}")), NodeTypeDisplayName, ValueName);

      const FText EnumValueTooltip = EnumMetadata->GetToolTipTextByIndex(i);
      NewAction.TooltipText = !EnumValueTooltip.IsEmpty()
         ? FText::FormatOrdered(FText::FromString(TEXT("{0}\n\n[{1}]: {2}")), BaseTooltip, ValueName, EnumValueTooltip)
         : BaseTooltip;

      const EnumType ValueAsEnum = static_cast<EnumType>(ValueAsInt);
      NewAction.CreateNode = [ValueAsEnum, NodeSetupCallback](UObject* outer) -> UOSEGenericGraphNode*
      {
         NodeType* NewNode = NewObject<NodeType>(outer);
         check(NewNode != nullptr);
         NodeSetupCallback(NewNode, ValueAsEnum);
         return NewNode;
      };
   }
}
#endif // WITH_EDITOR
