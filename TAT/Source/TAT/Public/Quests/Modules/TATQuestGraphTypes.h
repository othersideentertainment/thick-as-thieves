// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Variation/Clues/TATClueSpawnTypes.h"

// ose
#include "OSEGenericGraphNode.h"

// ue
#include "GameplayTagContainer.h"

#include "TATQuestGraphTypes.generated.h"

class UTATQuestGraphNode;
class UTATQuestGraphObjectiveNode;
class UTATQuestGraphModule;
class UTATSceneVariantConfig;
class UTATSceneAsset;

DECLARE_LOG_CATEGORY_EXTERN(LogTATQuestGraph, Log, All);


namespace TATQuestGraphUtil
{
   // Colors for subgraph types (affects subgraph nodes as well as the root nodes inside their respective subgraphs)
   static constexpr FLinearColor kGenericColor = FLinearColor(1.0f, 1.0f, 1.0f);

   static constexpr FLinearColor kCondenseColor = FLinearColor(0.2f, 0.1f, 0.2f);

   // Styles for nodes that select a random choice from a list of choices
   static constexpr EOSEGenericGraphNodeStyle kSelectorStyle = EOSEGenericGraphNodeStyle::BorderLight;
   static constexpr FLinearColor kSelectorColor = FLinearColor(0.2f, 0.9f, 0.9f);

   // Styles for nodes that set a specific value
   static constexpr EOSEGenericGraphNodeStyle kSetterStyle = EOSEGenericGraphNodeStyle::BorderLight;
   static constexpr FLinearColor kSetterColor = FLinearColor(0.85f, 0.85f, 0.25f);

   static constexpr FLinearColor kEdgeConditionColor = FLinearColor(0.75f, 0.25f, 0.1f);
   static constexpr FLinearColor kEdgeConditionMandatoryColor = FLinearColor(0.35f, 0.85f, 0.35f);
}


// Clue sets that are requested to be used for a given quest spawn
// Will be at used at most once per spawn.
USTRUCT()
struct FTATClueSetForSpawn
{
   GENERATED_BODY()

   UPROPERTY()
   FSoftObjectPath ClueSet;

   UPROPERTY()
   FGameplayTag ActorTag;
};

/// The value being assigned when using TATQuestGraphPropertyNode
UENUM(BlueprintType)
enum class ETATQuestGraphPropertyType : uint8
{
   None = 0,
   EndgameDuration,
   MatchMainPhaseDuration,

   MAX UMETA(Hidden)
};


/// Gameplay tag/random weight pair
USTRUCT(BlueprintType)
struct TAT_API FTATQuestRandomWeight
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quests")
   FGameplayTag Tag = FGameplayTag::EmptyTag;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quests")
   float Weight = 1.0f;

   FORCEINLINE bool operator==(const FTATQuestRandomWeight& rhs) const { return Tag == rhs.Tag && Weight == rhs.Weight; }
   FORCEINLINE bool operator!=(const FTATQuestRandomWeight& rhs) const { return !operator==(rhs); }
};


/// Scene variant/random weight pair
USTRUCT(BlueprintType)
struct TAT_API FTATQuestRandomWeight_SceneVariant
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quests")
   TObjectPtr<UTATSceneVariantConfig> SceneVariant;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quests")
   float Weight = 1.0f;

   FORCEINLINE bool operator==(const FTATQuestRandomWeight_SceneVariant& rhs) const { return SceneVariant == rhs.SceneVariant && Weight == rhs.Weight; }
   FORCEINLINE bool operator!=(const FTATQuestRandomWeight_SceneVariant& rhs) const { return !operator==(rhs); }
};


USTRUCT(BlueprintType)
struct FTATQuestGraphLogMessage
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quests")
   TObjectPtr<const UTATQuestGraphNode> SourceNode;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quests")
   bool IsError = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quests")
   FString Message;

   FString ToString() const;
};

