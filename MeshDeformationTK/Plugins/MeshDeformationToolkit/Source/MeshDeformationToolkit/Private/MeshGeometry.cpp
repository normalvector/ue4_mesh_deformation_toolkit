// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "Engine/StaticMesh.h"
#include "KismetProceduralMeshLibrary.h"
#include "Runtime/Core/Public/Math/UnrealMathUtility.h" // ClosestPointOnLine/ClosestPointOnInfiniteLine, GetMappedRangeValue
#include "SelectionSet.h"
#include "FastNoise.h"
#include "Utility.h"
#include "Developer/RawMesh/Public/RawMesh.h" // The structure for building static meshes
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h" // Allows registering new static meshes

#include "MeshGeometry.h"
#include "Engine.h" // GEngine

UMeshGeometry::UMeshGeometry()
{
	// Create empty data sets.
	Sections = TArray<FSectionGeometry>();
}

void UMeshGeometry::Project(
	UObject* WorldContextObject,
	FTransform Transform,
	TArray <AActor *> IgnoredActors,
	FVector Projection /*= FVector(0, 0, -100)*/,
	float HeightAdjust /*= 0*/,
	bool bTraceComplex /*=true*/,
	ECollisionChannel CollisionChannel /*= ECC_WorldStatic*/,
	USelectionSet *Selection /*= nullptr */
) {
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Project")))
	{
		return;
	}

	// Get the world content we're operating in
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		UE_LOG(MDTLog, Error, TEXT("Project: Cannot access game world"));
		return;
	}

	// Prepare the trace query parameters
	const FName TraceTag("ProjectTraceTag");
	FCollisionQueryParams TraceQueryParams = FCollisionQueryParams();
	TraceQueryParams.TraceTag = TraceTag;
	TraceQueryParams.bTraceComplex = bTraceComplex;
	TraceQueryParams.AddIgnoredActors(IgnoredActors);

	// Convert the projection into local space as we'll need it for the projection
	// calculations and don't want to do it per-vert.  Also calculate the normalized
	// version and store that.
	const FVector ProjectionInLS = Transform.InverseTransformVector(Projection);
	const FVector ProjectionNormalInLS = ProjectionInLS.GetSafeNormal();

	// Get the distance to the base plane
	const float DistanceToBasePlane = MiniumProjectionPlaneDistance(-ProjectionInLS);
	const FVector PointOnBasePlane = Projection.GetSafeNormal() * DistanceToBasePlane;
	const FVector PointOnBasePlaneLS = ProjectionNormalInLS * DistanceToBasePlane;

	// Iterate over the sections, and the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section : this->Sections)
	{
		for (auto &Vertex : Section.Vertices)
		{
			// Scale the Projection vector according to the selectionSet, giving varying strength projections, all in World Space
			const FVector ScaledProjection = Projection * (Selection ? Selection->Weights[NextWeightIndex++] : 1.0f);

			// Compute the start/end positions of the trace
			const FVector TraceStart = Transform.TransformPosition(Vertex);
			const FVector TraceEnd = Transform.TransformPosition(
				Utility::NearestPointOnPlane(
					Vertex,
					PointOnBasePlaneLS + ScaledProjection.Size() *ProjectionNormalInLS,
					ProjectionNormalInLS
				)
			);

			// Do the actual trace
			FHitResult HitResult;
			bool bHitSuccess = World->LineTraceSingleByChannel(
				HitResult,
				TraceStart, TraceEnd,
				CollisionChannel, TraceQueryParams, FCollisionResponseParams()
			);

			// Position the vertex based on whether we had a hit or not.
			if (HitResult.bBlockingHit) {
				// Calculate the offset for the vertex- it's based on the distance to the
				// base plane.
				const float DistanceFromVertexToBasePlane =
					FVector::PointPlaneDist(Vertex, PointOnBasePlaneLS, ProjectionNormalInLS);
				const float HitProjectionHeight =
					DistanceFromVertexToBasePlane - HeightAdjust;

				Vertex = 
					Transform.InverseTransformPosition(
						HitResult.ImpactPoint
					) + ProjectionNormalInLS * HitProjectionHeight;
			}
			else {
				// No collision- just add the projection to the vertex.
				Vertex = Vertex + Transform.InverseTransformVector(ScaledProjection);;
			}
		}
	}
}

void UMeshGeometry::ProjectDown(
	UObject* WorldContextObject, 
	FTransform Transform,
	TArray <AActor *> IgnoredActors /*= nullptr*/,
	float ProjectionLength /*= 100*/,
	float HeightAdjust /*= 0*/,
	bool bTraceComplex /*= true*/,
	ECollisionChannel CollisionChannel /*= ECC_WorldStatic*/,
	USelectionSet *Selection /*= nullptr */)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("ProjectDown")))
	{
		return;
	}

	// Get the world content we're operating in
	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World)
	{
		UE_LOG(MDTLog, Error, TEXT("ProjectDown: Cannot access game world"));
		return;
	}

	// Prepare the trace query parameters
	const FName TraceTag("ProjectDownTraceTag");
	FCollisionQueryParams TraceQueryParams = FCollisionQueryParams();
	TraceQueryParams.TraceTag = TraceTag;
	TraceQueryParams.bTraceComplex = bTraceComplex;
	TraceQueryParams.AddIgnoredActors(IgnoredActors);

	// Calculate the projection vector.
	const FVector Projection = FVector(0, 0, -ProjectionLength);

	// Iterate over the sections, and the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section : this->Sections)
	{
		for (auto &Vertex : Section.Vertices)
		{
			// Scale the Projection vector according to the selectionSet, giving varying strength projections, all in World Space
			const FVector ScaledProjection = Projection * (Selection ? Selection->Weights[NextWeightIndex++] : 1.0f);

			// Compute the start/end positions of the trace
			const FVector TraceStart = Transform.TransformPosition(Vertex);
			const FVector TraceEnd =
				Transform.TransformPosition(FVector(Vertex.X, Vertex.Y, 0)) + ScaledProjection;

			// Do the actual trace
			FHitResult HitResult;
			bool bHitSuccess = World->LineTraceSingleByChannel(
				HitResult,
				TraceStart, TraceEnd,
				CollisionChannel, TraceQueryParams, FCollisionResponseParams()
			);

			// Position the vertex based on whether we had a hit or not.
			if (HitResult.bBlockingHit) {
				// Add the original .Z and heightAdjust to the hit result for the final collision output.
				Vertex =
					Transform.InverseTransformPosition(
						HitResult.ImpactPoint
					) + FVector(0,0,Vertex.Z + HeightAdjust);
			}
			else {
				// No collision- just add the projection to the vertex.
				Vertex = Vertex + Transform.InverseTransformVector(ScaledProjection);;
			}
		}
	}
}

