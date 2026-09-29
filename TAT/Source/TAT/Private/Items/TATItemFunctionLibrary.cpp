// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/TATItemFunctionLibrary.h"

// tat
#include "Abilities/TATGameplayTags.h"
#include "Developer/TATProjectSettings.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Items/ToolComponent.h"
#include "Items/TATInventoryTypes.h"
#include "Items/TATItemInfo.h"
#include "Items/TATItemInventoryComponent.h"

// ue5
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATItemFunctionLibrary)


namespace ItemFunctionCVars
{
   static float MinItemBoundsDropOffset = 5.0f;
   FAutoConsoleVariableRef CVarEnabled(
      TEXT("TAT.Drop.MinItemBoundsOffset"),
      MinItemBoundsDropOffset,
      TEXT("Minimum value of offset from local origin to adjust when dropping items."),
      ECVF_Default);

   static float DropEyeTraceCollisionOffset = 10.0f;
   FAutoConsoleVariableRef CVarDropEyeTraceCollisionOffset(
      TEXT("TAT.Drop.EyeTraceCollisionOffset"),
      MinItemBoundsDropOffset,
      TEXT("Offset from eye trace collision to start drop from"),
      ECVF_Default);
}

bool UTATItemFunctionLibrary::IsWeaponTool(const UToolComponent* tool)
{
   if (tool)
   {
      const UTATProjectSettings& settings = *GetDefault<UTATProjectSettings>();
      FGameplayTag toolCategory = tool->GetToolInfo().ToolCategory;
      return toolCategory.MatchesAny(settings.WeaponToolTags);
   }
   return false;
}

bool UTATItemFunctionLibrary::DoesActorHaveWeaponEquipped(const AActor* actor)
{
   if (auto toolSetSystemInterface = Cast<IToolSetSystemInterface>(actor))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystemInterface->GetToolSetInterface())
      {
         if (UToolComponent* tool = toolSetInterface->GetCurrentTool())
         {
            return UTATItemFunctionLibrary::IsWeaponTool(tool);
         }
      }
   }
   return false;
}

bool UTATItemFunctionLibrary::FindDropLocationFromSuggestedStart(const AActor* actorToDrop, const AActor* droppingActor, FCollisionProfileName traceProfile, FVector suggestedStartWorldPosition, FVector& outDropPosition)
{
   UWorld* world = actorToDrop->GetWorld();

   // line trace to ground
   const float kMaxTraceDistance = 2000.f;
   FHitResult hit;
   const FVector traceEnd = suggestedStartWorldPosition + FVector(0, 0, -kMaxTraceDistance);
   FCollisionQueryParams params(SCENE_QUERY_STAT(ItemFunctionLibrary_FindDropLocation));
   if (droppingActor)
   {
      params.AddIgnoredActor(droppingActor);
   }

   world->LineTraceSingleByProfile(hit, suggestedStartWorldPosition, traceEnd, traceProfile.Name, params);
   if (!hit.bBlockingHit)
   {
      outDropPosition = traceEnd;
      return false;
   }
   outDropPosition = hit.ImpactPoint;

   FVector center;
   FVector boxExtent;
   _GetActorBoundingBox(actorToDrop, center, boxExtent);

   // Center is in world space, translate to local space
   center -= actorToDrop->GetActorLocation();

   // Check that the offset is significant.
   float offset = boxExtent.Z - center.Z;
   if (offset > ItemFunctionCVars::MinItemBoundsDropOffset)
   {
      outDropPosition.Z += offset;
      return true;
   }

   return world->FindTeleportSpot(actorToDrop, outDropPosition, FRotator());
}

FVector UTATItemFunctionLibrary::FindSuggestedDropStartFromActorEyes(const AActor* droppingActor, float horizontalDropDistance, FCollisionProfileName traceProfile)
{
   check(droppingActor);

   UWorld* world = droppingActor->GetWorld();

   // possibly offset to camera aim to be correct in third person
   FVector start, unitEnd;
   UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(droppingActor, FGameplayAbilityTargetingLocationInfo(), 1, start, unitEnd);

   // The scale horizontal distance to match range
   const FVector eyeForward = unitEnd - start;
   const float forwardSize2d = eyeForward.Size2D();
   const float distanceScale = horizontalDropDistance / FMath::Max(forwardSize2d, 0.1f);
   const FVector traceEnd = start + (eyeForward * distanceScale);

   // line trace in view direction
   FHitResult hit;
   FCollisionQueryParams params(SCENE_QUERY_STAT(ItemFunctionLibrary_FindDropStart));
   params.AddIgnoredActor(droppingActor);
   world->LineTraceSingleByProfile(hit, start, traceEnd, traceProfile.Name, params);
   if (!hit.bBlockingHit)
   {
      return traceEnd;
   }

   // adjust backward from hit to avoid catching on geo
   const float adjustDistance = FMath::Min(ItemFunctionCVars::DropEyeTraceCollisionOffset, hit.Distance);
   return hit.ImpactPoint - (eyeForward * adjustDistance);
}

bool UTATItemFunctionLibrary::CanCharacterPickUpThings(const ACharacter* character)
{
   if (!IsValid(character))
   {
      return false;
   }

   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(character))
   {
      return !tagInterface->HasMatchingGameplayTag(TAG_Status_PickupInteractionDisabled);
   }

   return false;
}

void UTATItemFunctionLibrary::DropItemsForKO(UTATItemInventoryComponent* itemInventory)
{
   if(itemInventory == nullptr)
   {
      return;
   }

   TArray<FInventoryStackId> stackIds;
   FGameplayTag categoryToDrop = UTATProjectSettings::Get().LegacyItemsToDropOnPlayerKO;
   itemInventory->ForEachInventorySlot(
      [&stackIds, categoryToDrop](const FTATInventorySlot& slot, bool& done)
      {
         if (const UTATItemInfo* itemCDO = slot.GetItemCDO())
         {
            if (itemCDO->Category.MatchesTag(categoryToDrop))
            {
               stackIds.Add(slot.StackId);
            }
         }
      }
   );

   for (FInventoryStackId stackId : stackIds)
   {
      itemInventory->ServerDropItemStack(stackId);
   }
}

// This is a variant of AActor::GetActorBounds which includes an extra check for
// collision channel with WorldStatic.
void UTATItemFunctionLibrary::_GetActorBoundingBox(const AActor* actor, FVector& center, FVector& extents)
{
   check(actor);

   FBox bounds(ForceInit);

   actor->ForEachComponent<UPrimitiveComponent>(false, [&](const UPrimitiveComponent* primComp)
      {
         // Only use collidable components to find collision bounding box.
         if (primComp->IsRegistered() &&
             primComp->IsCollisionEnabled() &&
             primComp->GetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic) == ECollisionResponse::ECR_Block)
         {
            bounds += primComp->Bounds.GetBox();
         }
      });

   bounds.GetCenterAndExtents(center, extents);
}

