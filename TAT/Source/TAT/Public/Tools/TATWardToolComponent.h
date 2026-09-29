// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATWardableInterface.h"
#include "Tools/TATPlacedActorToolComponent.h"

#include "TATWardToolComponent.generated.h"

class UGameplayEffect;
class ATATToolWorldActor_Ward;

UCLASS()
class TAT_API UTATWardToolComponent : public UTATPlacedActorToolComponent
{
   GENERATED_BODY()

public:
   /// Maximum number of wards that can co-exist in the world
   ///TODO: Implement this restriction, it currently does nothing
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ward Tool", Meta = (UIMin = 1, ClampMin = 1))
   int32 MaxSpawnedWards = 1;

   /// Gameplay effect that is applied after we've deployed a ward
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ward Tool")
   TSubclassOf<UGameplayEffect> DeployedWardEffect;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   virtual bool OnRemoveFromToolSet_Implementation() override;

   UFUNCTION(BlueprintPure, Category = "Ward Tool")
   static bool IsValidWardableTarget(AActor* target);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ward Tool")
   ATATToolWorldActor_Ward* AuthoritySpawnWardActor(AActor* targetActor, TSubclassOf<ATATToolWorldActor_Ward> wardClass);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ward Tool")
   ATATToolWorldActor_Ward* AuthoritySpawnWardActorWithPlacementInfo(AActor* targetActor, TSubclassOf<ATATToolWorldActor_Ward> wardClass, const FTATWardPlacementInfo& placementInfo);

   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ward Tool")
   TArray<ATATToolWorldActor_Ward*> GetAllSpawnedWards() const;

   UFUNCTION(BlueprintPure, Category = "Ward Tool")
   ATATToolWorldActor_Ward* GetFirstSpawnedWard() const;

   /// Called by TATToolWardActor_Ward on BeginPlay
   void AuthorityAddSpawnedWard(ATATToolWorldActor_Ward* ward);

   /// Called by TATToolWardActor_Ward on EndPlay
   void AuthorityRemoveSpawnedWard(ATATToolWorldActor_Ward* ward);

   /// Destroy all currently spawned ward actors
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Ward Tool")
   void AuthorityRemoveAllSpawnedWards();

private:
   static ATATToolWorldActor_Ward* _GetWardActorFromEffectHandle(UAbilitySystemComponent* asc, const FActiveGameplayEffectHandle& handle);

};


