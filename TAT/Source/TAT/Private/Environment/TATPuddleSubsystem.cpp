// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATPuddleSubsystem.h"

// tat
#include "Developer/TATDevToolSubsystem.h"
#include "Developer/TATImGuiHelpers.h"
#include "Environment/TATPuddleCluster.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPuddleSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATPuddleSubsystem, Warning, All);
DECLARE_STATS_GROUP(TEXT("Puddle Subsystem"),STATGROUP_PuddleSubsystem, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Puddle subsystem tick"), STAT_PuddleSubsystem_Tick, STATGROUP_PuddleSubsystem)

namespace PuddleHelpers
{
   FSphere CombinePuddleBoundingSpheres(TConstArrayView<FTATPuddleTransform> puddles)
   {
      check(!puddles.IsEmpty())

      // First compute the centroid of all puddle origins
      FVector boundingSphereOrigin = FVector::ZeroVector;
      for (const FTATPuddleTransform& puddle : puddles)
      {
         boundingSphereOrigin += puddle.Location;
      }
      boundingSphereOrigin /= static_cast<float>(puddles.Num());

      // Next, find the max squared distance from the centroid to a puddle
      float boundingSphereRadiusSquared = 0.0f;
      for (const FTATPuddleTransform& puddle : puddles)
      {
         // Instead of the distance to the center of the puddle, we want the distance to the farthest point on the puddle's bounding sphere from boundingSphereOrigin
         const FSphere puddleBoundingSphere = puddle.GetBoundingSphere();
         const FVector locationAtOppositeEndOfPuddle = puddleBoundingSphere.Center + ((puddleBoundingSphere.Center - boundingSphereOrigin).GetSafeNormal() * puddleBoundingSphere.W);

         const float distSq = FVector::DistSquared(locationAtOppositeEndOfPuddle, boundingSphereOrigin);
         if (distSq >= boundingSphereRadiusSquared)
         {
            boundingSphereRadiusSquared = distSq;
         }
      }

      return FSphere(boundingSphereOrigin, FMath::Sqrt(boundingSphereRadiusSquared));
   }
}

void UTATPuddleSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
}

void UTATPuddleSubsystem::Deinitialize()
{
   Super::Deinitialize();
}

bool UTATPuddleSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer) || outer == nullptr || outer->GetWorld() == nullptr)
   {
      return false;
   }

   // Game world only
   const UWorld* world = outer->GetWorld();
   if (!world->IsGameWorld())
   {
      return false;
   }

   return true;
}

void UTATPuddleSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

#if TAT_ENABLE_DEV_TOOLS
   if (UTATDevToolSubsystem* devToolSubsystem = GetWorld()->GetSubsystem<UTATDevToolSubsystem>())
   {
      devToolSubsystem->RegisterDevToolFunction(TEXT("Puddle Subsystem"), [this](const FTATDevToolContext& ctx, FTATDevToolState& state)
      {
         if (TATImGui::BeginDevToolWindow(state))
         {
            _DrawPuddleSubsystemDevTool(state);
            TATImGui::EndDevToolWindow();
         }
      }, this);
   }
#endif // TAT_ENABLE_DEV_TOOLS
}

bool UTATPuddleSubsystem::IsTickable() const
{
   return false;
}

void UTATPuddleSubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);
   SCOPE_CYCLE_COUNTER(STAT_PuddleSubsystem_Tick);
}

bool UTATPuddleSubsystem::AuthoritySpawnPuddle(ATATPuddleCluster*& puddleClusterActor, int32& puddleId, bool& refreshedExistingPuddle, TSubclassOf<ATATPuddleCluster> puddleClass,
   const FTATPuddleTransform& puddleTransform, APawn* puddleInstigator, float deduplicateDistance)
{
   puddleClusterActor = nullptr;
   puddleId = INDEX_NONE;
   refreshedExistingPuddle = false;

   UWorld* world = GetWorld();
   check(world != nullptr);
   check(world->GetAuthGameMode() != nullptr);

   FSphere puddleBoundingSphere = puddleTransform.GetBoundingSphere();
   puddleBoundingSphere.W = (puddleBoundingSphere.W * 1.2f) + 300.0f; // Add some padding

   // Do we have an existing cluster that makes sense to add the new puddle to?
   if (ATATPuddleCluster* bestCandidate = _FindClusterCandidate(puddleBoundingSphere, puddleClass, puddleInstigator))
   {
      if (deduplicateDistance > 0)
      {
         bool isNew = false;
         puddleId = bestCandidate->AuthorityAddOrRefreshPuddle(isNew, puddleTransform, deduplicateDistance);
         refreshedExistingPuddle = !isNew && puddleId != INDEX_NONE;
      }
      else
      {
         puddleId = bestCandidate->AuthorityAddPuddle(puddleTransform);
      }

      if (ensure(puddleId != INDEX_NONE))
      {
         puddleClusterActor = bestCandidate;
         return true;
      }
   }

   // No existing cluster actor available, spawn a new one.
   UE_LOG(LogTATPuddleSubsystem, Verbose, TEXT("Spawning new puddle cluster (none of the existing %i cluster actors were close enough)"), _puddleClusterActors.Num());
   const TArray<FTATPuddleTransform, TInlineAllocator<1>> puddleTransforms = { puddleTransform };
   ATATPuddleCluster* newCluster = _SpawnPuddleClusterActor(puddleClass, puddleTransforms, puddleInstigator,
      [&](int32 newPuddleId) { check(puddleId == INDEX_NONE); puddleId = newPuddleId; });
   if (newCluster == nullptr)
   {
      return false;
   }

   puddleClusterActor = newCluster;
   return true;
}

