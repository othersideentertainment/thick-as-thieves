// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATDestroySpawner.h"

// tat
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnerComponent.h"

// ue5
#include "DebugRenderSceneProxy.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDestroySpawner)

// Sets default values
ATATDestroySpawner::ATATDestroySpawner()
{
   PrimaryActorTick.bCanEverTick = false;
   bReplicates = true;
   NetDormancy = DORM_Initial;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
   RootComponent->SetMobility(EComponentMobility::Static);
#if WITH_EDITORONLY_DATA
   RootComponent->bVisualizeComponent = true;
#endif

   _spawnerComponent = CreateDefaultSubobject<UTATSpawnerComponent>(TEXT("VariationSpawner"));

#if WITH_EDITOR
   _visualizationComponent = CreateEditorOnlyDefaultSubobject<UTATDestroySpawnerVisualizationComponent>(TEXT("RenderComp"));
   if (_visualizationComponent)
   {
      _visualizationComponent->SetupAttachment(RootComponent);
   }
#endif
}

void ATATDestroySpawner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(ATATDestroySpawner, _didNotSpawn);
}

#if WITH_EDITOR
void ATATDestroySpawner::CheckForErrors()
{
   Super::CheckForErrors();

   for (AActor* actorToDestroy : ActorsToKeepOnSpawn)
   {
      if (actorToDestroy == this)
      {
         FMessageLog("MapCheck").Error()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("DestroySpawner has reference to itself"))));
      }
   }
}
#endif // WITH_EDITOR

void ATATDestroySpawner::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   _spawnerComponent->AuthorityOnNotSpawn.AddUniqueDynamic(this, &ATATDestroySpawner::_AuthorityOnSpawnerNotSpawn);
}

void ATATDestroySpawner::_AuthorityOnSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   bool hasNonReplicated = false;
   for (AActor* actorToDestroy : ActorsToKeepOnSpawn)
   {
      if (!IsValid(actorToDestroy))
      {
         continue;
      }

      hasNonReplicated |= !actorToDestroy->GetIsReplicated();
      actorToDestroy->Destroy();
   }

   if (hasNonReplicated)
   {
      FlushNetDormancy();
      _didNotSpawn = true;
   }
}

void ATATDestroySpawner::_OnRep_DidNotSpawn()
{
   if (!_didNotSpawn)
   {
      return;
   }

   for (AActor* actorToDestroy : ActorsToKeepOnSpawn)
   {
      if (IsValid(actorToDestroy) && !actorToDestroy->GetIsReplicated())
      {
         actorToDestroy->Destroy();
      }
   }
}


UTATDestroySpawnerVisualizationComponent::UTATDestroySpawnerVisualizationComponent()
{
   bIsEditorOnly = true;
   bHiddenInGame = true;
   SetGenerateOverlapEvents(false);
}

#if WITH_EDITOR
class FExternalNegativeSpawnerSceneProxy final : public FDebugRenderSceneProxy
{
public:
   FExternalNegativeSpawnerSceneProxy(const UPrimitiveComponent* component) : FDebugRenderSceneProxy(component) {}

   virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* view) const override
   {
      FPrimitiveViewRelevance result;
      result.bDrawRelevance = IsShown(view);
      result.bDynamicRelevance = true;
      result.bShadowRelevance = IsShadowCast(view);
      result.bEditorPrimitiveRelevance = UseEditorCompositing(view);
      return result;
   }
};

FPrimitiveSceneProxy* UTATDestroySpawnerVisualizationComponent::CreateSceneProxy()
{
   FDebugRenderSceneProxy* renderProxy = new FExternalNegativeSpawnerSceneProxy(this);

   if (const auto* spawner = Cast<ATATDestroySpawner>(GetOwner()))
   {
      for (const AActor* actor : spawner->ActorsToKeepOnSpawn)
      {
         if (!IsValid(actor))
         {
            continue;
         }

         constexpr float dashLength = 20.f;
         renderProxy->DashedLines.Emplace(GetComponentLocation(), actor->GetActorLocation(), FColor::Orange, dashLength);
      }
   }

   return renderProxy;
}

FBoxSphereBounds UTATDestroySpawnerVisualizationComponent::CalcBounds(const FTransform& localToWorld) const
{
   FBox boundingBox;
   boundingBox.Init();
   boundingBox += GetComponentLocation();

   if (const auto* spawner = Cast<ATATDestroySpawner>(GetOwner()))
   {
      for (const AActor* actor : spawner->ActorsToKeepOnSpawn)
      {
         if (IsValid(actor))
         {
            boundingBox += actor->GetActorLocation();
         }
      }
   }

   return FBoxSphereBounds(boundingBox);
}

#endif
