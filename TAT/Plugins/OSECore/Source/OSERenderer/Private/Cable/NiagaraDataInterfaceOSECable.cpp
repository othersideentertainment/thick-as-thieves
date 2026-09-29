// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT



#include "Cable/NiagaraDataInterfaceOSECable.h"

//ue4
#include "NiagaraCommon.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NiagaraDataInterfaceOSECable)

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceOSECable"
DEFINE_LOG_CATEGORY_STATIC(LogNiagaraDataInterfaceOSECable, Log, All);

UNiagaraDataInterfaceOSECable::UNiagaraDataInterfaceOSECable()
{}

void UNiagaraDataInterfaceOSECable::PostInitProperties()
{
   Super::PostInitProperties();

   if (HasAnyFlags(RF_ClassDefaultObject))
   {
      ENiagaraTypeRegistryFlags flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
      FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), flags);
   }
}

bool UNiagaraDataInterfaceOSECable::InitPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance)
{
   FNDIOSECable_InstanceData* instData = new (perInstanceData) FNDIOSECable_InstanceData();

   instData->CableComponent.Reset();

   return true;
}

void UNiagaraDataInterfaceOSECable::DestroyPerInstanceData(void* perInstanceData, FNiagaraSystemInstance* systemInstance)
{
   FNDIOSECable_InstanceData* instData = (FNDIOSECable_InstanceData*)perInstanceData;
   instData->~FNDIOSECable_InstanceData();
}

bool UNiagaraDataInterfaceOSECable::PerInstanceTick(void* perInstanceData, FNiagaraSystemInstance* systemInstance, float deltaSeconds)
{
   check(systemInstance);
   FNDIOSECable_InstanceData* instData = (FNDIOSECable_InstanceData*)perInstanceData;

   if (instData && instData->ResetRequired(this, systemInstance))
   {
      return true;
   }

   if (!instData)
   {
      return true;
   }

   UOSECableComponent* cable = instData->CableComponent.Get();
   if (cable == nullptr)
   {
      if (CableUserParameter.Parameter.IsValid() && instData)
      {
         UObject* userParamObject = instData->UserParamBinding.Init(systemInstance->GetInstanceParameters(), CableUserParameter.Parameter);
         instData->CachedUserParam = userParamObject;
         if (userParamObject)
         {
            if (UOSECableComponent* userCableComponent = Cast<UOSECableComponent>(userParamObject))
            {
               if (IsValid(userCableComponent))
               {
                  cable = userCableComponent;
               }
            }
            else
            {
               //We have a valid, non-null UObject parameter type but it is not a type we can use.
               UE_LOG(LogNiagaraDataInterfaceOSECable, Warning, TEXT("Cable data interface using object parameter with invalid type. OSE Cable Interfaces can only get a valid Cable from UOSECableComponents."));
               UE_LOG(LogNiagaraDataInterfaceOSECable, Warning, TEXT("Invalid Parameter : %s"), *userParamObject->GetFullName());
               UE_LOG(LogNiagaraDataInterfaceOSECable, Warning, TEXT("Niagara Component : %s"), *GetFullNameSafe(Cast<UNiagaraComponent>(systemInstance->GetAttachComponent())));
               UE_LOG(LogNiagaraDataInterfaceOSECable, Warning, TEXT("System : %s"), *GetFullNameSafe(systemInstance->GetSystem()));
            }
         }
      }
      instData->CableComponent = cable;
   }

   return false;
}

bool UNiagaraDataInterfaceOSECable::Equals(const UNiagaraDataInterface* other) const
{
   if (!Super::Equals(other))
   {
      return false;
   }

   const UNiagaraDataInterfaceOSECable* OtherTyped = CastChecked<const UNiagaraDataInterfaceOSECable>(other);
   return OtherTyped->CableUserParameter == CableUserParameter;
}

static const FName GetCableTransformName("GetCableTransform");
static const FName GetCableLengthName("GetCableLength");

void UNiagaraDataInterfaceOSECable::GetFunctions(TArray<FNiagaraFunctionSignature>& outFunctions)
{
   {
      FNiagaraFunctionSignature sig;
      sig.Name = GetCableTransformName;
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Cable")));
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("T")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetQuatDef(), TEXT("Rotation")));
      sig.bMemberFunction = true;
      sig.bRequiresContext = false;
      //Sig.Owner = *GetName();
      sig.SetDescription(LOCTEXT("NiagaraDataInterfaceOSECable_GetCableTransform", "Linearly Interpolate Along an OSECable and retreive it's transform in world space."));
      outFunctions.Add(sig);
   }
   {
      FNiagaraFunctionSignature sig;
      sig.Name = GetCableLengthName;
      sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Cable")));
      sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Length")));
      sig.bMemberFunction = true;
      sig.bRequiresContext = false;
      //Sig.Owner = *GetName();
      sig.SetDescription(LOCTEXT("NiagaraDataInterfaceOSECable_GetCableLength", "Returns the length in worldspace units of the cable."));
      outFunctions.Add(sig);
   }
}

DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceOSECable, GetCableTransform);
DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceOSECable, GetCableLength);

void UNiagaraDataInterfaceOSECable::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& bindingInfo, void* instanceData, FVMExternalFunction& outFunc)
{
   if (bindingInfo.Name == GetCableTransformName)
   {
      check(bindingInfo.GetNumInputs() == 2 && bindingInfo.GetNumOutputs() == 7);
      TNDIParamBinder<1, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceOSECable, GetCableTransform)>::Bind(this, bindingInfo, instanceData, outFunc);
   }
   else if (bindingInfo.Name == GetCableLengthName)
   {
      check(bindingInfo.GetNumInputs() == 2 && bindingInfo.GetNumOutputs() == 1);
      TNDIExplicitBinder<void, NDI_FUNC_BINDER(UNiagaraDataInterfaceOSECable, GetCableLength)>::Bind(this, bindingInfo, instanceData, outFunc);
   }
   
}

template<typename LerpTParamType>
void UNiagaraDataInterfaceOSECable::GetCableTransform(FVectorVMExternalFunctionContext& context)
{
   VectorVM::FUserPtrHandler<FNDIOSECable_InstanceData> instData(context);

   LerpTParamType tParam(context);
   
   // UE5 port: doubles instead of floats?
   FNDIOutputParam<FVector3f> outPos(context);
   FNDIOutputParam<FQuat4f> outRot(context);

   if (instData->IsValid())
   {
      UOSECableComponent* cable = instData->CableComponent.Get();
      for (int32 i = 0; i < context.GetNumInstances(); ++i)
      {
         float t = tParam.GetAndAdvance() * cable->NumSegments;

         //TODO: There's a SIMD way to do this.
         int aIndex = FMath::Clamp(static_cast<int>(FMath::Floor(t)), 0, cable->NumSegments - 1);
         int bIndex = FMath::Clamp(FMath::CeilToInt(t), 0, cable->NumSegments - 1);
         float tLerp = t - aIndex;

         FTransform result;
         result.Blend(cable->GetCableTransformAtIndex(aIndex), cable->GetCableTransformAtIndex(bIndex), tLerp);
         
         outPos.SetAndAdvance(FVector3f(result.GetLocation()));
         outRot.SetAndAdvance(FQuat4f(result.GetRotation()));
      }
   }
   else
   {
      for (int32 i = 0; i < context.GetNumInstances(); ++i)
      {
         float t = tParam.GetAndAdvance();
         
         outPos.SetAndAdvance(FVector3f::ZeroVector);
         outRot.SetAndAdvance(FQuat4f::Identity);
      }
   }
}

template <typename NoOp>
void UNiagaraDataInterfaceOSECable::GetCableLength(FVectorVMExternalFunctionContext& context)
{
   VectorVM::FUserPtrHandler<FNDIOSECable_InstanceData> instData(context);

   FNDIOutputParam<float> outLength(context);

   if (instData->IsValid())
   {
      UOSECableComponent* cable = instData->CableComponent.Get();
      outLength.SetAndAdvance(cable->GetCableTotalLength());
   }
   else
   {
      outLength.SetAndAdvance(0);
   }
}

bool UNiagaraDataInterfaceOSECable::CopyToInternal(UNiagaraDataInterface* destination) const
{
   if (!Super::CopyToInternal(destination))
   {
      return false;
   }

   UNiagaraDataInterfaceOSECable* otherTyped = CastChecked<UNiagaraDataInterfaceOSECable>(destination);
   otherTyped->CableUserParameter = CableUserParameter;
   return true;
}

////////////


bool FNDIOSECable_InstanceData::ResetRequired(UNiagaraDataInterfaceOSECable* interface, FNiagaraSystemInstance* systemInstance) const
{
   if (interface->CableUserParameter.Parameter.IsValid())
   {
      // Reset if the user object ptr has been changed to look at a new object
      if (UserParamBinding.GetValue() != CachedUserParam)
      {
         return true;
      }
   }

   return false;
}

bool FNDIOSECable_InstanceData::IsValid() const
{
   return CableComponent.IsValid();
}

#undef LOCTEXT_NAMESPACE

