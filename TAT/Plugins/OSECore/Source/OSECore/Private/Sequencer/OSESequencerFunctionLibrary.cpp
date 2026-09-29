// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Sequencer/OSESequencerFunctionLibrary.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Player/OSEPlayerState.h"

// ue4
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESequencerFunctionLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogOSESequencerFunctionLibrary, Log, All);

bool UOSESequencerFunctionLibrary::SetSkeletalMeshForPlayer(const UObject* contextObj, USkeletalMeshComponent* skeletalMeshToSet, int playerIdx, bool hideOnFailure)
{
   if (!skeletalMeshToSet)
   {
      UE_LOG(LogOSESequencerFunctionLibrary, Error, TEXT("SetSkeletalMeshForPlayer: No skeletal mesh component to update!"));
      return false;
   }

   if (AOSEPlayerState* ps = AOSEPlayerState::GetOSEPlayerState(contextObj, playerIdx))
   {
      if (AOSECharacterBase* character = Cast<AOSECharacterBase>(ps->GetPawn()))
      {
         if (USkeletalMeshComponent* characterMesh = character->GetMesh())
         {
            if (characterMesh->GetSkinnedAsset())
            {
               skeletalMeshToSet->SetSkinnedAsset(characterMesh->GetSkinnedAsset());
               return true;
            }
         }
      }
   }

   // failed one of our lookups, maybe there's no player at this index, or they don't have a pawn yet, etc.
   if(hideOnFailure)
   {
      check(skeletalMeshToSet);
      skeletalMeshToSet->SetHiddenInGame(true);
   }
   return false;
}

