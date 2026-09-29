// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ScreenPass/OSESceneViewExtension.h"
#include "OSERenderSubsystem.h"


FOSESceneViewExtension::FOSESceneViewExtension(const FAutoRegister& inAutoRegister, UOSERenderSubsystem* inRenderSubsystem)
   : FSceneViewExtensionBase(inAutoRegister)
   , _renderSubsystem(inRenderSubsystem)
{
   // The default scene view extension will turn it off when the world contexts don't match, but otherwise
   // will leave it up to derived classes by allowing them to add their own functor.
   IsActiveThisFrameFunctions.Add(FSceneViewExtensionIsActiveFunctor());
   IsActiveThisFrameFunctions.Last().IsActiveFunction =
      [this](const ISceneViewExtension* extension, const FSceneViewExtensionContext& context) -> TOptional<bool>
   {
      if (ensure(this == extension))
      {
         if (this->GetWorld() != context.GetWorld())
         {
            // Not the expected world
            return false;
         }
      }
      else
      {
         // Not the expected scene view extension
         return false;
      }

      return TOptional<bool>();
   };
}

FOSESceneViewExtension::~FOSESceneViewExtension()
{
   _renderSubsystem.Reset();
}

// Returns the world associated with the render subsystem
const UWorld* FOSESceneViewExtension::GetWorld() const
{
   if (const UOSERenderSubsystem* renderSys = GetRenderSubsystem())
   {
      return renderSys->GetWorld();
   }

   return nullptr;
}