void UMeshGeometry::FitToSpline(
	USplineComponent *SplineComponent,
	float StartPosition /*= 0.0f*/,
	float EndPosition /*= 1.0f*/,
	float MeshScale /*= 1.0f*/,
	UCurveFloat *ProfileCurve /*= nullptr*/,
	UCurveFloat *SectionProfileCurve /*= nullptr*/,
	USelectionSet *Selection /*= nullptr*/
)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("FitToSpline")))
	{
		return;
	}

	if (!SplineComponent)
	{
		UE_LOG(MDTLog, Warning, TEXT("FitToSpline: No SplineComponent"));
		return;
	}

	// Get the length of the spline
	const float SplineLength = SplineComponent->GetSplineLength();

	// Get the minimum X, and the range of X, for the mesh, we'll need them to build the spline.
	const FBox MeshBounds = this->GetBoundingBox();
	const float MinX = MeshBounds.Min.X;
	//const float rangeX = meshBounds.Max.X - minX

	// Build the ranges we'll be using for the remapping, we'll be going rangeX -> rangePosition,
	// and if we have a profile curve we'll need a range for that too, along with a '0-SplineLength' fixed range.
	const FVector2D RangeX = FVector2D(MeshBounds.Min.X, MeshBounds.Max.X);
	const FVector2D RangePosition = FVector2D(StartPosition, EndPosition);
	const FVector2D FullSplineRange = FVector2D(0.0f, SplineLength);

	// Iterate over the sections, and the vertices in the sections.
	int32 NextVertexIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// Remap the X position into the StartPosition/EndPosition range, then multiply by SplineLength to get a value we
			// can use for lookup.
			const float DistanceAlongSpline = FMath::GetMappedRangeValueClamped(RangeX, RangePosition, Vertex.X) * SplineLength;

			// If we have either profile curve we now need to find the position at a given point.  For efficiency
			//   we can combine this with MeshScale.
			float CombinedMeshScale = MeshScale;
			if (ProfileCurve)
			{
				// Get the range of the curve
				float ProfileCurveMin;
				float ProfileCurveMax;
				ProfileCurve->GetTimeRange(ProfileCurveMin, ProfileCurveMax);
				FVector2D ProfileCurveRange = FVector2D(ProfileCurveMin, ProfileCurveMax);

				const float PositionAlongCurve =
					FMath::GetMappedRangeValueClamped(FullSplineRange, ProfileCurveRange, DistanceAlongSpline);
				CombinedMeshScale = CombinedMeshScale * ProfileCurve->GetFloatValue(PositionAlongCurve);
			}
			if (SectionProfileCurve)
			{
				// Get the range of the curve
				float SectionCurveMin;
				float SectionCurveMax;
				SectionProfileCurve->GetTimeRange(SectionCurveMin, SectionCurveMax);
				FVector2D SectionProfileCurveRange = FVector2D(SectionCurveMin, SectionCurveMax);

				const float PositionAlongCurve =
					FMath::GetMappedRangeValueClamped(RangeX, SectionProfileCurveRange, Vertex.X);
				CombinedMeshScale = CombinedMeshScale * SectionProfileCurve->GetFloatValue(PositionAlongCurve);
			}

			// Get all of the splines's details at the distance we've converted X to- stick to local space
			const FVector Location = SplineComponent->GetLocationAtDistanceAlongSpline(
				DistanceAlongSpline, ESplineCoordinateSpace::Local
			);
			const FVector RightVector = SplineComponent->GetRightVectorAtDistanceAlongSpline(
				DistanceAlongSpline, ESplineCoordinateSpace::Local
			);
			const FVector UpVector = SplineComponent->GetUpVectorAtDistanceAlongSpline(
				DistanceAlongSpline, ESplineCoordinateSpace::Local
			);

			// Now we have the details we can use them to compute the final location that we need to use
			FVector SplineVertexPosition = Location+(RightVector * Vertex.Y * CombinedMeshScale)+(UpVector * Vertex.Z * CombinedMeshScale);
			Vertex = FMath::Lerp(
				Vertex, SplineVertexPosition,
				Selection ? Selection->Weights[NextVertexIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::FlipTextureUV(
	bool bFlipU /*= false*/,
	bool bFlipV /*= false*/,
	USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("FlipTextureUV")))
	{
		return;
	}

	// Iterate over the sections, and the uvs in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &UV:Section.UVs)
		{
			// Obtain the next weighting and check if it's >=0.5
			const bool bShouldFlip =
				Selection ? Selection->Weights[NextWeightIndex++]>=0.5 : true;

			// If we're meant to be flipping then flip the correct channels.
			if (bShouldFlip)
			{
				UV = FVector2D(
					bFlipU ? 1.0f-UV.X : UV.X,
					bFlipV ? 1.0f-UV.Y : UV.Y
				);
			}
		}
	}

}

bool UMeshGeometry::CheckGeometryIsValid(FString NodeNameForWarning) const
{
	// * Each section contains at least 3 vertices
	// * Each section contains at least 1 triangle
	// * Triangles contain a multiple of 3 points as every set of three defined one tri
	// * Has same number of normals as vertices

	// Track if any error occurred- means we can do multiple warnings.
	bool bErrorFound = false;

	// Iterate over the sections
	int32 NextSectionIndex = 0;
	for (auto Section:this->Sections)
	{
		// Each section should contain at least three vertices.
		const int32 SectionVertexCount = Section.Vertices.Num();
		if (SectionVertexCount<3)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d contains only %d vertices (3 required)"),
				*NodeNameForWarning, NextSectionIndex, SectionVertexCount
			);
			bErrorFound = true;
		}

		// Each section should contain at least one triangle
		const int32 TrianglePointNum = Section.Triangles.Num();
		if (TrianglePointNum<3)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d contains only %d triangle indices (3 required for one triangle)"),
				*NodeNameForWarning, NextSectionIndex, TrianglePointNum
			);
			bErrorFound = true;
		}

		/// Triangles contain a multiple of 3 points as every set of three defined one tri
		if ((TrianglePointNum%3)!=0)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d contains %d triangle indices (Should be a multiple of three as three per triangle)"),
				*NodeNameForWarning, NextSectionIndex, TrianglePointNum
			);
			bErrorFound = true;
		}

		/// Has same number of normals as vertices
		const int32 SectionNormalCount = Section.Normals.Num();
		if (SectionNormalCount!=SectionVertexCount)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d does not contain same number of vertices and normals (%d vertices, %d normals)"),
				*NodeNameForWarning, NextSectionIndex, SectionVertexCount, SectionNormalCount
			);
			bErrorFound = true;

		}

		++NextSectionIndex;
	}

	return bErrorFound;
}

