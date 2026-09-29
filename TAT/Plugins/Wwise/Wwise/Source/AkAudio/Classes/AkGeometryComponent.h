/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#pragma once
#include "Platforms/AkUEPlatform.h"
#include "AkAcousticTexture.h"
#include "WwiseUnrealDefines.h"
#include "Components/SceneComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "AkAcousticTextureSetComponent.h"
#include "AkGeometryData.h"
#include "AkGeometryComponent.generated.h"

class UAkSettings;
class UMaterialInterface;

DECLARE_DELEGATE(FOnRefreshDetails);

UENUM()
enum class AkMeshType : uint8
{
	StaticMesh,
	CollisionMesh UMETA(DisplayName = "Simple Collision")
};

USTRUCT(BlueprintType)
struct FAkGeometrySurfaceOverride
{
	GENERATED_BODY()

	/** The Acoustic Texture represents the sound absorption on the surface of the geometry when a sound bounces off of it.
	* If left to None, the mesh's physical material will be used to fetch an acoustic texture.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	TObjectPtr<UAkAcousticTexture> AcousticTexture = nullptr;

	/** Enable Transmission Loss Override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "Enable Transmission Loss Override", Category = "Geometry")
	bool bEnableOcclusionOverride = false;

	/** Transmission loss value to set when modeling sound transmission through geometry. Transmission is modeled only when there is no direct line of sight from the emitter to the listener.
	* If there is more than one surface between the emitter and the listener, the maximum of each surface's transmission loss value is used. If the emitter and listener are in different rooms, the room's transmission loss value is taken into account.
	* Valid range : (0.0, 1.0)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry", DisplayName = "Transmission Loss", meta = (EditCondition = "bEnableOcclusionOverride", ClampMin = "0.0", ClampMax = "1.0"))
	float OcclusionValue = 1.f;

	void SetSurfaceArea(float area) { SurfaceArea = area; }

	FAkGeometrySurfaceOverride()
	{
		AcousticTexture = nullptr;
		bEnableOcclusionOverride = false;
		OcclusionValue = 1.f;
	}

private:
	UPROPERTY()
	float SurfaceArea = 0.0f;

};

UCLASS(ClassGroup = Audiokinetic, BlueprintType, hidecategories = (Transform, Rendering, Mobility, LOD, Component, Activation, Tags), meta = (BlueprintSpawnableComponent))
class AKAUDIO_API UAkGeometryComponent : public UAkAcousticTextureSetComponent
{
	GENERATED_BODY()

public:
	UAkGeometryComponent(const class FObjectInitializer& ObjectInitializer);

	/** Convert the mesh into a local representation suited for Wwise:
	* a set of vertices, triangles, surfaces, acoustic textures and transmission loss values. */
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	void ConvertMesh();

	/** Add or update a geometry in Spatial Audio by sending the converted mesh, as well as the rest of the AkGeometryParams to Wwise.
	* It is necessary to create at least one geometry instance for each geometry set that is to be used for diffraction and reflection simulation. See UpdateGeometry(). */
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	void SendGeometry();

	/** Add or update an instance of the geometry by sending the transform of this component to Wwise.
	* A geometry instance is a unique instance of a geometry set with a specified transform (position, rotation and scale).
	* It is necessary to create at least one geometry instance for each geometry set that is to be used for diffraction and reflection simulation. */
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	void UpdateGeometry();

	/** Remove the geometry and the corresponding instance from Wwise. */
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	void RemoveGeometry();

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Geometry")
	AkMeshType MeshType = AkMeshType::CollisionMesh;

	/** The Static Mesh's LOD to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry", meta = (ClampMin = "0.0"))
	int LOD = 0;

	/** The local distance in Unreal units between two vertices to be welded together.
	* Any two vertices closer than this threshold will be treated as the same unique vertex and assigned the same position.
	* Increasing this threshold decreases the number of gaps between triangles, resulting in a more continuous mesh and less sound leaking though, as well as eliminating triangles that are too small to be significant.
	* Increasing this threshold also helps Spatial Audio's edge-finding algorithm to find more valid diffraction edges.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry", meta = (ClampMin = "0.0"))
	float WeldingThreshold = .0f;

	/** Override the acoustic properties of this mesh per material.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry", DisplayName = "Acoustic Properties Override")
	TMap<TObjectPtr<UMaterialInterface>, FAkGeometrySurfaceOverride> StaticMeshSurfaceOverride;

	/** Override the acoustic properties of the collision mesh.*/
	UPROPERTY(VisibleAnywhere, Category = "Geometry", DisplayName = "Acoustic Properties Override")
	FAkGeometrySurfaceOverride CollisionMeshSurfaceOverride;

