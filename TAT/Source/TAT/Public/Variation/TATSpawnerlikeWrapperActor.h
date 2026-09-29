// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATSpawnerlikeWrapperActor.generated.h"

// An actor meant to wrap a component, with icon and box to visualize where something might spawn
UCLASS(hideCategories=(Input,Collision,Replication,Rendering,Physics,HLOD,Cooking,"Actor Tick",Tags,Activation), ComponentWrapperClass)
class TAT_API ATATSpawnerlikeWrapperActor : public AActor
{
   GENERATED_BODY()

public:
   ATATSpawnerlikeWrapperActor(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

#if WITH_EDITOR
   virtual void OnConstruction(const FTransform& transform) override;
   virtual void PostEditMove(bool finished) override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

protected:
   void _SetRootComponent(USceneComponent* root);
#if WITH_EDITOR
   void _TryValidateCollision();
   void _ValidateCollision();
   void _UpdateIcon();
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
   // the icon to use for the spawner (when it is a valid placement)
   UPROPERTY(EditDefaultsOnly, Category = "Editor Settings")
   TObjectPtr<UTexture2D> _icon = nullptr;

   // The scale of the icon (when it is a valid placement)
   UPROPERTY(EditDefaultsOnly, Category = "Editor Settings")
   float _iconScale = 1;

   UPROPERTY(Transient)
   TObjectPtr<class UBoxComponent> _boxComponent = nullptr;

   UPROPERTY(Transient)
   TObjectPtr<class UArrowComponent> _arrowComponent = nullptr;

   // Normal editor sprite.
   UPROPERTY(Transient)
   TObjectPtr<UBillboardComponent> _goodSprite = nullptr;

   // Used to draw bad collision intersection in editor.
   UPROPERTY(Transient)
   TObjectPtr<UBillboardComponent> _badSprite = nullptr;
#endif
};
