// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Editor/TATCommonMapCheck.h"

// tat
#include "TATEditorModuleSettings.h"
#include "Audio/TATAudioPortalComponent.h"
#include "Audio/TATAudioRoomComponent.h"
#include "Audio/TATSpatialAudioVolume.h"
#include "GameFramework/TATWorldSettings.h"
#include "Lockpicking/TATCombinationScrape.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Validation/TATQuestGraphValidation.h"
#include "Validation/TATQuestSpawnValidation.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue5
#include "CableComponent.h"
#include "Components/LocalLightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerStart.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

namespace MapCheckCvars
{
   static TAutoConsoleVariable<int32> CheckShadowCacheInvalidationBehavior(
      TEXT("TAT.MapCheck.CheckShadowCacheInvalidationBehavior"),
      0,
      TEXT("Whether to check WPO on shadow casting meshes allows shadow invalidation"),
      ECVF_Default
   );

   static TAutoConsoleVariable<int32> CheckMaskedNanitePixelProgammableDistance(
      TEXT("TAT.MapCheck.CheckMaskedNanitePixelProgammableDistance"),
      1,
      TEXT("Whether to check NanitePixelProgammableDistance on masked nanite geometry"),
      ECVF_Default
   );
}

// check that local lights have a draw distance set
// doing this here, since that may be embedded in arbitrary actors
static void CheckActorForLightsWithNoDrawDistance(const AActor* actor)
{
   actor->ForEachComponent<ULocalLightComponent>(false, [actor](const ULocalLightComponent* light) {
      if (light->MaxDrawDistance == 0 && light->bAffectsWorld && !light->IsEditorOnly())
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Local light %s has a MaxDrawDistance of 0, please give it a distance as that could cause performance issues"), *light->GetReadableName()))));
      }
   });
}

// Check that we aren't using meshes meant for interactables (e.g. the throwable bottle) elsewhere where they aren't interactable,
// since doing so can confuse the player
static void CheckActorsForInteractionRequiredStaticMeshes(const AActor* actor)
{
   actor->ForEachComponent<UStaticMeshComponent>(false, [&](const UStaticMeshComponent* staticMeshComponent)
   {
      if (UStaticMesh* staticMesh = staticMeshComponent->GetStaticMesh())
      {
         // If this is a static mesh we only want to have on interactables
         const bool isStaticMeshOnlyForInteractables = UTATEditorModuleSettings::Get().InteractableOnlyStaticMeshes.ContainsByPredicate(
            [&](const TSoftObjectPtr<UStaticMesh>& staticMeshPtr)
            {
               return staticMeshPtr.Get() == staticMesh;
            });

         if (isStaticMeshOnlyForInteractables)
         {
            // Make sure it's only used on an interactable actor
            if (!actor->Implements<UInteractableInterface>())
            {
               FMessageLog("MapCheck").Warning()
                  ->AddToken(FUObjectToken::Create(actor))
                  ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
                     TEXT("Actor %s is using static mesh '%s', intended for interactables, but the actor is not interactable. ")
                     TEXT("Please use the interactable blueprint version or use another static mesh."),
                     *actor->GetActorNameOrLabel(),
                     *staticMesh->GetName()))));
            }
         }
      }
   });
}

static void CheckActorForCablesWithShadows(const AActor* actor)
{
   actor->ForEachComponent<UCableComponent>(false, [actor](const UCableComponent* cable) {
      // TODO: in 5.3, allow use of shadow invalidation settings instead.
      if (cable->CastShadow)
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Cable %s has shadow-casting enabled, this will invalidate shadows every frame, which can have performance issues"), *cable->GetReadableName()))));
      }
      });
}

// check that static mesh actors do not have movable mobility
static void CheckStaticMeshActorIsStatic(const AActor* actor)
{
   if (const AStaticMeshActor* meshActor = Cast<AStaticMeshActor>(actor))
   {
      const UStaticMeshComponent* root = meshActor->GetStaticMeshComponent();
      if (root && root->Mobility == EComponentMobility::Movable)
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
               TEXT("StaticMeshActor %s has Movable mobility, please set it to static"),
               *actor->GetActorNameOrLabel()))));
      }
   }
}

