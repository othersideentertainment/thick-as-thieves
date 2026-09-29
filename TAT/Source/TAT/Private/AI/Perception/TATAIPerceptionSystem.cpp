// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/TATAIPerceptionSystem.h"

// tat
#include "AI/Perception/TATResettableAIKnowledgeContainer.h"
#include "AI/TATAISettings.h"
#include "AI/Perception/TATAISense_Hearing.h"
#include "AI/Perception/TATHearingTypes.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"

// ue4 
#include "DrawDebugHelpers.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIPerceptionSystem)

//----------------------------------------------------------------------
// UTATAIPerceptionSystem
//----------------------------------------------------------------------

UTATAIPerceptionSystem::UTATAIPerceptionSystem(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   
}

void UTATAIPerceptionSystem::RegisterResettableKnowledgeContainer(UWorld* world,
   ITATResettableAIKnowledgeContainer* container)
{
   if(UTATAIPerceptionSystem* perceptionSystem = Cast<UTATAIPerceptionSystem>(GetCurrent(world)))
   {
      perceptionSystem->_RegisteredResettableKnowledgeContainer.AddUnique(container);
   }
}

void UTATAIPerceptionSystem::UnregisterResettableKnowledgeContainer(UWorld* world,
   ITATResettableAIKnowledgeContainer* container)
{
   if(UTATAIPerceptionSystem* perceptionSystem = Cast<UTATAIPerceptionSystem>(GetCurrent(world)))
   {
      perceptionSystem->_RegisteredResettableKnowledgeContainer.Remove(container);
   }
}

void UTATAIPerceptionSystem::ResetKnowledgeOfActor(UWorld* world, AActor* actor)
{
   if(UTATAIPerceptionSystem* perceptionSystem = Cast<UTATAIPerceptionSystem>(GetCurrent(world)))
   {
      for (TWeakInterfacePtr<ITATResettableAIKnowledgeContainer>& element : perceptionSystem->_RegisteredResettableKnowledgeContainer)
      {
         if(element.IsValid())
         {
            element->ResetKnowledgeOfActor(actor);
         }
      }
   }
}

int32 UTATAIPerceptionSystem::GenerateGlobalIdForHearingStim(const FTATAINoiseEvent& eventToGenerateIDFor)
{
   const UDataTable* hearingStimDataTable = UTATAISettings::Get().GetHearingStimSettings();
   FTATHearingEventStimSettings stimSettings;
   FName stimDataRowName;
   if(UTATPerceptionFunctionLibrary::GetStimInfoByGameplayTag(
      hearingStimDataTable,
      eventToGenerateIDFor.Tag,
      stimSettings,
      stimDataRowName
   ))
   {
      // if we aren't checking _everything_ then we need to assume that this tag should reuse the same global ID
      // BUT for now only reuse the ID if the check global ID flag is NOT set - so that the reaction coordinator gets the same ID for everyone to
      // react to.
      if (EnumHasAnyFlags(
         static_cast<EStimDatabaseQueryBitmaskValues>(stimSettings.QueryBitmask),
         EStimDatabaseQueryBitmaskValues::CheckGlobalID
      ) == false)
      {
         TMap<FGameplayTag, int>& foundRegisteredEvent = _RegisteredCharactersToGlobalTagsAndIDs.FindOrAdd(eventToGenerateIDFor.Instigator);
         if(const int* foundGlobalID = foundRegisteredEvent.Find(eventToGenerateIDFor.Tag))
         {
            return *foundGlobalID;
         }
         const int globalID = ++_nextHearingStimId;
         foundRegisteredEvent.Add(eventToGenerateIDFor.Tag, globalID);
         return globalID;
      }
      return ++_nextHearingStimId;
   }
   return ++_nextHearingStimId; 
}

void UTATAIPerceptionSystem::ResetGlobalIDForInstigator(AActor* actor)
{
   _RegisteredCharactersToGlobalTagsAndIDs.Remove(actor);
}
