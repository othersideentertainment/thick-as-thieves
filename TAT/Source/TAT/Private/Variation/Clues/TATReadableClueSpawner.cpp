// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATReadableClueSpawner.h"

// ue
#include "Logging/MessageLog.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATReadableClueSpawner)

#if WITH_EDITOR
EDataValidationResult UTATReadableClueVisuals::IsDataValid(class FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if(ReadableActorClass.IsNull())
   {
      context.AddError(FText::FormatOrdered(INVTEXT("[{0}] No ReadableActorClass"),
            FText::FromString(GetName())));
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

UTATReadableClueSpawner::UTATReadableClueSpawner()
{
}

FTATClueBucketKey UTATReadableClueSpawner::GetClueBucket() const
{
   return {ETATClueType::Readable, AsClueSubtype(ReadableType), PlacementTag};
}

#if WITH_EDITOR
void UTATReadableClueSpawner::CheckForErrors()
{
   Super::CheckForErrors();

   // TODO: probably do check on classes in IsDataValid rather than just map check, but not if owner is abstract
   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (!PlacementTag.IsValid())
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(GetOwner(), FText::FromString(GetOwner()->GetActorNameOrLabel())))
            ->AddToken(FTextToken::Create(INVTEXT("{ActorName} : Spawner has no PlacementTag")));
      }
   }
}
#endif
