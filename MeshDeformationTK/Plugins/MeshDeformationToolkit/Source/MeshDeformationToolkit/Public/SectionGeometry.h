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

	/// The vertices in this section
	UPROPERTY(
		BlueprintReadWrite,
		meta=(
			Keywords="points"
			)
	)
		TArray<FVector> Vertices;

	/// The triangles
	///
	/// These are indices of the vertex data and the first triangle will be stored at
	/// triangles[0], triangles[1], and triangles[2], the second as triangles[3],
	/// triangles[4], and triangles[5].. and so on.
	UPROPERTY(
		BlueprintReadWrite,
		meta=(
			Keywords="faces polys polygons tris"
		))
		TArray<int32> Triangles;

	/// These are the normals for each vertex
	UPROPERTY(BlueprintReadWrite)
		TArray<FVector> Normals;

	/// These are the UV values for each vertex
	UPROPERTY(BlueprintReadWrite, meta=(Keywords="texture map"))
		TArray<FVector2D> UVs;

	/// These are the tangents for each vertex
	UPROPERTY(BlueprintReadWrite)
		TArray<FProcMeshTangent> Tangents;

	/// These are the colors of each vertex
	UPROPERTY(BlueprintReadWrite)
		TArray<FLinearColor> VertexColors;

	/// Simple constructor of empty section
	FSectionGeometry()
	{
		Vertices=TArray<FVector>();
		Triangles=TArray<int32>();
		Normals=TArray<FVector>();
		UVs=TArray<FVector2D>();
		Tangents=TArray<FProcMeshTangent>();
		VertexColors=TArray<FLinearColor>();
	}

	/// Copy constructor - copies the contents of one SectionGeometry to another
	FSectionGeometry(const FSectionGeometry &SourceSectionGeometry) {
		// All of the items can be simply copied using the TArray<> copy constructor
		Vertices = TArray<FVector>(SourceSectionGeometry.Vertices);
		Triangles = TArray<int32>(SourceSectionGeometry.Triangles);
		Normals = TArray<FVector>(SourceSectionGeometry.Normals);
		UVs = TArray<FVector2D>(SourceSectionGeometry.UVs);
		Tangents = TArray<FProcMeshTangent>(SourceSectionGeometry.Tangents);
		VertexColors = TArray<FLinearColor>(SourceSectionGeometry.VertexColors);
	}
};