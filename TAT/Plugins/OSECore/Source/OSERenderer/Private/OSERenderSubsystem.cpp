// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSERenderSubsystem.h"
#include "ScreenPass/NPR/OSEScreenPassNPR.h"
#include "ScreenPass/SNN/OSEScreenPassSNN.h"
#include "ScreenPass/CustomPostProc/OSECustomPostProc.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSERenderSubsystem)


void UOSERenderSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   _sceneExtScreenPassNPR = FSceneViewExtensions::NewExtension<FOSEScreenPassNPR>(this);
   _sceneExtScreenPassSNN = FSceneViewExtensions::NewExtension<FOSEScreenPassSNN>(this);
   _sceneExtCustomPostProc = FSceneViewExtensions::NewExtension<FOSECustomPostProc>(this);
}

void UOSERenderSubsystem::Deinitialize()
{
   _sceneExtScreenPassNPR.Reset();
   _sceneExtScreenPassNPR = nullptr;
   _sceneExtScreenPassSNN.Reset();
   _sceneExtScreenPassSNN = nullptr;
   _sceneExtCustomPostProc.Reset();
   _sceneExtCustomPostProc = nullptr;

   Super::Deinitialize();
}

