// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Algo/Count.h"
#include "Misc/AutomationTest.h"


#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

// tat
#include "Variation/Clues/TATClueInfo.h"
#include "Variation/Clues/TATClueLocationInterface.h"
#include "Variation/Clues/TATClueSpawner.h"
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/Clues/TATClueType.h"

// ue
#include "Tests/AutomationEditorCommon.h"

namespace TestHelpers
{
   // not a real ustruct, but enough to test
   struct FTATClueInfo_Test final : public FTATClueInfo
   {
      // no generated body, since fake

      virtual FTATClueBucketKey GetClueBucket() const override { return { ClueType }; }

      ETATClueType ClueType = ETATClueType::None;
   };

   
   class FSpawnTestBase : public FAutomationTestBase
   {
   public:
      using FAutomationTestBase::FAutomationTestBase;
      void ReserveClues(int32 clueCount)
      {
         CluePool.SetNum(clueCount);
      }

      void Init()
      {
         World = FAutomationEditorCommonUtils::CreateNewMap();
         World->InitializeActorsForPlay(FURL());
         ReserveClues(100);
      }
      
      FTATClueInfoView MakeClue(ETATClueType type)
      {
         FTATClueInfo_Test& clue = CluePool[NextClueIndex++];
         clue.ClueType = type;
         return FTATClueInfoView::Make<FTATClueInfo>(clue);
      }

      FTATClueRequest MakeClueRequest(const FTATClueContext& context, std::initializer_list<FTATClueInfoView> clues)
      {
         return {context, clues};
      }

      UTATClueSpawnerComponent* MakeSpawner(ETATClueType clueType)
      {
         FScopedAllowAbstractClassAllocation allowAbstract;
         AActor* actor = World->SpawnActor<AActor>(); //< should this have a consistent name?
         UTATClueSpawnerComponent* spawner = NewObject<UTATClueSpawnerComponent>(actor, FName("UnitTestClueSpawner", NextSpawnerNumber++));
         spawner->Activate();
         spawner->SetClueType_TEST(clueType);
         return spawner;
      }

      FTATClueContext MakeContext()
      {
         return {UTATDummyClueLocation::Get(), FGameplayTag()};
      }

      static ETATClueType GetClueType(const FTATClueSpawnPlan::FClueEntry& clue)
      {
         return clue.Clue.Get<const FTATClueInfo>().GetClueBucket().Type;
      }

      static ETATClueType GetClueType(const FTATClueSpawnPlan::FSpawnerEntry& spawnerEntry)
      {
         return spawnerEntry.Spawner->GetClueBucket().Type;
      }

      // NOTE: this is probably more auto than I would use outside of test code (but technically needed to do this if
      //       making lamdas like this.) But it is just convenience
      static auto WithClueType(ETATClueType clueType)
      {
         return [clueType] (const auto& target) { return GetClueType(target) == clueType; };
      }
      
      static auto WithoutClueType(ETATClueType clueType)
      {
         return [clueType] (const auto& target) { return GetClueType(target) != clueType; };
      }

      static auto WithClueCount(int32 clueCount)
      {
         return [clueCount] (const FTATClueSpawnPlan::FSpawnerEntry& target) { return target.ClueCount == clueCount; };
      }

      template<typename PredA, typename PredB>
      static auto Both(PredA a, PredB b)
      {
         return [a, b] (const auto& target) { return a(target) && b(target); };
      }

      int CountClues(const FTATClueSpawnPlan& plan, TFunctionRef<bool (const FTATClueSpawnPlan::FClueEntry& clue)> predicate)
      {
         return Algo::CountIf(plan.Clues, predicate);
      }

      int CountSpawners(const FTATClueSpawnPlan& plan, TFunctionRef<bool (const FTATClueSpawnPlan::FSpawnerEntry& clue)> predicate)
      {
         return Algo::CountIf(plan.Spawners, predicate);
      }

