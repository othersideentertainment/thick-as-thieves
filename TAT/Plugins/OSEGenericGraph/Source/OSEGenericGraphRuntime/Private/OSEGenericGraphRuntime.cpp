// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraphRuntimePCH.h"

DEFINE_LOG_CATEGORY(OSEGenericGraphRuntime)

class FOSEGenericGraphRuntime : public IOSEGenericGraphRuntime
{
   /** IModuleInterface implementation */
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FOSEGenericGraphRuntime, OSEGenericGraphRuntime )



void FOSEGenericGraphRuntime::StartupModule()
{
   // This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FOSEGenericGraphRuntime::ShutdownModule()
{
   // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
   // we call this function before unloading the module.
}



