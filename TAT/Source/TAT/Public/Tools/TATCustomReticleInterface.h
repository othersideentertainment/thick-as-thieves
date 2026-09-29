// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "TATCustomReticleInterface.generated.h"

class UTATCustomReticleWidgetBase;

UINTERFACE(BlueprintType, Category = "Tools")
class UTATCustomReticleInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATCustomReticleInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TAT|Tools")
   FGameplayTag GetCurrentReticleState() const;
   virtual FGameplayTag GetCurrentReticleState_Implementation() const { return FGameplayTag(); }

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TAT|Tools")
   AActor* GetCurrentReticleTargetActor() const;
   virtual AActor* GetCurrentReticleTargetActor_Implementation() const { return nullptr; }

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TAT|Tools")
   TSubclassOf<UTATCustomReticleWidgetBase> GetReticleWidgetClass() const;
   virtual TSubclassOf<UTATCustomReticleWidgetBase> GetReticleWidgetClass_Implementation() const { return nullptr; }
};

