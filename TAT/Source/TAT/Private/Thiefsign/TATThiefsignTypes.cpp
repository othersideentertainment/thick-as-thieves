// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Thiefsign/TATThiefsignTypes.h"

// ue
#include "Engine/AssetManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefsignTypes)

namespace TATThiefsignMaterialParametersHelpers
{
   TFunction<bool(FTATThiefsignMaterialParamKey)> GetPerspectiveConditionCheck(bool isFirstPerson)
   {
      static TFunction<bool(FTATThiefsignMaterialParamKey)> firstPersonCheck = [](const FTATThiefsignMaterialParamKey& paramKey)
      {
         return paramKey.Perspective == ETATThiefsignMaterialParamPerspective::FirstAndThirdPerson 
            || paramKey.Perspective == ETATThiefsignMaterialParamPerspective::FirstPerson;
      };
      static TFunction<bool(FTATThiefsignMaterialParamKey)> thirdPersonCheck = [](const FTATThiefsignMaterialParamKey& paramKey)
      {
         return paramKey.Perspective == ETATThiefsignMaterialParamPerspective::FirstAndThirdPerson
            || paramKey.Perspective == ETATThiefsignMaterialParamPerspective::ThirdPerson;
      };

      return (isFirstPerson) ? firstPersonCheck : thirdPersonCheck;
   }
}

bool FTATThiefsignVFXConfig::IsValid() const
{
   return !MaterialClass.IsNull() || !NiagaraSystemClass.IsNull();
}

void FTATThiefsignVFXConfig::Merge(const FTATThiefsignVFXConfig& overrides)
{
   // Only combine params if we're not overwriting classes
   // If we are overwriting classes, our original params may not pertain to overwritten class

   if (!overrides.MaterialClass.IsNull())
   {
      MaterialClass = overrides.MaterialClass;
      MaterialParams = overrides.MaterialParams;
   }
   else
   {
      MaterialParams.AppendParams(overrides.MaterialParams);
   }

   if (!overrides.NiagaraSystemClass.IsNull())
   {
      NiagaraSystemClass = overrides.NiagaraSystemClass;
      NiagaraSystemParams = overrides.NiagaraSystemParams;
   }
   else
   {
      NiagaraSystemParams.AppendParams(overrides.NiagaraSystemParams);
   }
}

void FTATThiefsignMaterialParameters::AppendParams(const FTATThiefsignMaterialParameters& other)
{
   for (auto& param : other.TextureParameters)
   {
      TextureParameters.FindOrAdd(param.Key, param.Value);
   }

   for (auto& param : other.ColorParameters)
   {
      ColorParameters.FindOrAdd(param.Key, param.Value);
   }

   for (auto& param : other.FloatParameters)
   {
      FloatParameters.FindOrAdd(param.Key, param.Value);
   }
}

void FTATThiefsignMaterialParameters::ApplyToMaterial(UMaterialInstanceDynamic* material, bool isFirstPerson, TFunction<void()>&& applyFinishedCallback) const
{
   const TFunction<bool(FTATThiefsignMaterialParamKey)> conditionCheck = TATThiefsignMaterialParametersHelpers::GetPerspectiveConditionCheck(isFirstPerson);

   TArray<FSoftObjectPath> softObjectPaths;
   softObjectPaths.Reserve(TextureParameters.Num());
   for (const auto& param : TextureParameters)
   {
      softObjectPaths.Add(param.Value.ToSoftObjectPath());
   }
   if (!softObjectPaths.IsEmpty())
   {
      TWeakObjectPtr<UMaterialInstanceDynamic> weakMaterialInstance(material);
      UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(softObjectPaths), [weakMaterialInstance, textureParameters = TextureParameters, conditionCheck, applyFinishedCallback]
      {
         if (weakMaterialInstance.IsValid())
         {
            for (const auto& param : textureParameters)
            {
               if (conditionCheck(param.Key))
               {
                  weakMaterialInstance->SetTextureParameterValue(param.Key.ParamName, param.Value.Get());
               }
            }
         }

         if (applyFinishedCallback)
         {
            applyFinishedCallback();
         }
      });
   }

   for (const auto& param : ColorParameters)
   {
      if (conditionCheck(param.Key))
      {
         material->SetVectorParameterValue(param.Key.ParamName, param.Value);
      }
   }

   for (const auto& param : FloatParameters)
   {
      if (conditionCheck(param.Key))
      {
         material->SetScalarParameterValue(param.Key.ParamName, param.Value);
      }
   }
}

void FTATThiefsignNiagaraParameters::AppendParams(const FTATThiefsignNiagaraParameters& other)
{
   for (auto& param : other.TextureParameters)
   {
      TextureParameters.FindOrAdd(param.Key, param.Value);
   }

   for (auto& param : other.ColorParameters)
   {
      ColorParameters.FindOrAdd(param.Key, param.Value);
   }

   for (auto& param : other.FloatParameters)
   {
      FloatParameters.FindOrAdd(param.Key, param.Value);
   }

   for (auto& param : other.IntegerParameters)
   {
      IntegerParameters.FindOrAdd(param.Key, param.Value);
   }
}

void FTATThiefsignNiagaraParameters::ApplyToNiagaraSystem(UNiagaraComponent* niagaraSystem, bool isFirstPerson, TFunction<void()>&& applyFinishedCallback) const
{
   const TFunction<bool(FTATThiefsignMaterialParamKey)> conditionCheck = TATThiefsignMaterialParametersHelpers::GetPerspectiveConditionCheck(isFirstPerson);

   TArray<FSoftObjectPath> softObjectPaths;
   softObjectPaths.Reserve(TextureParameters.Num());
   for (const auto& param : TextureParameters)
   {
      softObjectPaths.Add(param.Value.ToSoftObjectPath());
   }
   if (!softObjectPaths.IsEmpty())
   {
      TWeakObjectPtr<UNiagaraComponent> weakNiagaraSystem(niagaraSystem);
      UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(softObjectPaths), [weakNiagaraSystem, textureParameters = TextureParameters, conditionCheck, applyFinishedCallback]
      {
         if (weakNiagaraSystem.IsValid())
         {
            for (const auto& param : textureParameters)
            {
               if (conditionCheck(param.Key))
               {
                  weakNiagaraSystem->SetVariableTexture(param.Key.ParamName, param.Value.Get());
               }
            }
         }

         if (applyFinishedCallback)
         {
            applyFinishedCallback();
         }
      });
   }

   for (const auto& param : ColorParameters)
   {
      if (conditionCheck(param.Key))
      {
         niagaraSystem->SetVariableLinearColor(param.Key.ParamName, param.Value);
      }
   }

   for (const auto& param : FloatParameters)
   {
      if (conditionCheck(param.Key))
      {
         niagaraSystem->SetVariableFloat(param.Key.ParamName, param.Value);
      }
   }

   for (const auto& param : IntegerParameters)
   {
      if (conditionCheck(param.Key))
      {
         niagaraSystem->SetVariableInt(param.Key.ParamName, param.Value);
      }
   }
}
