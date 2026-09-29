// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Graphics/TATAIVisionFieldRendererComponent.h"

// tat
#include "AI/TATAIStateWorldSubsystem.h"

// ose
#include "AI/Perception/OSEAISense_Sight.h"
#include "AI/Alertness/OSEAlertnessInterface.h"
#include "Camera/OSECameraUtils.h"
#include "Character/OSECharacterBase.h"

// ue5
#include "AIController.h"
#include "ProceduralMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIVisionFieldRendererComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATAIVisionFieldRenderer, Log, Log);

UTATAIVisionFieldRendererComponent::UTATAIVisionFieldRendererComponent()
{
   SetIsReplicatedByDefault(true);
}

void UTATAIVisionFieldRendererComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(UTATAIVisionFieldRendererComponent, _sightRuntimeSettings, COND_InitialOnly);
}

void UTATAIVisionFieldRendererComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority())
   {
      if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
      {
         if (AController* currentController = ownerCharacter->GetController())
         {
            _OnPossessedByController(currentController);
         }
         else
         {
            ownerCharacter->OnPossessedBy.AddUniqueDynamic(this, &UTATAIVisionFieldRendererComponent::_OnPossessedByController);
         }
      }
      else
      {
         UE_LOG(LogTATAIVisionFieldRenderer, Warning,
            TEXT("TATAIVisionFieldRendererComponent attached to '%s' which is not an AOSECharacterBase, will not display properly"),
            *GetOwner()->GetName());
      }
   }

   if (!IsRunningDedicatedServer())
   {
      static const FName kVisionFieldMeshName(TEXT("VisionFieldMesh"));

      TArray<UActorComponent*> meshComponents;
      GetOwner()->GetComponents(UProceduralMeshComponent::StaticClass(), meshComponents);
      for (UActorComponent* meshComponent : meshComponents)
      {
         if (meshComponent->ComponentHasTag(kVisionFieldMeshName))
         {
            _cachedVisionFieldMesh = CastChecked<UProceduralMeshComponent>(meshComponent);
            break;
         }
      }

      if (_cachedVisionFieldMesh == nullptr)
      {
         UE_LOG(LogTATAIVisionFieldRenderer, Error,
            TEXT("TATAIVisionFieldRendererComponent attached to '%s' could not find a mesh component with the '%s' component tag on it"),
            *GetOwner()->GetName(), *kVisionFieldMeshName.ToString());
      }
      else
      {
         FVector ownersEyeLocationInWorldSpace;
         FRotator unusedRotation;
         GetOwner()->GetActorEyesViewPoint(ownersEyeLocationInWorldSpace, unusedRotation);

         // Set the vision field mesh to match our owner's eyes location, since that is what is used by sight sense code
         _cachedVisionFieldMesh->SetWorldLocation(ownersEyeLocationInWorldSpace, false);
      }

      // Get the current alert level, and subscribe to future changes
      // The alert level influences the size of the sight frustum, so we also adjust the visuals
      if (IOSEAlertnessInterface* ownerAlertness = Cast<IOSEAlertnessInterface>(GetOwner()))
      {
         if (UOSEAlertnessComponent* alertnessComp = ownerAlertness->GetAlertnessComponent())
         {
            _OnAlertnessLevelChanged(_currentAlertLevel, alertnessComp->GetAlertnessLevel());
            alertnessComp->OnAlertnessLevelChanged.AddUniqueDynamic(this, &UTATAIVisionFieldRendererComponent::_OnAlertnessLevelChanged);
         }
      }

      if (UTATAIStateWorldSubsystem* aiStateWorldSubsystem = GetWorld()->GetSubsystem<UTATAIStateWorldSubsystem>())
      {
         // Set out initial state in case the player had the monocular opened when we spawned,
         // then bind to any future updates
         _OnLocalPlayerUsingMonocularChanged(aiStateWorldSubsystem->GetLocalPlayerIsUsingMonocular());
         aiStateWorldSubsystem->OnLocalPlayerMonocularUsageChanged.AddUniqueDynamic(this, &UTATAIVisionFieldRendererComponent::_OnLocalPlayerUsingMonocularChanged);
      }
   }
}


