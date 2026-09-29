// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Player/AimAssist/OSEAimAssistComponent.h"

// ue4
#include "Components/SceneComponent.h"

#include "OSEAimAssistProviderComponent.generated.h"

class APawn;

struct FAimAssistProviderContext
{
public:
   FAimAssistProviderContext(const AActor* providerActor, const APawn* playerPawn)
      : ProviderActor(providerActor)
      , PlayerPawn(playerPawn)
   {}


   const AActor* ProviderActor;
   const APawn* PlayerPawn;
};

UCLASS(Abstract)
class OSECORE_API UOSEAimAssistProviderPredicate : public UObject
{
   GENERATED_BODY()

public:
   virtual bool ShouldAddTarget(const FAimAssistProviderContext& context) const { return true; }
};


UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class OSECORE_API UOSEAimAssistProviderComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEAimAssistProviderComponent();

   // from USceneComponent
   virtual void BeginPlay() override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UFUNCTION(BlueprintCallable, Category = "Aim Assist")
   void SetAimAssistBoundsComponent(USceneComponent* component);
   UFUNCTION(BlueprintCallable, Category = "Aim Assist")
   void SetAimAssistCenterComponent(USceneComponent* component);

protected:
   virtual bool _ShouldAddTarget(const FAimAssistProviderContext& context) const;

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   bool Enabled = true;

   // Stop attempting to aim assist on this component at this distance.  Serves as an early-out
   // optimization so we're not taking into account objects very far away
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   float MaxAimAssistDistance = 1000.0f;

   // how big is the "inner box" in relation to the object bounding box?
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   float InnerBoxSizeMultiplier = 1.0f;

   // how much bigger is the outside "start aim assist here" box than the inside actor bounds box?
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   float OuterBoxSizeMultiplier = 1.2f;

   // The local offset of the center from the center component
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist", meta=(MakeEditWidget = true))
   FVector CenterComponentOffset;

   // other requirements to add the target
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist")
   TArray<TSubclassOf<UOSEAimAssistProviderPredicate>> AimAssistConditions;

private:
   void _UpdateAimAssistTarget();

private:
   FAimAssistTarget _aimAssistTarget;
   
   UPROPERTY(Transient)
   USceneComponent* _boundsComponent = nullptr;
   UPROPERTY(Transient)
   USceneComponent* _centerComponent = nullptr;
};