UMeshGeometry * UMeshGeometry::Clone() const
{
	// Create a new MeshGeo with the same outer objec7t as us.
	UMeshGeometry *NewMeshGeo = NewObject<UMeshGeometry>(this->GetOuter());
	if (!NewMeshGeo)
	{
		return nullptr;
	}

	// Copy all of our sections
	NewMeshGeo->LoadFromMeshGeometry(this);

	return NewMeshGeo;
}

FBox UMeshGeometry::GetBoundingBox() const
{
	// Track the two corners of the bounding box
	FVector Min = FVector::ZeroVector;
	FVector Max = FVector::ZeroVector;

	// When we hit the first vertex we'll need to set both Min and Max
	//  to it as we'll have no comparison
	bool bHaveProcessedFirstVector = false;

	// Iterate over the sections, and the vertices in the sections.
	int32 NextSelectionIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			if (bHaveProcessedFirstVector)
			{
				// Do the comparison of both min/max.
				Min.X = FMath::Min(Min.X, Vertex.X);
				Min.Y = FMath::Min(Min.Y, Vertex.Y);
				Min.Z = FMath::Min(Min.Z, Vertex.Z);

				Max.X = FMath::Max(Max.X, Vertex.X);
				Max.Y = FMath::Max(Max.Y, Vertex.Y);
				Max.Z = FMath::Max(Max.Z, Vertex.Z);
			}
			else
			{
				// Set min/max to the first vertex
				Min = Vertex;
				Max = Vertex;
				bHaveProcessedFirstVector = true;
			}
		}
	}

	// Build a bounding box from the result
	return FBox(Min, Max);
}

float UMeshGeometry::GetRadius() const
{
	// This is the radius so far
	float Radius;

	// When we hit the first vertex we'll need to set both Min and Max
	//  to it as we'll have no comparison
	bool bHaveProcessedFirstVector = false;

	// Iterate over the sections, and the vertices in the sections.
	for (auto &Section : this->Sections)
	{
		for (auto &Vertex : Section.Vertices)
		{
			Radius = bHaveProcessedFirstVector ?
				FMath::Max(Radius, Vertex.Size()) :
				Vertex.Size();
		}
		bHaveProcessedFirstVector = true;
	}

	return Radius;
}

FString UMeshGeometry::GetSummary() const
{
	return FString::Printf(
		TEXT("%d sections, %d vertices, %d triangles"),
		this->GetSectionCount(), this->GetTotalVertexCount(), this->GetTotalTriangleCount()
	);
}

int32 UMeshGeometry::GetSectionCount() const
{
	return this->Sections.Num();
}

int32 UMeshGeometry::GetTotalTriangleCount() const
{
	int32 TotalTriangleCount = 0;
	for (auto Section:this->Sections)
	{
		TotalTriangleCount += Section.Triangles.Num();
	}
	return TotalTriangleCount/3; // 3pts per triangle
}

int32 UMeshGeometry::GetTotalVertexCount() const
{
	int32 TotalVertexCount = 0;
	for (auto Section:this->Sections)
	{
		TotalVertexCount += Section.Vertices.Num();
	}
	return TotalVertexCount;
}

void UMeshGeometry::Inflate(float Offset /*= 0.0f*/, USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Jitter")))
	{
		return;
	}

	// Shouldn't need to check normals- MeshGeometry shouldn't allow that the be different

	// Iterate over the sections, and the the vertices in the sections.
	// As we need normals to we'll use an index-based for loop here for verts.
	for (auto &Section:this->Sections)
	{
		for (int32 VertexIndex = 0; VertexIndex<Section.Vertices.Num(); ++VertexIndex)
		{
			Section.Vertices[VertexIndex] = FMath::Lerp(
				Section.Vertices[VertexIndex],
				Section.Vertices[VertexIndex]+(Section.Normals[VertexIndex]*Offset),
				Selection ? Selection->Weights[VertexIndex] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Jitter(FRandomStream &RandomStream, FVector Min, FVector Max, USelectionSet *Selection /*=nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Jitter")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 NextWeightIndex = 0;

	// Iterate over the sections, and the vertices in each section.
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			const FVector RandomJitter = FVector(
				RandomStream.FRandRange(Min.X, Max.X),
				RandomStream.FRandRange(Min.Y, Max.Y),
				RandomStream.FRandRange(Min.Z, Max.Z)
			);
			Vertex = FMath::Lerp(
				Vertex,
				Vertex+RandomJitter,
				Selection ? Selection->Weights[NextWeightIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Lerp(UMeshGeometry *TargetMeshGeometry, float Alpha /*= 0.0f*/, USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Lerp")))
	{
		return;
	}
	if (!TargetMeshGeometry)
	{
		UE_LOG(MDTLog, Warning, TEXT("Lerp: No TargetMeshGeometry"));
		return;
	}
	if (this->Sections.Num()!=TargetMeshGeometry->Sections.Num())
	{
		UE_LOG(
			MDTLog, Warning, TEXT("Lerp: Cannot lerp geometries with different numbers of sections, %d compared to %d"),
			this->Sections.Num(), TargetMeshGeometry->Sections.Num()
		);
		return;
	}

	// Iterate over the sections, and the vertices in the sections.  Do it by index so we
	// can access the same data from TargetMeshGeometry
	int32 NextWeightIndex = 0;
	for (int32 SectionIndex = 0; SectionIndex<this->Sections.Num(); SectionIndex++)
	{
		if (this->Sections[SectionIndex].Vertices.Num()!=TargetMeshGeometry->Sections[SectionIndex].Vertices.Num())
		{
			UE_LOG(
				MDTLog, Warning, TEXT("Lerp: Cannot lerp geometries with different numbers of vertices, %d compared to %d for section %d"),
				this->Sections[SectionIndex].Vertices.Num(), TargetMeshGeometry->Sections[SectionIndex].Vertices.Num(), SectionIndex
			);
			return;
		}

		for (int32 VertexIndex = 0; VertexIndex<this->Sections[SectionIndex].Vertices.Num(); ++VertexIndex)
		{
			// Get the existing data from the two components.
			FVector VertexFromThis = this->Sections[SectionIndex].Vertices[VertexIndex];
			FVector VertexFromTarget = TargetMeshGeometry->Sections[SectionIndex].Vertices[VertexIndex];
			FVector NormalFromThis = this->Sections[SectionIndex].Normals[VertexIndex];
			FVector NormalFromTarget = TargetMeshGeometry->Sections[SectionIndex].Normals[VertexIndex];

			// TODO: World/local logic should live here.
			this->Sections[SectionIndex].Vertices[VertexIndex] = FMath::Lerp(
				VertexFromThis, VertexFromTarget,
				Alpha * (Selection ? Selection->Weights[NextWeightIndex++] : 1.0f)
			);
			// TODO: World/local logic should live here.
			this->Sections[SectionIndex].Normals[VertexIndex] = FMath::Lerp(
				NormalFromThis, NormalFromTarget,
				Alpha * (Selection ? Selection->Weights[NextWeightIndex++] : 1.0f)
			);
		}
	}
}

void UMeshGeometry::LerpVector(FVector Position, float Alpha /*= 0.0*/, USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Lerp")))
	{
		return;
	}

	// Iterate over the sections, and the vertices in the sections.
	int32 NextVertexIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			Vertex = FMath::Lerp(
				Vertex,
				Position,
				Alpha * (Selection ? Selection->Weights[NextVertexIndex++] : 1.0f)
			);
		}
	}
}

void UMeshGeometry::MoveTowards(FVector Position, float Distance, bool bLimitAtPosition, USelectionSet *Selection /*= nullptr */)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("MoveTowards")))
	{
		return;
	}

	// Iterate over the sections, and the vertices in the sections.
	int32 NextVertexIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// Calculate the actual distance including the SelectionSet strength.
			float AdjustedDistance = Distance * (Selection ? Selection->Weights[NextVertexIndex++] : 1.0f);

			// If we're moving 'past' the position and are limited then stop there, otherwise move
			// the point
			if (bLimitAtPosition && AdjustedDistance>=FVector::Distance(Vertex, Position))
			{
				// We're not allowed to move through the Position so move to it.
				Vertex = Position;
			}
			else
			{
				// Move the vertex the correct distance
				Vertex = Vertex+(AdjustedDistance * (Position-Vertex).GetSafeNormal());
			}
		}
	}
}

