// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/SmartObjects/TATSmartObjectComponent.h"

// ue
#include "GameplayTagContainer.h"
#include "SmartObjectRuntime.h"
#include "Components/ActorComponent.h"

#include "TATLivingWorldAgentComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATLivingWorldAgent, Log, All);

UCLASS(ClassGroup=("AI"), meta=(BlueprintSpawnableComponent))
class TAT_API UTATLivingWorldAgentComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATLivingWorldAgentComponent();

   static UTATLivingWorldAgentComponent* Get(AActor* actor);

   void ForceSmartObjects(TConstArrayView<TObjectPtr<AActor>> smartObjectActors);

   bool HasValidClaimedSmartObjectHandles() const;

   UFUNCTION(BlueprintCallable)
   FSmartObjectClaimHandle GetNextClaimHandle();
   
protected:
   UFUNCTION()
   void HandleSetupOfLivingWorldComponents();

   virtual void BeginPlay() override;
   
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer UserTags;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery ActivityTagQuery;

   UPROPERTY(EditAnywhere)
   int MaxNumberOfInteractions { 3 };
   
   UPROPERTY(EditAnywhere)
   float QuerySize { 1000.f };

   bool HasValidSmartObjectHandles { false };
   bool HasForcedSelection { false };

   
   UFUNCTION()
   void OnSlotStateChanged(const FSmartObjectEventData& event);

   TQueue<FSmartObjectClaimHandle> ClaimedSmartObjectHandles;
};
