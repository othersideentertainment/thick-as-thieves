// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

// tat
#include "Damage/TATDamageTags.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/SceneVariants/TATSceneSelection.h"
#include "Variation/SceneVariants/TATSceneSet.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"

// ue5
#include "Algo/Count.h"

BEGIN_DEFINE_SPEC(FTATSceneSelectionSpec,
   TEXT("TAT.MapVariation.SceneSelection"),
   EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
TObjectPtr<UTATSceneSetAsset> CurrentSet = nullptr;
TArray<TObjectPtr<UTATSceneVariantConfig>> Overrides;
using FResultPair = TPair<const UTATSceneVariantConfig*, int32>;
TArray<FResultPair> Results;
FTATSceneVariantSelectionParams Params;
TArray<FGameplayTag> ExtraTraits;
TArray<FTATSceneTraitWithLimit> LimitedTraits;
TMap<TObjectPtr<UTATSceneSetAsset>, FTATSceneSetOverride> SceneSetOverrides;
static constexpr int NumSeeds = 20;
static constexpr int InitialSeed = 362782812;

int32 nextSetNumber = 0;
int32 nextSceneNumber = 0;
int32 nextVariantNumber = 0;

UTATSceneVariantConfig* MakeVariant(std::initializer_list<FGameplayTag> traits = {})
{
   UTATSceneVariantConfig* variant = NewObject<UTATSceneVariantConfig>(GetTransientPackage(), FName("UnitTestSceneVariant", nextVariantNumber++));
   FGameplayTagContainer traitsContainer;
   for (FGameplayTag tag : traits)
   {
      traitsContainer.AddTagFast(tag);
   }
   variant->Test_SetTraits(MoveTemp(traitsContainer));
   return variant;
}

UTATSceneAsset* MakeScene(std::initializer_list<TObjectPtr<UTATSceneVariantConfig>> variants)
{
   UTATSceneAsset* scene = NewObject<UTATSceneAsset>(GetTransientPackage(), FName("UnitTestScene", nextSceneNumber++));
   scene->Variants = variants;
   return scene;
}

UTATSceneSetAsset* MakeSet(std::initializer_list<TObjectPtr<UTATSceneAsset>> scenes)
{
   UTATSceneSetAsset* set = NewObject<UTATSceneSetAsset>(GetTransientPackage(), FName("UnitTestSceneSet", nextSetNumber++));
   set->Scenes = scenes;
   return set;
};

UTATSceneSetAsset* MakeSet(std::initializer_list<FTATSceneVariantSelectionPhase> phases, std::initializer_list<TObjectPtr<UTATSceneAsset>> scenes)
{
   UTATSceneSetAsset* set = NewObject<UTATSceneSetAsset>(GetTransientPackage(), FName("UnitTestSceneSet", nextSetNumber++));
   set->SelectionPhases = phases;
   set->Scenes = scenes;
   return set;
};

void AddExtraTrait(FGameplayTag tag)
{
   ExtraTraits.Add(tag);
   Params.ExtraAllowedTraits = ExtraTraits;
}

void AddLimitedTrait(FGameplayTag tag, int32 limit)
{
   LimitedTraits.Add({tag, limit});
   Params.LimitedTraits = LimitedTraits;
}

void AddSceneSetOverride(UTATSceneSetAsset* sceneSet, FTATSceneSetOverride override)
{
   SceneSetOverrides.Emplace(sceneSet, MoveTemp(override));
   Params.SceneSetOverrides = &SceneSetOverrides;
}

void RunSelection()
{
   Results.Reset();
   TATSceneSelection::SelectVariants({ CurrentSet }, Params, [this](const UTATSceneAsset* scene, const UTATSceneVariantConfig* Variant, int32 index)
      {
         Results.Emplace(Variant, index);
      });
}

void TestResultsForEachScene()
{
   TestEqual(TEXT("Output length"), CurrentSet->Scenes.Num(), Results.Num());
   const int count = FMath::Min(CurrentSet->Scenes.Num(), Results.Num());
   for (int i = 0; i < count; ++i)
   {
      TestEqual(TEXT("Variant is owned by scene"), Results[i].Value, CurrentSet->Scenes[i]->Variants.IndexOfByKey(Results[i].Key));
   }
}

void TestOverrideInScene()
{
   for (UTATSceneVariantConfig* override : Overrides)
   {
      TestEqual(TEXT("Contains override"),
         Results.ContainsByPredicate([override] (const FResultPair& result) { return result.Key == override;}),
         true);
   }
}

void TestPhasesMet(TConstArrayView<FTATSceneVariantSelectionPhase> phases)
{
   for (const FTATSceneVariantSelectionPhase& phase : phases)
   {
      const int numMatching = CountMatching(phase.RequiredTrait);
      TestTrue(*WriteToString<64>(TEXT("Has enough for phase "), phase.RequiredTrait.GetTagName(), TEXT(" "), numMatching, TEXT(" < "), phase.SceneCount.Min),
         numMatching >= phase.SceneCount.Min);
      TestTrue(*WriteToString<64>(TEXT("Does not have too much for phase "), phase.RequiredTrait.GetTagName(), TEXT(" "), numMatching, TEXT(" > "), phase.SceneCount.Max),
         numMatching <= phase.SceneCount.Max);
   }
}

void TestPhasesNotOver(TConstArrayView<FTATSceneVariantSelectionPhase> phases)
{
   for (const FTATSceneVariantSelectionPhase& phase : phases)
   {
      const int numMatching = CountMatching(phase.RequiredTrait);
      TestTrue(*WriteToString<64>(TEXT("Does not have too much for phase "), phase.RequiredTrait.GetTagName(), TEXT(" "), numMatching, TEXT(" > "), phase.SceneCount.Max),
         numMatching <= phase.SceneCount.Max);
   }
}

void TestExactCount(FGameplayTag trait, int expected)
{
   const int numMatching = CountMatching(trait);
   TestTrue(*WriteToString<64>(TEXT("Count for "), trait.GetTagName(), TEXT(" should = "), expected, TEXT(" (actual "), numMatching, TEXT(") ")),
      expected == numMatching);
}

void TestPhasesMet()
{
   TestPhasesMet(CurrentSet->SelectionPhases);
}

void TestLimitsMet()
{
   for (const FTATSceneTraitWithLimit& trait : LimitedTraits)
   {
      const int numMatching = CountMatching(trait.Trait);
      TestTrue(*WriteToString<64>(TEXT("Does not have too much for limit "), trait.Trait.GetTagName(), TEXT(" "), numMatching, TEXT(" > "), trait.Limit),
         numMatching <= trait.Limit);
   }
}

void TestNoneMatching(FGameplayTag tag)
{
   const int numMatching = CountMatching(tag);
   TestTrue(*WriteToString<64>(TEXT("Does not have "), tag.GetTagName()), numMatching == 0);
}

bool HasAnyMatching(FGameplayTag tag)
{
   return CountMatching(tag) > 0;
}

int32 CountMatching(FGameplayTag tag)
{
   return Algo::CountIf(Results, [tag](const FResultPair& result) { return result.Key && result.Key->HasTraits(tag.GetSingleTagContainer()); });
}

void SetFirstVariantAsOverride()
{
   Overrides.Add(CurrentSet->Scenes[0]->Variants[0]);
   Params.VariantOverrides = MakeArrayView(Overrides);
}

void SetVariantAsOverride(int32 sceneIndex, int32 variantIndex)
{
   Overrides.Add(CurrentSet->Scenes[sceneIndex]->Variants[variantIndex]);
   Params.VariantOverrides = MakeArrayView(Overrides);
}

END_DEFINE_SPEC(FTATSceneSelectionSpec)


void FTATSceneSelectionSpec::Define()
{
   BeforeEach([this]
      {
         nextSetNumber = 0;
         nextSceneNumber = 0;
         nextVariantNumber = 0;
         Overrides.Reset();
         Results.Reset();
         SceneSetOverrides.Reset();
         LimitedTraits.Reset();
         ExtraTraits.Reset();
         SceneSetOverrides.Reset();
         Params = FTATSceneVariantSelectionParams();
      });

   AfterEach([this]
      {
         CurrentSet = nullptr;
      });

   Describe(TEXT("SimpleSceneSet"), [this] {
         BeforeEach([this] {
               CurrentSet = MakeSet({
                  MakeScene({MakeVariant(), MakeVariant(), MakeVariant()}),
                  MakeScene({MakeVariant(), MakeVariant()}),
                  MakeScene({MakeVariant()}),
                  });
            });

         It(TEXT("Produces valid results"), [this]
            {
               FRandomStream seedStream(InitialSeed);
               for (int i = 0; i < NumSeeds; ++i)
               {
                  Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

                  RunSelection();
                  TestResultsForEachScene();
               }
            });

         It(TEXT("Allows overrides"), [this]
            {
               SetFirstVariantAsOverride();
               FRandomStream seedStream(InitialSeed);
               for (int i = 0; i < NumSeeds; ++i)
               {
                  Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

                  RunSelection();
                  TestResultsForEachScene();
                  TestOverrideInScene();
               }
            });
      });
   Describe(TEXT("PhasedSceneSet"), [this] {
      BeforeEach([this] {
         CurrentSet = MakeSet(
            {
               {TAG_DamageType_Physical, {1,1}},
               {TAG_DamageType_Burn, {2,3}}
            },
            {
               MakeScene({MakeVariant({TAG_DamageType_Burn, TAG_DamageType_Physical}), MakeVariant({TAG_DamageType_Burn}), MakeVariant({TAG_DamageType_Shock}), MakeVariant()}),
               MakeScene({MakeVariant({TAG_DamageType_Burn, TAG_DamageType_Physical}), MakeVariant(), MakeVariant({TAG_DamageType_Shock}), MakeVariant()}),
               MakeScene({MakeVariant({TAG_DamageType_Burn}), MakeVariant({TAG_DamageType_Physical}), MakeVariant({TAG_DamageType_Shock}), MakeVariant()}),
               MakeScene({MakeVariant({TAG_DamageType_Burn}), MakeVariant({TAG_DamageType_Burn, TAG_DamageType_Shock}), MakeVariant()}),
               MakeScene({MakeVariant({TAG_DamageType_Burn}), MakeVariant({TAG_DamageType_Burn, TAG_DamageType_Shock}), MakeVariant()}),
               MakeScene({MakeVariant({TAG_DamageType_Burn}), MakeVariant()}),
               MakeScene({MakeVariant({TAG_DamageType_Shock}), MakeVariant()}),
            });
         });

      It(TEXT("Produces valid results"), [this]
         {
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet();
               TestNoneMatching(TAG_DamageType_Shock);
            }
         });

      It(TEXT("Allows overrides"), [this]
         {
            SetFirstVariantAsOverride();
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet();
               TestNoneMatching(TAG_DamageType_Shock);
               TestOverrideInScene();
            }
         });

      It(TEXT("Allows phase overrides"), [this]
         {
            AddSceneSetOverride(CurrentSet, { {{TAG_DamageType_Burn, {2,3}}} });
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet({{TAG_DamageType_Burn, {2,3}}});
               TestNoneMatching(TAG_DamageType_Shock);
               TestNoneMatching(TAG_DamageType_Physical);
            }
         });

      It(TEXT("Ignore phase overrides for other sets"), [this]
         {
            AddSceneSetOverride(nullptr, { {{TAG_DamageType_Burn, {2,3}}} });
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet();
               TestNoneMatching(TAG_DamageType_Shock);
            }
         });

      It(TEXT("Allows extra tags from set override"), [this]
         {
            AddSceneSetOverride(CurrentSet, { .ExtraSceneTraits = FGameplayTagContainer(TAG_DamageType_Shock), .OverrideSelectionPhases = false });
            bool hadAny = false;
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet();
               hadAny |= HasAnyMatching(TAG_DamageType_Shock);
            }

            TestTrue(TEXT("Had any with extra tag"), hadAny);
         });

      It(TEXT("Ignores extra tags from set override for different set"), [this]
         {
            AddSceneSetOverride(nullptr, { .ExtraSceneTraits = FGameplayTagContainer(TAG_DamageType_Shock), .OverrideSelectionPhases = false  });
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet();
               TestNoneMatching(TAG_DamageType_Shock);
            }
         });

      It(TEXT("Allows limited tags"), [this]
         {
            AddLimitedTrait(TAG_DamageType_Shock, 2);
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet();
               TestLimitsMet();
               TestExactCount(TAG_DamageType_Shock, 2);
            }
         });

      It(TEXT("Allows more limited tags"), [this]
         {
            AddLimitedTrait(TAG_DamageType_Shock, 7);
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               // Limited traits take precedence, so meeting the phases may not always be possible
               TestPhasesNotOver(CurrentSet->SelectionPhases);
               TestLimitsMet();
               // For now, it does not attempt to use variant with traits also
               // would be added in phases
               TestExactCount(TAG_DamageType_Shock, 4);
            }
         });

      It(TEXT("Allows limited tags with override"), [this]
         {
            AddLimitedTrait(TAG_DamageType_Shock, 2);
            SetVariantAsOverride(3, 1);
            bool hadAny = false;
            FRandomStream seedStream(InitialSeed);
            for (int i = 0; i < NumSeeds; ++i)
            {
               Params.Seed = static_cast<int32>(seedStream.GetUnsignedInt());

               RunSelection();
               TestResultsForEachScene();
               TestPhasesMet();
               TestLimitsMet();
               TestOverrideInScene();
               hadAny |= HasAnyMatching(TAG_DamageType_Shock);
            }

            TestTrue(TEXT("Had any with extra tag"), hadAny);
         });
      });
}

#endif