bool UMeshGeometry::LoadFromMeshGeometry(const UMeshGeometry *SourceMeshGeometry)
{
	// If there's no source geometry we have nothing to do..
	if (!SourceMeshGeometry)
	{
		UE_LOG(MDTLog, Warning, TEXT("LoadFromMeshGeometry: No SourceMeshGeometry provided"));
		return false;
	}

	// Clear any existing geometry.
	this->Sections.Empty();

	// Iterate over the sections

	for (auto &SourceMeshSection : SourceMeshGeometry->Sections)
	{
		// Create the geometry for the section
		FSectionGeometry NewSectionGeometry = FSectionGeometry(SourceMeshSection);

		// Add the finished struct to the mesh's section list
		this->Sections.Emplace(NewSectionGeometry);
	}

	// Warn if the mesh doesn't look valid.  For now return it anyway but at least let
	// them know..
	CheckGeometryIsValid(TEXT("LoadFromMeshGeometry"));

	// All done
	return true;
}

bool UMeshGeometry::LoadFromStaticMesh(UStaticMesh *StaticMesh, int32 LOD /*= 0*/)
{
	// If there's no static mesh we have nothing to do..
	if (!StaticMesh)
	{
		UE_LOG(MDTLog, Warning, TEXT("LoadFromStaticMesh: No StaticMesh provided"));
		return false;
	}

	// Clear any existing geometry.
	this->Sections.Empty();

	// Iterate over the sections
	const int32 NumSections = StaticMesh->GetNumSections(LOD);
	for (int SectionIndex = 0; SectionIndex<NumSections; ++SectionIndex)
	{
		// Create the geometry for the section
		FSectionGeometry SectionGeometry;

		// Copy the static mesh's geometry for the section to the struct.
		UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(
			StaticMesh, LOD, SectionIndex,
			SectionGeometry.Vertices, SectionGeometry.Triangles,
			SectionGeometry.Normals, SectionGeometry.UVs, SectionGeometry.Tangents
		);

		// Load vertex colors with default values for as many vertices as needed
		SectionGeometry.VertexColors.InsertDefaulted(0, SectionGeometry.Vertices.Num());

		// Add the finished struct to the mesh's section list
		this->Sections.Emplace(SectionGeometry);
	}

	// Warn if the mesh doesn't look valid.  For now return it anyway but at least let
	// them know..
	CheckGeometryIsValid(TEXT("LoadFromStaticMesh"));

	// All done
	return true;
}

void UMeshGeometry::Rotate(
	FRotator Rotation /*= FRotator::ZeroRotator*/,
	FVector CenterOfRotation /*= FVector::ZeroVector*/,
	USelectionSet *Selection)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Rotate")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			Vertex = FMath::Lerp(
				Vertex,
				CenterOfRotation+Rotation.RotateVector(Vertex-CenterOfRotation),
				Selection ? Selection->Weights[NextWeightIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::RotateAroundAxis(
	FVector CenterOfRotation /*= FVector::ZeroVector*/,
	FVector Axis /*= FVector::UpVector*/, float AngleInDegrees /*= 0.0f*/,
	USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Jitter")))
	{
		return;
	}

	// Normalize the axis direction.
	auto NormalizedAxis = Axis.GetSafeNormal();
	if (NormalizedAxis.IsNearlyZero(0.01f))
	{
		UE_LOG(MDTLog, Warning, TEXT("RotateAroundAxis: Could not normalize Axis, zero vector?"));
		return;
	}

	// Iterate over the sections, and the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			FVector ClosestPointOnLine = FMath::ClosestPointOnInfiniteLine(
				CenterOfRotation, CenterOfRotation+Axis, Vertex
			);
			FVector OffsetFromClosestPoint = Vertex-ClosestPointOnLine;
			float ScaledRotation = FMath::Lerp(
				0.0f, AngleInDegrees,
				Selection ? Selection->Weights[NextWeightIndex++] : 1.0f
			);
			FVector RotatedOffset = OffsetFromClosestPoint.RotateAngleAxis(ScaledRotation, NormalizedAxis);
			Vertex = ClosestPointOnLine+RotatedOffset;
		}
	}
}

bool UMeshGeometry::SaveToProceduralMeshComponent(
	UProceduralMeshComponent *ProceduralMeshComponent,
	bool bCreateCollision)
{
	// If there's no PMC we have nothing to do..
	if (!ProceduralMeshComponent)
	{
		UE_LOG(MDTLog, Warning, TEXT("SaveToProceduralMeshComponent: No ProceduralMeshComponent provided"));
		return false;
	}


	// Clear the geometry
	ProceduralMeshComponent->ClearAllMeshSections();

	// Iterate over the mesh sections, creating a PMC MeshSection for each one.
	int32 NextSectionIndex = 0;
	for (auto section:this->Sections)
	{
		// Create the PMC section with the StaticMesh's data.
		ProceduralMeshComponent->CreateMeshSection_LinearColor(
			NextSectionIndex++,
			section.Vertices, section.Triangles, section.Normals,
			section.UVs, section.VertexColors, section.Tangents,
			bCreateCollision
		);
	}
	return true;
}

