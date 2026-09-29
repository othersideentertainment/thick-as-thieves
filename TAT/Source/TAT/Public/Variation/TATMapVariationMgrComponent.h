// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Clues/TATClueFact.h"
#include "Clues/TATClueSpawnTypes.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"
#include "Variation/TATSpawnPlan.h"
#include "SceneVariants/TATActiveLayerSet.h"

// ue4
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATMapVariationMgrComponent.generated.h"

struct FTATBriefingClueData;
class UDataLayerAsset;
class UTATSpawnDataAsset;
struct FTATPendingClueSource;
struct FTATQuestActorSpawn;
struct FTATClueFactThunk;
struct FStreamableHandle;

UENUM()
enum class ETATMapVariationLoadingState : uint8
{
   NotStarted,
   WaitingForLoad,
   WaitingForClues,
   CompleteWithVariation,
   CompleteNoVariation,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapVariationMgrStateChanged,const ETATMapVariationLoadingState, CurrentState);
DECLARE_DELEGATE(FOnMapVariationMgrVariantsReplicated)


UCLASS()
class TAT_API UTATMapVariationMgrComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATMapVariationMgrComponent();

   // Whether the map variation has run and probably-replicated
   bool IsMapReady() const;

   void AuthorityChooseSceneVariants();

   UPROPERTY(BlueprintAssignable)
   FOnMapVariationMgrStateChanged OnMapVariationMgrStateChanged;
   ETATMapVariationLoadingState GetCurrentMapVariationLoadingState() const;
   void AuthorityCallOrWaitForComplete(FSimpleDelegate&& delegate);

   bool AreVariantsInitialized() const { return _variantsInitialized; }
   FOnMapVariationMgrVariantsReplicated OnVariantsReplicated;
   const FTATSceneVariantCollection& GetActiveVariants() const
   {
      ensure(AreVariantsInitialized());
      return _activeVariants;
   }

   // Pokes in a clue source to be loaded and used
   // NOTE: pervasive use of this push-style API may make simulation more difficult
   void AuthorityAddPendingClues(const FTATPendingClueSource& clues);


   
   UFUNCTION(BlueprintPure, Category = "Clues")
   int32 GetBriefingClueCount() const;

   UFUNCTION(BlueprintPure, Category = "Clues")
   FText GetBriefingClueAt(int32 index) const;

   void ForEachInitialClueThunk(TFunctionRef<void(const FTATClueFactThunk&)> handler) const;

   void AuthorityExecuteSpawnTiming(ETATSpawnTiming timing);

   const TMap<FWeakObjectPtr, FTATClueFactNamespace>& AuthorityGetClueFactNamespacesByQuestSpawner() const { return _clueFactNamespaceByQuestSpawner; }

protected:
   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

private:
   void _AuthorityBeginVariation();
   void _AuthorityTickWaitForLoad();
   void _AuthorityRunQuestSpawners(TArray<const AActor*>& chosenSpawners);
   void _AddClueRequestsForQuests(const TArray<FTATQuestActorSpawn>& plan);
   void _AuthorityRunSpawners(TConstArrayView<const AActor*> externalSpawners);
   void _AuthorityLoadClues();
   void _AuthoritySpawnClues();
   bool _AreDataLayersLoaded() const;
   bool _AreSublevelsLoaded() const;
   void _AuthorityRequestSublevelLoad();
   bool _AreLevelInstancesLoaded() const;
   void _AuthoritySetLoadingState(ETATMapVariationLoadingState newState);
   int32 _AuthorityGetSeed() const;
   void _AuthorityTickTimeslicedSpawns();
   void _InitActiveLayers();
   void _OnLevelAddedToWorld(ULevel* level, UWorld* world);

   UFUNCTION()
   void _OnRep_VariantIndices();

private:
   UPROPERTY(Transient)
   TObjectPtr<UTATSpawnDataAsset> _authoritySpawnData = nullptr;

   UPROPERTY(Replicated)
   ETATMapVariationLoadingState _authorityLoadingState = ETATMapVariationLoadingState::NotStarted;

   // indirectly replicated
   UPROPERTY(Transient)
   FTATSceneVariantCollection _activeVariants;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_VariantIndices)
   FTATSceneVariantIndices _replicatedVariantIndices;

   UPROPERTY(Transient, Replicated)
   TArray<FTATBriefingClueData> _briefingClues;
   
   TArray<FTATPendingClueSource> _pendingClueSources;
   FGameplayTagContainer _missionChoiceTags;

   UPROPERTY(Transient)
   FTATSpawnPlan _spawnPlan;
   TSharedPtr<FStreamableHandle> _spawnPreloadHandle;
   uint8 _executedSpawnTimingsMask;
   TArray<FTATSpawnCursor> _timeslicedSpawns;

   FTATActiveLayerSet _activeLayers;

   TMap<FWeakObjectPtr, FTATClueFactNamespace> _clueFactNamespaceByQuestSpawner;

   FSimpleMulticastDelegate _onVariationComplete;

   bool _variantsInitialized = false;
};
