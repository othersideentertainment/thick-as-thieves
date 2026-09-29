// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/SmartObjects/TATAmbientSmartObject.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAmbientSmartObject)

ATATAmbientSmartObject::ATATAmbientSmartObject()
{
   PrimaryActorTick.bCanEverTick = false;
   
   USceneComponent* sceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   RootComponent = sceneComponent;
   RootComponent->SetMobility(EComponentMobility::Static);
   
   _ambientNode = CreateDefaultSubobject<UTATActionNodeComponent_Ambient>(TEXT("ActionNode"));
   _ambientNode->SetupAttachment(RootComponent);
}
