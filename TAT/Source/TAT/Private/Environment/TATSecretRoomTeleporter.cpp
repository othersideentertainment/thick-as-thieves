// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATSecretRoomTeleporter.h"

// tat
#include "Developer/TATActorDependencyVisComponent.h"
#include "Developer/TATEditorActorDependencySubsystem.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Interactables/TATInteractHighlightUtils.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/UObjectToken.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSecretRoomTeleporter)

// Sets default values
ATATSecretRoomTeleporter::ATATSecretRoomTeleporter()
{
   PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
   _visComponent = CreateEditorOnlyDefaultSubobject<UTATActorDependencyVisComponent>(TEXT("VisComponent"));
   if(_visComponent)
   {
      _visComponent->GroupKey = ATATSecretRoomTeleporter::StaticClass()->GetFName();
      _visComponent->Color = FColor::Cyan;
      _visComponent->IsReversed = false;
   }
#endif
}

void ATATSecretRoomTeleporter::BeginPlay()
{
   Super::BeginPlay();

   if(!IsNetMode(NM_DedicatedServer))
   {
      if(_requireThiefVision)
      {
         if(auto* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>())
         {
            thiefVisionSubsystem->OnLocalPlayerThiefVisionStatusChanged.AddUniqueDynamic(this, &ThisClass::_OnLocalThiefVisionChanged);
            if(thiefVisionSubsystem->IsThiefVisionEnabledForLocalPlayer())
            {
               BP_OnVisibilityChanged(true);
            }
         }
      }
   }
}

#if WITH_EDITOR
void ATATSecretRoomTeleporter::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (_targets.Num() < _minimumTargetCount)
      {
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FormatOrdered(INVTEXT("Teleporter has fewer than the minimum targets ({0} < {1}). You can change the minimum in the teleporter blueprint."),
               _targets.Num(), _minimumTargetCount)));
      }
      
      for (int i = 0; i < _targets.Num(); i++)
      {
         if (!IsValid(_targets[i]))
         {
            FMessageLog("MapCheck").Warning()
               ->AddToken(FUObjectToken::Create(this))
               ->AddToken(FTextToken::Create(FText::FormatOrdered(INVTEXT("Missing target at index {0}"), i)));
         }
      }
   }
}
#endif

bool ATATSecretRoomTeleporter::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if(_requireThiefVision && !UTATThiefVisionSubsystem::IsThiefVisionEnabled(interactingCharacter))
   {
      return false;
   }
   
   // CONSIDER: Should check for valid space?
   return _targets.Num() > 0;
}

void ATATSecretRoomTeleporter::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = _interactPrompt;
}

FInteractStartResult ATATSecretRoomTeleporter::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   // For now, assume that teleport will be successful

   _TryExecutePreTeleportCue(interactingCharacter);
   
   if(HasAuthority())
   {
      // Try to teleport to one of the targets, in order
      // NOTE: This will currently err on the side of teleporting to an earlier partially-occluded
      //       location that it can adjust over a later location that is completely clear. If we care,
      //       the code could find the target location in two passes by first checking EncroachingGeometry,
      //       and then doing a TestOnly Teleport. But not sure if we care.
      for(const ATATSecretRoomTeleportTarget* target : _targets)
      {
         if(!ensure(IsValid(target)))
         {
            continue;
         }

         if(interactingCharacter->TeleportTo(target->GetActorLocation(), target->GetActorRotation()))
         {
            if(APlayerController* playerController = interactingCharacter->GetController<APlayerController>())
            {
               // Set control rotation via RPC, so that aligns with teleport position
               playerController->ClientSetRotation(target->GetActorRotation());
            }

            _TryExecutePostTeleportCue(interactingCharacter);
            break;
         }
      }
   }
   
   return FInteractStartResult{};
}

void ATATSecretRoomTeleporter::ShowHighlight_Implementation(bool showHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, showHighlight);
}

#if WITH_EDITOR
void ATATSecretRoomTeleporter::PostRegisterAllComponents()
{
   Super::PostRegisterAllComponents();
   _RefreshEditorDependency();
}

void ATATSecretRoomTeleporter::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName propertyName = (propertyChangedEvent.Property != nullptr) ? propertyChangedEvent.Property->GetFName() : NAME_None;
   if (propertyName == GET_MEMBER_NAME_CHECKED(ThisClass, _targets))
   {
      _RefreshEditorDependency();
   }
}

