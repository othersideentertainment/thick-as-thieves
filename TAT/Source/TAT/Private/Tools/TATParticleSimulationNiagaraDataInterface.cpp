// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATParticleSimulationNiagaraDataInterface.h"

//ue4
#include "NiagaraCommon.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATParticleSimulationNiagaraDataInterface)

#define LOCTEXT_NAMESPACE "TATParticleSimulationNiagaraDataInterface"

DEFINE_LOG_CATEGORY_STATIC(LogTATParticleSimulationNiagaraDataInterface, Log, All);

bool FTATParticleSimulationNiagaraInstanceData::IsValid() const
{
   return Component.IsValid();
}

UTATParticleSimulationNiagaraDataInterface::UTATParticleSimulationNiagaraDataInterface()
{
}

void UTATParticleSimulationNiagaraDataInterface::PostInitProperties()
{
   Super::PostInitProperties();

   if (HasAnyFlags(RF_ClassDefaultObject))
   {
      ENiagaraTypeRegistryFlags flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
      FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), flags);
   }
}

bool UTATParticleSimulationNiagaraDataInterface::InitPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance)
{
   FTATParticleSimulationNiagaraInstanceData* instData = new (perInstanceData) FTATParticleSimulationNiagaraInstanceData();
   instData->Component.Reset();
   return true;
}

void UTATParticleSimulationNiagaraDataInterface::DestroyPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance)
{
   FTATParticleSimulationNiagaraInstanceData* instData = static_cast<FTATParticleSimulationNiagaraInstanceData*>(perInstanceData);
   instData->~FTATParticleSimulationNiagaraInstanceData();
}

bool UTATParticleSimulationNiagaraDataInterface::PerInstanceTick(void* perInstanceData, FNiagaraSystemInstance* systemInstance, float deltaSeconds)
{
   check(systemInstance);
   FTATParticleSimulationNiagaraInstanceData* instData = static_cast<FTATParticleSimulationNiagaraInstanceData*>(perInstanceData);

   if (instData && instData->ResetRequired(this, systemInstance))
   {
      return true;
   }

   if (!instData)
   {
      return true;
   }

   UTATParticleSimulationComponent* simComponent = instData->Component.Get();
   if (simComponent == nullptr)
   {
      if (ParticleSimComponentUserParameter.Parameter.IsValid() && instData)
      {
         UObject* userParamObject = instData->UserParamBinding.Init(systemInstance->GetInstanceParameters(), ParticleSimComponentUserParameter.Parameter);
         instData->CachedUserParam = userParamObject;
         if (userParamObject)
         {
            if (UTATParticleSimulationComponent* userSimComponent = Cast<UTATParticleSimulationComponent>(userParamObject))
            {
               if (IsValid(userSimComponent))
               {
                  simComponent = userSimComponent;
               }
            }
            else
            {
               // We have a valid, non-null UObject parameter type but it is not a type we can use.
               UE_LOG(LogTATParticleSimulationNiagaraDataInterface, Warning,
                  TEXT("TAT Particle Simulation data interface using object parameter with invalid type. This interface can only get valid data from UTATParticleSimulationComponents."));
               UE_LOG(LogTATParticleSimulationNiagaraDataInterface, Warning, TEXT("Invalid Parameter : %s"), *userParamObject->GetFullName());
               UE_LOG(LogTATParticleSimulationNiagaraDataInterface, Warning, TEXT("Niagara Component : %s"), *GetFullNameSafe(Cast<UNiagaraComponent>(systemInstance->GetAttachComponent())));
               UE_LOG(LogTATParticleSimulationNiagaraDataInterface, Warning, TEXT("System : %s"), *GetFullNameSafe(systemInstance->GetSystem()));
            }
         }
      }
      instData->Component = simComponent;
   }

   return false;
}

