// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "AI/Perception/OSEAISenseConfig_Sight.h"

#include "TATAIVisionFieldRendererComponent.generated.h"

USTRUCT()
struct FTATAISightRuntimeSettings
{
   GENERATED_BODY()

   UPROPERTY(Transient)
   FOSEPerAlertLevelSettings NeutralValues;

   UPROPERTY(Transient)
   FOSEPerAlertLevelSettings SuspiciousValues;

   UPROPERTY(Transient)
   FOSEPerAlertLevelSettings AlertedValues;

   UPROPERTY(Transient)
   FOSEPerAlertLevelSettings CombatValues;

   UPROPERTY(Transient)
   bool HasSightSense = false;
};

class UProceduralMeshComponent;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class UTATAIVisionFieldRendererComponent : public UActorComponent
{
GENERATED_BODY()
public:
   UTATAIVisionFieldRendererComponent();

   // From UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;
#if !NO_LOGGING
   // We override PreReplication just so we can validate the timing of initial replication
   // If we are in Test or Shipping, we don't override it at all
   virtual void PreReplication(IRepChangedPropertyTracker& changedPropertyTracker) override;
#endif

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   /// Used to create the mesh we create based on the current parameters to sight sense
   /// The default C++ implementation creates a frustum mesh that matches the shape used in sight calculations
   UFUNCTION(BlueprintNativeEvent)
   void RebuildMeshFromSightSettings(const FOSEPerAlertLevelSettings& sightSettings, UProceduralMeshComponent* proceduralMesh);

private:

   UFUNCTION()
   void _OnRep_SightRuntimeSettings();

   UFUNCTION()
   void _OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel);

   UFUNCTION()
   void _OnLocalPlayerUsingMonocularChanged(bool isLocalPLayerUsingMonocular);

   UFUNCTION()
   void _OnPossessedByController(AController* currentController);

   void _OnSightRuntimeSettingsChanged();

   void _RebuildVisionMesh();

   const FOSEPerAlertLevelSettings& _GetSightSettingsForCurrentAlertLevel() const;

public:
   /// The material applied to the vision field mesh that we create
   UPROPERTY(EditAnywhere)
   TObjectPtr<class UMaterialInterface> VisionFieldMaterial;

private:
   /// The values from our controller's perception w.r.t. AI Sight
   UPROPERTY(Transient, ReplicatedUsing= _OnRep_SightRuntimeSettings)
   FTATAISightRuntimeSettings _sightRuntimeSettings;

   UPROPERTY(Transient)
   UProceduralMeshComponent* _cachedVisionFieldMesh = nullptr;

   EAlertnessLevel _currentAlertLevel = EAlertnessLevel::Neutral;

#if !NO_LOGGING
   bool _hasReplicatedAtLeastOnce = false;
#endif
};

