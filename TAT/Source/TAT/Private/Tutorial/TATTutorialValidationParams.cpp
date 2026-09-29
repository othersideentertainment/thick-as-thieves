// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tutorial/TATTutorialValidationParams.h"

#include "GameplayTagContainer.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTutorialValidationParams)

void FTATTutorialValidationParams::ReportError(const FText& message) const
{
   if (ErrorReporter)
   {
      ErrorReporter.GetValue()(message);
   }
}

void FTATTutorialValidationParams::RequireSoftActor(const TSoftObjectPtr<AActor>& softActor, const FStringView name) const
{
   if (softActor.IsNull())
   {
      ReportError(FText::FormatOrdered(INVTEXT("{0} is null"), FText::FromStringView(name)));
   }
   else if (World && !softActor.IsValid())
   {
      ReportError(FText::FormatOrdered(INVTEXT("{0} is not found in level ({1})"), FText::FromStringView(name), FText::FromString(softActor.ToString())));
   }
}

void FTATTutorialValidationParams::RequireSoftClass(const TSoftClassPtr<>& softClass, const FStringView name) const
{
   if (softClass.IsNull())
   {
      ReportError(FText::FormatOrdered(INVTEXT("{0} is null"), FText::FromStringView(name)));
   }
}

void FTATTutorialValidationParams::RequireSoftAsset(const TSoftObjectPtr<>& softObject, const FStringView name) const
{
   if (softObject.IsNull())
   {
      ReportError(FText::FormatOrdered(INVTEXT("{0} is null"), FText::FromStringView(name)));
   }
}

void FTATTutorialValidationParams::RequireTag(const FGameplayTag& tag, const FStringView name) const
{
   if (!tag.IsValid())
   {
      ReportError(FText::FormatOrdered(INVTEXT("{0} is not set"), FText::FromStringView(name)));
   }
}

void FTATTutorialValidationParams::RequireClass(const UClass* clazz, const FStringView name) const
{
   if (clazz == nullptr)
   {
      ReportError(FText::FormatOrdered(INVTEXT("{0} is not set"), FText::FromStringView(name)));
   }
}

void UTATTutorialValidationUtils::TutorialRequireSoftActor(const FTATTutorialValidationParams& params, const TSoftObjectPtr<AActor>& softActor,
   FName name)
{
#if WITH_EDITOR
   if (!softActor.IsValid())
   {
      params.RequireSoftActor(softActor, name.ToString());
   }
#endif
}
