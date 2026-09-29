// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(OSEGenericGraphEditor, Log, All);

/**
 * The public interface to this module
 */
class IOSEGenericGraphEditor : public IModuleInterface
{
public:
   /**
    * Singleton-like access to this module's interface.  This is just for convenience!
    * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
    *
    * @return Returns singleton instance, loading the module on demand if needed
    */
   static IOSEGenericGraphEditor& Get()
   {
      return FModuleManager::LoadModuleChecked<IOSEGenericGraphEditor>("OSEGenericGraphEditor");
   }

   /**
    * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
    *
    * @return True if the module is loaded and ready to use
    */
   static bool IsAvailable()
   {
      return FModuleManager::Get().IsModuleLoaded("OSEGenericGraphEditor");
   }
};

