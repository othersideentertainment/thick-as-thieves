// (c) 2021-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// it technically works in non-editor, but don't really want to use it there
#if WITH_EDITOR

// ue5
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

namespace BlueprintComponentScrape
{
   // Finds components in an actor blueprint added in blueprint
   // narrowly useful for certain validation
   template<typename TComponentClass, typename TFunction>
   void FindBlueprintClassComponents(const UClass* possibleClass, const TFunction& handler)
   {
      const UBlueprintGeneratedClass* blueprintClass = Cast<UBlueprintGeneratedClass>(possibleClass);
      while (blueprintClass)
      {
         USimpleConstructionScript* classSCS = blueprintClass->SimpleConstructionScript;
         if (classSCS)
         {
            // This in not available from game code, but ought to be moot from the TATBuilding use-case when called on self
            /*if (classSCS->HasAnyFlags(RF_NeedLoad))
            {
               classSCS->PreloadChain();
            }*/

            for (USCS_Node* node : classSCS->GetAllNodes())
            {
               if (TComponentClass* actualComponent = Cast<TComponentClass>(node->ComponentTemplate))
               {
                  handler(actualComponent);
               }
            }
         }

         if (UInheritableComponentHandler* ich = blueprintClass->InheritableComponentHandler)
         {
            for (auto recordIter = ich->CreateRecordIterator(); recordIter; ++recordIter)
            {
               if (TComponentClass* actualComponent = Cast<TComponentClass>(recordIter->ComponentTemplate))
               {
                  handler(actualComponent);
               }
            }
         }
         blueprintClass = Cast<UBlueprintGeneratedClass>(blueprintClass->GetSuperClass());
      }
   }
}

#endif
