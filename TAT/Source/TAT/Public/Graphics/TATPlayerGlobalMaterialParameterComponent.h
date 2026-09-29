// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Components/ActorComponent.h"

#include "TATPlayerGlobalMaterialParameterComponent.generated.h"


USTRUCT()
struct FTATPlayerGlobalMaterialParameterSource
{
   GENERATED_BODY()
   virtual ~FTATPlayerGlobalMaterialParameterSource() = default;
   virtual float GetValue(const APlayerController* controller, float deltaTime) { unimplemented(); return 0.0f; }
};

USTRUCT(DisplayName="Stealth Detection Score")
struct FTATPlayerGlobalMaterialParameterSource_StealthDetectionScore : public FTATPlayerGlobalMaterialParameterSource
{
   GENERATED_BODY()

   virtual float GetValue(const APlayerController* controller, float deltaTime) override;
};

USTRUCT(DisplayName="Has Tags")
struct FTATPlayerGlobalMaterialParameterSource_HasTags : public FTATPlayerGlobalMaterialParameterSource
{
   GENERATED_BODY()

   virtual float GetValue(const APlayerController* controller, float deltaTime) override;

   UPROPERTY(EditAnywhere)
   FGameplayTagContainer RequiredTags;
};

USTRUCT()
struct FTATPlayerGlobalMaterialParameterFilter
{
   GENERATED_BODY()
   virtual ~FTATPlayerGlobalMaterialParameterFilter() = default;
   virtual float ProcessValue(float value, float deltaTime) { unimplemented(); return value; }
};

USTRUCT(DisplayName="Smooth")
struct FTATPlayerGlobalMaterialParameterFilter_Smooth : public FTATPlayerGlobalMaterialParameterFilter
{
   GENERATED_BODY()

   virtual float ProcessValue(float value, float deltaTime) override;

   UPROPERTY(EditAnywhere)
   float IncreaseRate = 5.f;
   
   UPROPERTY(EditAnywhere)
   float DecreaseRate = 5.f;

private:
   float _previousValue = -1;
   bool _hasValue = false;
};

USTRUCT(DisplayName="Remap Range")
struct FTATPlayerGlobalMaterialParameterFilter_RemapRange : public FTATPlayerGlobalMaterialParameterFilter
{
   GENERATED_BODY()

   virtual float ProcessValue(float value, float deltaTime) override;

   UPROPERTY(EditAnywhere)
   FFloatRange InRange = FFloatRange{ 0, 1 };
   
   UPROPERTY(EditAnywhere)
   FFloatRange OutRange = FFloatRange{ 0, 1 };
};

USTRUCT(DisplayName="Invert")
struct FTATPlayerGlobalMaterialParameterFilter_Invert : public FTATPlayerGlobalMaterialParameterFilter
{
   GENERATED_BODY()

   virtual float ProcessValue(float value, float deltaTime) override { return 1 - value; }
};

USTRUCT(DisplayName="Curve")
struct FTATPlayerGlobalMaterialParameterFilter_Curve : public FTATPlayerGlobalMaterialParameterFilter
{
   GENERATED_BODY()

   virtual float ProcessValue(float value, float deltaTime) override;

   UPROPERTY(EditAnywhere)
   TObjectPtr<UCurveFloat> Curve = nullptr;
};

USTRUCT()
struct FTATPlayerGlobalMaterialParameter
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   TObjectPtr<UMaterialParameterCollection> MaterialParameterCollection = nullptr;

   UPROPERTY(EditAnywhere, meta = (GetOptions = "GetAllPropertyNames"))
   FName ParameterName;

   UPROPERTY(EditAnywhere, meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATPlayerGlobalMaterialParameterSource"))
   FInstancedStruct Source;

   // optional remapping curve
   UPROPERTY(EditAnywhere, meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATPlayerGlobalMaterialParameterFilter"))
   TArray<FInstancedStruct> Filters;
};

UCLASS()
class UTATPlayerGlobalMaterialParameterConfig : public UDataAsset
{
   GENERATED_BODY()

public:
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

   // just for GetOptions meta
   UFUNCTION()
   TArray<FName> GetAllPropertyNames() const;
#endif

   UPROPERTY(EditDefaultsOnly)
   TArray<FTATPlayerGlobalMaterialParameter> Parameters;
};

// A helper component for updating MaterialParameterCollection parameters that:
// 1. Are reasonably pollable via the Pawn/Controller
// 2. Are reasonably expected to change continously enough that polling is appropriate
//
// Should be on the player controller
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATPlayerGlobalMaterialParameterComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATPlayerGlobalMaterialParameterComponent();

protected:
   virtual void BeginPlay() override;

public:
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

private:
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATPlayerGlobalMaterialParameterConfig> _config;

   UPROPERTY(Transient)
   TObjectPtr<APlayerController> _playerController;
};