bool UTATPuddleSubsystem::AuthoritySpawnMultiplePuddles(ATATPuddleCluster*& puddleClusterActor, TArray<int32>& puddleIds, TSubclassOf<ATATPuddleCluster> puddleClass,
   const TArray<FTATPuddleTransform>& puddleTransforms, APawn* puddleInstigator, float deduplicateDistance)
{
   puddleClusterActor = nullptr;
   puddleIds.Empty(puddleTransforms.Num());

   UWorld* world = GetWorld();
   check(world != nullptr);
   check(world->GetAuthGameMode() != nullptr);

   if (puddleTransforms.IsEmpty())
   {
      return false;
   }

   FSphere combinedBoundingSphere = PuddleHelpers::CombinePuddleBoundingSpheres(puddleTransforms);
   combinedBoundingSphere.W = (combinedBoundingSphere.W * 1.2f) + 300.0f; // Add some padding

   // Do we have an existing cluster that makes sense to add the new puddle to?
   if (ATATPuddleCluster* existingCluster = _FindClusterCandidate(combinedBoundingSphere, puddleClass, puddleInstigator))
   {
      for (const FTATPuddleTransform& puddleTransform : puddleTransforms)
      {
         int32 puddleId = INDEX_NONE;

         if (deduplicateDistance > 0)
         {
            bool isNew = false;
            puddleId = existingCluster->AuthorityAddOrRefreshPuddle(isNew, puddleTransform, deduplicateDistance);
         }
         else
         {
            puddleId = existingCluster->AuthorityAddPuddle(puddleTransform);
         }

         if (ensure(puddleId != INDEX_NONE))
         {
            puddleIds.Add(puddleId);
         }
      }
      puddleClusterActor = existingCluster;
      return true;
   }

   // No existing clusters to use, spawn a new one
   UE_LOG(LogTATPuddleSubsystem, Verbose, TEXT("Spawning new puddle cluster (none of the existing %i cluster actors were close enough)"), _puddleClusterActors.Num());
   puddleClusterActor = _SpawnPuddleClusterActor(puddleClass, puddleTransforms, puddleInstigator, [&](int32 newPuddleId) { puddleIds.Add(newPuddleId); });
   return puddleClusterActor != nullptr;
}

void UTATPuddleSubsystem::RegisterPuddleCluster(ATATPuddleCluster* actor)
{
   if (actor == nullptr)
   {
      return;
   }

   _puddleClusterActors.Add(actor);

   UE_LOG(LogTATPuddleSubsystem, Verbose, TEXT("Registered actor %s with the puddle subsystem"), *actor->GetName());
}

void UTATPuddleSubsystem::UnregisterPuddleCluster(ATATPuddleCluster* actor)
{
   if (actor == nullptr)
   {
      return;
   }

   _puddleClusterActors.Remove(actor);

   UE_LOG(LogTATPuddleSubsystem, Verbose, TEXT("Unregistered actor %s with the puddle subsystem"), *actor->GetName());
}

