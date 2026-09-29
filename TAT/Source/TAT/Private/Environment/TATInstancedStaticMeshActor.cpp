// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATInstancedStaticMeshActor.h"

// ue 
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "AI/NavigationSystemBase.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInstancedStaticMeshActor)

ATATInstancedStaticMeshActor::ATATInstancedStaticMeshActor()
{
   PrimaryActorTick.bCanEverTick = false;

   USceneComponent* sceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   sceneComponent->Mobility = EComponentMobility::Static;
   RootComponent = sceneComponent;

   _Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
   _Arrow->SetupAttachment(RootComponent);
   _Arrow->MarkAsEditorOnlySubobject();

   _NegativeBoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("NegativeBoxCollision"));
   _NegativeBoxCollision->SetupAttachment(RootComponent);
   _NegativeBoxCollision->SetMobility(EComponentMobility::Static);
}

#if WITH_EDITOR
void ATATInstancedStaticMeshActor::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FString propertyNameAsString = propertyChangedEvent.GetPropertyName().ToString();
   // NOTE: Can't use the native variable names via GET_MEMBER_NAME_CHECKED - because we need to also react to BP
   // defined variables like Line Meshes, Negative Volume Extents.
   if (
      propertyNameAsString.Contains(TEXT("Negative")) || 
      propertyNameAsString.Contains(TEXT("Line"))
      )
   {
      HandleSetup();
   }
}

void ATATInstancedStaticMeshActor::PostEditMove(bool bFinished)
{
   Super::PostEditMove(bFinished);
   if (bFinished)
   {
      // After we've moved, run the setup. I'm not certain this is needed right now as the line end is in local space
      // so it shouldn't matter. However, this mirrors the previous functionality.
      HandleSetup();
   }
}

void ATATInstancedStaticMeshActor::PostEditUndo()
{
   Super::PostEditUndo();
   HandleSetup();
}

void ATATInstancedStaticMeshActor::TryHandleSetup()
{
   // In the editor, there is a chance that either the actor, or the component itself is null or trashed after a frame
   // For example the negative box collision was missing from several actors in the test scene due to an issue with 
   // serialization post nativization, this was fixed, but let's be safe here (it's only calling in editor anyway)
   if (IsValid(this) &&
      IsValid(_NegativeBoxCollision) &&
      IsValid(_Arrow))
   {
      HandleSetup();
      _HasInitialSetupTriggered = true;
   }
}
#endif


void ATATInstancedStaticMeshActor::PostLoad()
{
   Super::PostLoad();
#if WITH_EDITOR
   // If we're not the default instance, we have an instanced static mesh component AND we've never set an instance count
   // then run the setup. We can't directly call the function on this frame so we delay until the next frame. Note: This
   // will dirty the object (note HandleSetup calls Modify). Once the object is saved again this code path won't need to
   // run again.
   if (HasAnyFlags(RF_ArchetypeObject | RF_ClassDefaultObject) == false && _HasInitialSetupTriggered == false)
   {
      if (GEditor)
      {
         GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::TryHandleSetup));
      }
   }
#endif
}

void ATATInstancedStaticMeshActor::HandleSetup_Implementation()
{
#if WITH_EDITOR
   FNavigationSystem::UpdateActorAndComponentData(*this, true);
   Modify();
#endif
}
