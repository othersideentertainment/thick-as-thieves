// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tentacle/OSETentacleNiagaraDataInterface.h"
#include "Tentacle/OSETentacleComponent.h"

// ue5
#include "NiagaraTypes.h"
#include "NiagaraSystemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSETentacleNiagaraDataInterface)


#define LOCTEXT_NAMESPACE "OSETentacleNiagaraDataInterface"


namespace TentacleUtil
{
   /// Per-instance data calculated on the game thread
   struct FPerInstanceData
   {
      FNiagaraLWCConverter             LWCConverter;
      TArray< FOSETentacleVisualData > TentacleArray;
      int32                            TentacleCount;
      bool                             TentacleValid;
   };

   struct FFuncData
   {
      FName Name;
      FText Desc;
   };

   namespace Func
   {
      static const FFuncData GetTentacleCount = { TEXT("GetTentacleCount"), LOCTEXT("GetTentacleCount", "Returns the total number of tentacles to render") };
      static const FFuncData GetTentacleData  = { TEXT("GetTentacleData"),  LOCTEXT("GetTentacleData",  "Returns all of the data for the specified tentacle") };
   }

   /// Resolves the tentacle component to use
   static UOSETentacleComponent* ResolveComponent(FNiagaraSystemInstance* systemInstance)
   {
      check(systemInstance != nullptr);
      if (USceneComponent* attachComponent = systemInstance->GetAttachComponent())
      {
         for (USceneComponent* curr = attachComponent; curr; curr = curr->GetAttachParent())
         {
            if (auto tentacleComponent = Cast<UOSETentacleComponent>(curr))
            {
               return tentacleComponent;
            }
         }
      }

      return nullptr;
   }

   /// Updates the default data to use when we want to return invalid results, making sure it is somewhat reasonable (at least in the correct location)
   static void UpdateDefaultData(FOSETentacleVisualData& outDefaultData, const FNiagaraSystemInstance* systemInstance)
   {
      outDefaultData = FOSETentacleVisualData();

      if (systemInstance != nullptr)
      {
         const FTransform& systemXfm = systemInstance->GetWorldTransform();
         const FVector unitDirVec = systemXfm.TransformVector(FVector::ForwardVector);

         const FVector bodyOrigin = systemXfm.GetLocation();
         const float bodyRadius = 50;

         const FVector rootPosition = bodyOrigin + (unitDirVec * bodyRadius);
         const float rootRadius = 10;

         // Length is needed for tip calc
         const float lengthTotal = 100;
         const float lengthCurr  = lengthTotal;
         
         const FVector tipTarget  = rootPosition + (unitDirVec * lengthTotal);
         const FVector tipCurrent = rootPosition + (unitDirVec * lengthCurr);

         outDefaultData.BodyOrigin = bodyOrigin;
         outDefaultData.BodyRadius = bodyRadius;
         outDefaultData.RootPosition = rootPosition;
         outDefaultData.RootRadius = rootRadius;
         outDefaultData.TipPositionTarget = tipTarget;
         outDefaultData.TipPositionCurrent = tipCurrent;
         outDefaultData.LengthCurrent = lengthCurr;
         outDefaultData.LengthTotal = lengthTotal;
         outDefaultData.LengthNormalized = outDefaultData.LengthCurrent / outDefaultData.LengthTotal;
         outDefaultData.RetractWeight = 1;
      }
   }

   /// Updates the instance data. Valid to use both in init and tick methods.
   static bool UpdateInstanceData(FPerInstanceData* instData, FNiagaraSystemInstance* systemInstance)
   {
      if (instData != nullptr)
      {
         instData->LWCConverter = systemInstance->GetLWCConverter();
         instData->TentacleArray.Reset();
         instData->TentacleCount = 0;
         instData->TentacleValid = false;

         if (const UOSETentacleComponent* tentacleComponent = TentacleUtil::ResolveComponent(systemInstance))
         {
            tentacleComponent->GetVisualDataArray(instData->TentacleArray);
            instData->TentacleCount = tentacleComponent->GetTentacleCount();

            if ((instData->TentacleCount > 0) && (instData->TentacleCount == instData->TentacleArray.Num()))
            {
               // We have a tentacle component and at least one tentacle
               instData->TentacleValid = true;
            }
         }

         if (!instData->TentacleValid)
         {
            // Set some reasonable default data for the one and only tentacle
            FOSETentacleVisualData defaultData;
            UpdateDefaultData(defaultData, systemInstance);

            instData->TentacleCount = 1;
            instData->TentacleArray.Reset();
            instData->TentacleArray.Add(defaultData);
         }

         // Always succeed as long as we have per-instance data
         check(instData->TentacleCount == instData->TentacleArray.Num());
         return true;
      }

      // Fail if we don't have per-instance data
      return false;
   }
}

