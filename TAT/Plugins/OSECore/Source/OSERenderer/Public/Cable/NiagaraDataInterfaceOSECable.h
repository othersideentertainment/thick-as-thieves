// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "NiagaraParameterStore.h"
#include "NiagaraDataInterface.h"

// ose renderer
#include "OSECableComponent.h"

#include "NiagaraDataInterfaceOSECable.generated.h"

struct FNDIOSECable_InstanceData
{
   FNDIOSECable_InstanceData() : CachedUserParam(nullptr) {}

   //Cached ptr to component we sample from. 
   TWeakObjectPtr<UOSECableComponent> CableComponent;

   UObject* CachedUserParam;

   /** A binding to the user ptr we're reading cable data from */
   FNiagaraParameterDirectBinding<UObject*> UserParamBinding;

   FORCEINLINE_DEBUGGABLE bool ResetRequired(UNiagaraDataInterfaceOSECable* interface, FNiagaraSystemInstance* systemInstance) const;
   bool IsValid() const;
};

UCLASS(EditInlineNew, Category = "Cable", meta = (DisplayName = "OSE Cable"))
class OSERENDERER_API UNiagaraDataInterfaceOSECable : public UNiagaraDataInterface
{
   GENERATED_BODY()
public:
   UNiagaraDataInterfaceOSECable();

   /** Reference to a user parameter if we're reading one. This should  be an Object user parameter that is a UOSECableComponent. */
   UPROPERTY(EditAnywhere, Category = "Cable")
   FNiagaraUserParameterBinding CableUserParameter;


   //UObject Interface
   //We need this to register the type with Niagara so it shows up in the menus.
   virtual void PostInitProperties() override;
   //UObject Interface End

   //UNiagaraDataInterface Interface
   virtual bool InitPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance) override;
   virtual void DestroyPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance) override;
   virtual bool PerInstanceTick(void* perInstanceData, FNiagaraSystemInstance* systemInstance, float deltaSeconds) override;
   virtual int32 PerInstanceDataSize() const override { return sizeof(FNDIOSECable_InstanceData); }
   virtual bool CanExecuteOnTarget(ENiagaraSimTarget target) const override { return target == ENiagaraSimTarget::CPUSim; } //For now we only work on the CPU.
   virtual bool HasPreSimulateTick() const override { return true; }
   virtual bool Equals(const UNiagaraDataInterface* other) const override;

   virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& outFunctions)override;
   virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& bindingInfo, void* instanceData, FVMExternalFunction& outFunc) override;
   virtual int32 PerInstanceDataPassedToRenderThreadSize() const override { return 0; }
   //UNiagaraDataInterface Interface End

   template<typename LerpTParamType>
   void GetCableTransform(FVectorVMExternalFunctionContext& context);
   template<typename NoOp>
   void GetCableLength(FVectorVMExternalFunctionContext& context);

protected:
   

   virtual bool CopyToInternal(UNiagaraDataInterface* destination) const override;
};
