// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/UnifiedStealthSystem/TATVisionVisualization.h"

// tat
#include "Player/TATCharacter.h"
#include "Developer/TATEditorSettings.h"

// ue
#include "Components/CapsuleComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATVisionVisualization)

UTATVisionVisualization::UTATVisionVisualization(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void UTATVisionVisualization::BeginPlay()
{
   Super::BeginPlay();
   _visualizationMaterialInstance = UMaterialInstanceDynamic::Create(_visualizationMaterial, this);
   SetMaterial(0, _visualizationMaterialInstance);

#if WITH_EDITOR
   if (UTATEditorSettings::Get().HidePlayerStealthVisualization)
   {
      SetHiddenInGame(true);
   }
#endif
}

void UTATVisionVisualization::TickComponent(const float deltaTime, const ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
      return;
   if(const ATATCharacter* character = Cast<ATATCharacter>(GetOwner()))
   {
      const float heightOffset = character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
      _visualizationMaterialInstance->SetVectorParameterValue(FName("FootPosition"), character->GetActorLocation() - FVector(0,0, heightOffset));
      FVector origin, bounds;
      GetOwner()->GetActorBounds(true, origin, bounds);
      const float visionRangePlusBounds = bounds.Size2D() + _guardVisionRange;
      const float currentVisionRange = FMath::Max(_minGuardVisionRange,visionRangePlusBounds * (1-character->GetStealthScore()));
      SetWorldScale3D(FVector(currentVisionRange, currentVisionRange, currentVisionRange));
   
   }
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
}
