// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEXraySubsystem.h"

// OSE
#include "OSEXraySceneViewExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEXraySubsystem)

void UOSEXraySubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   _sceneViewExtension = FSceneViewExtensions::NewExtension<FOSEXraySceneViewExtension>();
}

void UOSEXraySubsystem::Deinitialize()
{
   Super::Deinitialize();

   _sceneViewExtension.Reset();
}

bool UOSEXraySubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}

void UOSEXraySubsystem::RegisterComponent(UOSEXrayComponent* component) const
{
   check(component);
   if (_sceneViewExtension)
   {
      _sceneViewExtension->RegisteredComponents.Add(component);
   }
}

void UOSEXraySubsystem::UnregisterComponent(UOSEXrayComponent* component) const
{
   check(component);
   if (_sceneViewExtension)
   {
      _sceneViewExtension->RegisteredComponents.Remove(component);
   }
}