	/**
	 * Get the Acoustic Properties overriding this Geometry.
	 * @param InMaterialInterface - If this Geometry's Mesh Type is set to Static Mesh, provide the Material Interface that the requested Acoustic Properties override. Leave empty if the Mesh Type is set to Simple Collision.
	 * @param OutAcousticPropertiesOverride - The requested Acoustic Properties Override.
	 * @return True if OutAcousticPropertiesOverride is valid.
	**/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	bool GetAcousticPropertiesOverride(UMaterialInterface* InMaterialInterface, FAkGeometrySurfaceOverride& OutAcousticPropertiesOverride);

	/**
	 * Set the Acoustic Properties overriding this Geometry.
	 * @param InMaterialInterface - If this Geometry's Mesh Type is set to Static Mesh, provide the Material Interface to override. Leave empty if the Mesh Type is set to Simple Collision.
	 * @param InAcousticPropertiesOverride - Structure of Acoustic Properties Override to set with.
	 * @param OutAcousticPropertiesOverride - Reference to the modified Acoustic Properties Override.
	 * @return True if OutAcousticPropertiesOverride is valid.
	**/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	bool SetAcousticPropertiesOverride(UMaterialInterface* InMaterialInterface, FAkGeometrySurfaceOverride InAcousticPropertiesOverride, FAkGeometrySurfaceOverride& OutAcousticPropertiesOverride);

	/**
	 * Set the Acoustic Texture overriding this Geometry.
	 * @param InMaterialInterface - If this Geometry's Mesh Type is set to Static Mesh, provide the Material Interface to override. Leave empty if the Mesh Type is set to Simple Collision.
	 * @param InAcousticTexture - Acoustic Texture to set with.
	 * @param OutAcousticPropertiesOverride - Reference to the modified Acoustic Properties Override.
	 * @return True if OutAcousticPropertiesOverride is valid.
	**/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	bool SetAcousticTextureOverride(UMaterialInterface* InMaterialInterface, UAkAcousticTexture* InAcousticTexture, FAkGeometrySurfaceOverride& OutAcousticPropertiesOverride);

	/**
	 * Set the Transmission Loss overriding this Geometry.
	 * @param InMaterialInterface - If this Geometry's Mesh Type is set to Static Mesh, provide the Material Interface to override. Leave empty if the Mesh Type is set to Simple Collision.
	 * @param InTransmissionLoss - Transmission Loss value to set with.
	 * @param OutAcousticPropertiesOverride - Reference to the modified Acoustic Properties Override.
	 * @return True if OutAcousticPropertiesOverride is valid.
	**/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	bool SetTransmissionLossOverride(UMaterialInterface* InMaterialInterface, float InTransmissionLoss, bool bInEnableTransmissionLossOverride, FAkGeometrySurfaceOverride& OutAcousticPropertiesOverride);

	/**
	 * Enable or disable the transmission loss of this Geometry to be overriden.
	 * @param InMaterialInterface - If this Geometry's Mesh Type is set to Static Mesh, provide the Material Interface to override. Leave empty if the Mesh Type is set to Simple Collision.
	 * @param bInEnableTransmissionLossOverride - Set to true to enable Transmission Loss override.
	 * @param OutAcousticPropertiesOverride - Reference to the modified Acoustic Properties Override.
	 * @return True if OutAcousticPropertiesOverride is valid.
	**/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	bool SetEnableTransmissionLossOverride(UMaterialInterface* InMaterialInterface, bool bInEnableTransmissionLossOverride, FAkGeometrySurfaceOverride& OutAcousticPropertiesOverride);