void UTATAIVisionFieldRendererComponent::EndPlay(const EEndPlayReason::Type reason)
{
   if (UWorld* world = GetWorld())
   {
      if (UTATAIStateWorldSubsystem* aiStateWorldSubsystem = world->GetSubsystem<UTATAIStateWorldSubsystem>())
      {
         aiStateWorldSubsystem->OnLocalPlayerMonocularUsageChanged.RemoveAll(this);
      }
   }

   if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
   {
      ownerCharacter->OnPossessedBy.RemoveAll(this);
   }

   Super::EndPlay(reason);
}

#if !NO_LOGGING
void UTATAIVisionFieldRendererComponent::PreReplication(IRepChangedPropertyTracker& changedPropertyTracker)
{
   Super::PreReplication(changedPropertyTracker);

   _hasReplicatedAtLeastOnce = true;
}
#endif

void UTATAIVisionFieldRendererComponent::RebuildMeshFromSightSettings_Implementation(const FOSEPerAlertLevelSettings& sightSettings, UProceduralMeshComponent* proceduralMesh)
{
   float nearClip = FMath::Max(sightSettings.NearClippingRadius, 1.0f);
   float farClip = FMath::Max(sightSettings.SightRadius, nearClip + 1.0f);

   const float halfFOVInRadians = FMath::DegreesToRadians(FMath::Clamp(sightSettings.PeripheralVisionAngleDegrees, 0.1f, 89.9f));
   const float aspectRatio = FMath::Max(sightSettings.FrustumAspectRatio, KINDA_SMALL_NUMBER);

   FRotator pitchedObserverRotation = FRotator(sightSettings.FrustumPitch, 0, 0);

   FMatrix projectionMatrix = UOSECameraUtils::BuildViewProjectionMatrix(
      FVector(-sightSettings.PointOfViewBackwardOffset, 0, 0),
      pitchedObserverRotation,
      halfFOVInRadians,
      aspectRatio,
      nearClip,
      farClip);

   FMatrix invProjectionMatrix = projectionMatrix.Inverse();

   TArray<FVector> vertices;
   TArray<FVector2D> uv0;
   vertices.Reserve(8);
   uv0.Reserve(8);
   for (uint32 Z = 0; Z < 2; Z++)
   {
      for (uint32 Y = 0; Y < 2; Y++)
      {
         for (uint32 X = 0; X < 2; X++)
         {
            // Unproject a normalized vertex to get the coordinates of the frustum corners
            FVector4 unprojectedVertex = invProjectionMatrix.TransformFVector4(
               FVector4(
                  (X ? -1.0f : 1.0f),
                  (Y ? -1.0f : 1.0f),
                  (Z ? 0.0f : 1.0f),
                  1.0f
               )
            );

            vertices.Add(FVector(unprojectedVertex) / unprojectedVertex.W);

            // Put depth into the x coord of the UVs
            uv0.Add(FVector2D((Z ? 0.0f : 1.0f), 0.0f));
         }
      }
   }

   TArray<int32> triangleIndices = {
      // far face
      2, 1, 0,
      3, 1, 2,

      // near face
      4, 5, 6,
      6, 5, 7,

      // right face
      4, 2, 0,
      6, 2, 4,

      // left face
      1, 3, 5,
      5, 3, 7,

      // bottom face
      2, 7, 3,
      6, 7, 2,

      // top face
      5, 0, 1,
      4, 0, 5
   };

   TArray<FVector> normals;
   TArray<FColor> vertexColors;
   TArray<FProcMeshTangent> tangents;

   proceduralMesh->CreateMeshSection(
      0,
      vertices,
      triangleIndices,
      normals,
      uv0,
      vertexColors,
      tangents,
      false
   );
}

void UTATAIVisionFieldRendererComponent::_OnRep_SightRuntimeSettings()
{
   _OnSightRuntimeSettingsChanged();
}

void UTATAIVisionFieldRendererComponent::_OnAlertnessLevelChanged(EAlertnessLevel /*oldAlertnessLevel*/, EAlertnessLevel newAlertnessLevel)
{
   _currentAlertLevel = newAlertnessLevel;
   _RebuildVisionMesh();
}

