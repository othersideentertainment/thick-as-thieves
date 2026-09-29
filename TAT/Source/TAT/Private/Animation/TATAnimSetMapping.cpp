// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Animation/TATAnimSetMapping.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnimSetMapping)

TSubclassOf<UAnimInstance> UTATAnimSetMapping::GetAnimSetByTag(FGameplayTag tag) const
{
   if(!tag.IsValid())
   {
      return DefaultAnimSet;
   }

   return AnimSetsByTag.FindRef(tag);
}

#if WITH_EDITOR
EDataValidationResult UTATAnimSetMapping::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if (DefaultAnimSet == nullptr)
   {
      context.AddWarning(INVTEXT("No DefaultAnimSet is set"));
   }

   for (const TPair<FGameplayTag, TSubclassOf<UAnimInstance>>& entry : AnimSetsByTag)
   {
      if (entry.Value == nullptr)
      {
         context.AddWarning(FText::FormatOrdered(INVTEXT("AnimSet for '{0}' is null"), FText::FromName(entry.Key.GetTagName())));
      }
   }

   if (Schema)
   {
      for (const FGameplayTag& tag : Schema->RequiredAnimSets)
      {
         if(!AnimSetsByTag.Contains(tag))
         {
            context.AddWarning(FText::FormatOrdered(INVTEXT("Missing AnimSet '{0}' (required by schema '{1}')"),
               FText::FromName(tag.GetTagName()),
               FText::FromName(Schema->GetFName())));
         }
      }

      if (Schema->WarnOnExtraAnimSets)
      {
         for (const TPair<FGameplayTag, TSubclassOf<UAnimInstance>>& entry : AnimSetsByTag)
         {
            if (!Schema->RequiredAnimSets.HasTagExact(entry.Key))
            {
               context.AddWarning(FText::FormatOrdered(INVTEXT("Have anim set '{0}' not in schema '{1}', which has WarnOnExtraAnimSets enabled."),
                  FText::FromName(entry.Key.GetTagName()),
                  FText::FromName(Schema->GetFName())));
            }
         }
      }
   }

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