bool UMeshGeometry::SaveToStaticMesh(
	UStaticMesh *StaticMesh,
	UProceduralMeshComponent *ProceduralMeshComponent,
	TArray<UMaterialInterface *> Materials)
{
	// This will only work in the editor..
#if !WITH_EDITOR
	UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Cannot run outside of editor"));
	return false;
#endif

	// Check we have a static mesh
	if (!StaticMesh)
	{
		UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: No Static Mesh provided"));
		return false;
	}

	// Get the name of the object
	const FStringAssetReference StaticMeshAssetReference = FStringAssetReference(StaticMesh);
	if (!StaticMeshAssetReference.IsValid())
	{
		UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Cannot access name of Static Mesh"));
		return false;
	}
	const FName StaticMeshName = FName(*StaticMeshAssetReference.ToString());

	// Get the package name and asset name from the reference
	const FString PackageName = StaticMeshAssetReference.GetLongPackageName();
	const FString AssetName = StaticMeshAssetReference.GetAssetName();

	// Check the name is a valid long path
	if (!FPackageName::IsValidLongPackageName(PackageName))
	{
		UE_LOG(
			MDTLog, Error, TEXT("SaveToStaticMesh: '%s' is not a valid long package name"),
		   *PackageName
		);
		return false;
	}
	// We now have the name
	UE_LOG(
		MDTLog, Warning, TEXT("SaveToStaticMesh: Static mesh packagename=%s, assetname=%s"),
		*PackageName, *AssetName);

	// Check that we're not trying to update this too often using timer.
	// TODO: There should be a cleaner way to do this..
	static TMap<FString, double> StartTimesByName = TMap<FString, double>();
	const FString TimerName = StaticMeshAssetReference.ToString();
	const double StartTime = FPlatformTime::Seconds();
	const double RequiredTimeSinceLastBuild = 0.1f;	// In seconds.
	const double MinimumLastRebuildTime = StartTime-RequiredTimeSinceLastBuild;
	UE_LOG(
		MDTLog, Warning, TEXT("SaveToStaticMesh: Building at time=%f"),
		StartTime
	);
	if (StartTimesByName.Contains(TimerName) && StartTimesByName[TimerName]>MinimumLastRebuildTime)
	{
		UE_LOG(
			MDTLog, Warning, TEXT("SaveToStaticMesh: Rebuilt too recently, cannot rebuild again yet")
		);
		return false;
	}
	StartTimesByName.Emplace(TimerName, StartTime);

	// Save the object to the PMC
	this->SaveToProceduralMeshComponent(ProceduralMeshComponent, true);

	// Build the raw mesh data based on
	// https://github.com/EpicGames/UnrealEngine/blob/f794321ffcad597c6232bc706304c0c9b4e154b2/Engine/Plugins/Runtime/ProceduralMeshComponent/Source/ProceduralMeshComponentEditor/Private/ProceduralMeshComponentDetails.cpp
	FRawMesh RawMesh;
	const int32 NumSections = ProceduralMeshComponent->GetNumSections();
	int32 VertexBase = 0;
	for (int32 SectionIdx = 0; SectionIdx<NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection = ProceduralMeshComponent->GetProcMeshSection(SectionIdx);

		// Copy verts
		for (FProcMeshVertex& Vert:ProcSection->ProcVertexBuffer)
		{
			RawMesh.VertexPositions.Add(Vert.Position);
		}

		// Copy 'wedge' info
		int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
		for (int32 IndexIdx = 0; IndexIdx<NumIndices; IndexIdx++)
		{
			int32 Index = ProcSection->ProcIndexBuffer[IndexIdx];

			RawMesh.WedgeIndices.Add(Index+VertexBase);

			FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[Index];

			FVector TangentX = ProcVertex.Tangent.TangentX;
			FVector TangentZ = ProcVertex.Normal;
			FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal() * (ProcVertex.Tangent.bFlipTangentY ? -1.f : 1.f);

			RawMesh.WedgeTangentX.Add(TangentX);
			RawMesh.WedgeTangentY.Add(TangentY);
			RawMesh.WedgeTangentZ.Add(TangentZ);

			RawMesh.WedgeTexCoords[0].Add(ProcVertex.UV0);
			RawMesh.WedgeColors.Add(ProcVertex.Color);
		}

		// copy face info
		int32 NumTris = NumIndices/3;
		for (int32 TriIdx = 0; TriIdx<NumTris; TriIdx++)
		{
			RawMesh.FaceMaterialIndices.Add(SectionIdx);
			RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
		}

		// Update offset for creating one big index/vertex buffer
		VertexBase += ProcSection->ProcVertexBuffer.Num();
	}

	UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Built RawData"));

	//  Check we got valid data
	if (RawMesh.VertexPositions.Num()<3||RawMesh.WedgeIndices.Num()<3)
	{
		UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Mesh data not valid, need at least 3 vertices"));
		return false;
	}

	// Create the static mesh resource
	UPackage *Package = CreatePackage(nullptr, *PackageName);
	UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Now have new package"));

	UStaticMesh *NewStaticMesh = NewObject<UStaticMesh>(
		Package, FName(*AssetName), RF_Public|RF_Standalone
		);
	//UStaticMesh *staticMesh = NewObject<UStaticMesh>(
	//	package, staticMeshName, RF_Public|RF_Standalone
	//	);
	UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Now have new UStaticMesh"));
	NewStaticMesh->InitResources();
	NewStaticMesh->LightingGuid = FGuid::NewGuid();

	UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Now have new SM"));

	// Add the data to the static mesh
	FStaticMeshSourceModel* SourceModel = new (NewStaticMesh->SourceModels) FStaticMeshSourceModel();
	SourceModel->BuildSettings.bRecomputeNormals = false;
	SourceModel->BuildSettings.bRecomputeTangents = false;
	SourceModel->BuildSettings.bRemoveDegenerates = false;
	SourceModel->BuildSettings.bUseHighPrecisionTangentBasis = false;
	SourceModel->BuildSettings.bUseFullPrecisionUVs = false;
	SourceModel->BuildSettings.bGenerateLightmapUVs = true;
	SourceModel->BuildSettings.SrcLightmapIndex = 0;
	SourceModel->BuildSettings.DstLightmapIndex = 1;
	SourceModel->RawMeshBulkData->SaveRawMesh(RawMesh);
	UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Added data to StaticMesh"));

	// Copy materials
	for (auto &material:Materials)
	{
		NewStaticMesh->StaticMaterials.Add(FStaticMaterial(material));
	}

	//Set the Imported version before calling the build
	NewStaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

	// Build mesh from source
	NewStaticMesh->Build(false);
	NewStaticMesh->PostEditChange();

	// Notify asset registry of new asset
	FAssetRegistryModule::AssetCreated(NewStaticMesh);
	UE_LOG(MDTLog, Warning, TEXT("SaveToStaticMesh: Asset created"));
	return true;
}

