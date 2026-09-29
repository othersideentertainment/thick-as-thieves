// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

// ue4
#include "Perception/AISightTargetInterface.h"

#include "TATIllusion.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TAT_API ATATIllusion
   : public AActor
   , public IAISightTargetInterface
   , public IUtilityAITokenOwnerInterface
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;

   // from IAISightTargetInterface
   virtual bool CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor = nullptr,
      const bool* wasVisible = nullptr, int32* userData = nullptr) const override;

   // from IUtilityAITokenOwnerInterface
   virtual UUtilityAITokenOwner* AuthorityGetTokenOwner_Implementation() const override { check(HasAuthority()); return _tokenOwner; }

protected:
   /// The sight strength to report when seen by AIs. Sight strength is used to determine detection rate in NPCs.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "1", UIMin = "1"))
   int32 SightStrengthMultiplier = 1;

   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   TArray<FOSEAITokenInfo> DefaultAITokens;

private:
   UPROPERTY(Transient)
   UUtilityAITokenOwner* _tokenOwner = nullptr;
};
