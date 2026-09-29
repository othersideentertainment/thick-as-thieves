// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Thiefsign/TATThiefsignTypes.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Misc/DataValidation.h"

#include "TATThiefsignDecalActor.generated.h"

class AOSEPlayerState;
class APlayerController;
class UArrowComponent;
class UBillboardComponent;
class UTATSceneRequirementVisComponent;

UCLASS(Abstract, Blueprintable, HideCategories=(Activation, Actor, "Actor Tick", Collision, Cooking, HLOD, Input, LOD, Networking, Physics, Rendering, Replication))
class TAT_API ATATThiefsignDecalActor : public AActor
{
   GENERATED_BODY()

   ATATThiefsignDecalActor();

public:
#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

protected:
   // from AActor
#if WITH_EDITOR
   virtual void PostActorCreated() override;
#endif // WITH_EDITOR
   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
   virtual void Tick(float deltaTime) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

   UPROPERTY(BlueprintReadOnly)
   UDecalComponent* DecalComponent = nullptr;

   // Identifier to Thiefsign info row in ThiefsignTable
   UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, meta = (Categories = "Thiefsign", ExposeOnSpawn = true))
   FGameplayTag ThiefsignIdentifier = FGameplayTag::EmptyTag;

   // Width/Height of the decal in world-space
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FVector2f Size = FVector2f(64.0f, 64.0f);

   // Time in seconds for the symbol to fade from visible <-> invisible
   UPROPERTY(EditDefaultsOnly, meta = (UIMin = "0.1", ClampMin = "0.1"))
   float OpacityFadeSeconds = 1.0f;

   // Name of the material parameter pertaining to the symbol's opacity
   UPROPERTY(EditDefaultsOnly)
   FName MaterialOpacityParamName = NAME_None;

   // Player state of the player that created this Thiefsign decal actor (if one designer pre-placed), defined on spawn in blueprints
   UPROPERTY(Replicated, Transient, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
   TObjectPtr<AOSEPlayerState> InstigatorPlayer;

#if WITH_EDITOR
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   UTATSceneRequirementVisComponent* SceneRequirementVisComponent = nullptr;

   // Arrow component to indicate forward direction of decal
   UPROPERTY(Transient)
   UArrowComponent* ArrowComponent = nullptr;

   // Billboard component to make it easier to click decal in editor
   UPROPERTY(Transient)
   UBillboardComponent* SpriteComponent = nullptr;
#endif

private:
   void _LoadMaterial();
   void _LoadDynamicMaterialInstance();
   void _OnDynamicMaterialInstanceLoaded();

   UFUNCTION()
   void _OnLocalPlayerThiefVisionStatusChanged(APlayerController* controller, bool thiefVisionEnabled);

   // Updates _isThiefVisionEnabled, the cached status of the local player's ThiefVision enabled status
   void _UpdateThiefVisionEnabledCache(bool enabled);

   UPROPERTY(Transient)
   FTATThiefsignVFXConfig _cachedVFXConfig;

   UPROPERTY(Transient)
   UMaterialInstanceDynamic* _dynamicMaterialInstance = nullptr;

   uint8 _isMaterialLoaded : 1;
   uint8 _isInstigatorLocal : 1;
   uint8 _isThiefVisionEnabled : 1;
   uint8 _isVisible : 1;

   FORCEINLINE bool _IsEditorPreview() const
   {
#if WITH_EDITOR
      const UWorld* world = GetWorld();
      return world != nullptr && world->WorldType == EWorldType::Editor;
#else
      return false;
#endif // WITH_EDITOR
   }
   FORCEINLINE bool _ShouldBeVisible() const { return _isMaterialLoaded && (_isInstigatorLocal || _isThiefVisionEnabled || _IsEditorPreview()); }

   void _UpdateVisibility();

   UPROPERTY(EditInstanceOnly, Category = Requirement, meta = (ShowOnlyInnerProperties))
   FTATSceneRequirement _requirement;
};
