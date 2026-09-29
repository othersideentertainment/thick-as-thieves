// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

#include "OSEGoalNodeComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGoalNode, Log, All);

class USphereComponent;

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OSEAI_API UOSEGoalNodeComponent : public USceneComponent, public IUtilityAITokenOwnerInterface
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;

   virtual void OnRegister() override;

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|GoalNode")
   FVector AuthorityAssignGuardToLocation(AActor* guard);
   
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|GoalNode")
   void AuthorityReleaseGuardLocation(AActor* guard);

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|GoalNode")
   const float GetGoalRadius() const { return GoalRadius; }

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditAnywhere, Category = "AI|OSE|GoalNode")
   bool DrawDebug = false;
#endif

   // from IUtilityAITokenOwnerInterface
   virtual UUtilityAITokenOwner* AuthorityGetTokenOwner_Implementation() const override { check(GetOwner() && GetOwner()->HasAuthority()); return _tokenOwner; }

#if WITH_EDITOR
   // Begin UObject
   virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
   // End UObject

   void RefreshDebugRepresentation();
#endif

protected:
   UPROPERTY(EditAnywhere, Category = "AI|OSE|GoalNode")
   FOSEAITokenInfo GoalNodeToken;

   UPROPERTY(EditAnywhere, Category = "AI|OSE|GoalNode")
   float GoalRadius = 200.0;

#if WITH_EDITORONLY_DATA
   UPROPERTY()
   USphereComponent* DebugSphereComponent;
#endif

private:
   TArray<TWeakObjectPtr<AActor>> _guardLocationAssignments;

   UPROPERTY(Transient)
   UUtilityAITokenOwner* _tokenOwner = nullptr;
};
