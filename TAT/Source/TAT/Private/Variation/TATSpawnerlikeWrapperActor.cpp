// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATSpawnerlikeWrapperActor.h"

// ue
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpawnerlikeWrapperActor)

ATATSpawnerlikeWrapperActor::ATATSpawnerlikeWrapperActor(const FObjectInitializer& objectInitializer)
{
   PrimaryActorTick.bCanEverTick = false;
   bReplicates = false;
   bNetLoadOnClient = false; // this is a server-only object that spawns in the actual thing we care about
   bCollideWhenPlacing = true;
   SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
   
   RootComponent = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
   if(RootComponent)
   {
      RootComponent->SetMobility(EComponentMobility::Static);
   }

#if WITH_EDITORONLY_DATA
   if (!IsRunningCommandlet())
   {
      _goodSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("GoodSprite"), true);
      _badSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("BadSprite"), true);
      _arrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"), true);

      const FVector boxExtent = FVector(20.0f, 20.0f, 20.0f);
      _boxComponent = CreateEditorOnlyDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"), true);
      if (_boxComponent)
      {
         _boxComponent->ShapeColor = FColor(255, 138, 5, 255);
         _boxComponent->bDrawOnlyIfSelected = true;
         _boxComponent->InitBoxExtent(boxExtent);
         _boxComponent->SetRelativeLocation(FVector(0.0f, 0.0f, boxExtent.Z));
         _boxComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
         _boxComponent->bShouldCollideWhenPlacing = true;
         _boxComponent->Mobility = EComponentMobility::Static;
         _boxComponent->SetupAttachment(RootComponent);
      }

      // Structure to hold one-time initialization
      struct FConstructorStatics
      {
         ConstructorHelpers::FObjectFinderOptional<UTexture2D> ActorSpawnerTextureObject;
         ConstructorHelpers::FObjectFinderOptional<UTexture2D> BadPlacementTextureObject;
         FName ID_ActorSpawner;
         FText NAME_ActorSpawner;
         FName ID_ActorSpawner_BadPlacement;
         FText NAME_ActorSpawner_BadPlacement;
         FConstructorStatics()
            : ActorSpawnerTextureObject(TEXT("/Engine/EditorResources/S_Actor")) // default, can override with EditorSettings struct
            , BadPlacementTextureObject(TEXT("/Game/Editor/Icons/BadPlacement")) // default, can override with EditorSettings struct
            , ID_ActorSpawner(TEXT("ActorSpawner"))
            , NAME_ActorSpawner(NSLOCTEXT("SpriteCategory", "ActorSpawner", "Actor Spawner"))
            , ID_ActorSpawner_BadPlacement(TEXT("ActorSpawner_BadPlacement"))
            , NAME_ActorSpawner_BadPlacement(NSLOCTEXT("SpriteCategory", "ActorSpawner_BadPlacement", "Actor Spawner BadPlacement"))
         {
         }
      };
      static FConstructorStatics sConstructorStatics;

      // default icons
      _icon = sConstructorStatics.ActorSpawnerTextureObject.Get();

      if (_arrowComponent)
      {
         _arrowComponent->ArrowColor = FColor(235, 64, 52);
         _arrowComponent->ArrowSize = 1.0f;
         _arrowComponent->ArrowLength = 30.f;
         _arrowComponent->bTreatAsASprite = true;
         _arrowComponent->SpriteInfo.Category = sConstructorStatics.ID_ActorSpawner;
         _arrowComponent->SpriteInfo.DisplayName = sConstructorStatics.NAME_ActorSpawner;
         _arrowComponent->SetupAttachment(RootComponent);
         _arrowComponent->bIsScreenSizeScaled = true;
      }

      if (_goodSprite)
      {
         _goodSprite->Sprite = _icon;
         _goodSprite->SetRelativeScale3D(FVector(_iconScale, _iconScale, _iconScale));
         _goodSprite->bHiddenInGame = true;
         _goodSprite->SpriteInfo.Category = sConstructorStatics.ID_ActorSpawner;
         _goodSprite->SpriteInfo.DisplayName = sConstructorStatics.NAME_ActorSpawner;
         _goodSprite->SetupAttachment(_boxComponent);
         _goodSprite->SetUsingAbsoluteScale(true);
         _goodSprite->bIsScreenSizeScaled = true;
      }

      if (_badSprite)
      {
         _badSprite->Sprite = sConstructorStatics.BadPlacementTextureObject.Get();
         _badSprite->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
         _badSprite->bHiddenInGame = true;
         _badSprite->SpriteInfo.Category = sConstructorStatics.ID_ActorSpawner_BadPlacement;
         _badSprite->SpriteInfo.DisplayName = sConstructorStatics.NAME_ActorSpawner_BadPlacement;
         _badSprite->SetUsingAbsoluteScale(true);
         _badSprite->SetupAttachment(_boxComponent);
         _badSprite->bIsScreenSizeScaled = true;
      }

      if (_badSprite)
      {
         _badSprite->SetVisibility(false);
      }
   }
