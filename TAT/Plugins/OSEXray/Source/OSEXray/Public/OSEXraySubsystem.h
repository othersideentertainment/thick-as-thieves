// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <Subsystems/WorldSubsystem.h>

#include "OSEXraySubsystem.generated.h"

class FOSEXraySceneViewExtension;
class UOSEXrayComponent;

UCLASS(MinimalAPI)
class UOSEXraySubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   OSEXRAY_API virtual void Initialize(FSubsystemCollectionBase& collection) override;
   OSEXRAY_API virtual void Deinitialize() override;
   OSEXRAY_API virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

   OSEXRAY_API void RegisterComponent(UOSEXrayComponent* component) const;
   OSEXRAY_API void UnregisterComponent(UOSEXrayComponent* component) const;

private:
   TSharedPtr<FOSEXraySceneViewExtension> _sceneViewExtension;
};