void UMeshGeometry::Scale(
	FVector Scale3d /*= FVector(1, 1, 1)*/,
	FVector CenterOfScale /*= FVector::ZeroVector*/,
	USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Scale")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			Vertex = FMath::Lerp(
				Vertex,
				CenterOfScale+(Vertex-CenterOfScale) * Scale3d,
				Selection ? Selection->Weights[NextWeightIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::ScaleAlongAxis(
	FVector CenterOfScale /*= FVector::ZeroVector*/,
	FVector Axis /*= FVector::UpVector*/,
	float Scale /*= 1.0f*/,
	USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Jitter")))
	{
		return;
	}
	// Check the axis 
	if (Axis.IsNearlyZero(0.01f))
	{
		UE_LOG(MDTLog, Warning, TEXT("ScaleAlongAxis: Axis can not be zero"));
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			FVector ClosestPointOnLine = FMath::ClosestPointOnInfiniteLine(CenterOfScale, CenterOfScale+Axis, Vertex);
			FVector OffsetFromClosestPoint = Vertex-ClosestPointOnLine;
			FVector ScaledPointOnLine = Scale * (ClosestPointOnLine-CenterOfScale)+CenterOfScale;
			Vertex = FMath::Lerp(Vertex, ScaledPointOnLine+OffsetFromClosestPoint, Selection ? Selection->Weights[NextWeightIndex++] : 1.0f);
		}
	}
}

USelectionSet *UMeshGeometry::SelectAll()
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectAll: Cannot create new SelectionSet"));
	}
	NewSelectionSet->CreateSelectionSet(this->GetTotalVertexCount());
	NewSelectionSet->SetAllWeights(1.0f);
	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectByNoise(
	FTransform Transform /* AutoCreateRefTerm */,
	int32 Seed /*= 1337*/,
	float Frequency /*= 0.01*/,
	ENoiseInterpolation NoiseInterpolation /*= ENoiseInterpolation::Quintic*/,
	ENoiseType NoiseType /*= ENoiseType::Simplex */,
	uint8 FractalOctaves /*= 3*/,
	float FractalLacunarity /*= 2.0*/,
	float FractalGain /*= 0.5*/,
	EFractalType FractalType /*= EFractalType::FBM*/,
	ECellularDistanceFunction CellularDistanceFunction /*= ECellularDistanceFunction::Euclidian*/
)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectAll: Cannot create new SelectionSet"));
	}


	// Set up all of the noise details from the parameters provided
	FastNoise Noise;
	Noise.SetSeed(Seed);
	Noise.SetFrequency(Frequency);
	Noise.SetInterp((FastNoise::Interp)NoiseInterpolation);
	Noise.SetNoiseType((FastNoise::NoiseType)NoiseType);
	Noise.SetFractalOctaves(FractalOctaves);
	Noise.SetFractalLacunarity(FractalLacunarity);
	Noise.SetFractalGain(FractalGain);
	Noise.SetFractalType((FastNoise::FractalType) FractalType);
	Noise.SetCellularDistanceFunction((FastNoise::CellularDistanceFunction) CellularDistanceFunction);
	/// \todo Is this needed.. ?  FastNoise doesn't seem to have a SetPositionWarpAmp param
	///noise.SetPositionWarpAmp(PositionWarpAmp);

	// Iterate over the sections, and the vertices in each section.
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// Apply the noise transform to the vertex and use the transformed vertex for the noise generation
			const FVector TransformedVertex = Transform.TransformPosition(Vertex);
			float NoiseValue = Noise.GetNoise(TransformedVertex.X, TransformedVertex.Y, TransformedVertex.Z);
			NewSelectionSet->Weights.Emplace(NoiseValue);
		}
	}

	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectBySection(int32 SectionIndex)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectBySection: Cannot create new SelectionSet"));
	}

	// Iterate over the sections, and the vertices in each section
	int32 CurrentSectionIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// Add a new weight- 1.0 if section indices match, 0.0 otherwise.
			NewSelectionSet->Weights.Emplace(SectionIndex==CurrentSectionIndex ? 1.0f : 0.0f);
		}
		// Increment the current section index, we've finished with this section
		CurrentSectionIndex++;
	}

	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectByTexture(UTexture2D *Texture2D, ETextureChannel TextureChannel /*=ETextureChannel::Red*/)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectByTexture: Cannot create new SelectionSet"));
	}

	// Check we have a texture and that it's in the right format
	if (!Texture2D)
	{
		UE_LOG(MDTLog, Warning, TEXT("SelectByTexture: No Texture2D provided"));
		return nullptr;
	}

	/// \todo Log a message when we do this...
	Texture2D->SRGB = false;
	Texture2D->CompressionSettings = TC_VectorDisplacementmap;
	Texture2D->UpdateResource();

	// Get the raw color data from the texture
	FTexture2DMipMap *MipMap0 = &Texture2D->PlatformData->Mips[0];
	int32 TextureWidth = MipMap0->SizeX;
	int32 TextureHeight = MipMap0->SizeY;
	FByteBulkData *BulkData = &MipMap0->BulkData;

	// Check we got the data and lock it
	if (!BulkData)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectByTexture: Could not lock bulk data for texture"));
		return nullptr;
	}
	FColor *ColorArray = static_cast<FColor*>(BulkData->Lock(LOCK_READ_ONLY));

	// Iterate over the sections, and the vertices in each section.
	for (auto &Section:this->Sections)
	{
		for (auto &UV:Section.UVs)
		{
			// Convert our UV to a texture index in pixels
			int32 TextureX = (int32)FMath::RoundHalfFromZero(UV.X * TextureWidth);
			int32 TextureY = (int32)FMath::RoundHalfFromZero(UV.Y * TextureHeight);

			/// \todo Wrap/Clamp
			TextureX = FMath::Clamp(TextureX, 0, TextureWidth-1);
			TextureY = FMath::Clamp(TextureY, 0, TextureHeight-1);

			// Get the color and access the correct channel.
			int32 ArrayIndex = (TextureY * TextureWidth)+TextureX;
			FLinearColor Color = ColorArray[ArrayIndex];

			switch (TextureChannel)
			{
				case ETextureChannel::Red:
					NewSelectionSet->Weights.Emplace(Color.R);
					break;
				case ETextureChannel::Green:
					NewSelectionSet->Weights.Emplace(Color.G);
					break;
				case ETextureChannel::Blue:
					NewSelectionSet->Weights.Emplace(Color.B);
					break;
				case ETextureChannel::Alpha:
					NewSelectionSet->Weights.Emplace(Color.A);
					break;
			}
		}
	}

	// Unlock the texture data
	BulkData->Unlock();

	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectByNormal(
	FVector Normal /*= FVector::UpVector*/,
	float InnerRadiusInDegrees /*= 0*/,
	float OuterRadiusInDegrees /*= 30.0f*/)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectFacing: Cannot create new SelectionSet"));
	}

	// Normalize the facing vector.
	if (!Normal.Normalize())
	{
		UE_LOG(MDTLog, Error, TEXT("SelectFacing: Cannot normalize Facing vector"));
		return NewSelectionSet;
	}

	// Calculate the selection radius- we need it for falloff
	float SelectionRadius = OuterRadiusInDegrees-InnerRadiusInDegrees;

	// Iterate over the sections, and the the normals in the sections.
	for (auto &Section:this->Sections)
	{
		for (auto VertexNormal:Section.Normals)
		{
			const FVector NormalizedVertexNormal = VertexNormal.GetSafeNormal();

			if (NormalizedVertexNormal.IsNearlyZero(0.01f))
			{
				UE_LOG(MDTLog, Warning, TEXT("SelectFacing: Cannot normalize normal vector"));
				NewSelectionSet->Weights.Emplace(0);
			}
			else
			{
				// Calculate the dot product between the normal and the Facing.
				const float AngleToNormal = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(VertexNormal, Normal)));
				const float AngleBias = 1.0f-FMath::Clamp((AngleToNormal-InnerRadiusInDegrees)/SelectionRadius, 0.0f, 1.0f);
				NewSelectionSet->Weights.Emplace(AngleBias);
			}
		}
	}

	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectByVertexRange(
	int32 RangeStart,
	int32 RangeEnd,
	int32 RangeStep, /*= 1*/
	int32 SectionIndex /* =0*/
)
{

	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectBySection: Cannot create new SelectionSet"));
	}

	// Iterate over the sections, and the vertices in each section
	int32 CurrentSectionIndex = 0;
	for (auto &Section:this->Sections)
	{
		int CurrentVertexIndex = 0;
		for (auto &Vertex:Section.Vertices)
		{
			// Work out if this is part of the range or not.
			bool bIsInRange =
				(CurrentSectionIndex==SectionIndex)&&	// Right section
				(CurrentVertexIndex>=RangeStart)&& // At or beyond start of range
				(CurrentVertexIndex<=RangeEnd)&& // At or before end of range
				((CurrentVertexIndex-RangeStart)%RangeStep==0); // Step is right

			// Add a new weight- 1.0 if section indices match, 0.0 otherwise.
			NewSelectionSet->Weights.Emplace(bIsInRange ? 1.0f : 0.0f);

			// Increment the current vertex index, we've finished with this section
			CurrentVertexIndex++;
		}
		// Increment the current section index, we've finished with this section
		CurrentSectionIndex++;
	}

	return NewSelectionSet;
}

