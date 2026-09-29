// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Graphics/TATPlayerGlobalMaterialParameterComponent.h"

// tat
#include "AI/UnifiedStealthSystem/TATStealthScoreInterface.h"
#include "AI/UnifiedStealthSystem/TATUnifiedStealthSettings.h"

// ue
#include "GameplayTagAssetInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerGlobalMaterialParameterComponent)

float FTATPlayerGlobalMaterialParameterSource_StealthDetectionScore::GetValue(const APlayerController* controller, float deltaTime)
{
   const ITATStealthScoreInterface* stealthScoreInterface = Cast<ITATStealthScoreInterface>(controller->GetPawn());
   if(stealthScoreInterface == nullptr)
   {
      return 0.0f;
   }
   const UTATUnifiedStealthSettings& settings = UTATUnifiedStealthSettings::Get();
   const float stealthScore = stealthScoreInterface->GetStealthScore();
   return stealthScore >= settings.MaterialShowingThreshold  ? stealthScore : 0.f;
}

float FTATPlayerGlobalMaterialParameterSource_HasTags::GetValue(const APlayerController* controller, float deltaTime)
{
   if(const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(controller->GetPawn()))
   {
      return tagInterface->HasAllMatchingGameplayTags(RequiredTags) ? 1.0f : 0.0f;
   }

   return 0.0f;
}


float FTATPlayerGlobalMaterialParameterFilter_Smooth::ProcessValue(float value, float deltaTime)
{
   if(!_hasValue)
   {
      _hasValue = true;
      _previousValue = value;
   }
   else
   {
      const float rate = value > _previousValue ? IncreaseRate : DecreaseRate;
      _previousValue = FMath::FInterpTo(_previousValue, value, deltaTime, rate);
   }

   return _previousValue;
}

float FTATPlayerGlobalMaterialParameterFilter_RemapRange::ProcessValue(float value, float deltaTime)
{
   return FMath::GetMappedRangeValueClamped(InRange, OutRange, value);
}

float FTATPlayerGlobalMaterialParameterFilter_Curve::ProcessValue(float value, float deltaTime)
{
   if (Curve)
   {
      return Curve->GetFloatValue(value);
   }
   else
   {
      return value;
   }
}

UTATPlayerGlobalMaterialParameterComponent::UTATPlayerGlobalMaterialParameterComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
}

void UTATPlayerGlobalMaterialParameterComponent::BeginPlay()
{
   Super::BeginPlay();

   _playerController = GetOwner<APlayerController>();

   if (_config)
   {
      SetComponentTickEnabled(true);
   }
}

void UTATPlayerGlobalMaterialParameterComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if(!IsValid(_playerController) || !_playerController->IsLocalPlayerController())
   {
      return;
   }

   check(IsValid(_config));

   for(FTATPlayerGlobalMaterialParameter& param : _config->Parameters)
   {
      if(!param.Source.IsValid())
      {
         continue;
      }
      
      FTATPlayerGlobalMaterialParameterSource& source = param.Source.GetMutable<FTATPlayerGlobalMaterialParameterSource>();
      float value = source.GetValue(_playerController, deltaTime);

      for (FInstancedStruct& filterStruct : param.Filters)
      {
         if (auto* filter = filterStruct.GetMutablePtr<FTATPlayerGlobalMaterialParameterFilter>())
         {
            value = filter->ProcessValue(value, deltaTime);
         }
      }

      if(UMaterialParameterCollectionInstance* instance = GetWorld()->GetParameterCollectionInstance(param.MaterialParameterCollection))
      {
         instance->SetScalarParameterValue(param.ParameterName, value);
      }
   }
}

#if WITH_EDITOR
EDataValidationResult UTATPlayerGlobalMaterialParameterConfig::IsDataValid(FDataValidationContext& context) const
{
   // TODO: check stuff
   return Super::IsDataValid(context);
}

TArray<FName> UTATPlayerGlobalMaterialParameterConfig::GetAllPropertyNames() const
{
   TArray<FName> result;

   for (const FTATPlayerGlobalMaterialParameter& param : Parameters)
   {
      if (param.MaterialParameterCollection)
      {
         param.MaterialParameterCollection->GetParameterNames(result, false);
      }
   }
   return result;
}
#endif
