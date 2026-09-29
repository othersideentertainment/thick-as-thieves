// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat editor
#include "Upgrades/TATUpgradeGraphFactories.h"

// tat
#include "Upgrades/TATUpgradeType.h"
#include "Upgrades/TATUpgradeGraph.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUpgradeGraphFactories)

UTATUpgradeGraphFactory::UTATUpgradeGraphFactory()
{
   bCreateNew = true;
   bEditAfterNew = true;
   SupportedClass = UTATUpgradeGraph::StaticClass();
}

UObject* UTATUpgradeGraphFactory::FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   return NewObject<UObject>(parent, cls, name, flags | RF_Transactional);
}

UTATUpgradeTypeFactory::UTATUpgradeTypeFactory(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   SupportedClass = UTATUpgradeType::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATUpgradeTypeFactory::FactoryCreateNew(UClass* clazz, UObject* inParent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(clazz == SupportedClass);

   return NewObject<UTATUpgradeType>(inParent, name, flags);
}
