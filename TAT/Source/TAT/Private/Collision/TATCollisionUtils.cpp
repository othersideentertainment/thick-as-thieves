// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Collision/TATCollisionUtils.h"

// tat
#include "Collision/Overlay/CollisionOverlayInterface.h"
#include "Developer/TATProjectSettings.h"

// ue
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

void TATCollisionUtils::SetOverlayForLyingDown(ACharacter* character, bool lyingDown)
{
   check(character);

   if (UCapsuleComponent* capsuleComponent = character->GetCapsuleComponent())
   {
      // if character is downed, don't want to create overlaps here either. assume that we do if character is upright.
      capsuleComponent->SetGenerateOverlapEvents(!lyingDown);

      if (auto overlayCapsule = Cast<ICollisionOverlayInterface>(capsuleComponent))
      {
         const UTATProjectSettings& settings = UTATProjectSettings::Get();
         FObjectKey key = &settings; // use settings as key for now, arbitrarily
         if (lyingDown)
         {
            overlayCapsule->AddCollisionOverlay(key, settings.GetLyingDownCollisionMask());
         }
         else
         {
            overlayCapsule->RemoveCollisionOverlayByKey(key);
         }
      }
   }

}