#endif // WITH_EDITORONLY_DATA
}

void ATATSpawnerlikeWrapperActor::_SetRootComponent(USceneComponent* root)
{
   RootComponent = root;
#if WITH_EDITORONLY_DATA
   if (_boxComponent)
   {
      _boxComponent->SetupAttachment(RootComponent);
   }
   if (_arrowComponent)
   {
      _arrowComponent->SetupAttachment(RootComponent);
   }
#endif
}

#if WITH_EDITOR
void ATATSpawnerlikeWrapperActor::OnConstruction(const FTransform& transform)
{
   Super::OnConstruction(transform);
   _UpdateIcon();
}

void ATATSpawnerlikeWrapperActor::PostEditMove(bool finished)
{
   if (finished)
   {
      _TryValidateCollision();
   }
   Super::PostEditMove(finished);
}

void ATATSpawnerlikeWrapperActor::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName memberPropertyName = (propertyChangedEvent.MemberProperty != nullptr) ? propertyChangedEvent.MemberProperty->GetFName() : NAME_None;
   if (memberPropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, _icon) ||
       memberPropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, _iconScale))
   {
      _UpdateIcon();
   }
}

void ATATSpawnerlikeWrapperActor::PostEditUndo()
{
   Super::PostEditUndo();

   // undo can move the object without calling post edit move, we need to validate on any movement
   _TryValidateCollision();
}

void ATATSpawnerlikeWrapperActor::_TryValidateCollision()
{
   if (!IsTemplate() && (_goodSprite || _badSprite))
   {
      // Validate collision
      _ValidateCollision();

      // Force update of icon(s)
      MarkComponentsRenderStateDirty();
   }
}

void ATATSpawnerlikeWrapperActor::_UpdateIcon()
{
   if (_goodSprite)
   {
      _goodSprite->Sprite = _icon;
      _goodSprite->SetRelativeScale3D(FVector(_iconScale, _iconScale, _iconScale));
   }
}

void ATATSpawnerlikeWrapperActor::_ValidateCollision()
{
   FVector origLocation = GetActorLocation();
   FVector boxExtents = _boxComponent->GetUnscaledBoxExtent();

   FCollisionQueryParams params = FCollisionQueryParams(SCENE_QUERY_STAT(TATQuestActorSpawner_ValidateCollision), false, this);

   // Validation for items just ensures that we don't have any blocking geo immediately on top of us for a few cm.  It's a fairly imprecise test...
   FHitResult hit(ForceInit);
   const FVector traceStart = GetActorLocation() + FVector(0.f, 0.f, 2.0f);
   const FVector traceEnd = traceStart + FVector(0.f, 0.f, 5.0f);
   GetWorld()->LineTraceSingleByChannel(hit, traceStart, traceEnd, ECC_Pawn, FCollisionQueryParams(SCENE_QUERY_STAT(TATQuestActorSpawner_ValidateCollision), false, this));
   bool result = !hit.bBlockingHit;

   // Update sprites by result
   if (_goodSprite)
   {
      _goodSprite->SetVisibility(result);
   }
   if (_badSprite)
   {
      _badSprite->SetVisibility(!result);
   }
}
#endif