// Cache of bitmask of scenes that are reachable from a given node
// Index of bits is dynamically assigned as scenes are requested
struct FTATQuestGraphSceneCache
{
   using MaskType = uint64;

   MaskType GetMaskForScene(const UTATSceneAsset* scene)
   {
      return 1ull << _GetIndexForScene(scene);
   }

   MaskType GetReachableMaskForNode(const UTATQuestGraphNode* node);

private:
   int32 _GetIndexForScene(const UTATSceneAsset* scene)
   {
      int32 index = _sceneIndexes.IndexOfByKey(scene);
      if (index != INDEX_NONE)
      {
         return index;
      }

      return _sceneIndexes.Add(scene);
   }

   // NB: Raw pointers, but never de-referencing
   TArray<const UTATSceneAsset*> _sceneIndexes;
   TMap<const UTATQuestGraphNode*, MaskType> _cache;
};


// A struct that represents the compatibility of a node (and nodes reachable with it)
// with the _currently_ selected scene variants.
//
// Notably:
// 1. It wants to prioritize paths that select variants that have already been forced/chosen
// 2. It wants to reject paths that _always_ select a variant that is incompatible with chosen variants
// 
// NOTE: Since this is evaluated based on the _current_ forced variants, evaluating this is effectively
//       quadratic (or O(M*N) over the full execution of a graph (although there are some optimizations to reduce the constant
//       factor). In testing, it increased the the guildhall evaluation time from ~3us -> ~10us (but this would be
//       super-linear). Enabling logging added another ~4us.
//
//       One could imagine doing a context free version of this in a single pass. That is straightforward-ish for the
//       prioritized variants, but the always-incompatible would be trickier to do. At best you could scenes that are
//       always selected, but that isn't quite the same if there is a path that is always incompatible but for different
//       reasons (although that is less likely to come up).
struct FTATQuestGraphCompatibility
{
   // Mask of forced scene variants that are reachable from a node
   // Index of bit is index of chosen variant in eval context
   uint64 ReachableVariantsMask = 0;
   // True if all paths are incompatible
   bool Incompatible = false;

   static constexpr int32 IncompatiblePriority = -1000;

   int GetPriority() const
   {
      const int32 numVariants = FMath::CountBits(ReachableVariantsMask);
      return numVariants + (Incompatible ? IncompatiblePriority : 0);
   }

   // Combines when both are certain (not alternate paths)
   FTATQuestGraphCompatibility Concat(const FTATQuestGraphCompatibility& other) const {
      return FTATQuestGraphCompatibility{
         .ReachableVariantsMask = ReachableVariantsMask | other.ReachableVariantsMask,
         .Incompatible = Incompatible || other.Incompatible
      };
   }

   struct FPathCombiner
   {
   public:
      void Add(const FTATQuestGraphCompatibility& path);
      FTATQuestGraphCompatibility Build() const;

   private:
      uint64 _variantsMask = 0;
      int32 _currentCount = 0;
      int32 _incompatibleCount = 0;
   };
};

struct TAT_API FTATQuestGraphCompatibilityCache
{
   FTATQuestGraphCompatibility Lookup(const UTATQuestGraphNode* node, const FTATQuestGraphEvalParams& params, const FTATQuestGraphEvalContext& context);
   void Reset();

   using SceneMask = FTATQuestGraphSceneCache::MaskType;
   void SetUsedSceneMask(SceneMask mask) { _usedSceneMask = mask; }
   SceneMask GetMaskForScene(const UTATSceneAsset* scene) { return _sceneCache.GetMaskForScene(scene); }
   SceneMask GetReachableMaskForNode(const UTATQuestGraphNode* node) { return _sceneCache.GetReachableMaskForNode(node); }
private:
   struct FEntry
   {
      FTATQuestGraphCompatibility Compatibility;
      // mask of scenes that are reachable from the node but were not used at the time of evaluation
      uint64 UnaccountedSceneMask = 0;
   };

