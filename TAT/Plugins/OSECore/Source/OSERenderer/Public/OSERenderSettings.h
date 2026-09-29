// (c) 2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"

// OSE
#include "OSERenderSettings.generated.h"


//--------------------------------------------------------------------------------------------------
/// OSE render settings.
//--------------------------------------------------------------------------------------------------

UCLASS(Config = Engine, DefaultConfig, Const, Meta = (DisplayName = "[OSE] Render Settings"))
class OSERENDERER_API UOSERenderSettings : public UDeveloperSettingsBackedByCVars
{
   GENERATED_BODY()

public:

   static const UOSERenderSettings& Get() { return *(GetDefault<UOSERenderSettings>()); }

   UFUNCTION(BlueprintGetter)
   const TArray< TSoftObjectPtr< UMaterialInterface > >& GetCustomPostProcMaterials() const { return _customPostProcMaterials; }

protected:

   /// When enabled, custom data will be enabled for default lit materials.
   /// Changing this setting will require shaders to be recompiled.
   UPROPERTY(Config, EditAnywhere, Category = ProjectRenderSettings, meta = (ConsoleVariable = "OSE.Render.AllowCustomDataByDefault", ConfigRestartRequired = true))
   uint32 _materialsAllowCustomDataByDefault : 1;

   /// Enables or disables the non-photorealistic rendering (NPR) module
   UPROPERTY(Config, EditAnywhere, Category = NPR, meta = (ConsoleVariable = "OSE.NPR.Enabled"))
   uint32 _enableNPR : 1;

   /// Enables or disables the symmetrical nearest-neighbor (SNN) module
   UPROPERTY(Config, EditAnywhere, Category = SNN, meta = (ConsoleVariable = "OSE.SNN.Enabled"))
   uint32 _enableSNN : 1;

   /// Enables or disables the custom post proc materials module
   UPROPERTY(Config, EditAnywhere, Category = CustomPostProcess, meta = (ConsoleVariable = "OSE.CustomPostProcess.Enabled"))
   uint32 _enableCustomPostProcess : 1;

   /// Soft references to the materials we'll load for the custom post process
   UPROPERTY(Config, EditAnywhere, Category = CustomPostProcess, BlueprintGetter = GetCustomPostProcMaterials, meta = (ConfigRestartRequired = true, editcondition = "_enableCustomPostProcess"))
   TArray< TSoftObjectPtr< UMaterialInterface > > _customPostProcMaterials;
};
