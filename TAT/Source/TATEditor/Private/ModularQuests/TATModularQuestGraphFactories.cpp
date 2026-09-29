// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat editor
#include "ModularQuests/TATModularQuestGraphFactories.h"

// tat
#include "TATEditor.h"
#include "Quests/Modules/TATQuestGraph.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATModularQuestGraphFactories)


UTATModularQuestGraphModuleFactory::UTATModularQuestGraphModuleFactory()
{
   bCreateNew = true;
   bEditAfterNew = true;
   SupportedClass = UTATQuestGraphModule::StaticClass();
}

UObject* UTATModularQuestGraphModuleFactory::FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   return NewObject<UObject>(parent, cls, name, flags | RF_Transactional);
}

uint32 UTATModularQuestGraphModuleFactory::GetMenuCategories() const
{
   FTATEditor& tatEditor = FModuleManager::LoadModuleChecked<FTATEditor>("TATEditor");
   return tatEditor.GetTATQuestCategoryBit();
}


UTATModularQuestGraphFactory::UTATModularQuestGraphFactory()
{
   bCreateNew = true;
   bEditAfterNew = true;
   SupportedClass = UTATQuestGraph::StaticClass();
}

UObject* UTATModularQuestGraphFactory::FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   return NewObject<UObject>(parent, cls, name, flags | RF_Transactional);
}

uint32 UTATModularQuestGraphFactory::GetMenuCategories() const
{
   FTATEditor& tatEditor = FModuleManager::LoadModuleChecked<FTATEditor>("TATEditor");
   return tatEditor.GetTATQuestCategoryBit();
}
