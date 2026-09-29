// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATTokenFactories.h"

// tat
#include "Items/Tokens/TATInventoryToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTokenFactories)

UTATInventoryTokenFactory::UTATInventoryTokenFactory()
{
   SupportedClass = UTATInventoryToken::StaticClass();
   bCreateNew = true;
   bEditAfterNew = true;
}

UObject* UTATInventoryTokenFactory::FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn)
{
   check(cls == SupportedClass);

   return NewObject<UTATInventoryToken>(parent, name, flags);
}
