// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Modules/ModuleManager.h"
#include "Logging/LogCategory.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSEMetrics, Log, All);

/**
 * The public interface to this module
 */
class IOSEMetrics : public IModuleInterface
{
public:
   /**
    * Singleton-like access to this module's interface.  This is just for convenience!
    * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
    *
    * @return Returns singleton instance, loading the module on demand if needed
    */
   static IOSEMetrics& Get()
   {
      return FModuleManager::LoadModuleChecked<IOSEMetrics>("OSEMetrics");
   }

   /**
    * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
    *
    * @return True if the module is loaded and ready to use
    */
   static bool IsAvailable()
   {
      return FModuleManager::Get().IsModuleLoaded("OSEMetrics");
   }
};

