// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "Tentacle/OSETentacleSettings.h"
#include "Tentacle/OSETentacleState.h"
#include "Tentacle/OSETentacleVisualData.h"
#include "OSETentacleComponent.generated.h"

// Forward references
class UFXSystemComponent;


//---------------------------------------------------------------------------------------
/// Tentacle component: creates and updates tentacles that reach out to and
/// retract from specified object types.
/// 
/// * Most properties can be animated and changed at run-time.
/// * Optimized to limit the maximum number of traces per tick to the tentacle count.
/// * Tentacles project from a local-space sphere.
/// * Fibonacci lattice for deterministic, even distribution across the surface area.
/// * Supports non-uniform scale.
/// * Tentacle length can be uniform, or specified per local-space axis.
/// * Search distance for new tentacles can be decreased.
/// * New tentacles can be skipped for a certain angle threshold.
/// * Randomized lifetime per tentacle.
/// * Updates and recycles FX components per tentacle.
/// * Tentacle hit locations are stored relative to the component they hit.
/// * and more...
//---------------------------------------------------------------------------------------

UCLASS(ClassGroup = ("Rendering|OSE"), Abstract, BlueprintType, Blueprintable,
   meta = (BlueprintSpawnableComponent, IsBlueprintBase = "true"),
   hidecategories = (Activation, AssetUserData, Collision, ComponentReplication, ComponentTick, Cooking, LOD, Physics, Replication, Tags))
class OSERENDERER_API UOSETentacleComponent : public USceneComponent
{
   GENERATED_BODY()

public:

   UOSETentacleComponent();

   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

protected:

   virtual void BeginPlay() override;

private:

   //---------------------------------------------------------------------------------------
   // Default properties
   //---------------------------------------------------------------------------------------

   /// Maximum number of tentacles out at any given time
   UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetTentacleCount, Category = Tentacle, meta = (AllowPrivateAccess = true, ClampMin = 1))
   int32 _tentacleCount = 5;

   /// The _optional_ FX system template to spawn. By default we will look for an FX component
   /// that is a direct child of this component. If an FX system template is specified here,
   /// we will dynamically create one and use that instead.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tentacle, AdvancedDisplay, meta = (AllowPrivateAccess = true))
   TObjectPtr<class UFXSystemAsset> _visualFXTemplate;

   /// The object types that the tentacles will collide with
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tentacle, AdvancedDisplay, meta = (AllowPrivateAccess = true))
   TArray< TEnumAsByte<enum EObjectTypeQuery> > _collisionTypes;


   //---------------------------------------------------------------------------------------
   // Animatable properties that support changes at runtime
   //---------------------------------------------------------------------------------------

   /// Size properties
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TentacleSize, meta = (ShowOnlyInnerProperties, AllowPrivateAccess = true))
   FOSETentacleSettings_Size _sizeSettings;

   /// Speed properties
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TentacleSpeed, meta = (ShowOnlyInnerProperties, AllowPrivateAccess = true))
   FOSETentacleSettings_Speed _speedSettings;

   /// Spawn properties
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TentacleSpawn, meta = (ShowOnlyInnerProperties, AllowPrivateAccess = true))
   FOSETentacleSettings_Spawn _spawnSettings;

   /// Length properties
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TentacleLength, meta = (ShowOnlyInnerProperties, AllowPrivateAccess = true))
   FOSETentacleSettings_Length _lengthSettings;

public:

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTentacleStateChanged, FHitResult, hitResult);

   /// Delegate fired when a tentacle starts extending.
   /// The hit result represents the trace from the root (TraceStart) to the target (Location).
   UPROPERTY(BlueprintAssignable, Category = Tentacle)
   FOnTentacleStateChanged OnTentacleBeginExtend;

   /// Delegate fired when a tentacle starts retracting.
   /// The hit result represents the trace from the root (TraceStart) to the target (Location),
   /// but may be retracting before reaching the target.
   UPROPERTY(BlueprintAssignable, Category = Tentacle)
   FOnTentacleStateChanged OnTentacleBeginRetract;

   /// NOTE: This is a convenience function only. Returns an aggregated, const-only
   /// alias to the individual settings structs. For more details \see FOSETentacleSettings
   FORCEINLINE FOSETentacleSettings GetSettings() const { return FOSETentacleSettings(_sizeSettings, _speedSettings, _spawnSettings, _lengthSettings); }

   UFUNCTION(BlueprintGetter)
   FORCEINLINE int32 GetTentacleCount() const { return _tentacleCount; }

   /// Returns the visual data for the i-th tentacle (remapping is applied)
   const FOSETentacleVisualData& GetVisualData(int32 index) const;

   /// Returns all visual data in an array.
   /// (No remapping from unique ID to index is applied)
   void GetVisualDataArray(TArray<FOSETentacleVisualData>& outDataArray) const;