ATATPuddleCluster* UTATPuddleSubsystem::_SpawnPuddleClusterActor(const TSubclassOf<ATATPuddleCluster>& puddleClass, TConstArrayView<FTATPuddleTransform> puddleTransforms,
   APawn* puddleInstigator, TFunctionRef<void(int32)> puddleAddedCallback) const
{
   if (puddleTransforms.IsEmpty())
   {
      return nullptr;
   }
   const FTransform transform{ puddleTransforms[0].Location };
   AActor* owner = nullptr;
   ATATPuddleCluster* cluster = GetWorld()->SpawnActorDeferred<ATATPuddleCluster>(puddleClass, transform, owner, puddleInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
   if (cluster == nullptr)
   {
      return nullptr;
   }
   for (const FTATPuddleTransform& puddle : puddleTransforms)
   {
      puddleAddedCallback(cluster->AuthorityAddPuddle(puddle));
   }
   cluster->FinishSpawning(transform);
   return cluster;
}

ATATPuddleCluster* UTATPuddleSubsystem::_FindClusterCandidate(const FSphere& boundingSphere, const TSubclassOf<ATATPuddleCluster>& puddleClass, APawn* puddleInstigator) const
{
   ATATPuddleCluster* bestCandidate = nullptr;
   float bestCandidateSquaredDist = std::numeric_limits<float>::max();

   for (ATATPuddleCluster* cluster : _puddleClusterActors)
   {
      // Make sure this cluster is available and that the instigator matches (so we can pass the instigator through to applied gameplay effects)
      if (!ensure(cluster) || !cluster->IsA(puddleClass) || cluster->GetInstigator() != puddleInstigator || !cluster->IsAvailableForNewPuddles())
      {
         continue;
      }

      // Any cluster that has a puddle which overlaps boundingSphere is fair game
      if (!cluster->IsSphereOverlappingPuddleCluster(boundingSphere))
      {
         continue;
      }

      // Using the distance to the cluster's actor location (as opposed to the centroid of all its puddles) isn't quite correct because it's really just the
      // location of the first puddle that was added (which may not even still exist), but here we're just using it as a simple heuristic to pick between
      // valid clusters in a deterministic way, so it's probably fine. Note that _puddleClusterActors is a TSet, so the ordering is NOT deterministic.
      const FVector puddleClusterLocation = cluster->GetActorLocation();

      // Could switch to this if more accuracy is needed:
      //const FVector puddleClusterLocation = cluster->GetPuddleClusterCentroid();

      const float distSq = FVector::DistSquared(puddleClusterLocation, boundingSphere.Center);
      if (distSq < bestCandidateSquaredDist)
      {
         bestCandidateSquaredDist = distSq;
         bestCandidate = cluster;
      }
   }

   return bestCandidate;
}

#if TAT_ENABLE_DEV_TOOLS

void UTATPuddleSubsystem::FDevToolUIState::Add(ATATPuddleCluster* actor)
{
   if (!IsValid(actor))
   {
      return;
   }
   UClass* cls = actor->GetClass();
   check(cls);
   const int32* idx = _classToGroupMap.Find(cls);
   if (idx != nullptr && ensure(Groups.IsValidIndex(*idx)) && ensure(Groups[*idx].Class == cls))
   {
      Groups[*idx].Puddles.Add(actor);
   }
   else
   {
      const int32 newIdx = Groups.Add(FPuddleList{ .Label = cls->GetName(), .Class = cls, .Puddles = { actor } });
      _classToGroupMap.Add(cls, newIdx);
   }
}

void UTATPuddleSubsystem::_DrawPuddleSubsystemDevTool(FTATDevToolState& state)
{
   if (_puddleClusterActors.IsEmpty())
   {
      return;
   }

   _puddleUIState.Reset();
   for (const auto& cluster : _puddleClusterActors)
   {
      _puddleUIState.Add(cluster.Get());
   }
   _puddleUIState.Sort();

   static const TCHAR* columnNames[] = {
      TEXT("Puddle Actor"),
      TEXT("Properties"),
   };
   if (TATImGui::BeginColumnGroup("##Puddles", columnNames, 0, ImGui::GetContentRegionAvail()))
   {
      if (TATImGui::NextColumnGroupColumn())
      {
         if (ImGui::BeginChild("##ActorList", {}, ImGuiChildFlags_FrameStyle))
         {
            for (const auto& group : _puddleUIState.Groups)
            {
               TATImGui::FScopedID groupId{ group.Label };

               TATImGui::TextUnformatted(group.Label);
               ImGui::Indent();
               for (ATATPuddleCluster* cluster : group.Puddles)
               {
                  TATImGui::FScopedID clusterId{ cluster };
                  const int32 numPuddles = cluster->NumPuddles();
                  const bool isSelected = state.SelectedObject == cluster;
                  if (ImGui::Selectable(TATImGui::ConvertString(FString::Printf(TEXT("%i puddle cluster"), numPuddles)), isSelected))
                  {
                     state.SelectedObject = cluster;
                  }
               }
               ImGui::Unindent();
               ImGui::Separator();
            }

         }
         ImGui::EndChild();
      }

      ATATPuddleCluster* selectedCluster = Cast<ATATPuddleCluster>(state.SelectedObject.Get());
      if (TATImGui::NextColumnGroupColumn() && IsValid(selectedCluster))
      {
         TATImGui::Text(TEXT("Selected actor: %s"), *GetNameSafe(selectedCluster));
         if (ImGui::BeginChild("##Actor"))
         {
            selectedCluster->DrawPuddleClusterDevTool();
         }
         ImGui::EndChild();
      }

      TATImGui::EndColumnGroup();
   }
}
#endif // TAT_ENABLE_DEV_TOOLS
