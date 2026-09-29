// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "NiagaraShared.h"
#include "NiagaraDataInterface.h"
#include "OSETentacleNiagaraDataInterface.generated.h"


UCLASS(EditInlineNew, Category = "Tentacle", meta = (DisplayName = "OSE Tentacle Interface"))
class OSERENDERER_API UOSETentacleNiagaraDataInterface : public UNiagaraDataInterface
{
   GENERATED_BODY()

public:

   virtual void PostInitProperties() override;

   virtual int32 PerInstanceDataSize() const override;

   virtual bool InitPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance) override;

   virtual void DestroyPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance) override;

   virtual bool PerInstanceTick(void* perInstanceData, FNiagaraSystemInstance* systemInstance, float deltaSeconds) override;

   virtual bool CanExecuteOnTarget(ENiagaraSimTarget target) const override { return target == ENiagaraSimTarget::CPUSim; }

   virtual bool HasPreSimulateTick() const override { return true; }

   //---------------------------------------------------------------------------------------
   // VM functions and binding
   //---------------------------------------------------------------------------------------
   
   virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& outFunctions) override;
   virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& bindingInfo, void* perInstanceData, FVMExternalFunction& outFunc) override;

   void GetTentacleCount(FVectorVMExternalFunctionContext& context);
   void GetTentacleData(FVectorVMExternalFunctionContext& context);
};