void ATATSecretRoomTeleporter::_RefreshEditorDependency()
{
   if(UTATEditorActorDependencySubsystem* dependencySubsystem = GetWorld()->GetSubsystem<UTATEditorActorDependencySubsystem>())
   {
      dependencySubsystem->UpdateDependencies(ATATSecretRoomTeleporter::StaticClass()->GetFName(), this, MakeConstArrayView(_targets));
   }
}
#endif

void ATATSecretRoomTeleporter::_OnLocalThiefVisionChanged(APlayerController* controller, bool thiefVisionEnabled)
{
   BP_OnVisibilityChanged(thiefVisionEnabled);
}

void ATATSecretRoomTeleporter::_TryExecutePreTeleportCue(ACharacter* interactingCharacter)
{
   if (_preTeleportCue.IsValid())
   {
      // Execute gameplay cue for the pre-teleport
      // This will be predicted
      FGameplayCueParameters params;
      params.Location = interactingCharacter->GetActorLocation();
      UOSEAbilityFunctionLibrary::ExecuteGameplayCueOnActorWithParams(interactingCharacter, _preTeleportCue, params);
   }
}

void ATATSecretRoomTeleporter::_TryExecutePostTeleportCue(ACharacter* interactingCharacter)
{
   if (_postTeleportCue.IsValid())
   {
      // Since the post-teleport cue is only executed on authority, explicitly pass an empty
      // prediction key so it doesn't get dropped on the predicting client. (There is a valid prediction
      // key during interact start)
      UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(interactingCharacter);
      check(asc);
      FGameplayCueParameters params;
      params.Location = interactingCharacter->GetActorLocation();

      UAbilitySystemGlobals::Get().GetGameplayCueManager()->InvokeGameplayCueExecuted_WithParams(asc, _postTeleportCue, FPredictionKey(), params);
   }
}

ATATSecretRoomTeleportTarget::ATATSecretRoomTeleportTarget()
{
   // NOTE: adapted from ATATPlayerStart
   GetCapsuleComponent()->InitCapsuleSize(40.0f, 92.0f);
   GetCapsuleComponent()->SetShouldUpdatePhysicsVolume(false);

#if WITH_EDITORONLY_DATA
   _arrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

   if (!IsRunningCommandlet())
   {
      // Structure to hold one-time initialization
      struct FConstructorStatics
      {
         ConstructorHelpers::FObjectFinderOptional<UTexture2D> PlayerStartTextureObject;
         FName ID_PlayerStart;
         FText NAME_PlayerStart;
         FName ID_Navigation;
         FText NAME_Navigation;
         FConstructorStatics()
            : PlayerStartTextureObject(TEXT("/Engine/EditorResources/S_Thruster"))
            , ID_PlayerStart(TEXT("PlayerStart"))
            , NAME_PlayerStart(NSLOCTEXT("SpriteCategory", "PlayerStart", "Player Start"))
            , ID_Navigation(TEXT("Navigation"))
            , NAME_Navigation(NSLOCTEXT("SpriteCategory", "Navigation", "Navigation"))
         {
         }
      };
      static FConstructorStatics sConstructorStatics;

      if (GetGoodSprite())
      {
         GetGoodSprite()->Sprite = sConstructorStatics.PlayerStartTextureObject.Get();
         GetGoodSprite()->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
         GetGoodSprite()->SpriteInfo.Category = sConstructorStatics.ID_PlayerStart;
         GetGoodSprite()->SpriteInfo.DisplayName = sConstructorStatics.NAME_PlayerStart;
      }
      if (GetBadSprite())
      {
         GetBadSprite()->SetVisibility(false);
      }

      if (_arrowComponent)
      {
         _arrowComponent->ArrowColor = FColor(150, 200, 255);

         _arrowComponent->ArrowSize = 1.0f;
         _arrowComponent->bTreatAsASprite = true;
         _arrowComponent->SpriteInfo.Category = sConstructorStatics.ID_Navigation;
         _arrowComponent->SpriteInfo.DisplayName = sConstructorStatics.NAME_Navigation;
         _arrowComponent->SetupAttachment(GetCapsuleComponent());
         _arrowComponent->bIsScreenSizeScaled = true;
      }
   }

   bIsSpatiallyLoaded = false;

   _visComponent = CreateEditorOnlyDefaultSubobject<UTATActorDependencyVisComponent>(TEXT("VisComponent"));
   if(_visComponent)
   {
      _visComponent->GroupKey = ATATSecretRoomTeleporter::StaticClass()->GetFName();
      _visComponent->Color = FColor::Cyan;
      _visComponent->IsReversed = true;
   }
#endif // WITH_EDITORONLY_DATA
}
