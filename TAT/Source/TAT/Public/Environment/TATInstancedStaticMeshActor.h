// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATInstancedStaticMeshActor.generated.h"

class UBoxComponent;
class UArrowComponent;

// This class is a native wrapper for an already existing level design tool originally made in blueprint.
// There is an issue with adding / removing mesh instances within the blueprint constructor, so instead we now 
// Listen for the edit change property, and edit move, then trigger our own "HandleSetup" function which routes
// execution to the blueprint layer where the original implementation lives.
UCLASS()
class TAT_API ATATInstancedStaticMeshActor : public AActor
{
   GENERATED_BODY()

public:
   ATATInstancedStaticMeshActor();

protected:
#if WITH_EDITOR
   virtual void PostEditChangeProperty(struct FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditMove(bool bFinished) override;
   virtual void PostEditUndo() override;
   void TryHandleSetup();
#endif
   
   virtual void PostLoad() override;
   
   UFUNCTION(BlueprintNativeEvent, CallInEditor)
   void HandleSetup();
   
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MakeEditWidget = true))
   FVector _LineEnd;
   
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   UArrowComponent* _Arrow { nullptr };
   
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   UBoxComponent* _NegativeBoxCollision { nullptr };  
   
   UPROPERTY()
   bool _HasInitialSetupTriggered { false };
   
};
