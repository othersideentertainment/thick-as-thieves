// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEXrayShaders.h"

// UE
#include <GBufferInfo.h>

IMPLEMENT_GLOBAL_SHADER(FOSEXrayMaskPS, "/OSEXray/XrayMask.usf", "MainPS", SF_Pixel);

FOSEXrayMeshVS::FOSEXrayMeshVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& initializer)
   : FMeshMaterialShader(initializer)
{
}

bool FOSEXrayMeshVS::ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& parameters)
{
   return IsFeatureLevelSupported(parameters.Platform, ERHIFeatureLevel::SM5);
}

void FOSEXrayMeshVS::ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& parameters, FShaderCompilerEnvironment& environment)
{
   FMeshMaterialShader::ModifyCompilationEnvironment(parameters, environment);
}

void FOSEXrayMeshVS::GetShaderBindings(const FScene* scene, ERHIFeatureLevel::Type featureLevel, const FPrimitiveSceneProxy* primitiveSceneProxy, const FMaterialRenderProxy& materialRenderProxy,
                                       const FMaterial& material, const FMeshMaterialShaderElementData& shaderElementData, FMeshDrawSingleShaderBindings& shaderBindings) const
{
   FMeshMaterialShader::GetShaderBindings(scene, featureLevel, primitiveSceneProxy, materialRenderProxy, material, shaderElementData, shaderBindings);
}

IMPLEMENT_MATERIAL_SHADER_TYPE(, FOSEXrayMeshVS, TEXT("/OSEXray/XrayMesh.usf"), TEXT("MainVS"), SF_Vertex);

FOSEXrayMeshPS::FOSEXrayMeshPS(const FMeshMaterialShaderType::CompiledShaderInitializerType& initializer)
   : FMeshMaterialShader(initializer)
{
}

bool FOSEXrayMeshPS::ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& parameters)
{
   return IsFeatureLevelSupported(parameters.Platform, ERHIFeatureLevel::SM5);
}

void FOSEXrayMeshPS::ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& parameters, FShaderCompilerEnvironment& environment)
{
   FMeshMaterialShader::ModifyCompilationEnvironment(parameters, environment);

   environment.SetDefine(TEXT("GBUFFER_LAYOUT"), GBL_ForceVelocity);
}

void FOSEXrayMeshPS::GetShaderBindings(const FScene* scene, ERHIFeatureLevel::Type featureLevel, const FPrimitiveSceneProxy* primitiveSceneProxy, const FMaterialRenderProxy& materialRenderProxy,
                                       const FMaterial& material, const FMeshMaterialShaderElementData& shaderElementData, FMeshDrawSingleShaderBindings& shaderBindings) const
{
   FMeshMaterialShader::GetShaderBindings(scene, featureLevel, primitiveSceneProxy, materialRenderProxy, material, shaderElementData, shaderBindings);
}

IMPLEMENT_MATERIAL_SHADER_TYPE(, FOSEXrayMeshPS, TEXT("/OSEXray/XrayMesh.usf"), TEXT("MainPS"), SF_Pixel);
