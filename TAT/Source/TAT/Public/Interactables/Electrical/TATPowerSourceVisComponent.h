// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATPowerSourceVisComponent.generated.h"

class ATATPowerSource;

UCLASS()
class TAT_API UTATPowerSourceVisComponent : public UActorComponent
{
   UTATPowerSourceVisComponent();

   GENERATED_BODY()

   // From UActorComponent
   virtual void OnRegister() override;

public:
   const ATATPowerSource* GetPowerSource() const;
   void TryRefreshChildPowerSources() const;

#if WITH_EDITOR
   const TArray<TWeakObjectPtr<ATATPowerSource>>& GetChildPowerSources() const { return _childPowerSources; }
#endif // WITH_EDITOR

public:
   /// Set to true when _childPowerSources needs to be re-evaluated
   mutable bool IsDirty = false;

private:
#if WITH_EDITORONLY_DATA
   /// Cached references to child power sources. 
   /// Must be mutable for RefreshChildPowerSources() to be callable on const-instance from FTATPowerSourceComponentVisualizer::DrawVisualization() / DrawVisualizationHUD()
   UPROPERTY(Transient)
   mutable TArray<TWeakObjectPtr<ATATPowerSource>> _childPowerSources;
#endif // WITH_EDITORONLY_DATA
};
