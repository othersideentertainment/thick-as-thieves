// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Utility/UtilityAITokenOwner.h"
#include "AI/Utility/UtilityAITokenOwnerSmartObject.h"

// ue
#include "CoreMinimal.h"
#include "SmartObjectComponent.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectSubsystem.h"

#include "TATSmartObjectComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTATSmartObjectSlotStateChanged, UTATSmartObjectComponent*, smartObjectComponent, FSmartObjectSlotHandle, smartObjectSlotHandle, ESmartObjectSlotState, PrevState, ESmartObjectSlotState, NewState);

class ITATSmartObjectTagInterface;
UCLASS(Blueprintable, ClassGroup = Gameplay, meta = (BlueprintSpawnableComponent), config = Game, HideCategories = (Activation, AssetUserData, Collision, Cooking, HLOD, Lighting, LOD, Mobile, Mobility, Navigation, Physics, RayTracing, Rendering, Tags, TextureStreaming), AutoExpandCategories = ("AI|TAT|SmartObject"))
class TAT_API UTATSmartObjectComponent
   : public USmartObjectComponent
   , public IUtilityAITokenOwnerInterface
{
   GENERATED_BODY()

public:
   UTATSmartObjectComponent(const FObjectInitializer& objectInitializer);

   // from UActorComponent
   virtual void BeginPlay() override;

   // from IUtilityAITokenOwnerInterface
   virtual UUtilityAITokenOwner* AuthorityGetTokenOwner_Implementation() const override { check(GetOwner()->HasAuthority()); return _tokenOwner; }

   UFUNCTION(BlueprintCallable)
   FSmartObjectClaimHandle TryClaim(const FSmartObjectRequestFilter& filter) const;
   FSmartObjectClaimHandle TryClaim(const FSmartObjectSlotHandle& slotHandle) const;

   UFUNCTION(BlueprintCallable)
   FSmartObjectClaimHandle TryClaimWithSlotIndex(int slotIndex) const;
   
private:
   UPROPERTY(Transient)
   UUtilityAITokenOwnerSmartObject* _tokenOwner = nullptr;

};