bool UTATParticleSimulationNiagaraDataInterface::Equals(const UNiagaraDataInterface* other) const
{
   if (!Super::Equals(other))
   {
      return false;
   }
   const UTATParticleSimulationNiagaraDataInterface* otherTyped = CastChecked<const UTATParticleSimulationNiagaraDataInterface>(other);
   return otherTyped->ParticleSimComponentUserParameter == ParticleSimComponentUserParameter;
}

static const FName GetParticleStateName("GetParticleState");
static const FName GetParticleCountName("GetParticleCount");
static const FName GetActiveParticleCountName("GetActiveParticleCount");

void UTATParticleSimulationNiagaraDataInterface::GetFunctions(TArray<FNiagaraFunctionSignature>& outFunctions)
{
   {
      FNiagaraFunctionSignature sig;
      sig.Name = GetParticleStateName;
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Sim")));
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("ParticleIndex")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("Position")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Velocity")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Alive")));
      sig.bMemberFunction = true;
      sig.bRequiresContext = false;
      //Sig.Owner = *GetName();
      sig.SetDescription(LOCTEXT("TATParticleSimulationNiagaraDataInterface_GetParticleState", "Get the current state of a simulated particle in world space."));
      outFunctions.Add(sig);
   }
   {
      FNiagaraFunctionSignature sig;
      sig.Name = GetParticleCountName;
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Sim")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Count")));
      sig.bMemberFunction = true;
      sig.bRequiresContext = false;
      //Sig.Owner = *GetName();
      sig.SetDescription(LOCTEXT("TATParticleSimulationNiagaraDataInterface_GetParticleCount", "Returns the number of simulated particles."));
      outFunctions.Add(sig);
   }
   {
      FNiagaraFunctionSignature sig;
      sig.Name = GetActiveParticleCountName;
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Sim")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Count")));
      sig.bMemberFunction = true;
      sig.bRequiresContext = false;
      //Sig.Owner = *GetName();
      sig.SetDescription(LOCTEXT("TATParticleSimulationNiagaraDataInterface_GetActiveParticleCount", "Returns the number of currently active simulated particles."));
      outFunctions.Add(sig);
   }
}

DEFINE_NDI_FUNC_BINDER(UTATParticleSimulationNiagaraDataInterface, GetParticleState);
DEFINE_NDI_FUNC_BINDER(UTATParticleSimulationNiagaraDataInterface, GetParticleCount);
DEFINE_NDI_FUNC_BINDER(UTATParticleSimulationNiagaraDataInterface, GetActiveParticleCount);

void UTATParticleSimulationNiagaraDataInterface::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& bindingInfo, void* instanceData, FVMExternalFunction& outFunc)
{
   if (bindingInfo.Name == GetParticleStateName)
   {
      check(bindingInfo.GetNumInputs() == 2 && bindingInfo.GetNumOutputs() == 7);
      TNDIParamBinder<1, int32, NDI_FUNC_BINDER(UTATParticleSimulationNiagaraDataInterface, GetParticleState)>::Bind(this, bindingInfo, instanceData, outFunc);
   }
   else if (bindingInfo.Name == GetParticleCountName)
   {
      check(bindingInfo.GetNumInputs() == 1 && bindingInfo.GetNumOutputs() == 1);
      TNDIExplicitBinder<void, NDI_FUNC_BINDER(UTATParticleSimulationNiagaraDataInterface, GetParticleCount)>::Bind(this, bindingInfo, instanceData, outFunc);
   }
   else if (bindingInfo.Name == GetActiveParticleCountName)
   {
      check(bindingInfo.GetNumInputs() == 1 && bindingInfo.GetNumOutputs() == 1);
      TNDIExplicitBinder<void, NDI_FUNC_BINDER(UTATParticleSimulationNiagaraDataInterface, GetActiveParticleCount)>::Bind(this, bindingInfo, instanceData, outFunc);
   }
}

