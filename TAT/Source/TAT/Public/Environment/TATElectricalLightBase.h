// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATInhibitableInterface.h"

// ue
#include "GameFramework/Actor.h"

#include "TATElectricalLightBase.generated.h"

class UTATElectricalDeviceComponent;

// A Stub class to use as a native base class of Light blueprints
//
// May add more functionality later
// NOTE: May want to rename later, as not all users will require
//       electricity, but that is easy
UCLASS()
class TAT_API ATATElectricalLightBase
   : public AActor
   , public ITATInhibitableInterface
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATElectricalLightBase();

   /// From ITATInhibitableInterface
   virtual bool CanBeInhibitedBy_Implementation(FGameplayTag inhibitorType) const;
   virtual FGameplayTag GetInhibitableType_Implementation() const override { return InhibitableType; }
   virtual FTATInhibitorPlacementInfo GetInhibitorPlacementInfo_Implementation() const override;
   virtual void OnInhibitorActivated_Implementation(ATATInhibitorActor* inhibitorActor, APawn* instigator, int32 newInhibitorCount) override {}
   virtual void OnInhibitorDeactivated_Implementation(ATATInhibitorActor* inhibitorActor, int32 newInhibitorCount, bool allInhibitorsRemoved) override {}

   /// Allow inhibitors to work on this light
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable")
   bool IsInhibitable = false;

   /// The type/category of this light in terms of things that can be inhibited in the world.
   /// Allows things that apply inhibitors to decide what they can inhibit (eg. a tool that can inhibit this actor only if it has the type "Inhibitable.Light")
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inhibitable", Meta = (EditCondition = "IsInhibitable", Categories = "Inhibitable"))
   FGameplayTag InhibitableType;

protected:
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
   TObjectPtr<UTATElectricalDeviceComponent> _electricalDeviceComponent;
};
