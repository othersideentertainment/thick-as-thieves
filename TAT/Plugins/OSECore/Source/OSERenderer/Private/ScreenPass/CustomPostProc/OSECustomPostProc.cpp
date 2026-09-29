// (c) 2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "ScreenPass/CustomPostProc/OSECustomPostProc.h"
#include "OSERenderSettings.h"

// UE4
#include "Runtime/Renderer/Private/PostProcess/PostProcessing.h"
#include "MaterialDomain.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECustomPostProc)


//--------------------------------------------------------------------------------------------------
/// Custom Post Process console variables
//--------------------------------------------------------------------------------------------------

namespace CustomPostProcess
{
   namespace CVar
   {
      TAutoConsoleVariable<int32> Enabled(
         TEXT("OSE.CustomPostProcess.Enabled"),
         0,
         TEXT("Enables or disables the custom post process materials\n")
         TEXT(" 0: disabled (default)\n")
         TEXT(" 1: enabled"),
         ECVF_RenderThreadSafe);
   }
}


//--------------------------------------------------------------------------------------------------
/// Custom Post Process material container
//--------------------------------------------------------------------------------------------------

void UCustomPPMaterialContainer::Load()
{
   Reset();

   // From project settings; NOT dynamic at run-time
   const TArray< TSoftObjectPtr< UMaterialInterface > >& materialArray
      = UOSERenderSettings::Get().GetCustomPostProcMaterials();

   for (auto materialSoftPtr : materialArray)
   {
      if (UMaterialInterface* matInterface = materialSoftPtr.LoadSynchronous())
      {
         if (const UMaterial* material = matInterface->GetMaterial())
         {
            if (material->MaterialDomain != MD_PostProcess)
               continue;

            if (UMaterialInstanceDynamic* mid = UMaterialInstanceDynamic::Create(matInterface, this))
            {
               _materialInstances.AddUnique(mid);
            }
         }
      }
   }
}


//--------------------------------------------------------------------------------------------------
/// Custom Post Process Scene View Extension
//--------------------------------------------------------------------------------------------------

FOSECustomPostProc::FOSECustomPostProc(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem)
   : FOSESceneViewExtension(inAutoRegister, inRenderSubsystem)
{
   IsActiveThisFrameFunctions.Add(FSceneViewExtensionIsActiveFunctor());
   IsActiveThisFrameFunctions.Last().IsActiveFunction = 
      [](const ISceneViewExtension* extension, const FSceneViewExtensionContext& context) -> TOptional<bool>
      {
         const bool isEnabled = CustomPostProcess::CVar::Enabled.GetValueOnAnyThread() != 0;
         return isEnabled;
      };

   _ppMaterials = NewObject<UCustomPPMaterialContainer>();
   _ppMaterials->Load();
}

FOSECustomPostProc::~FOSECustomPostProc()
{
   _ppMaterials->Reset();
   _ppMaterials = nullptr;
}

void FOSECustomPostProc::SetupView(FSceneViewFamily& inViewFamily, FSceneView& inView)
{
   if (ensure(_ppMaterials != nullptr))
   {
      FBlendableManager& blendMgr = inView.FinalPostProcessSettings.BlendableManager;
      for (auto instanceDynamic : _ppMaterials->_materialInstances)
      {
         if (instanceDynamic != nullptr)
         {
            const UMaterial* asMaterial = instanceDynamic->GetMaterial();
            FPostProcessMaterialNode materialNode(instanceDynamic, asMaterial->BlendableLocation, asMaterial->BlendablePriority, asMaterial->bIsBlendable);
            blendMgr.PushBlendableData(1.0f, materialNode);
         }
      }
   }
}

void FOSECustomPostProc::AddReferencedObjects(FReferenceCollector& inCollector)
{
   inCollector.AddReferencedObject(_ppMaterials);
}

