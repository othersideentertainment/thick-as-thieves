// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphTypes.h"

// ose
#include "OSEGenericGraph.h"

#include "TATQuestGraph.generated.h"


class UTATQuestGraphRootNode;
class UTATClueSetBase;
struct FTATQuestGraphMapCheckContext;


UCLASS()
class TAT_API UTATQuestGraphBase : public UOSEGenericGraph
{
   GENERATED_BODY()

public:
   UTATQuestGraphBase();

   /// Find the graph's root node to start evaluation with.
   /// A valid graph has exactly one enabled root node (root nodes can be disabled to allow for quick iteration).
   UTATQuestGraphRootNode* SelectRootNode(int32* outNumRootNodesFound = nullptr) const;

   /// Evaluates a quest graph, returning true if a valid quest plan was generated.
   bool EvalQuestGraph(const FTATQuestGraphEvalParams& params, FTATQuestGraphEvalContext& ctx, UTATQuestGraphRootNode* rootNode = nullptr) const;

#if WITH_EDITORONLY_DATA
   // Map used for validating against
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest Graph")
   TSoftObjectPtr<UWorld> Map;
#endif
   
#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
   virtual EDataValidationResult IsNodeDataValid(FDataValidationContext& context, const UTATQuestGraphNode* node) const;

   void ValidateAgainstWorld(FTATQuestGraphMapCheckContext& context) const;
#endif // WITH_EDITOR
};


/// Quest graph module that can be referenced from any other quest graph
UCLASS()
class TAT_API UTATQuestGraphModule : public UTATQuestGraphBase
{
   GENERATED_BODY()

public:
   UTATQuestGraphModule();

#if WITH_EDITORONLY_DATA
   /// The display name of this module - only used in the editor. Defaults to the name of the class if not specified.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest Module")
   FText ModuleDisplayName;
#endif

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR
};


/// Top-level modular quest graph asset.
/// Each map should have exactly one of these.
UCLASS()
class TAT_API UTATQuestGraph : public UTATQuestGraphBase
{
   GENERATED_BODY()

public:
   UTATQuestGraph();

   /// Evaluates a quest graph and returns the result for debugging purposes.
   /// Should probably delete this later, but it's handy for the short term.
   //UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Quest Graph", Meta = (WorldContext = "contextObject"))
   bool DebugEvalQuestGraph(const UObject* contextObject, int32 seed, const FGameplayTagContainer& questTags, const FGameplayTagContainer& worldTags,
      FTATQuestGraphEvalContext& outQuestGraphContext) const;

#if WITH_EDITOR
   virtual void GetAssetRegistryTags(FAssetRegistryTagsContext context) const override;
   static TArray<TSoftObjectPtr<UTATQuestGraph>> FindForMap(const TSoftObjectPtr<UWorld>& map);
   
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR
};
