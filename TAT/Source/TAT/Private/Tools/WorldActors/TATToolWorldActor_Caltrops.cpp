// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tools/WorldActors/TATToolWorldActor_Caltrops.h"

// tat
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "AI/Target/TATTargetingGroups.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_Caltrops)

ATATToolWorldActor_Caltrops::ATATToolWorldActor_Caltrops()
{
   _smartObjectComponent = CreateDefaultSubobject<UTATSmartObjectComponent>(TEXT("SmartObject"));
}

UTATSmartObjectComponent* ATATToolWorldActor_Caltrops::GetSmartObjectComponent() const
{
   return _smartObjectComponent;
}

FGameplayTag ATATToolWorldActor_Caltrops::GetUtilityAITargetingGroup() const
{
   return TAG_AI_TargetingGroup_SmartObject_PlayerTool;
}
