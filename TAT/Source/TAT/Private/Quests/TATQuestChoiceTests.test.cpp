// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

// tat
#include "Damage/TATDamageTags.h"
#include "Lockpicking/TATCombinationHelpers.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Quests/TATMatchQuestDescription.h"

// ue
#include "Misc/AutomationTest.h"

namespace TestHelpers
{
   class FChoiceTestBase : public FAutomationTestBase
   {
   public:
      using FAutomationTestBase::FAutomationTestBase;

      void Init()
      {
         Teardown();
         NextVariantNumber = 0;
         NextSceneNumber = 0;
         for(int i = 0; i < 4; ++i)
         {
            ScenePool.Add(MakeScene({
               MakeVariant(), MakeVariant(), MakeVariant(), MakeVariant(),
            }));
         }
      }

      void Teardown()
      {
         QuestDescription = nullptr;
         ScenePool.Empty();
      }

      UTATSceneVariantConfig* MakeVariant()
      {
         UTATSceneVariantConfig* variant = NewObject<UTATSceneVariantConfig>(GetTransientPackage(), FName("UnitTestSceneVariant", NextVariantNumber++));
         return variant;
      }

      UTATSceneAsset* MakeScene(std::initializer_list<TObjectPtr<UTATSceneVariantConfig>> variants)
      {
         UTATSceneAsset* scene = NewObject<UTATSceneAsset>(GetTransientPackage(), FName("UnitTestScene", NextSceneNumber++));
         scene->Variants = variants;
         return scene;
      }

      UTATSceneVariantConfig* GetVariant(int sceneIndex, int variantIndex)
      {
         return ScenePool[sceneIndex]->Variants[variantIndex];
      }

      UTATMatchQuestDescription* MakeQuestDescription(std::initializer_list<FTATQuestChoice> choices)
      {
         UTATMatchQuestDescription* desc = NewObject<UTATMatchQuestDescription>(GetTransientPackage(), FName("UnitTestMatchDesc"));
         desc->Choices = choices;
         return desc;
      }

      const UTATSceneAsset* FindParentScene(const UTATSceneVariantConfig* variant) const
      {
         UTATSceneAsset* const* found = ScenePool.FindByPredicate([variant](const UTATSceneAsset* scene) { return scene->HasVariant(variant); });
         return found ? *found : nullptr;
      }

      auto MakeFindParentScene() const
      {
         return [this] (const UTATSceneVariantConfig* variant) { return FindParentScene(variant); };
      }

      void TestNoSceneOverlap(TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> variants)
      {
         for(int i = 0; i < variants.Num(); ++i)
         {
            const UTATSceneVariantConfig* variant = variants[i];
            const UTATSceneAsset* parent = FindParentScene(variant);
            for(int j = i + 1; j < variants.Num(); ++j)
            {
               const UTATSceneVariantConfig* otherVariant = variants[j];
               TestNotEqual(TEXT("Two variants have the same scene"), parent, FindParentScene(otherVariant));
            }
         }
      }
      
      void TestHasSceneForEachChoice(TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> variants)
      {
         for(const FTATQuestChoice& choice : QuestDescription->Choices)
         {
            TestTrue(TEXT("Has matching variant for choice"), choice.Options.ContainsByPredicate([variants](const FTATQuestChoiceOption& option)
               {
                  return option.SceneVariant == nullptr || variants.Contains(option.SceneVariant);
               }));
         }
      }

      static constexpr int kSeedCount = 10;
      static constexpr int kInitialSeed = 12345;

      UTATMatchQuestDescription* QuestDescription = nullptr;
      TArray<UTATSceneAsset*> ScenePool;
      int32 NextVariantNumber = 0;
      int32 NextSceneNumber = 0;
   };
}


IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.Basic",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest::RunTest(const FString& parameters)
{
   Init();
   
   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            { .SceneVariant = GetVariant(0, 0) },
            { .SceneVariant = GetVariant(0, 1) },
            { .SceneVariant = GetVariant(1, 2) },
         },
      },
      {
         .Options = {
            {.SceneVariant = GetVariant(0, 3) },
            {.SceneVariant = GetVariant(1, 1) },
            {.SceneVariant = GetVariant(2, 2) },
         },
      },
   });

   FRandomStream randomStream(kInitialSeed);
   for(int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, FTATQuestChoiceParams {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .FindParentScene = MakeFindParentScene()
         });

      TestNoSceneOverlap(plan.ForcedSceneVariants);
      TestHasSceneForEachChoice(plan.ForcedSceneVariants);
      TestEqual(TEXT("Plan has variants for each choice"), plan.ForcedSceneVariants.Num(), QuestDescription->Choices.Num());
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_ForcedVariants, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.ForcedVariants",
   EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_ForcedVariants::RunTest(const FString& parameters)
{
   Init();

   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            {.SceneVariant = GetVariant(0, 0) },
            {.SceneVariant = GetVariant(0, 1) },
            {.SceneVariant = GetVariant(1, 2) },
         },
      },
      {
         .Options = {
            {.SceneVariant = GetVariant(0, 3) },
            {.SceneVariant = GetVariant(1, 1) },
            {.SceneVariant = GetVariant(2, 2) },
         },
      },
   });

   QuestDescription->ForcedSceneVariants.Add(GetVariant(0, 2));

   FRandomStream randomStream(kInitialSeed);
   for (int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, FTATQuestChoiceParams {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .FindParentScene = MakeFindParentScene()
         });

      TestNoSceneOverlap(plan.ForcedSceneVariants);
      TestTrueExpr(plan.ForcedSceneVariants.Contains(GetVariant(0, 2)));
      TestTrueExpr(plan.ForcedSceneVariants.Contains(GetVariant(1, 2)));
      TestTrueExpr(plan.ForcedSceneVariants.Contains(GetVariant(2, 2)));
      TestTrueExpr(plan.ForcedSceneVariants.Num() == 3);
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_Tags, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.Tags",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_Tags::RunTest(const FString& parameters)
{
   Init();
   
   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            { .AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Burn), .SceneVariant = GetVariant(0, 0) },
            { .AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Shock), .SceneVariant = GetVariant(0, 1) },
            { .AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Physical), .SceneVariant = GetVariant(1, 2) },
         },
      },
      {
         .Options = {
            { .SceneVariant = GetVariant(0, 3) },
            { .SceneVariant = GetVariant(1, 1) },
            { .SceneVariant = GetVariant(2, 2) },
         },
      },
   });

   FRandomStream randomStream(kInitialSeed);
   for(int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .FindParentScene = MakeFindParentScene()
       });

      TestNoSceneOverlap(plan.ForcedSceneVariants);
      TestHasSceneForEachChoice(plan.ForcedSceneVariants);
      TestTrueExpr(plan.ChoiceTags.HasTag(TAG_DamageType));
      TestTrueExpr(plan.ChoiceTags.Num() == 1);
      TestEqual(TEXT("Plan has variants for each choice"), plan.ForcedSceneVariants.Num(), QuestDescription->Choices.Num());
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_ComboLock, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.ComboLock",
   EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_ComboLock::RunTest(const FString& parameters)
{
   Init();

   const FName burnName = "Burn";
   const FName shockName = "Shock";
   const FName somethingName = "Something";

   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Burn), .LockCombinations = {{TEXT("Combo"), {burnName}}}},
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Shock), .LockCombinations = {{TEXT("Combo"), {shockName}}} }
         },
      },
   });
   QuestDescription->LockCombinations.Add(TEXT("Always"), { somethingName });

   FRandomStream randomStream(kInitialSeed);
   for (int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      const int32 seed = static_cast<int32>(randomStream.GetUnsignedInt());
      QuestDescription->GenerateChoices(plan, FTATQuestChoiceParams {
         .Seed = seed,
         .FindParentScene = MakeFindParentScene()
      });

      auto makeComboText = [seed](FName comboName) {
         return CombinationHelpers::FormatCombinationFromName(comboName, seed);
      };

      TestTrueExpr(plan.ChoiceTags.Num() == 1);
      TestTrueExpr(plan.FormatParams.Num() == 2);
      TestTrueExpr(plan.FormatParams.FindRef(TEXT("Always")).EqualTo(makeComboText(somethingName)));
      TestTrueExpr(plan.ChoiceTags.HasTagExact(TAG_DamageType_Burn) == plan.FormatParams.FindRef(TEXT("Combo")).EqualTo(makeComboText(burnName)));
      TestTrueExpr(plan.ChoiceTags.HasTagExact(TAG_DamageType_Shock) == plan.FormatParams.FindRef(TEXT("Combo")).EqualTo(makeComboText(shockName)));
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_Multiple, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.Multiple",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_Multiple::RunTest(const FString& parameters)
{
   Init();
   
   QuestDescription = MakeQuestDescription({
      {
         .NumToChoose = {2, 2},
         .Options = {
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Burn), .SceneVariant = GetVariant(0, 0) },
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Shock), .SceneVariant = GetVariant(0, 1) },
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Physical), .SceneVariant = GetVariant(1, 2) },
         },
      }
   });

   FRandomStream randomStream(kInitialSeed);
   for(int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, FTATQuestChoiceParams {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .FindParentScene = MakeFindParentScene()
         });

      TestNoSceneOverlap(plan.ForcedSceneVariants);
      TestHasSceneForEachChoice(plan.ForcedSceneVariants);
      TestTrueExpr(plan.ChoiceTags.HasTag(TAG_DamageType));
      TestTrueExpr(plan.ChoiceTags.Num() == 2);
      TestTrueExpr(plan.ForcedSceneVariants.Num() == 2);
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_NoVariants, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.NoVariants",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_NoVariants::RunTest(const FString& parameters)
{
   Init();
   
   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Burn) },
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Shock) },
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Physical) },
         },
      },
   });

   FRandomStream randomStream(kInitialSeed);
   for(int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, FTATQuestChoiceParams {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .FindParentScene = MakeFindParentScene()
      });
      
      TestTrueExpr(plan.ChoiceTags.HasTag(TAG_DamageType));
      TestTrueExpr(plan.ChoiceTags.Num() == 1);
      TestTrueExpr(plan.ForcedSceneVariants.IsEmpty());
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_Conjoined, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.Conjoined",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_Conjoined::RunTest(const FString& parameters)
{
   Init();
   
   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            {.SceneVariant = GetVariant(0, 0) },
         },
      },
      {
         .Options = {
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Burn), .SceneVariant = GetVariant(0, 0) },
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Shock), .SceneVariant = GetVariant(0, 1) },
            {.AddedChoiceTags = FGameplayTagContainer(TAG_DamageType_Physical), .SceneVariant = GetVariant(1, 2) },
         },
      }
   });

   FRandomStream randomStream(kInitialSeed);
   for(int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, FTATQuestChoiceParams {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .FindParentScene = MakeFindParentScene()
         });

      TestNoSceneOverlap(plan.ForcedSceneVariants);
      TestHasSceneForEachChoice(plan.ForcedSceneVariants);
      TestTrueExpr(plan.ChoiceTags.HasTag(TAG_DamageType_Burn));
      TestTrueExpr(plan.ChoiceTags.Num() == 1);
      TestTrueExpr(plan.ForcedSceneVariants.Num() == 1);
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_ConjoinedOnlyFirst, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.ConjoinedOnlyFirst",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_ConjoinedOnlyFirst::RunTest(const FString& parameters)
{
   Init();
   
   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            { .SceneVariant = GetVariant(0, 0) },
            { .SceneVariant = GetVariant(1, 1) },
         },
      },
      {
         .Options = {
            { .SceneVariant = GetVariant(0, 0) },
            { .SceneVariant = GetVariant(2, 1) },
         },
      }
   });

   FRandomStream randomStream(kInitialSeed);
   for(int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .FindParentScene = MakeFindParentScene()
      });

      TestNoSceneOverlap(plan.ForcedSceneVariants);
      TestHasSceneForEachChoice(plan.ForcedSceneVariants);
      // The choices must agree (and variants imply choices), so if it was selected in the first choice, it should not be selected in the second
      TestTrueExpr(plan.ForcedSceneVariants.Contains(GetVariant(0, 0)) != plan.ForcedSceneVariants.Contains(GetVariant(1, 1)));
   }

   Teardown();
   return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TATQuestChoiceTest_Override, TestHelpers::FChoiceTestBase, "TAT.MapVariation.Quests.Choice.Override",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool TATQuestChoiceTest_Override::RunTest(const FString& parameters)
{
   Init();
   
   QuestDescription = MakeQuestDescription({
      {
         .Options = {
            { .SceneVariant = GetVariant(0, 0) },
            { .SceneVariant = GetVariant(0, 1) },
            { .SceneVariant = GetVariant(1, 2) },
         },
      },
      {
         .Options = {
            { .SceneVariant = GetVariant(0, 3) },
            { .SceneVariant = GetVariant(1, 1) },
            { .SceneVariant = GetVariant(2, 2) },
         },
      },
   });

   FRandomStream randomStream(kInitialSeed);
   for(int i = 0; i < kSeedCount; ++i)
   {
      FTATQuestChoicePlan plan;
      QuestDescription->GenerateChoices(plan, FTATQuestChoiceParams {
         .Seed = static_cast<int32>(randomStream.GetUnsignedInt()),
         .VariantOverrides = {GetVariant(1, 2)},
         .FindParentScene = MakeFindParentScene()
         });

      TestNoSceneOverlap(plan.ForcedSceneVariants);
      TestHasSceneForEachChoice(plan.ForcedSceneVariants);
      TestTrueExpr(plan.ForcedSceneVariants.Contains(GetVariant(1, 2)));
      TestEqual(TEXT("Plan has variants for each choice"), plan.ForcedSceneVariants.Num(), QuestDescription->Choices.Num());
   }
   
   Teardown();
   return true;
}

#endif
