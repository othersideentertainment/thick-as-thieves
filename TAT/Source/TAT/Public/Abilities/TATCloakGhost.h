// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Perception/OSEAIVisibilityTargetInterface.h"
#include "AI/Utility/UtilityAITokenOwner.h"

// ue
#include "GameplayTagAssetInterface.h"
#include "GameplayEffectTypes.h"
#include "Perception/AISightTargetInterface.h"

#include "TATCloakGhost.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TAT_API ATATCloakGhost
   : public AActor
   , public IAISightTargetInterface
   , public IOSEAIVisibilityTargetInterface
   , public IGameplayTagAssetInterface
   , public IUtilityAITokenOwnerInterface
{
   GENERATED_BODY()

public:
   ATATCloakGhost();

   virtual void BeginPlay() override;

   // from IAISightTargetInterface
   virtual bool CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor = nullptr,
      const bool* wasVisible = nullptr, int32* userData = nullptr) const override;

   // from IOSEAIVisibilityTargetInterface
   virtual void AuthorityOnEnterVisibleByActor(AActor* viewingActor) override;
   virtual void AuthorityOnExitVisibleByActor(AActor* viewingActor) override;

   // from IUtilityAITokenOwnerInterface
   virtual UUtilityAITokenOwner* AuthorityGetTokenOwner_Implementation() const override { check(HasAuthority()); return _tokenOwner; }

   // from IGameplayTagAssetInterface
   FORCEINLINE bool HasMatchingGameplayTag(FGameplayTag tagToCheck) const override
   {
      return _ownedGameplayTags.HasTag(tagToCheck);
   }

   FORCEINLINE bool HasAllMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override
   {
      return _ownedGameplayTags.HasAll(tagContainer);
   }

   FORCEINLINE bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& tagContainer) const override
   {
      return _ownedGameplayTags.HasAny(tagContainer);
   }

   FORCEINLINE void GetOwnedGameplayTags(FGameplayTagContainer& tagContainer) const override
   {
      tagContainer.AppendTags(_ownedGameplayTags);
   }

protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "Cloak Ghost")
   void BP_AuthorityOnEnterVisibleByActor(AActor* viewingActor);

   UFUNCTION(BlueprintImplementableEvent, Category = "Cloak Ghost")
   void BP_AuthorityOnExitVisibleByActor(AActor* viewingActor);

protected:
   /// Owned tags, not dynamic, just "granted" at map start.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "1", UIMin = "1"), Category = "Ghost Config")
   FGameplayTagContainer _ownedGameplayTags;

   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   TArray<FOSEAITokenInfo> DefaultAITokens;

private:
   UPROPERTY(Transient)
   UUtilityAITokenOwner* _tokenOwner = nullptr;
};
