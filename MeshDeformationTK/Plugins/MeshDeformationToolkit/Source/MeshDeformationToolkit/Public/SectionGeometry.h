// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "ProceduralMeshComponent.h"	// Needed for FProcMeshTangent
#include "SectionGeometry.generated.h"

/// This struct stores all of the data for a single section of geometry
/// and is basically all of the results from *UKismetProceduralMeshLibrary::GetSectionFromStaticMesh*,
/// or passed into *ProceduralMeshComponent::CreateMeshSection_LinearColor*,
/// packaged into a single entity.
USTRUCT(BlueprintType)
struct FSectionGeometry
{
	GENERATED_USTRUCT_BODY()

	/// The vertex data
		UPROPERTY(BlueprintReadWrite, meta=(Keywords="points"))
		TArray<FVector> vertices;

	/// The triangles
	///
	/// These are indices of the vertex data and the first triangle will be stored at
	/// triangles[0], triangles[1], and triangles[2], the second as triangles[3],
	/// triangles[4], and triangles[5].. and so on.
	UPROPERTY(BlueprintReadWrite, meta=(Keywords="faces polys polygons tris"))
		TArray<int32> triangles;

	/// These are the normals for each vertex
	UPROPERTY(BlueprintReadWrite)
		TArray<FVector> normals;

	/// These are the UV values for each vertex
	/// \todo Think about supporting multiple UV sets.
	UPROPERTY(BlueprintReadWrite, meta=(Keywords="texture map"))
		TArray<FVector2D> uvs;

	/// These are the tangents for each vertex
	UPROPERTY(BlueprintReadWrite)
		TArray<FProcMeshTangent> tangents;

	/// These are the colors of each vertex
	UPROPERTY(BlueprintReadWrite)
		TArray<FLinearColor> vertexColors;

	/// Simple constructor of empty section
	FSectionGeometry()
	{
		vertices=TArray<FVector>();
		triangles=TArray<int32>();
		normals=TArray<FVector>();
		uvs=TArray<FVector2D>();
		tangents=TArray<FProcMeshTangent>();
		vertexColors=TArray<FLinearColor>();
	}
};