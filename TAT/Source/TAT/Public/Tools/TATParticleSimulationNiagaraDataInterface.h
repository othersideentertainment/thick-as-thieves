// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Tools/TATParticleSimulationComponent.h"

// ue
#include "CoreMinimal.h"
#include "NiagaraParameterStore.h"
#include "NiagaraDataInterface.h"

#include "TATParticleSimulationNiagaraDataInterface.generated.h"

class UTATParticleSimulationNiagaraDataInterface;

struct FTATParticleSimulationNiagaraInstanceData
{
   TWeakObjectPtr<UTATParticleSimulationComponent> Component;

   UObject* CachedUserParam = nullptr;

   /// A binding to the user ptr we're reading particle data from
   FNiagaraParameterDirectBinding<UObject*> UserParamBinding;

   bool ResetRequired(UTATParticleSimulationNiagaraDataInterface* interface, FNiagaraSystemInstance* systemInstance) const;

   bool IsValid() const;
};

UCLASS(EditInlineNew, Category = "TAT", meta = (DisplayName = "TAT Particle Simulation Component"))
class TAT_API UTATParticleSimulationNiagaraDataInterface : public UNiagaraDataInterface
{
   GENERATED_BODY()

public:
   UTATParticleSimulationNiagaraDataInterface();

   /// Reference to a user parameter if we're reading one. This should be an Object user parameter that is a UTATParticleSimulationComponent.
   UPROPERTY(EditAnywhere, Category = "Particle Simulation Component")
   FNiagaraUserParameterBinding ParticleSimComponentUserParameter;

   //UObject Interface
   //We need this to register the type with Niagara so it shows up in the menus.
   virtual void PostInitProperties() override;
   //UObject Interface End

   //UNiagaraDataInterface Interface
   virtual bool InitPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance) override;
   virtual void DestroyPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance) override;
   virtual bool PerInstanceTick(void* perInstanceData, FNiagaraSystemInstance* systemInstance, float deltaSeconds) override;
   virtual int32 PerInstanceDataSize() const override { return sizeof(FTATParticleSimulationNiagaraInstanceData); }
   virtual bool CanExecuteOnTarget(ENiagaraSimTarget target) const override { return target == ENiagaraSimTarget::CPUSim; } //For now we only work on the CPU.
   virtual bool HasPreSimulateTick() const override { return true; }
   virtual bool Equals(const UNiagaraDataInterface* other) const override;

   virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& outFunctions)override;
   virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& bindingInfo, void* instanceData, FVMExternalFunction& outFunc) override;
   virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return 0; }
   //UNiagaraDataInterface Interface End

   template<typename InputT>
   void GetParticleState(FVectorVMExternalFunctionContext& context);
   template<typename NoOp>
   void GetParticleCount(FVectorVMExternalFunctionContext& context);
   template<typename NoOp>
   void GetActiveParticleCount(FVectorVMExternalFunctionContext& context);

protected:
   virtual bool CopyToInternal(UNiagaraDataInterface* destination) const override;
};
