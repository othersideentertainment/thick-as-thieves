// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATPuddleTypes.h"
#include "Developer/TATDevToolTypes.h"

// ue
#include "Subsystems/WorldSubsystem.h"

#include "TATPuddleSubsystem.generated.h"

class ATATPuddleCluster;

///
UCLASS()
class TAT_API UTATPuddleSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()

public:
   // From USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   // From UWorldSubsystem
   virtual void OnWorldBeginPlay(UWorld& InWorld) override;
   // From UTickableWorldSubsystem
   virtual bool IsTickable() const override;

   // From UObject
   virtual void Tick(float deltaTime) override;
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTATPuddleSubsystem, STATGROUP_Tickables); }

   /// Spawns a puddle, either by adding it to a nearby existing puddle cluster, or spawning a new puddle cluster actor.
   /// If deduplicateDistance is greater than zero, it will attempt to reset the health on an existing nearby puddle rather than spawn a new one.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Puddle Subsystem")
   bool AuthoritySpawnPuddle(ATATPuddleCluster*& puddleClusterActor, int32& puddleId, bool& refreshedExistingPuddle, TSubclassOf<ATATPuddleCluster> puddleClass,
      const FTATPuddleTransform& puddleTransform, APawn* puddleInstigator, float deduplicateDistance = 0.0f);

   /// Spawns a number of puddles, either by adding them to a nearby existing puddle cluster, or spawning a new puddle cluster actor.
   /// If deduplicateDistance is greater than zero, it will attempt to reset the health on an existing nearby puddle rather than spawn a new one.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Puddle Subsystem")
   bool AuthoritySpawnMultiplePuddles(ATATPuddleCluster*& puddleClusterActor, TArray<int32>& puddleIds, TSubclassOf<ATATPuddleCluster> puddleClass,
      const TArray<FTATPuddleTransform>& puddleTransforms, APawn* puddleInstigator, float deduplicateDistance = 0.0f);

   void RegisterPuddleCluster(ATATPuddleCluster* actor);
   void UnregisterPuddleCluster(ATATPuddleCluster* actor);

protected:
   UPROPERTY(Transient)
   TSet<TObjectPtr<ATATPuddleCluster>> _puddleClusterActors;

private:
   ATATPuddleCluster* _SpawnPuddleClusterActor(const TSubclassOf<ATATPuddleCluster>& puddleClass, TConstArrayView<FTATPuddleTransform> puddleTransforms,
      APawn* puddleInstigator, TFunctionRef<void(int32)> puddleAddedCallback) const;

   ATATPuddleCluster* _FindClusterCandidate(const FSphere& boundingSphere, const TSubclassOf<ATATPuddleCluster>& puddleClass, APawn* puddleInstigator) const;

#if TAT_ENABLE_DEV_TOOLS
   struct FDevToolUIState
   {
      struct FPuddleList
      {
         FString Label;
         TSubclassOf<ATATPuddleCluster> Class;
         TArray<ATATPuddleCluster*, TInlineAllocator<8>> Puddles;
      };
      TArray<FPuddleList, TInlineAllocator<8>> Groups;
      TMap<TSubclassOf<ATATPuddleCluster>, int32> _classToGroupMap;

      void Add(ATATPuddleCluster* actor);
      void Reset() { Groups.Reset(); _classToGroupMap.Reset(); }

      void Sort()
      {
         Groups.Sort([](const FPuddleList& a, const FPuddleList& b) { return a.Label < b.Label; });
         for (FPuddleList& pair : Groups)
         {
            pair.Puddles.Sort();
         }
      }
   };
   FDevToolUIState _puddleUIState{};

   void _DrawPuddleSubsystemDevTool(FTATDevToolState& state);
#endif // TAT_ENABLE_DEV_TOOLS
};