	/** Enable or disable geometric diffraction for this mesh. Check this box to have Wwise Spatial Audio generate diffraction edges on the geometry. The diffraction edges will be visible in the Wwise game object viewer when connected to the game. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Geometry")
	bool bEnableDiffraction = false;

	/** Enable or disable geometric diffraction on boundary edges for this Geometry. Boundary edges are edges that are connected to only one triangle. Depending on the specific shape of the geometry, boundary edges may or may not be useful and it is beneficial to reduce the total number of diffraction edges to process.  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Geometry", meta = (EditCondition = "bEnableDiffraction"))
	bool bEnableDiffractionOnBoundaryEdges = false;

	/**
	* Enable or disable geometric diffraction for this mesh.
	* @param bInEnableDiffraction - Set to true to have Wwise Spatial Audio generate diffraction edges on the geometry.
	* @param bInEnableDiffractionOnBoundaryEdges - Set to true to enable geometric diffraction on boundary edges for this Geometry. Boundary edges are edges that are connected to only one triangle.
	*/
	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkGeometry")
	void SetEnableDiffraction(bool bInEnableDiffraction, bool bInEnableDiffractionOnBoundaryEdges);

	/**
	* When set to false (default), the intersection of the geometry instance with any portal bounding box is subtracted from the geometry.In effect, an opening is created at the portal location through which sound can pass.
	* When set to true, portals cannot create openings in the geometry instance. Enable this to allow the geometry instance to be an obstacle to paths going into or through portal bounds.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry", DisplayName = "Bypass Portal Subtraction [Experimental]")
	bool bBypassPortalSubtraction = false;

	/** A solid geometry instance applies transmission loss once for each time a transmission path enters and exits its volume, using the max transmission loss between each hit surface. A non-solid geometry instance is one where each surface is infinitely thin, applying transmission loss at each surface. This option has no effect if the Transmission Operation is set to Max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry")
	bool bSolid = false;

	float GetSurfaceAreaSquaredMeters(const int& surfaceIndex) const;

	void UpdateStaticMeshOverride();

#if WITH_EDITORONLY_DATA
	void SetOnRefreshDetails(const FOnRefreshDetails& in_delegate) { OnRefreshDetails = in_delegate; }
	void ClearOnRefreshDetails() { OnRefreshDetails.Unbind(); }
	const FOnRefreshDetails* GetOnRefreshDetails() { return &OnRefreshDetails; }

	bool bMeshMaterialChanged = false;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
#endif

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;
	virtual bool MoveComponentImpl(
		const FVector & Delta,
		const FQuat & NewRotation,
		bool bSweep,
		FHitResult * Hit,
		EMoveComponentFlags MoveFlags,
		ETeleportType Teleport) override;
	virtual void Serialize(FArchive& Ar) override;

	void GetTexturesAndSurfaceAreas(TArray<FAkAcousticTextureParams>& textures, TArray<float>& surfaceAreas) const override;

	/** Indicates whether this component was added dynamically by a sibling room component in order to send geometry to Wwise. */
	bool bWasAddedByRoom = false;

private:

	UPrimitiveComponent* Parent = nullptr;
	void InitializeParent();

	void CalculateSurfaceArea(UStaticMeshComponent* StaticMeshComponent);

	void ConvertStaticMesh(UStaticMeshComponent* StaticMeshComponent, const UAkSettings* AkSettings);
	void ConvertCollisionMesh(UPrimitiveComponent* PrimitiveComponent, const UAkSettings* AkSettings);
	void UpdateMeshAndArchetype(UStaticMeshComponent* StaticMeshComponent);
	void _UpdateStaticMeshOverride(UStaticMeshComponent* StaticMeshComponent);

	UPROPERTY()
	FAkGeometryData GeometryData;

	UPROPERTY()
	TMap<int, double> SurfaceAreas;
	
	TMap<TObjectPtr<UMaterialInterface>, FAkGeometrySurfaceOverride> PreviousStaticMeshSurfaceOverride;

	void BeginPlayInternal();
#if WITH_EDITOR
	virtual void HandleObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap) override;
	bool bRequiresDeferredBeginPlay = false;
	void RegisterAllTextureParamCallbacks() override;
	bool ContainsTexture(const FGuid& textureID) override;
#endif

#if WITH_EDITORONLY_DATA
	FOnRefreshDetails OnRefreshDetails;
	FDelegateHandle OnMeshMaterialChangedHandle;
#endif

	bool _SetAcousticPropertiesOverride(UMaterialInterface* InMaterialInterface, FAkGeometrySurfaceOverride InAcousticPropertiesOverride, FAkGeometrySurfaceOverride& OutAcousticPropertiesOverride);
	void OnCollisionAcousticPropertiesOverrideChanged();
	void OnStaticMeshAcousticPropertiesOverrideChanged(UMaterialInterface* InMaterialInterface);
};
