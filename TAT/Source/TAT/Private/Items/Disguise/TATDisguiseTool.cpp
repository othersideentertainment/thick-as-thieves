// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Disguise/TATDisguiseTool.h"

// tat
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"
#include "Disguise/TATDisguiseComponent.h"

// ose
#include "Character/OSETeamInterface.h"


// ue4
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDisguiseTool)


DEFINE_LOG_CATEGORY_STATIC(LogDisguise, Log, Log);

FDisguiseSnapshot UTATDisguiseFunctionLibrary::CreateDisguiseSnapshotFromTargetActor(ACharacter* character)
{
   check(character && character->Implements<UOSETeamInterface>());
   FDisguiseSnapshot snapshot;
   snapshot.Team = CastChecked<IOSETeamInterface>(character)->GetOriginalTeam();
   snapshot.ActorClass = character->GetClass();
   if(ITATPrivateSpaceCharacterInterface* privateSpaceCharacterInterface = Cast<ITATPrivateSpaceCharacterInterface>(character))
   {
      snapshot.AllowedPrivateZones = privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent()->AuthorityGetAllAllowedPrivateZone();
   }
   return snapshot;
}


// static
bool UTATDisguiseFunctionLibrary::DoesCharacterHaveActiveDisguise(ACharacter* character)
{
   if (character != nullptr)
   {
      if (UTATDisguiseComponent* disguiseComponent = character->GetComponentByClass<UTATDisguiseComponent>())
      {
         return disguiseComponent->IsDisguiseActive();
      }
   }
   return false;
}