template<typename InputT>
void UTATParticleSimulationNiagaraDataInterface::GetParticleState(FVectorVMExternalFunctionContext& context)
{
   VectorVM::FUserPtrHandler<FTATParticleSimulationNiagaraInstanceData> instData(context);

   InputT inParticleIndex(context);

   FNDIOutputParam<FNiagaraPosition> outPos(context);
   FNDIOutputParam<FVector3f> outVelocity(context);
   FNDIOutputParam<bool> outAlive(context);

   if (instData->IsValid())
   {
      UTATParticleSimulationComponent* simComponent = instData->Component.Get();
      check(simComponent != nullptr);
      for (int32 i = 0; i < context.GetNumInstances(); ++i)
      {
         const int32 particleIdx = inParticleIndex.GetAndAdvance();
         if (particleIdx >= 0 && particleIdx < simComponent->GetParticleCount())
         {
            const PuddleHelpers::FLiquidParticle& p = simComponent->GetParticleStateChecked(particleIdx);
            outPos.SetAndAdvance(FNiagaraPosition(p.Position));
            outVelocity.SetAndAdvance(FVector3f(p.Velocity));
            outAlive.SetAndAdvance(p.IsAlive());
         }
         else
         {
            outPos.SetAndAdvance(FNiagaraPosition::ZeroVector);
            outVelocity.SetAndAdvance(FVector3f::ZeroVector);
            outAlive.SetAndAdvance(false);
         }
      }
   }
   else
   {
      for (int32 i = 0; i < context.GetNumInstances(); ++i)
      {
         const int32 particleIdx = inParticleIndex.GetAndAdvance();
         outPos.SetAndAdvance(FNiagaraPosition::ZeroVector);
         outVelocity.SetAndAdvance(FVector3f::ZeroVector);
         outAlive.SetAndAdvance(false);
      }
   }
}

template<typename NoOp>
void UTATParticleSimulationNiagaraDataInterface::GetParticleCount(FVectorVMExternalFunctionContext& context)
{
   VectorVM::FUserPtrHandler<FTATParticleSimulationNiagaraInstanceData> instData(context);

   FNDIOutputParam<int32> outCount(context);

   if (instData->IsValid())
   {
      UTATParticleSimulationComponent* simComponent = instData->Component.Get();
      check(simComponent != nullptr);
      outCount.SetAndAdvance(simComponent->GetParticleCount());
   }
   else
   {
      outCount.SetAndAdvance(0);
   }
}

template<typename NoOp>
void UTATParticleSimulationNiagaraDataInterface::GetActiveParticleCount(FVectorVMExternalFunctionContext& context)
{
   VectorVM::FUserPtrHandler<FTATParticleSimulationNiagaraInstanceData> instData(context);

   FNDIOutputParam<int32> outCount(context);

   if (instData->IsValid())
   {
      UTATParticleSimulationComponent* simComponent = instData->Component.Get();
      check(simComponent != nullptr);
      outCount.SetAndAdvance(simComponent->GetActiveParticleCount());
   }
   else
   {
      outCount.SetAndAdvance(0);
   }
}

bool UTATParticleSimulationNiagaraDataInterface::CopyToInternal(UNiagaraDataInterface* destination) const
{
   if (!Super::CopyToInternal(destination))
   {
      return false;
   }

   UTATParticleSimulationNiagaraDataInterface* otherTyped = CastChecked<UTATParticleSimulationNiagaraDataInterface>(destination);
   otherTyped->ParticleSimComponentUserParameter = ParticleSimComponentUserParameter;
   return true;
}

////////////


bool FTATParticleSimulationNiagaraInstanceData::ResetRequired(UTATParticleSimulationNiagaraDataInterface* interface, FNiagaraSystemInstance* systemInstance) const
{
   if (interface->ParticleSimComponentUserParameter.Parameter.IsValid())
   {
      // Reset if the user object ptr has been changed to look at a new object
      if (UserParamBinding.GetValue() != CachedUserParam)
      {
         return true;
      }
   }

   return false;
}

#undef LOCTEXT_NAMESPACE

