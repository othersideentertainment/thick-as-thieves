// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATToolTypes.h"
#include "Tools/TATGearWorldActorInterface.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "Character/OSETeamInterface.h"

// ue5
#include "GameFramework/Actor.h"
#include "Perception/AISightTargetInterface.h"

#include "TATToolWorldActor_Base.generated.h"

UCLASS()
class TAT_API ATATToolWorldActor_Base : public AActor
   , public IInteractableInterface
   , public ITATGearWorldActorInterface
   , public IAISightTargetInterface
   , public IOSETeamInterface
{
   GENERATED_BODY()
public:
   ATATToolWorldActor_Base();

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void BeginPlay() override;
   
   // From IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;

   // From ITATGearWorldActorInterface
   virtual void AuthorityDeploy_Implementation(const FTATGearWorldActorParameters& worldActorParams) override;

   // IAISightTargetInterface start
   virtual bool CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor = nullptr,
      const bool* wasVisible = nullptr, int32* userData = nullptr) const override;
   // IAISightTargetInterface end

   // begin IOSETeamInterface
   virtual uint8 GetTeam() const override;
   // end IOSETeamInterface
   
   UFUNCTION(BlueprintPure, meta = (Categories = "Tool.Usage"))
   int32 GetAmmoCostByUsageType(FGameplayTag ToolUsageTag) const;
   
   /// When can this world actor be picked up by players?
   UPROPERTY(EditDefaultsOnly, Category = "Pickup")
   ETATDeployablePickupCapability DeployablePickupCapability = ETATDeployablePickupCapability::Never;

   /// If true, an interacting player with a full ammo count of the granted tool type will destroy this actor instance
   UPROPERTY(EditDefaultsOnly, Category = "Pickup", meta = (EditCondition="DeployablePickupCapability != ETATDeployablePickupCapability::Never", EditConditionHides))
   bool AllowPlayerToDestroyIfAmmoFull = true;

   /// The gameplay cues used during the hold interaction for picking up this deployable
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup", Meta = (EditCondition = "DeployablePickupCapability != ETATDeployablePickupCapability::Never", EditConditionHides))
   FOSEHeldActionCues PickUpHoldActionCues;

   TSubclassOf<UTATToolComponent> GetParentToolClass() const { return _parentToolClass; }

protected:
   UPROPERTY(EditDefaultsOnly, Category = "AI|Sense")
   bool _shouldRegisterForPerceptionSource { false };
   
   virtual bool _HasBeenActivated() const { return false; }

   int32 _GetAmmoCountToAdd() const;
   
   UPROPERTY(EditDefaultsOnly)
   class UAIPerceptionStimuliSourceComponent* _perceptionStimuliSource = nullptr;
   
private:
   UPROPERTY(Transient, Replicated)
   TSubclassOf<UTATToolComponent> _parentToolClass;
};