      UWorld* World = nullptr;
      TArray<FTATClueRequest> ClueRequests;
      TArray<TObjectPtr<UTATClueSpawnerComponent>> Spawners;
      TArray<FTATClueInfo_Test> CluePool;
      int32 NextClueIndex = 0;
      int32 NextSpawnerNumber = 0;
   };
}


IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATClueSpawnTest, TestHelpers::FSpawnTestBase, "TAT.MapVariation.Clues.ClueSpawnTest",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATClueSpawnTest::RunTest(const FString& parameters)
{
   Init();
   
   ClueRequests = {
      MakeClueRequest(MakeContext(), {
         MakeClue(ETATClueType::Readable),
         MakeClue(ETATClueType::SpawnActor)
      }),
      MakeClueRequest(MakeContext(), {
         MakeClue(ETATClueType::Readable),
         MakeClue(ETATClueType::SpawnActor)
      }),
      MakeClueRequest(MakeContext(), {
         MakeClue(ETATClueType::Readable),
      })
   };
   
   Spawners = {
      MakeSpawner(ETATClueType::Readable),
      MakeSpawner(ETATClueType::Readable),
      MakeSpawner(ETATClueType::SpawnActor),
   };

   FTATClueSpawnParams params;
   params.SpawnRequests = ClueRequests;
   params.Spawners = Spawners;
   params.Seed = 123456;

   FTATClueSpawnPlan plan = TATClueSpawnUtils::GeneratePlan(params);

   TestEqual(TEXT("Plan has all spawners"), plan.Spawners.Num(), Spawners.Num());
   TestEqual(TEXT("Plan has all #spawners clues"), plan.Clues.Num(), Spawners.Num());
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATClueSpawnTestElectro, TestHelpers::FSpawnTestBase, "TAT.MapVariation.Clues.ClueSpawnTestWithElectrotype",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATClueSpawnTestElectro::RunTest(const FString& parameters)
{
   Init();
   
   ClueRequests = {
      MakeClueRequest(MakeContext(), {
         MakeClue(ETATClueType::Readable),
         MakeClue(ETATClueType::SpawnActor),
         MakeClue(ETATClueType::Electrotype),
      }),
      MakeClueRequest(MakeContext(), {
         MakeClue(ETATClueType::Readable),
         MakeClue(ETATClueType::SpawnActor),
         MakeClue(ETATClueType::Electrotype),
         MakeClue(ETATClueType::Electrotype),
      }),
      MakeClueRequest(MakeContext(), {
         MakeClue(ETATClueType::Readable),
      })
   };
   
   Spawners = {
      MakeSpawner(ETATClueType::Readable),
      MakeSpawner(ETATClueType::Readable),
      MakeSpawner(ETATClueType::SpawnActor),
      MakeSpawner(ETATClueType::Electrotype),
      MakeSpawner(ETATClueType::Electrotype)
   };

   FTATClueSpawnParams params;
   params.SpawnRequests = ClueRequests;
   params.Spawners = Spawners;
   params.Seed = 123456;

   FTATClueSpawnPlan plan = TATClueSpawnUtils::GeneratePlan(params);

   TestEqual(TEXT("Plan has all spawners"), plan.Spawners.Num(), Spawners.Num());
   TestEqual(TEXT("Plan has  #non-electrotype-spawners non-electrotype-clues"), CountClues(plan, WithoutClueType(ETATClueType::Electrotype)), 3);
   TestEqual(TEXT("Plan has all electrotype clues"), CountClues(plan, WithoutClueType(ETATClueType::Electrotype)), 3);
   TestEqual(TEXT("Plan has 1 electrotype with 2 clues"), CountSpawners(plan, Both(WithClueCount(2), WithClueType(ETATClueType::Electrotype))), 1);
   TestEqual(TEXT("Plan has 1 electrotype with 1 clue"), CountSpawners(plan, Both(WithClueCount(1), WithClueType(ETATClueType::Electrotype))), 1);
   return true;
}

#endif