static void CheckPlayerStartHasEnoughRoom(const AActor* actor)
{
   if (const APlayerStart* playerStart = Cast<const APlayerStart>(actor))
   {
      UWorld* world = playerStart->GetWorld();

      FVector startLocation = playerStart->GetActorLocation();
      const FRotator startRotation = playerStart->GetActorRotation();

      const FTATPlayerStartValidationSettings& capsuleSettings = UTATEditorModuleSettings::Get().PlayerStartCollision;

      const FCollisionShape shape = FCollisionShape::MakeCapsule(capsuleSettings.CapsuleRadius, capsuleSettings.CapsuleHalfHeight);
      if(world->OverlapBlockingTestByProfile(startLocation, startRotation.Quaternion(), capsuleSettings.CapsuleProfile.Name, shape))
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
               TEXT("PlayerStart %s does not have enough room to spawn"),
               *actor->GetActorNameOrLabel()))));
      }
   }
}

static bool DoesMeshComponentHaveMaskedMaterials(const UStaticMeshComponent* meshComponent)
{
   check(meshComponent);
   const int materialCount = meshComponent->GetNumMaterials();
   for (int i = 0; i < materialCount; ++i)
   {
      const UMaterialInterface* material = meshComponent->GetMaterial(i);
      if (material && material->GetMaterial()->IsMasked())
      {
         return true;
      }
   }

   return false;
}

static bool DoesMeshComponentHaveWPOMaterials(const UStaticMeshComponent* meshComponent)
{
   check(meshComponent);
   const int materialCount = meshComponent->GetNumMaterials();
   for (int i = 0; i < materialCount; ++i)
   {
      // This can't detect if the material instance disabled WPO via a static switch, but I couldn't see a good way to do that
      const UMaterialInterface* material = meshComponent->GetMaterial(i);
      if (material && material->GetMaterial()->HasVertexPositionOffsetConnected())
      {
         return true;
      }
   }

   return false;
}

// check nanite meshes with WPO have a max WPO distance
static void CheckStaticMeshWPO(const AActor* actor)
{
   actor->ForEachComponent<UStaticMeshComponent>(false, [actor](const UStaticMeshComponent* meshComponent) {
      UStaticMesh* meshAsset = meshComponent->GetStaticMesh();
      if (meshAsset == nullptr || meshComponent->bIsEditorOnly)
      {
         return;
      }

      if (DoesMeshComponentHaveWPOMaterials(meshComponent) && meshComponent->WorldPositionOffsetDisableDistance == 0)
      {
         // TODO: add some component tag opt out if there is a use-case
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Mesh component with WPO %s has a WorldPositionOffsetDisableDistance of 0, please give it a distance as that could cause performance issues (and a reasonable one, not just a really large number)\nReminder: Making blueprints can make this less fiddly to configure"), *meshComponent->GetReadableName()))));
      }
      
      if (MapCheckCvars::CheckMaskedNanitePixelProgammableDistance.GetValueOnGameThread() && meshAsset->IsNaniteEnabled() && !meshComponent->bDisallowNanite && meshComponent->NanitePixelProgrammableDistance == 0 && DoesMeshComponentHaveMaskedMaterials(meshComponent))
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Mesh component with Nanite+Masked %s has a NanitePixelProgrammableDistance of 0, please give it a distance as that could cause performance issues (and a reasonable one, not just a really large number).\nReminder: Making blueprints can make this less fiddly to configure"), *meshComponent->GetReadableName()))));
      }

      if (MapCheckCvars::CheckShadowCacheInvalidationBehavior.GetValueOnGameThread() && meshComponent->CastShadow && meshComponent->bCastDynamicShadow && DoesMeshComponentHaveWPOMaterials(meshComponent) && meshComponent->ShadowCacheInvalidationBehavior == EShadowCacheInvalidationBehavior::Auto)
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Mesh component with shadows + WPO %s has a ShadowCacheInvalidationBehavior=Auto, consider using rigid"), *meshComponent->GetReadableName()))));
      }
      });
}