USelectionSet *UMeshGeometry::SelectInVolume(FVector CornerA, FVector CornerB)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectInVolume: Cannot create new SelectionSet"));
	}

	// Get the minimum/maximum of X, Y, and Z from the corner vectors so we can check
	const float MinX = FMath::Min(CornerA.X, CornerB.X);
	const float MaxX = FMath::Max(CornerA.X, CornerB.X);
	const float MinY = FMath::Min(CornerA.Y, CornerB.Y);
	const float MaxY = FMath::Max(CornerA.Y, CornerB.Y);
	const float MinZ = FMath::Min(CornerA.Z, CornerB.Z);
	const float MaxZ = FMath::Max(CornerA.Z, CornerB.Z);

	// Iterate over the sections, and the vertices in each section
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// We only need to know if the vertex is between min/max inclusive.
			const bool bVertexInVolume =
				(Vertex.X>=MinX)&&(Vertex.X<=MaxX)&&
				(Vertex.Y>=MinY)&&(Vertex.Y<=MaxY)&&
				(Vertex.Z>=MinZ)&&(Vertex.Z<=MaxZ);
			// Add a new weighting based on whether it's insider or outside
			NewSelectionSet->Weights.Emplace(bVertexInVolume ? 1.0f : 0.0f);
		}
	}

	return NewSelectionSet;
}


USelectionSet * UMeshGeometry::SelectLinear(
	FVector LineStart,
	FVector LineEnd,
	bool bReverse /*= false*/,
	bool bLimitToLine /*= false*/)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectLinear: Cannot create new SelectionSet"));
	}

	// Do the reverse if needed..
	if (bReverse)
	{
		FVector TmpVector = LineStart;
		LineStart = LineEnd;
		LineEnd = TmpVector;
	}

	// Calculate the length of the line.
	float LineLength = (LineEnd-LineStart).Size();
	if (LineLength<0.01f)
	{
		UE_LOG(MDTLog, Warning, TEXT("SelectLinear: LineStart and LineEnd too close"));
		return nullptr;
	}

	// Iterate over the sections, and the vertices in each section
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// Get the nearest point on the line
			const FVector NearestPointOnLine = FMath::ClosestPointOnLine(LineStart, LineEnd, Vertex);

			// If we've hit one of the end points then return the limits
			if (NearestPointOnLine==LineEnd)
			{
				NewSelectionSet->Weights.Emplace(bLimitToLine ? 0.0f : 1.0f);
			}
			else if (NearestPointOnLine==LineStart)
			{
				NewSelectionSet->Weights.Emplace(0.0f);
			}
			else
			{
				// Get the distance to the two start point- it's the ratio we're after.
				float DistanceToLineStart = (NearestPointOnLine-LineStart).Size();
				NewSelectionSet->Weights.Emplace(DistanceToLineStart/LineLength);
			}
		}
	}

	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNear(
	FVector Center /*=FVector::ZeroVector*/,
	float InnerRadius/*=0*/,
	float OuterRadius/*=100*/)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectNear: Cannot create new SelectionSet"));
	}

	// Calculate the selection radius- we need it for falloff
	const float SelectionRadius = OuterRadius-InnerRadius;

	// Iterate over the sections, and the vertices in each section.
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			const float DistanceFromCenter = (Vertex-Center).Size();
			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			const float DistanceBias = 1.0f-FMath::Clamp((DistanceFromCenter-InnerRadius)/SelectionRadius, 0.0f, 1.0f);
			NewSelectionSet->Weights.Emplace(DistanceBias);
		}
	}

	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNearLine(
	FVector LineStart,
	FVector LineEnd,
	float InnerRadius /*=0*/,
	float OuterRadius/*= 100*/,
	bool bLineIsInfinite/* = false */)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectNearLine: Cannot create new SelectionSet"));
	}

	// Calculate the selection radius- we need it for falloff
	const float SelectionRadius = OuterRadius-InnerRadius;

	// Iterate over the sections, and the vertices in each section.
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// Get the distance from the line based on whether we're looking at an infinite line or not.
			const FVector NearestPointOnLine = bLineIsInfinite ?
				FMath::ClosestPointOnInfiniteLine(LineStart, LineEnd, Vertex) :
				FMath::ClosestPointOnLine(LineStart, LineEnd, Vertex);

			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			const float DistanceToLine = (Vertex-NearestPointOnLine).Size();
			const float DistanceBias = 1.0f-FMath::Clamp((DistanceToLine-InnerRadius)/SelectionRadius, 0.0f, 1.0f);
			NewSelectionSet->Weights.Emplace(DistanceBias);
		}
	}

	return NewSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNearSpline(
	USplineComponent *Spline,
	FTransform Transform,
	float InnerRadius /*= 0*/,
	float OuterRadius /*= 100*/)
{
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(this);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectNearSpline: Cannot create new SelectionSet"));
		return nullptr;
	}

	if (!Spline)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectNearSpline: No spline provided"));
		return nullptr;
	}

	// Calculate the selection radius- we need it for falloff
	const float SelectionRadius = OuterRadius-InnerRadius;

	// Iterate over the sections, and the vertices in each section.
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			// Convert the vertex location to local space- and then get the nearest point on the spline in local space.
			const FVector ClosestPointOnSpline = Spline->FindLocationClosestToWorldLocation(
				Transform.TransformPosition(Vertex),
				ESplineCoordinateSpace::Local
			);
			const float DistanceFromSpline = (Vertex-ClosestPointOnSpline).Size();
			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			const float DistanceBias = 1.0f-FMath::Clamp((DistanceFromSpline-InnerRadius)/SelectionRadius, 0.0f, 1.0f);
			NewSelectionSet->Weights.Emplace(DistanceBias);
		}
	}

	return NewSelectionSet;
}

