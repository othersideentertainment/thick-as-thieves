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
#include "AkAcousticTexture.h"
#include "AkSurfaceReflectorSetUtils.generated.h"

namespace AkSurfaceReflectorUtils
{
	static float EQUALITY_THRESHOLD = 0.001f;
}

/** An edge between two vertices */
USTRUCT()
struct FAkSurfaceEdgeVerts
{
	GENERATED_BODY()
	static bool EdgesShareVertex(const FAkSurfaceEdgeVerts& Edge1, const FAkSurfaceEdgeVerts& Edge2);

	FAkSurfaceEdgeVerts() {}
	FAkSurfaceEdgeVerts(FVector InV0, FVector InV1) : V0(InV0), V1(InV1) {}
	FVector V0 = FVector::ZeroVector;
	FVector V1 = FVector::ZeroVector;

	FVector GetUnitVector() const { return (V1 - V0).GetSafeNormal(); }

	FAkSurfaceEdgeVerts GetTransformedEdge(const FTransform& Transform) const;
	void TransformEdge(const FTransform& Transform);
	void Invert();
};

/** Information about an edge */
USTRUCT()
struct FAkSurfaceEdgeInfo
{
	GENERATED_BODY()
	FAkSurfaceEdgeVerts EdgeVerts;
	FVector Normal;
	bool IsEnabled = true;
	bool IsBoundary = true;
	bool IsFlat = false;

	const FVector V0() const { return EdgeVerts.V0; }
	const FVector V1() const { return EdgeVerts.V1; }

	void SetV0(FVector V0) { EdgeVerts.V0 = V0; }
	void SetV1(FVector V1) { EdgeVerts.V1 = V1; }

	int64 GetHash()
	{
		int64 H0 = GetTypeHash(V0());
		int64 H1 = GetTypeHash(V1());
		if (H1 > H0)
		{
			int64 temp = H0;
			H0 = H1;
			H1 = temp;
		}
		return H1 << 32 | H0;
	}

	FAkSurfaceEdgeInfo();
	FAkSurfaceEdgeInfo(FVector InV0, FVector InV1);
};

/** Contains the properties of a face from the ParentBrush of a UAkSurfaceReflectorSetComponent. */
USTRUCT(BlueprintType)
struct FAkSurfacePoly
{
	GENERATED_BODY()

	/**
	* The Acoustic Texture associated with the selected surfaces.
	* When set to None, the texture is completely reflective.
	* If the Surface Reflector component is disabled, the geometry is not used for reflections or diffractions.
	* In this case, Acoustic Textures are used exclusively to measure Environment Decay and HFDamping as part of the Reverb Estimation process.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface Properties")
	TObjectPtr<UAkAcousticTexture> Texture = nullptr;

	/**
	* Indicates how much sound is transmitted through the selected surfaces. Valid range is 0 to 1.
	* With a Transmission Loss value of 0, all sounds pass through the surface, and the Acoustic Texture has no effect.
	* With a value of 1, 100% Transmission Loss is applied to sounds that pass through the selected surfaces.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface Properties", DisplayName = "Transmission Loss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Occlusion = 1.f;

	/**
	* An enabled surface is associated with the selected Acoustic Texture and Transmission Loss value above.
	* A disabled surface is not associated with an Acoustic Texture, and has a Transmission Loss value of 0 (sound passes through the surface).
	* If Surface Reflector Set is disabled, there is no Transmission Loss property to customize.
	* In this case, enabled surfaces do not let sound pass through (equivalent to a Transmission Loss value of 1) and
	* disabled surfaces let sound pass through (equivalent to a Transmission Loss value of 0).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface Properties")
	bool EnableSurface = true;

	void SetSurfaceArea(float area) { SurfaceArea = area; }
	float GetSurfaceArea() const { return SurfaceArea; }

#if WITH_EDITOR
	FVector Normal;
	FVector MidPoint;
	/* The edges of the polygon */
	TArray<FAkSurfaceEdgeVerts> Edges;
	/* Keeps track of the optimal dot product between the chosen up vector and view camera up vector.
		This is used to minimize the flickering of text as it switches between edges each frame. */
	mutable float OptimalEdgeDP = 0.0f;

	void ClearEdgeInfo();

	FText GetPolyText(bool includeOcclusion) const;
#endif

private:
	UPROPERTY()
	float SurfaceArea = 0.0f;
};
