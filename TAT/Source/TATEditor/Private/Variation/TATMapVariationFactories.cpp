// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

//tat editor
#include "Variation/TATMapVariationFactories.h"

//tat
#include "Variation/TATSpawnData.h"
#include "Variation/Clues/TATClueSet.h"
#include "Variation/Clues/TATCompoundClueSet.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneSet.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Quests/TATMatchQuestDescription.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMapVariationFactories)

#define LOCTEXT_NAMESPACE "TATMapVariationFactories"

UTATSpawnDataAssetFactory::UTATSpawnDataAssetFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UTATSpawnDataAsset::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATSpawnDataAssetFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATSpawnDataAsset>(inParent, name, flags);
}

UTATSceneVariantConfigFactory::UTATSceneVariantConfigFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UTATSceneVariantConfig::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATSceneVariantConfigFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATSceneVariantConfig>(inParent, name, flags);
}

UTATSceneAssetFactory::UTATSceneAssetFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UTATSceneAsset::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATSceneAssetFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATSceneAsset>(inParent, name, flags);
}

UTATSceneSetFactory::UTATSceneSetFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UTATSceneSetAsset::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATSceneSetFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATSceneSetAsset>(inParent, name, flags);
}

UTATClueSetFactory::UTATClueSetFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UTATClueSet::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATClueSetFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATClueSet>(inParent, name, flags);
}

FString UTATClueSetFactory::GetDefaultNewAssetName() const
{
   return TEXT("CLS_NewClueSet");
}

UTATCompoundClueSetFactory::UTATCompoundClueSetFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UTATCompoundClueSet::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATCompoundClueSetFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATCompoundClueSet>(inParent, name, flags);
}

FString UTATCompoundClueSetFactory::GetDefaultNewAssetName() const
{
   return TEXT("CLS_NewCompoundClueSet");
}

UTATMatchQuestDescriptionFactory::UTATMatchQuestDescriptionFactory(const FObjectInitializer& objectInitializer)
{
   SupportedClass = UTATMatchQuestDescription::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATMatchQuestDescriptionFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATMatchQuestDescription>(inParent, name, flags);
}

FString UTATMatchQuestDescriptionFactory::GetDefaultNewAssetName() const
{
   return TEXT("MQD_NewMatchQuestDesctiption");
}

#undef LOCTEXT_NAMESPACE