   TMap<FObjectKey, FEntry> _cache;
   SceneMask _usedSceneMask = 0;
   FTATQuestGraphSceneCache _sceneCache;
};


/// Quest graph evaluation input parameters that are constant during quest graph evaluation
USTRUCT()
struct TAT_API FTATQuestGraphEvalParams
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = "Quests")
   TObjectPtr<UWorld> World;

   UPROPERTY(EditAnywhere, Category = "Quests")
   FGameplayTagContainer WorldTags;
   
   UPROPERTY(EditAnywhere, Category = "Quests")
   int32 MapSeed = 0;

   /// Nodes in this set will always be selected. Intended for editor overrides and cheats.
   UPROPERTY(EditAnywhere, Category = "Quests")
   TSet<FOSEGenericGraphNodeHandle> ForceSelectNodes;
};


// A subset of the graph eval context that something may want to hold onto long-term
// (can change what it has)
//
// NOTE: The need for this struct may go away if we take what we need from it immediately
//       Exposure is limited to make that easy
USTRUCT()
struct TAT_API FTATQuestGraphResult
{
   GENERATED_BODY()

   /// Gameplay tags selected by the quest graph
   UPROPERTY(EditAnywhere, Category = "Quests")
   FGameplayTagContainer QuestTags;

   /// Any scene variants selected by the quest graph
   UPROPERTY(EditAnywhere, Category = "Quests")
   TArray<TObjectPtr<UTATSceneVariantConfig>> SceneVariants;

   /// A handle to the objective node selected by the quest graph (if any)
   UPROPERTY(EditAnywhere, Category = "Quests")
   FOSEGenericGraphNodeHandle ObjectiveNodeHandle;

   /// Properties set by TATQuestGraphPropertyNode.
   /// Not blueprint-exposed because FVariant doesn't have blueprint bindings - not an issue in practice though.
   TMap<ETATQuestGraphPropertyType, FVariant> Properties;

   // Clue format params injected by the system
   // NOTE: We may want to segregate this by module
   FTATClueFormatParams ClueFormatParams;
   
   // Clue sets that are requested
   // For now, assuming that they all have the same context
   TArray<FSoftObjectPath> ClueSets;

   // Clue sets that are requested to be used for a given quest spawn
   // Will be at used at most once per spawn.
   TArray<FTATClueSetForSpawn> SpawnSpecificClueSets;


   const UTATQuestGraphObjectiveNode* TryGetObjectiveNode() const;
};


/// This context is passed to each quest graph node during evaluation.
/// Nodes can read and write all properties.
/// For example, one node could add a quest tag, while a subsequent node does something conditionally if that quest tag exists.
USTRUCT()
struct TAT_API FTATQuestGraphEvalContext
{
   GENERATED_BODY()

   /// The linear sequence of quest graph nodes taken when the quest graph was evaluated
   UPROPERTY(EditAnywhere, Category = "Quests")
   TArray<TObjectPtr<UTATQuestGraphNode>> QuestPlan;

   /// Gameplay tags selected by the quest graph
   UPROPERTY(EditAnywhere, Category = "Quests")
   FGameplayTagContainer QuestTags;

   /// A handle to the objective node selected by the quest graph (if any)
   UPROPERTY(EditAnywhere, Category = "Quests")
   FOSEGenericGraphNodeHandle ObjectiveNodeHandle;

   /// Properties set by TATQuestGraphPropertyNode.
   /// Not blueprint-exposed because FVariant doesn't have blueprint bindings - not an issue in practice though.
   TMap<ETATQuestGraphPropertyType, FVariant> Properties;

   // Clue format params injected by the system
   // NOTE: We may want to segregate this by module
   FTATClueFormatParams ClueFormatParams;

   // Clue sets that are requested
   // For now, assuming that they all have the same context
   TArray<FSoftObjectPath> ClueSets;

