// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT




#include "Tutorial/TATTutorialRespawnPoint.h"

// ue
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTutorialRespawnPoint)

// Sets default values
ATATTutorialRespawnPoint::ATATTutorialRespawnPoint()
{
   // NOTE: adapted from ATATPlayerStart
   GetCapsuleComponent()->InitCapsuleSize(40.0f, 92.0f);
   GetCapsuleComponent()->SetShouldUpdatePhysicsVolume(false);

#if WITH_EDITORONLY_DATA
   _arrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

   if (!IsRunningCommandlet())
   {
      // Structure to hold one-time initialization
      struct FConstructorStatics
      {
         ConstructorHelpers::FObjectFinderOptional<UTexture2D> PlayerStartTextureObject;
         FName ID_PlayerStart;
         FText NAME_PlayerStart;
         FName ID_Navigation;
         FText NAME_Navigation;
         FConstructorStatics()
            : PlayerStartTextureObject(TEXT("/Engine/EditorResources/S_Player"))
            , ID_PlayerStart(TEXT("PlayerStart"))
            , NAME_PlayerStart(NSLOCTEXT("SpriteCategory", "PlayerStart", "Player Start"))
            , ID_Navigation(TEXT("Navigation"))
            , NAME_Navigation(NSLOCTEXT("SpriteCategory", "Navigation", "Navigation"))
         {
         }
      };
      static FConstructorStatics sConstructorStatics;

      if (GetGoodSprite())
      {
         GetGoodSprite()->Sprite = sConstructorStatics.PlayerStartTextureObject.Get();
         GetGoodSprite()->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
         GetGoodSprite()->SpriteInfo.Category = sConstructorStatics.ID_PlayerStart;
         GetGoodSprite()->SpriteInfo.DisplayName = sConstructorStatics.NAME_PlayerStart;
      }
      if (GetBadSprite())
      {
         GetBadSprite()->SetVisibility(false);
      }

      if (_arrowComponent)
      {
         _arrowComponent->ArrowColor = FColor(150, 200, 255);

         _arrowComponent->ArrowSize = 1.0f;
         _arrowComponent->bTreatAsASprite = true;
         _arrowComponent->SpriteInfo.Category = sConstructorStatics.ID_Navigation;
         _arrowComponent->SpriteInfo.DisplayName = sConstructorStatics.NAME_Navigation;
         _arrowComponent->SetupAttachment(GetCapsuleComponent());
         _arrowComponent->bIsScreenSizeScaled = true;
      }
   }

   bIsSpatiallyLoaded = false;
#endif // WITH_EDITORONLY_DATA
}


