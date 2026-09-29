// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimMetadata_Combat.h"

// ose

// ue4
#include "Animation/AnimMetaData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimMetadata_Combat)

/* static */
UAnimMetadata_CombatHitboxes* UAnimMetadata_CombatHitboxes::GetAnimMetadata(const UAnimSequenceBase* animSequenceBase)
{
   const TArray<UAnimMetaData*>& metaData = animSequenceBase->GetMetaData();
   for (UAnimMetaData* metaDataInstance : metaData)
   {
      if (metaDataInstance && metaDataInstance->GetClass() == UAnimMetadata_CombatHitboxes::StaticClass())
      {
         return CastChecked<UAnimMetadata_CombatHitboxes>(metaDataInstance);
      }
   }
   return nullptr;
}

#if WITH_EDITOR
bool UAnimMetadata_CombatHitboxes::CanEditChange(const FProperty* inProperty) const
{
   const bool parentVal = Super::CanEditChange(inProperty);

   // read-only property
   if (inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAnimMetadata_CombatHitboxes, _maxSwingReach))
      return false;

   return parentVal;
}
#endif

void UAnimMetadata_CombatHitboxes::AddHitboxMetadata(int index, const FCombatHitboxMetadata& metadata)
{
   _hitboxMetadata.FindOrAdd(index) = metadata;
   _CalculateMaxSwingReach();
}

void UAnimMetadata_CombatHitboxes::ResetHitboxLocations()
{
   _hitboxMetadata.Reset();
}

const FCombatHitboxMetadata& UAnimMetadata_CombatHitboxes::GetMetadataForHitboxIndex(int index) const
{
   // this metadata is so tightly coupled with the code that generates it that I
   // am going to assume the index is always valid by asserting!
   check(_hitboxMetadata.Contains(index));
   return _hitboxMetadata[index];
}

void UAnimMetadata_CombatHitboxes::_CalculateMaxSwingReach()
{
   _maxSwingReach = 0.0f;
   for(auto& entry : _hitboxMetadata)
   {
      const FCombatHitboxMetadata& meta = entry.Value;
      const float reach = meta.Location.Y + meta.Radius;
      if(reach > _maxSwingReach)
      {
         _maxSwingReach = reach;
      }
   }
}

