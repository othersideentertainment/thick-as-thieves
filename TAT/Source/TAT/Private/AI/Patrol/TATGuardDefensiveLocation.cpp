// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Patrol/TATGuardDefensiveLocation.h"

// ue
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGuardDefensiveLocation)

ATATGuardDefensiveLocation::ATATGuardDefensiveLocation(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   PrimaryActorTick.bCanEverTick = false;
   bNetLoadOnClient = false;
#if WITH_EDITORONLY_DATA
   if(UCapsuleComponent* capsuleComponent = GetCapsuleComponent())
   {
      capsuleComponent->InitCapsuleSize(40.0f, 92.0f);
   }
   
   _arrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
   if (_arrowComponent)
   {
      _arrowComponent->ArrowColor = FColor(235, 64, 52);
      _arrowComponent->ArrowSize = 1.0f;
      _arrowComponent->bTreatAsASprite = true;
      _arrowComponent->SetupAttachment(GetCapsuleComponent());
      _arrowComponent->bIsScreenSizeScaled = true;
   }
#endif // WITH_EDITORONLY_DATA
}
