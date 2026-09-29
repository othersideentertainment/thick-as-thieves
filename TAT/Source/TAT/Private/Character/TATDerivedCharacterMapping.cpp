// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Character/TATDerivedCharacterMapping.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDerivedCharacterMapping)

DEFINE_LOG_CATEGORY_STATIC(LogTATDerivedCharacterMapping, Log, All);


TSoftClassPtr<AActor> UTATDerivedCharacterMapping::FindDerivedClassForActor(const AActor* actorToDouble) const
{
   if (actorToDouble == nullptr)
   {
      return nullptr;
   }

   FSoftClassPath sourceActorPath = actorToDouble->GetClass();
   const TSoftClassPtr<AActor> result = _classesByCharacterType.FindRef(TSoftClassPtr<ATATCharacter>(sourceActorPath));
   if (result.IsNull())
   {
      UE_LOG(LogTATDerivedCharacterMapping, Warning, TEXT("Could not find derived class for class %s, falling back to %s"), *sourceActorPath.ToString(), *_fallbackClass.ToString());
      return _fallbackClass;
   }
   return result;
}

#if WITH_EDITOR
EDataValidationResult UTATDerivedCharacterMapping::IsDataValid(FDataValidationContext& context) const
{
   if (!IsValid(_baseClass))
   {
      context.AddError(FText::FromString(TEXT("Base class not specified")));
      return EDataValidationResult::Invalid;
   }

   for (const TPair<TSoftClassPtr<ATATCharacter>, TSoftClassPtr<AActor>>& entry : _classesByCharacterType)
   {
      TSubclassOf<AActor> loadedClass = entry.Value.LoadSynchronous();
      if (!IsValid(loadedClass))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("class for %s is blank"), *entry.Key.ToString())));
         continue;
      }

      if (!loadedClass->IsChildOf(_baseClass))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("%s does not inherit from %s"), *loadedClass->GetName(), *_baseClass->GetName())));
      }
   }


   TSubclassOf<AActor> loadedFallback = _fallbackClass.LoadSynchronous();
   if (!IsValid(loadedFallback))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("fallback is blank"))));
   }
   else if (!loadedFallback->IsChildOf(_baseClass))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("%s does not inherit from %s"), *loadedFallback->GetName(), *_baseClass->GetName())));
   }


   return context.GetNumErrors() + context.GetNumWarnings() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