   // Clue sets that are requested to be used for a given quest spawn
   // Will be at used at most once per spawn.
   TArray<FTATClueSetForSpawn> SpawnSpecificClueSets;

   /// Optional log messages emitted while evaluating quest graph nodes
   /// CONSIDER: Should this be in params?
   TArray<FTATQuestGraphLogMessage>* EvalLog = nullptr;

   FTATQuestGraphEvalContext() = default;

   FTATQuestGraphResult IntoResult()
   {
      return FTATQuestGraphResult{
         .QuestTags = MoveTemp(QuestTags),
         .SceneVariants = MoveTemp(_sceneVariants),
         .ObjectiveNodeHandle = MoveTemp(ObjectiveNodeHandle),
         .Properties = MoveTemp(Properties),
         .ClueFormatParams = MoveTemp(ClueFormatParams),
         .ClueSets = MoveTemp(ClueSets),
         .SpawnSpecificClueSets = MoveTemp(SpawnSpecificClueSets),
      };
   }

   void Reset()
   {
      QuestPlan.Reset();
      QuestTags.Reset();
      _sceneVariants.Reset();
      _usedScenes.Reset();
      _usedSceneMask = 0;
      _compatibilityCache.Reset();
      ObjectiveNodeHandle.Reset();
      Properties.Reset();
      ClueFormatParams.Reset();
      ClueSets.Reset();
      SpawnSpecificClueSets.Reset();
      // Not sure what the semantics should be?
      if (EvalLog)
      {
         EvalLog->Reset();
      }
   }

   FORCEINLINE const TArray<TObjectPtr<UTATSceneVariantConfig>>& GetSceneVariants() const { return _sceneVariants; }
   FORCEINLINE const TArray<TObjectPtr<const UTATSceneAsset>>& GetUsedScenes() const { return _usedScenes; }
   void AddSceneVariant(TObjectPtr<UTATSceneVariantConfig> variant);
   void AddSceneVariants(TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> variants);

   FTATQuestGraphCompatibility GetCompatibilityForVariant(const UTATSceneVariantConfig* variant) const;
   FTATQuestGraphCompatibility LookupNodeCompatibility(const UTATQuestGraphNode* node, const FTATQuestGraphEvalParams& params);


   void ForEachModule(TFunctionRef<void(UTATQuestGraphModule*)> callback) const;

   template <typename TFunc>
   void LogMessage(const UTATQuestGraphNode* node, TFunc&& msgFunc)
   {
      if (EvalLog)
      {
         EvalLog->Add(FTATQuestGraphLogMessage{
            .SourceNode = node,
            .IsError = false,
            .Message = msgFunc(),
         });
      }
   }

   template <typename TFunc>
   void LogError(const UTATQuestGraphNode* node, TFunc&& msgFunc)
   {
      if (EvalLog)
      {
         EvalLog->Add(FTATQuestGraphLogMessage{
            .SourceNode = node,
            .IsError = true,
            .Message = msgFunc(),
         });
      }
   }

private:
   FTATQuestGraphCompatibilityCache _compatibilityCache;

   /// Any scene variants selected by the quest graph
   UPROPERTY()
   TArray<TObjectPtr<UTATSceneVariantConfig>> _sceneVariants;
   
   UPROPERTY()
   TArray<TObjectPtr<const UTATSceneAsset>> _usedScenes;
   uint64 _usedSceneMask = 0;
};

// Convenience macros so quest graph logging can change how it is routed without affecting callsites (such as directly to logs in some cases)
// (Or compile out)
#define TAT_QUESTGRAPH_LOG_MESSAGE(context, node, pattern, ...) context.LogMessage(node, [&] { return FString::Printf(pattern __VA_OPT__(,) __VA_ARGS__); })
#define TAT_QUESTGRAPH_LOG_ERROR(context, node, pattern, ...) context.LogError(node, [&] { return FString::Printf(pattern __VA_OPT__(,) __VA_ARGS__); })