void UOSETentacleNiagaraDataInterface::PostInitProperties()
{
   Super::PostInitProperties();

   if (HasAnyFlags(RF_ClassDefaultObject))
   {
      ENiagaraTypeRegistryFlags flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
      FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), flags);
   }
}

int32 UOSETentacleNiagaraDataInterface::PerInstanceDataSize() const
{
   return sizeof(TentacleUtil::FPerInstanceData);
}

bool UOSETentacleNiagaraDataInterface::InitPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance)
{
   if (auto* instData = new (perInstanceData) TentacleUtil::FPerInstanceData)
   {
      if (TentacleUtil::UpdateInstanceData(instData, systemInstance))
      {
         return true;
      }
   }

   return false;
}

void UOSETentacleNiagaraDataInterface::DestroyPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance)
{
   auto* instData = static_cast<TentacleUtil::FPerInstanceData*>(perInstanceData);
   instData->~FPerInstanceData();
}

bool UOSETentacleNiagaraDataInterface::PerInstanceTick(void* perInstanceData, FNiagaraSystemInstance* systemInstance, float deltaSeconds)
{
   check(systemInstance != nullptr);

   if (auto* instData = static_cast<TentacleUtil::FPerInstanceData*>(perInstanceData))
   {
      if (TentacleUtil::UpdateInstanceData(instData, systemInstance))
      {
         // Returning false means we succeeded
         return false;
      }
   }

   // Returning true means we failed and need to re-initialize.
   return true;
}

void UOSETentacleNiagaraDataInterface::GetFunctions(TArray<FNiagaraFunctionSignature>& outFunctions)
{
   FNiagaraFunctionSignature defaultSig;
   defaultSig.bMemberFunction = true;
   defaultSig.bRequiresContext = false;
   defaultSig.bSupportsGPU = false;

   // Inputs
   defaultSig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Tentacle")));

   // GetTentacleCount
   {
      FNiagaraFunctionSignature& sig = outFunctions.Add_GetRef(defaultSig);
      sig.Name = TentacleUtil::Func::GetTentacleCount.Name;
      sig.SetDescription(TentacleUtil::Func::GetTentacleCount.Desc);

      // Outputs
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Count")));
   }

   // GetTentacleData
   {
      FNiagaraFunctionSignature& sig = outFunctions.Add_GetRef(defaultSig);
      sig.Name = TentacleUtil::Func::GetTentacleData.Name;
      sig.SetDescription(TentacleUtil::Func::GetTentacleData.Desc);

      // Inputs
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));

      // Outputs
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(),     TEXT("IsValid")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("BodyOrigin")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("BodyRadius")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("RootPosition")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("RootRadius")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("TipPositionTarget")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("TipPositionCurrent")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("LengthTotal")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("LengthCurrent")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("LengthNormalized")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("ExtendWeight")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("RetractWeight")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(),    TEXT("TimeRemaining")));
   }
}

DEFINE_NDI_DIRECT_FUNC_BINDER(UOSETentacleNiagaraDataInterface, GetTentacleCount);
DEFINE_NDI_DIRECT_FUNC_BINDER(UOSETentacleNiagaraDataInterface, GetTentacleData);