void UTATAIVisionFieldRendererComponent::_OnLocalPlayerUsingMonocularChanged(bool isLocalPLayerUsingMonocular)
{
   if (_cachedVisionFieldMesh)
   {
      _cachedVisionFieldMesh->SetVisibility(isLocalPLayerUsingMonocular);
   }
}

void UTATAIVisionFieldRendererComponent::_OnPossessedByController(AController* currentController)
{
   check(GetOwner()->HasAuthority());

#if !NO_LOGGING
   if (_hasReplicatedAtLeastOnce)
   {
      UE_LOG(LogTATAIVisionFieldRenderer, Error,
         TEXT("TATAIVisionFieldRendererComponent on '%s' was possessed after it had initially replicated. ")
         TEXT("This will very likely break it on clients, since _sightRuntimeSettings is set to only replicate the initial numbers, ")
         TEXT("which will need to know their controller before the actor is first replicated"),
         *GetOwner()->GetName());
   }
#endif

   if (AAIController* controller = Cast<AAIController>(currentController))
   {
      if (UAIPerceptionComponent* perceptionComponent = controller->GetPerceptionComponent())
      {
         if (auto* sightConfig = Cast<UOSEAISenseConfig_Sight>(perceptionComponent->GetSenseConfig(UAISense::GetSenseID<UOSEAISense_Sight>())))
         {
            _sightRuntimeSettings.NeutralValues = sightConfig->NeutralValues;
            _sightRuntimeSettings.SuspiciousValues = sightConfig->SuspiciousValues;
            _sightRuntimeSettings.AlertedValues = sightConfig->AlertedValues;
            _sightRuntimeSettings.CombatValues = sightConfig->CombatValues;
            _sightRuntimeSettings.HasSightSense = true;

            _OnSightRuntimeSettingsChanged();
         }
         else
         {
            UE_LOG(LogTATAIVisionFieldRenderer, Warning,
               TEXT("TATAIVisionFieldRendererComponent attached to '%s' whose controller '%s' has no OSEAISense_Sight on its PerceptionComponent, will not display properly"),
               *GetOwner()->GetName(), *controller->GetName());
         }
      }
      else
      {
         UE_LOG(LogTATAIVisionFieldRenderer, Warning,
            TEXT("TATAIVisionFieldRendererComponent attached to '%s' whose controller '%s' has no PerceptionComponent, will not display properly"),
            *GetOwner()->GetName(), *controller->GetName());
      }
   }
   else
   {
      UE_LOG(LogTATAIVisionFieldRenderer, Warning,
         TEXT("TATAIVisionFieldRendererComponent attached to '%s' with no AI controller at BeginPlay, will not display properly"),
         *GetOwner()->GetName());
   }
}

void UTATAIVisionFieldRendererComponent::_OnSightRuntimeSettingsChanged()
{
   _RebuildVisionMesh();
}

void UTATAIVisionFieldRendererComponent::_RebuildVisionMesh()
{
   if (_cachedVisionFieldMesh)
   {
      _cachedVisionFieldMesh->ClearAllMeshSections();

      if (_sightRuntimeSettings.HasSightSense)
      {
         const FOSEPerAlertLevelSettings& sightSettings = _GetSightSettingsForCurrentAlertLevel();
         RebuildMeshFromSightSettings(sightSettings, _cachedVisionFieldMesh);

         _cachedVisionFieldMesh->SetMaterial(0, VisionFieldMaterial);
      }
   }
}

const FOSEPerAlertLevelSettings& UTATAIVisionFieldRendererComponent::_GetSightSettingsForCurrentAlertLevel() const
{
   check(_sightRuntimeSettings.HasSightSense);
   switch (_currentAlertLevel)
   {
      case EAlertnessLevel::Neutral:
         return _sightRuntimeSettings.NeutralValues;
      case EAlertnessLevel::Suspicious:
         return _sightRuntimeSettings.SuspiciousValues;
      case EAlertnessLevel::Alerted:
         return _sightRuntimeSettings.AlertedValues;
      case EAlertnessLevel::Combat:
         return _sightRuntimeSettings.CombatValues;
      default:
         checkf(false, TEXT("UTATAIVisionFieldRendererComponent::_GetSightSettingsForCurrentAlertLevel called on unknown alert level: %d"), (int32)_currentAlertLevel);
         return _sightRuntimeSettings.NeutralValues;
   }
}

