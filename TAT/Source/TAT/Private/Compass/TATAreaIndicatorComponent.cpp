// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Compass/TATAreaIndicatorComponent.h"

// tat
#include "Compass/TATGenericIndicator.h"
#include "Environment/TATAreaVolume.h"
#include "Player/TATCharacter.h"
#include "UI/TATCompassWidget.h"

// ose
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue4
#include "Components/ShapeComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAreaIndicatorComponent)

void UTATAreaIndicatorComponent::BeginPlay()
{
   Super::BeginPlay();

   if (!IsNetMode(NM_DedicatedServer))
   {
      _genericIndicator = CastChecked<ATATGenericIndicator>(GetOwner());
      check(IsValid(_genericIndicator));

      if (_genericIndicator->GetCloseRangeDetectionMode() == ETATIndicatorCloseRangeDetectionMode::AreaVolume)
      {
         // Make sure the referenced area volume has been loaded
         if (_areaVolume.IsPending())
         {
            // If this ever happens, we should add some tick-logic to initialize once the volume is loaded in
            UE_LOG(LogTATGenericIndicator, Error, TEXT("TATAreaIndicatorComponent - indicator %s referencing AreaVolume that hasn't loaded in yet!"), *_genericIndicator->GetName());
            return;
         }

         const ATATAreaVolume* areaVolume = _areaVolume.Get();
         if (!IsValid(areaVolume))
         {
            UE_LOG(LogTATGenericIndicator, Error, TEXT("TATAreaIndicatorComponent - indicator %s set to AreaVolume close-range detection mode, but no AreaVolume is assigned"), *_genericIndicator->GetName());
            return;
         }

         // Bind to shape collision tracking component's overlap detection
         _shapeCollisionTrackerComponent = areaVolume->GetShapeCollisionTrackerComponent();
         check(IsValid(_shapeCollisionTrackerComponent));

         _shapeCollisionTrackerComponent->OnActorEnteredShape.AddDynamic(this, &UTATAreaIndicatorComponent::OnActorOverlapBegin);
         _shapeCollisionTrackerComponent->OnActorExitedShape.AddDynamic(this, &UTATAreaIndicatorComponent::OnActorOverlapEnd);
      }
   }
}

void UTATAreaIndicatorComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // Unbind from shape collision tracking component
   if (!IsNetMode(NM_DedicatedServer))
   {
      check(IsValid(_genericIndicator));
      if (_genericIndicator->GetCloseRangeDetectionMode() == ETATIndicatorCloseRangeDetectionMode::AreaVolume && IsValid(_shapeCollisionTrackerComponent))
      {
         _shapeCollisionTrackerComponent->OnActorEnteredShape.RemoveDynamic(this, &UTATAreaIndicatorComponent::OnActorOverlapBegin);
         _shapeCollisionTrackerComponent->OnActorExitedShape.RemoveDynamic(this, &UTATAreaIndicatorComponent::OnActorOverlapEnd);
      }
   }

   Super::EndPlay(endPlayReason);
}

void UTATAreaIndicatorComponent::OnActorOverlapBegin(AActor* actor)
{
   const ATATCharacter* character = Cast<ATATCharacter>(actor);
   if (IsValid(character) && character->IsLocallyControlled())
   {
      check(IsValid(_genericIndicator));
      _genericIndicator->SetPlayerInCloseRange(true);

      UTATCompassWidget* compassWidget = UTATCompassWidget::TryGetCompass(this);
      check(IsValid(compassWidget));
      compassWidget->HandlePlayerInCloseRange(_genericIndicator, true);
   }
}

void UTATAreaIndicatorComponent::OnActorOverlapEnd(AActor* actor)
{
   const ATATCharacter* character = Cast<ATATCharacter>(actor);
   if (IsValid(character) && character->IsLocallyControlled())
   {
      check(IsValid(_genericIndicator));
      _genericIndicator->SetPlayerInCloseRange(false);

      UTATCompassWidget* compassWidget = UTATCompassWidget::TryGetCompass(this);
      check(IsValid(compassWidget));
      compassWidget->HandlePlayerInCloseRange(_genericIndicator, false);
   }
}