private:

   /// Returns the visual data from the array at the specified index.
   /// (No remapping from unique ID to index is applied)
   FORCEINLINE const FOSETentacleVisualData& _GetVisualData_DirectIdx(int32 i) const { return _visualDataArray[i]; }

   /// Set the visual data in the array at the specified index.
   /// (No remapping from unique ID to index is applied)
   FORCEINLINE void _SetVisualData_DirectIdx(int32 i, const FOSETentacleVisualData& visualData) { _visualDataArray[i] = visualData; }

   /// Sets the visual data for the i-th tentacle (remapping is applied)
   void _SetVisualData(int32 index, const FOSETentacleVisualData& visualData);

   /// Updates the distribution on the sphere based on area
   void _UpdateSphereDistribution();

   /// Returns the minimum length, which is currently based on the radius / diameter.
   /// This is just to filter out excessively short lengths; the _queryLengthDecrease
   /// property is used to filter out new entries.
   float _GetMinLength() const;

   /// Returns the minimum / maximum length to use for a given rotation
   FFloatInterval _GetLengthForRotation(const FQuat& inRotation) const;

   /// Returns true if the rotation and length is valid; used to filter out certain results if desired
   bool _IsValidSphereRotation(const FQuat& inRotation, const FFloatInterval& inLength) const;

   /// Sets the sphere rotation and length to use given a unique index.
   /// Returns true if the rotation and length are valid.
   bool _GetSphereRotation(int32 inSphereIdx, FQuat& outRotation, FFloatInterval& outLength) const;

   /// Sets the next sphere rotation and length to use and advances the index.
   /// Returns true if the rotation and length are valid.
   bool _GetNextSphereRotation(FQuat& outRotation, FFloatInterval& outLength);

   /// Performs a trace at the surface point as specified by the local space rotation, towards the world space target, at the world space distance.
   /// Returns true if the trace is valid to use.
   bool _PerformTrace(FHitResult& outHitResult, const FHitResult& inPreviousHit, const FQuat& inSphereRotation, const FFloatInterval& inLength) const;
   bool _PerformTrace(FHitResult& outHitResult, const FHitResult& inPreviousHit, const FQuat& inSphereRotation, const FVector& inWorldTargetPos, const FFloatInterval& inLength) const;

   /// Acquires the next available visual index
   int32 _AcquireVisualDataIndex();

   /// Recycles a previously acquired visual index for reuse
   void _ReleaseVisualDataIndex(int32 inVisualIdx);

   /// Returns the local to world transform to use for updating visuals. This may be identity if we're operating in world space.
   FTransform _GetVisualLocalToWorldTransform() const;

   /// Updates the specified visualization data with the specified local to world transform.
   /// The struct is not initialized; only new values are set.
   void _UpdateVisualData(FOSETentacleVisualData& visualData, const FOSETentacleState& state, const FTransform& localToWorld) const;

   /// Updates the data for a single tentacle for visualization
   void _UpdateVisualData(const FOSETentacleState& state, const FTransform& localToWorld);

   /// Updates all data for visualization
   void _UpdateVisualData();

#if ENABLE_DRAW_DEBUG

   /// Draws debug visualizations for all tentacles
   void _DrawDebugState() const;
   
   /// Draws debug visualization of a tentacle
   void _DrawDebugState(const FOSETentacleVisualData& visualData) const;
#endif

private:

   //---------------------------------------------------------------------------------------
   // Internal state
   //---------------------------------------------------------------------------------------

   FCollisionObjectQueryParams _objectQueryParams;
   FCollisionQueryParams _collisionQueryParams;
   FVector _localOrigin = FVector::Zero();
   FVector _worldOrigin = FVector::Zero();
   FVector _prevWorldOrigin = FVector::Zero();

   /// The number of points we want to distribute. Given the two radii, use approximate area to determine this. The value can change at run-time!
   uint16 _numSpherePoints = 0;

   /// Deterministic order of sphere points (modulo the number), so works even when count changes!
   uint16 _currSphereIndex = 0;

   /// Timer to keep track of how frequently we've extended new tentacles.
   float _timeSinceLastExtend = 0;

   /// Bookkeeping value used to recycle and index into the visual data remap
   int32 _nextAvailFX = 0;

   /// Remaps any n-th tentacle to the specific index used for the visual data array
   TArray<int32> _visualDataRemap;

   /// The single FX component responsible for rendering every tentacle
   UPROPERTY(Transient, DuplicateTransient)
   TWeakObjectPtr<UFXSystemComponent> _visualFXComponent;

   /// The array of visual data per tentacle. This array is never sorted. Use the remap array to map a specific tentacle to it's index.
   UPROPERTY(Transient, DuplicateTransient)
   TArray<FOSETentacleVisualData> _visualDataArray;

   /// Extended or in the act of extending
   UPROPERTY(Transient, DuplicateTransient)
   TArray<FOSETentacleState> _extendArray;

   /// Retracted or in the act of retracting
   UPROPERTY(Transient, DuplicateTransient)
   TArray<FOSETentacleState> _retractArray;
};