// Check that we are using the TAT-level overrides for rooms and portals
static void CheckTATAudioComponents(const AActor* actor)
{
   // The room component is most like going to be on the spatial audio volume actor: in that case, give a more specific error,
   // since we have a TAT-level override of that actor
   if (actor->IsA<AAkSpatialAudioVolume>() && !actor->IsA<ATATSpatialAudioVolume>())
   {
      FMessageLog("MapCheck").Warning()
         ->AddToken(FUObjectToken::Create(actor))
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
            TEXT("Actor %s is an AkSpatialAudioVolume, it should be using the TAT-level override, TATSpatialAudioVolume"),
            *actor->GetActorNameOrLabel()))));
   }
   else
   {
      // If not, still enforce that we use the TAT-level override for room components
      actor->ForEachComponent<UAkRoomComponent>(false, [&](const UAkRoomComponent* roomComponent)
      {
         if (!roomComponent->IsA<UTATAudioRoomComponent>())
         {
            FMessageLog("MapCheck").Warning()
               ->AddToken(FUObjectToken::Create(actor))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
                  TEXT("Actor %s has an Ak-level room component '%s', it must use the TAT-level override, TATAudioRoomComponent"),
                  *actor->GetActorNameOrLabel(),
                  *roomComponent->GetReadableName()))));
         }
      });
   }

   // Also check portals, which are likely to be on windows, doors, etc
   actor->ForEachComponent<UAkPortalComponent>(false, [&](const UAkPortalComponent* portalComponent)
   {
      if (!portalComponent->IsA<UTATAudioPortalComponent>())
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(actor))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
               TEXT("Actor %s has an Ak-level portal component '%s', it must use the TAT-level override, TATAudioPortalComponent"),
               *actor->GetActorNameOrLabel(),
               *portalComponent->GetReadableName()))));
      }
   });
}

// checks scene requirement structs on the actor itself (e.g. those added in blueprints)
static void CheckInlineSceneRequirements(const AActor* actor)
{
   // I initially considered a marker interface to opt into this validation,
   // but decided to check how slow unconditionally scraping for this is.
   // In AbbotsfordPark (2024-03-13), it took ~300ns per actor, adding ~5ms (out of ~160ms)
   // to the map-check time. This is not likely to have a perceptible impact on absolute
   // map-check times, and would be dwarfed by loading times in the commandlet.
   //
   // So holding off on being more selective until there is a use-case
   UClass* actorClass = actor->GetClass();
   for(const FStructProperty* property : TFieldRange<FStructProperty>(actorClass, EFieldIterationFlags::IncludeSuper))
   {
      if (property->Struct == StaticStruct<FTATSceneRequirement>())
      {
         const FTATSceneRequirement* requirement = property->ContainerPtrToValuePtr<FTATSceneRequirement>(actor);
         if (!requirement->IsNone())
         {
            FMessageLog msgLog("MapCheck");
            requirement->ValidateRequirement(msgLog, actor, [actor]() { return FUObjectToken::Create(actor); });
         }
      }
   }
}

void TATCommonMapCheck::CheckActor(const AActor* actor)
{
   check(actor);

   CheckActorForLightsWithNoDrawDistance(actor);
   CheckActorsForInteractionRequiredStaticMeshes(actor);
   CheckStaticMeshActorIsStatic(actor);
   CheckStaticMeshWPO(actor);
   CheckActorForCablesWithShadows(actor);
   CheckPlayerStartHasEnoughRoom(actor);
   CheckTATAudioComponents(actor);
   CheckInlineSceneRequirements(actor);
}

void TATCommonMapCheck::CheckWorld(UWorld* world)
{
   check(world);

   ATATWorldSettings& worldSettings = ATATWorldSettings::Get(world);
   if (worldSettings.SpawnData)
   {
      FMessageLog msgLog("MapCheck");
      MapVariationValidationHelper::ValidateSpawnConfig(*worldSettings.SpawnData, world, msgLog, ETATSpawnValidationReason::MapCheck);
      TATQuestSpawnValidation::TryValidate(world, msgLog);

      TATQuestGraphValidation::ValidatedRelatedQuestGraphs(world, msgLog);

      TSet<FName> lockCombinations = CombinationScrape::GetLockCombinationNamesInWorld(world);
      CombinationScrape::CheckForDuplicateCombinations(world, msgLog);
   }
}
