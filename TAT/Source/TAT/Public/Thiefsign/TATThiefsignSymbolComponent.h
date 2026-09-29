// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Thiefsign/TATThiefsignTypes.h"

// ue
#include "Components/MaterialBillboardComponent.h"
#include "GameplayEffectTypes.h"

#include "TATThiefsignSymbolComponent.generated.h"

class UMaterialInterface;

UCLASS(BlueprintType, Blueprintable, DontCollapseCategories, HideCategories = (Replication, ComponentReplication, Streaming, Mobile, LOD, HLOD, Cooking, ComponentTick), Meta = (BlueprintSpawnableComponent))
class TAT_API UTATThiefsignSymbolComponent : public UMaterialBillboardComponent
{
   GENERATED_BODY()

public:
   UTATThiefsignSymbolComponent();

   // Setup the symbol visualization
   UFUNCTION(BlueprintCallable)
   void ConfigureSymbol(FGameplayTag thiefsignIdentifier, const FVector2D& size, bool isFirstPerson);

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

protected:
   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
   virtual void TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UMaterialInstanceDynamic* GetOrCreateDynamicMaterialInstance();
   UMaterialInstanceDynamic* ConstructDynamicMaterialInstance();

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Thiefsign|Symbol")
   FMaterialSpriteElement SymbolMaterialElement;

   // Time in seconds for the symbol to fade from visible <-> invisible
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Thiefsign|Symbol", meta = (UIMin = "0.1", ClampMin = "0.1"))
   float OpacityFadeSeconds = 1.0f;

   // Name of the material parameter pertaining to the symbol's opacity
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Thiefsign|Symbol")
   FName MaterialOpacityParamName;

   // Speed in uu per second the symbol should float upwards (when not in first person)
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Thiefsign|Symbol")
   float FloatUpSpeed = 50.0f;

   // If this condition tag is added to the owning player, end the lifespan early
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Thiefsign|Symbol", meta = (Categories = "Condition"))
   FGameplayTag PlayerUnconsciousTag = FGameplayTag::EmptyTag;

private:
   FORCEINLINE bool _ShouldApplyFloatingMotion() const { return !_isFirstPerson; }

   // Callback when material supplied to ConfigureSymbol has finished async load
   void _MaterialLoaded(UMaterialInterface* material);

   // Set the symbol to be visible or invisible.
   void _SetSymbolVisibility(bool visibility);

   // Tag listener binding to check for attached-player knock-out
   void _OnPlayerConditionChanged(const FGameplayTag tag, int32 newTagCount);

   // Timer callback for when we need to prepare to destroy this symbol
   void _OnLifespanTimeElapsed();

   // Timer callback for destroying this symbol
   void _ReadyForDestruction();

   bool _isVisible = false;
   
   bool _isFirstPerson = false;

   UPROPERTY(Transient)
   UMaterialInstanceDynamic* _dynamicMaterialInstance = nullptr;

   UPROPERTY(Transient)
   FTATThiefsignMaterialParameters _cachedMaterialParameters;

   FDelegateHandle _onHealthChangedHandle;

   FTimerHandle _lifespanTimerHandle;
   FTimerHandle _destructionTimerHandle;
};
