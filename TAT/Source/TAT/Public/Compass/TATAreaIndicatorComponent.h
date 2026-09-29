// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/LazyObjectPtr.h"

#include "TATAreaIndicatorComponent.generated.h"

// tat
class ATATAreaVolume;
class ATATGenericIndicator;

// ose
class UOSEShapeCollisionTrackerComponent;

// ue4
class UShapeComponent;

UCLASS()
class TAT_API UTATAreaIndicatorComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // From AActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   
   const TSoftObjectPtr<ATATAreaVolume>& GetAreaVolume() const { return _areaVolume; }

private:
   UFUNCTION()
   void OnActorOverlapBegin(AActor* actor);
   UFUNCTION()
   void OnActorOverlapEnd(AActor* actor);

private:
   UPROPERTY()
   UOSEShapeCollisionTrackerComponent* _shapeCollisionTrackerComponent;
   
   UPROPERTY()
   ATATGenericIndicator* _genericIndicator;

   // Lazy pointer to area volume used by owning indicator.
   UPROPERTY(EditAnywhere, Category = "Compass Settings")
   TSoftObjectPtr<ATATAreaVolume> _areaVolume;
};
