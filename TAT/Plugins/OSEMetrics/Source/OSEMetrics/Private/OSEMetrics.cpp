// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEMetrics.h"

DEFINE_LOG_CATEGORY(LogOSEMetrics);

class FOSEMetrics : public IOSEMetrics
{
   // From IModuleInterface
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FOSEMetrics, OSEMetrics)


void FOSEMetrics::StartupModule()
{
   // This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FOSEMetrics::ShutdownModule()
{
   // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
   // we call this function before unloading the module.
}