void UOSETentacleNiagaraDataInterface::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& bindingInfo, void* perInstanceData, FVMExternalFunction& outFunc)
{
   if (bindingInfo.Name == TentacleUtil::Func::GetTentacleCount.Name)
   {
      check(bindingInfo.GetNumInputs() == 1 && bindingInfo.GetNumOutputs() == 1);
      NDI_FUNC_BINDER(UOSETentacleNiagaraDataInterface, GetTentacleCount)::Bind(this, outFunc);
   }
   else if (bindingInfo.Name == TentacleUtil::Func::GetTentacleData.Name)
   {
      check(bindingInfo.GetNumInputs() == 2 && bindingInfo.GetNumOutputs() == 21);
      NDI_FUNC_BINDER(UOSETentacleNiagaraDataInterface, GetTentacleData)::Bind(this, outFunc);
   }
}

void UOSETentacleNiagaraDataInterface::GetTentacleCount(FVectorVMExternalFunctionContext& context)
{
   VectorVM::FUserPtrHandler<TentacleUtil::FPerInstanceData> instanceData(context);
   const int32 tentacleCount = instanceData->TentacleCount;

   // Outputs
   FNDIOutputParam<int32> outCount(context);

   for (int32 i = 0; i < context.GetNumInstances(); ++i)
   {
      outCount.SetAndAdvance(instanceData->TentacleCount);
   }
}

void UOSETentacleNiagaraDataInterface::GetTentacleData(FVectorVMExternalFunctionContext& context)
{
   VectorVM::FUserPtrHandler<TentacleUtil::FPerInstanceData> instanceData(context);

   // References to instance data
   const TArray< FOSETentacleVisualData >& dataArray = instanceData->TentacleArray;
   const FNiagaraLWCConverter& LWCConverter = instanceData->LWCConverter;
   const int32 tentacleCount = instanceData->TentacleCount;
   const bool tentacleValid = instanceData->TentacleValid;

   // Inputs
   FNDIInputParam<int32> inIndex(context);

   // Outputs
   FNDIOutputParam<bool>               outIsValid(context);
   FNDIOutputParam<FNiagaraPosition>   outBodyOrigin(context);
   FNDIOutputParam<float>              outBodyRadius(context);
   FNDIOutputParam<FNiagaraPosition>   outRootPosition(context);
   FNDIOutputParam<float>              outRootRadius(context);
   FNDIOutputParam<FNiagaraPosition>   outTipPositionTarget(context);
   FNDIOutputParam<FNiagaraPosition>   outTipPositionCurrent(context);
   FNDIOutputParam<float>              outLengthTotal(context);
   FNDIOutputParam<float>              outLengthCurrent(context);
   FNDIOutputParam<float>              outLengthNormalized(context);
   FNDIOutputParam<float>              outExtendWeight(context);
   FNDIOutputParam<float>              outRetractWeight(context);
   FNDIOutputParam<float>              outTimeRemaining(context);

   for (int32 i = 0; i < context.GetNumInstances(); ++i)
   {
      check(tentacleCount > 0);
      check(tentacleCount == dataArray.Num());

      // Tentacle data
      const int32 idx = inIndex.GetAndAdvance() % tentacleCount;
      const FOSETentacleVisualData& data = dataArray[idx];

      // Validity
      outIsValid.SetAndAdvance(tentacleValid);

      // Body
      outBodyOrigin.SetAndAdvance(LWCConverter.ConvertWorldToSimulationPosition(data.BodyOrigin));
      outBodyRadius.SetAndAdvance(data.BodyRadius);

      // Root
      outRootPosition.SetAndAdvance(LWCConverter.ConvertWorldToSimulationPosition(data.RootPosition));
      outRootRadius.SetAndAdvance(data.RootRadius);

      // Tip
      outTipPositionTarget.SetAndAdvance(LWCConverter.ConvertWorldToSimulationPosition(data.TipPositionTarget));
      outTipPositionCurrent.SetAndAdvance(LWCConverter.ConvertWorldToSimulationPosition(data.TipPositionCurrent));

      // Length
      outLengthTotal.SetAndAdvance(data.LengthTotal);
      outLengthCurrent.SetAndAdvance(data.LengthCurrent);
      outLengthNormalized.SetAndAdvance(data.LengthNormalized);

      // State
      outExtendWeight.SetAndAdvance(data.ExtendWeight);
      outRetractWeight.SetAndAdvance(data.RetractWeight);

      // Time
      outTimeRemaining.SetAndAdvance(data.TimeRemaining);
   }
}


#undef LOCTEXT_NAMESPACE
