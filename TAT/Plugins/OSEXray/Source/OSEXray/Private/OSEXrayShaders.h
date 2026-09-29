// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <MeshMaterialShader.h>
#include <SceneTextureParameters.h>
#include <SceneView.h>
#include <ShaderParameterStruct.h>

class FOSEXrayMaskPS : public FGlobalShader
{
public:
   DECLARE_GLOBAL_SHADER(FOSEXrayMaskPS);
   SHADER_USE_PARAMETER_STRUCT(FOSEXrayMaskPS, FGlobalShader);

   BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
      SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
      SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureParameters, SceneTextures)
      RENDER_TARGET_BINDING_SLOTS()
   END_SHADER_PARAMETER_STRUCT()
};

class FOSEXrayMeshVS : public FMeshMaterialShader
{
public:
   DECLARE_SHADER_TYPE(FOSEXrayMeshVS, MeshMaterial);

   FOSEXrayMeshVS() = default;
   FOSEXrayMeshVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& initializer);

   static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& parameters);
   static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& parameters, FShaderCompilerEnvironment& environment);

   void GetShaderBindings(const FScene* scene, ERHIFeatureLevel::Type featureLevel, const FPrimitiveSceneProxy* primitiveSceneProxy, const FMaterialRenderProxy& materialRenderProxy,
                          const FMaterial& material, const FMeshMaterialShaderElementData& shaderElementData, FMeshDrawSingleShaderBindings& shaderBindings) const;
};

class FOSEXrayMeshPS : public FMeshMaterialShader
{
public:
   DECLARE_SHADER_TYPE(FOSEXrayMeshPS, MeshMaterial);

   FOSEXrayMeshPS() = default;
   FOSEXrayMeshPS(const FMeshMaterialShaderType::CompiledShaderInitializerType& initializer);

   static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& parameters);
   static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& parameters, FShaderCompilerEnvironment& environment);

   void GetShaderBindings(const FScene* scene, ERHIFeatureLevel::Type featureLevel, const FPrimitiveSceneProxy* primitiveSceneProxy, const FMaterialRenderProxy& materialRenderProxy,
                          const FMaterial& material, const FMeshMaterialShaderElementData& shaderElementData, FMeshDrawSingleShaderBindings& shaderBindings) const;
};