float UMeshGeometry::MiniumProjectionPlaneDistance(FVector Projection)
{
	// The projection needs to be normalized to act as plane
	Projection = Projection.GetSafeNormal();
	if (Projection.IsZero()) {
		return 0;
	}

	// Get the radius and store it- we don't want to keep getting it
	const float Radius = GetRadius();

	// Iterate over the sections, and the vertices in each section.
	float FurthestPlane;
	bool bHaveProcessedFirstVertex = false;
	for (auto &Section : this->Sections)
	{
		for (auto &Vertex : Section.Vertices)
		{
			// Get the nearest point from the origin to a plane with the
			// supplied projection and passing through the vector.
			const FVector NearestPointOnVertexPlane =
				Utility::NearestPointOnPlane(FVector::ZeroVector, Vertex, Projection);

			// Check we're on the correct side of the plane
			const FPlane Plane = FPlane(NearestPointOnVertexPlane, -Projection);
			const bool bDotTestForPlaneSide = FVector::DotProduct(Vertex.GetSafeNormal(), -Projection.GetSafeNormal()) >=0;
			const float DistanceFromVertexToPlane = NearestPointOnVertexPlane.Size() * (bDotTestForPlaneSide ? 1 : -1);

			// Update furthestPlane info
			FurthestPlane =
				bHaveProcessedFirstVertex ?
				FMath::Max(FurthestPlane, DistanceFromVertexToPlane) :
				DistanceFromVertexToPlane;
			bHaveProcessedFirstVertex = true;
		}
	}
	return FurthestPlane;
}

void UMeshGeometry::RebuildNormals()
{
	// Iterate over the sections
	for (auto &Section : this->Sections)
	{
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
			Section.Vertices, Section.Triangles, Section.UVs,	// These are inputs
			Section.Normals, Section.Tangents					// These are outputs
		);
	}
}

bool UMeshGeometry::SelectionSetIsRightSize(USelectionSet *Selection, FString NodeNameForWarning) const
{
	// No selection set is fine...
	if (!Selection)
	{
		return true;
	}

	// Get the sizes
	const int32 SelectionSetSize = Selection->Size();
	const int32 GeometrySize = GetTotalVertexCount();

	// Check them
	if (SelectionSetSize!=GeometrySize)
	{
		UE_LOG(
			MDTLog, Warning, TEXT("%s: Selection set is the wrong size, %d weights in set for %d vertices in mesh"),
			*NodeNameForWarning, SelectionSetSize, GeometrySize
		);
		return false;
	}
	return true;
}

void UMeshGeometry::Spherize(
	float SphereRadius /*= 100.0f*/,
	float FilterStrength /*= 1.0f*/,
	FVector SphereCenter /*= FVector::ZeroVector*/,
	USelectionSet *Selection)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Spherize")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			const FVector VertexRelativeToCenter = Vertex-SphereCenter;

			// Calculate the required length- incorporating both the SphereRadius and Selection.
			const float TargetVectorLength = FMath::Lerp(
				VertexRelativeToCenter.Size(),
				SphereRadius,
				FilterStrength * (Selection ? Selection->Weights[NextWeightIndex++] : 1.0f)
			);

			Vertex = SphereCenter+(VertexRelativeToCenter.GetSafeNormal() * TargetVectorLength);
		}
	}
}

void UMeshGeometry::Transform(
	FTransform Transform /*= FTransform::Identity*/,
	FVector CenterOfTransform /*= FVector::ZeroVector*/,
	USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Transform")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &Vertex:Section.Vertices)
		{
			Vertex = FMath::Lerp(
				Vertex,
				CenterOfTransform+Transform.TransformPosition(Vertex-CenterOfTransform),
				Selection ? Selection->Weights[NextWeightIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::TransformUV(FTransform Transform, FVector2D CenterOfTransform /*= FVector::ZeroVector*/, USelectionSet *Selection /*= nullptr */)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("TransformUV")))
	{
		return;
	}
	
	// Iterate over the sections, and the the vertices in the sections.
	int32 NextWeightIndex = 0;
	for (auto &Section:this->Sections)
	{
		for (auto &UV:Section.UVs)
		{
			// Convert to FVectors to allow us to use FTransform on them
			const FVector UVAsVector = FVector(UV.X, UV.Y, 0);
			const FVector CenterOfTransformAsVector = FVector(CenterOfTransform.X, CenterOfTransform.Y, 0);

			const FVector TransformedUVAsVector = FMath::Lerp(
				UVAsVector,
				CenterOfTransformAsVector+Transform.TransformPosition(
					UVAsVector-CenterOfTransformAsVector
				),
				Selection ? Selection->Weights[NextWeightIndex++] : 1.0f
			);
			
			// Cast back to Vector2D
			UV = FVector2D(TransformedUVAsVector.X, TransformedUVAsVector.Y);
		}
	}
}

void UMeshGeometry::Translate(FVector Delta, USelectionSet *Selection)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Translate")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->Sections)
	{
		for (auto &vertex:section.Vertices)
		{
			vertex = FMath::Lerp(
				vertex,
				vertex+Delta,
				Selection ? Selection->Weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}
