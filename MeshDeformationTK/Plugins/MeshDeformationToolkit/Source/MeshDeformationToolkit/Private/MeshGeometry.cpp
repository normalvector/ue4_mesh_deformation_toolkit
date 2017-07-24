// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "Engine/StaticMesh.h"
#include "KismetProceduralMeshLibrary.h"
#include "Runtime/Core/Public/Math/UnrealMathUtility.h" // ClosestPointOnLine/ClosestPointOnInfiniteLine, GetMappedRangeValue
#include "SelectionSet.h"
#include "FastNoise.h"
#include "MeshGeometry.h"

UMeshGeometry::UMeshGeometry()
{
	// Create empty data sets.
	sections = TArray<FSectionGeometry>();
}

// New experimental version of Conform using line projections
void UMeshGeometry::Conform(
	UObject* WorldContextObject,
	FTransform Transform,
	TArray <AActor *> IgnoredActors,
	FVector Projection /*= FVector(0, 0, -100)*/,
	float HeightAdjust /*= 0*/,
	bool TraceComplex /*=true*/,
	ECollisionChannel CollisionChannel /*= ECC_WorldStatic*/,
	USelectionSet *Selection /*= nullptr */
) {
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Conform")))
	{
		return;
	}

	// Get the world content we're operating in
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if (!World)
	{
		UE_LOG(MDTLog, Error, TEXT("Conform: Cannot access game world"));
		return;
	}

	// Prepare the trace query parameters
	const FName traceTag("ConformTraceTag");
	FCollisionQueryParams traceQueryParams = FCollisionQueryParams();
	traceQueryParams.TraceTag = traceTag;
	traceQueryParams.bTraceComplex = TraceComplex;
	traceQueryParams.AddIgnoredActors(IgnoredActors);

	// Convert the projection into local space as we'll need it for the projection
	// calculations and don't want to do it per-vert.  Also calculate the normalized
	// version and store that.
	const FVector projectionInLS = Transform.InverseTransformVector(Projection);
	const FVector projectionNormalInLS = projectionInLS.GetSafeNormal();

	// Get the distance to the base plane
	const float distanceToBasePlane = MiniumProjectionPlaneDistance(-projectionInLS);
	const FVector pointOnBasePlane = Projection.GetSafeNormal() * distanceToBasePlane;
	const FVector pointOnBasePlaneLS = projectionNormalInLS * distanceToBasePlane;

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections)
	{
		for (auto &vertex : section.vertices)
		{
			// Scale the Projection vector according to the selectionSet, giving varying strength conform, all in World Space
			const FVector scaledProjection = Projection * (Selection ? Selection->weights[nextSelectionIndex++] : 1.0f);

			// Compute the start/end positions of the trace
			const FVector traceStart = Transform.TransformPosition(vertex);
			const FVector traceEnd = Transform.TransformPosition(
				NearestPointOnPlane(
					vertex,
					pointOnBasePlaneLS + scaledProjection.Size() *projectionNormalInLS,
					projectionNormalInLS
				)
			);

			// Do the actual trace
			FHitResult hitResult;
			bool hitSuccess = World->LineTraceSingleByChannel(
				hitResult,
				traceStart, traceEnd,
				CollisionChannel, traceQueryParams, FCollisionResponseParams()
			);

			// Position the vertex based on whether we had a hit or not.
			if (hitResult.bBlockingHit) {
				// Calculate the offset for the vertex- it's based on the distance to the
				// base plane.
				const float distanceFromVertexToBasePlane =
					FVector::PointPlaneDist(vertex, pointOnBasePlaneLS, projectionNormalInLS);
				const float hitProjectionHeight =
					distanceFromVertexToBasePlane - HeightAdjust;

				vertex = 
					Transform.InverseTransformPosition(
						hitResult.ImpactPoint
					) + projectionNormalInLS * hitProjectionHeight;
			}
			else {
				// No collision- just add the projection to the vertex.
				vertex = vertex + Transform.InverseTransformVector(scaledProjection);;
			}
		}
	}
}

void UMeshGeometry::ConformDown(
	UObject* WorldContextObject, 
	FTransform Transform,
	TArray <AActor *> IgnoredActors /*= nullptr*/,
	float ProjectionLength /*= 100*/,
	float HeightAdjust /*= 0*/,
	bool TraceComplex /*= true*/,
	ECollisionChannel CollisionChannel /*= ECC_WorldStatic*/,
	USelectionSet *Selection /*= nullptr */)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Conform")))
	{
		return;
	}

	// Get the world content we're operating in
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if (!World)
	{
		UE_LOG(MDTLog, Error, TEXT("Conform: Cannot access game world"));
		return;
	}

	// Prepare the trace query parameters
	const FName traceTag("ConformTraceTag");
	FCollisionQueryParams traceQueryParams = FCollisionQueryParams();
	traceQueryParams.TraceTag = traceTag;
	traceQueryParams.bTraceComplex = TraceComplex;
	traceQueryParams.AddIgnoredActors(IgnoredActors);

	// Calculate the projection vector.
	const FVector projection = FVector(0, 0, -ProjectionLength);

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections)
	{
		for (auto &vertex : section.vertices)
		{
			// Scale the Projection vector according to the selectionSet, giving varying strength conform, all in World Space
			const FVector scaledProjection = projection * (Selection ? Selection->weights[nextSelectionIndex++] : 1.0f);

			// Compute the start/end positions of the trace
			const FVector traceStart = Transform.TransformPosition(vertex);
			const FVector traceEnd =
				Transform.TransformPosition(FVector(vertex.X, vertex.Y, 0)) + scaledProjection;

			// Do the actual trace
			FHitResult hitResult;
			bool hitSuccess = World->LineTraceSingleByChannel(
				hitResult,
				traceStart, traceEnd,
				CollisionChannel, traceQueryParams, FCollisionResponseParams()
			);

			// Position the vertex based on whether we had a hit or not.
			if (hitResult.bBlockingHit) {
				// Add the original .Z and heightAdjust to the hit result for the final collision output.
				vertex =
					Transform.InverseTransformPosition(
						hitResult.ImpactPoint
					) + FVector(0,0,vertex.Z + HeightAdjust);
			}
			else {
				// No collision- just add the projection to the vertex.
				vertex = vertex + Transform.InverseTransformVector(scaledProjection);;
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
	const float splineLength = SplineComponent->GetSplineLength();

	// Get the minimum X, and the range of X, for the mesh, we'll need them to build the spline.
	const FBox meshBounds = this->GetBoundingBox();
	const float minX = meshBounds.Min.X;
	//const float rangeX = meshBounds.Max.X - minX

	// Build the ranges we'll be using for the remapping, we'll be going rangeX -> rangePosition,
	// and if we have a profile curve we'll need a range for that too, along with a '0-SplineLength' fixed range.
	const FVector2D rangeX = FVector2D(meshBounds.Min.X, meshBounds.Max.X);
	const FVector2D rangePosition = FVector2D(StartPosition, EndPosition);
	const FVector2D fullSplineRange = FVector2D(0.0f, splineLength);

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			// Remap the X position into the StartPosition/EndPosition range, then multiply by SplineLength to get a value we
			// can use for lookup.
			const float distanceAlongSpline = FMath::GetMappedRangeValueClamped(rangeX, rangePosition, vertex.X) * splineLength;

			// If we have either profile curve we now need to find the position at a given point.  For efficiency
			//   we can combine this with MeshScale.
			float combinedMeshScale = MeshScale;
			if (ProfileCurve)
			{
				// Get the range of the curve
				float profileCurveMin;
				float profileCurveMax;
				ProfileCurve->GetTimeRange(profileCurveMin, profileCurveMax);
				FVector2D profileCurveRange = FVector2D(profileCurveMin, profileCurveMax);

				const float positionAlongCurve =
					FMath::GetMappedRangeValueClamped(fullSplineRange, profileCurveRange, distanceAlongSpline);
				combinedMeshScale = combinedMeshScale * ProfileCurve->GetFloatValue(positionAlongCurve);
			}
			if (SectionProfileCurve)
			{
				// Get the range of the curve
				float sectionCurveMin;
				float sectionCurveMax;
				SectionProfileCurve->GetTimeRange(sectionCurveMin, sectionCurveMax);
				FVector2D sectionProfileCurveRange = FVector2D(sectionCurveMin, sectionCurveMax);

				const float positionAlongCurve =
					FMath::GetMappedRangeValueClamped(rangeX, sectionProfileCurveRange, vertex.X);
				combinedMeshScale = combinedMeshScale * SectionProfileCurve->GetFloatValue(positionAlongCurve);
			}

			// Get all of the splines's details at the distance we've converted X to- stick to local space
			const FVector location = SplineComponent->GetLocationAtDistanceAlongSpline(
				distanceAlongSpline, ESplineCoordinateSpace::Local
			);
			const FVector rightVector = SplineComponent->GetRightVectorAtDistanceAlongSpline(
				distanceAlongSpline, ESplineCoordinateSpace::Local
			);
			const FVector upVector = SplineComponent->GetUpVectorAtDistanceAlongSpline(
				distanceAlongSpline, ESplineCoordinateSpace::Local
			);

			// Now we have the details we can use them to compute the final location that we need to use
			FVector splineVertexPosition = location+(rightVector * vertex.Y * combinedMeshScale)+(upVector * vertex.Z * combinedMeshScale);
			vertex = FMath::Lerp(
				vertex, splineVertexPosition,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::FlipNormals(USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("FlipNormals")))
	{
		return;
	}

	// Iterate over the sections, and the normals in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &normal:section.normals)
		{
			// Obtain the next weighting and check if it's >=0.5
			const bool shouldFlip =
				Selection ? Selection->weights[nextSelectionIndex++]>=0.5 : true;

			// If we need to flip then rebuild the UV based on which channels we're
			//  flipping.
			if (shouldFlip)
			{
				normal = -normal;
			}
		}
	}
}

void UMeshGeometry::FlipTextureUV(bool FlipU /*= false*/, bool FlipV /*= false*/, USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("FlipTextureUV")))
	{
		return;
	}

	// Iterate over the sections, and the uvs in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &uv:section.uvs)
		{
			// Obtain the next weighting and check if it's >=0.5
			const bool shouldFlip =
				Selection ? Selection->weights[nextSelectionIndex++]>=0.5 : true;

			// If we're meant to be flipping then flip the correct channels.
			if (shouldFlip)
			{
				uv = FVector2D(
					FlipU ? 1.0f-uv.X : uv.X,
					FlipV ? 1.0f-uv.Y : uv.Y
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
	bool errorFound = false;

	// Iterate over the sections
	int32 sectionIndex = 0;
	for (auto section:this->sections)
	{
		// Each section should contain at least three vertices.
		const int32 sectionVertexCount = section.vertices.Num();
		if (sectionVertexCount<3)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d contains only %d vertices (3 required)"),
				*NodeNameForWarning, sectionIndex, sectionVertexCount
			);
			errorFound = true;
		}

		// Each section should contain at least one triangle
		const int32 trianglePointNum = section.triangles.Num();
		if (trianglePointNum<3)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d contains only %d triangle indices (3 required for one triangle)"),
				*NodeNameForWarning, sectionIndex, trianglePointNum
			);
			errorFound = true;
		}

		/// Triangles contain a multiple of 3 points as every set of three defined one tri
		if ((trianglePointNum%3)!=0)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d contains %d triangle indices (Should be a multiple of three as three per triangle)"),
				*NodeNameForWarning, sectionIndex, trianglePointNum
			);
			errorFound = true;
		}

		/// Has same number of normals as vertices
		const int32 sectionNormalCount = section.normals.Num();
		if (sectionNormalCount!=sectionVertexCount)
		{
			UE_LOG(
				MDTLog, Warning,
				TEXT("%s: Section %d does not contain same number of vertices and normals (%d vertices, %d normals)"),
				*NodeNameForWarning, sectionIndex, sectionVertexCount, sectionNormalCount
			);
			errorFound = true;

		}

		++sectionIndex;
	}

	return errorFound;
}

FBox UMeshGeometry::GetBoundingBox() const
{
	// Track the two corners of the bounding box
	FVector min = FVector::ZeroVector;
	FVector max = FVector::ZeroVector;

	// When we hit the first vertex we'll need to set both Min and Max
	//  to it as we'll have no comparison
	bool haveProcessedFirstVector = false;

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			if (haveProcessedFirstVector)
			{
				// Do the comparison of both min/max.
				min.X = FMath::Min(min.X, vertex.X);
				min.Y = FMath::Min(min.Y, vertex.Y);
				min.Z = FMath::Min(min.Z, vertex.Z);

				max.X = FMath::Max(max.X, vertex.X);
				max.Y = FMath::Max(max.Y, vertex.Y);
				max.Z = FMath::Max(max.Z, vertex.Z);
			}
			else
			{
				// Set min/max to the first vertex
				min = vertex;
				max = vertex;
				haveProcessedFirstVector = true;
			}
		}
	}

	// Build a bounding box from the result
	return FBox(min, max);
}

float UMeshGeometry::GetRadius() const
{
	// This is the radius so far
	float radius;

	// When we hit the first vertex we'll need to set both Min and Max
	//  to it as we'll have no comparison
	bool haveProcessedFirstVector = false;

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections)
	{
		for (auto &vertex : section.vertices)
		{
			radius = haveProcessedFirstVector ?
				FMath::Max(radius, vertex.Size()) :
				vertex.Size();
		}
	}

	return radius;
}

FString UMeshGeometry::GetSummary() const
{
	return FString::Printf(
		TEXT("%d sections, %d vertices, %d triangles"),
		this->sections.Num(), this->GetTotalVertexCount(), this->GetTotalTriangleCount()
	);
}

int32 UMeshGeometry::GetTotalTriangleCount() const
{
	int32 totalTriangleCount = 0;
	for (auto section:this->sections)
	{
		totalTriangleCount += section.triangles.Num();
	}
	return totalTriangleCount/3; // 3pts per triangle
}

int32 UMeshGeometry::GetTotalVertexCount() const
{
	int32 totalVertexCount = 0;
	for (auto section:this->sections)
	{
		totalVertexCount += section.vertices.Num();
	}
	return totalVertexCount;
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
	for (auto &section:this->sections)
	{
		for (int32 vertexIndex = 0; vertexIndex<section.vertices.Num(); ++vertexIndex)
		{
			section.vertices[vertexIndex] = FMath::Lerp(
				section.vertices[vertexIndex],
				section.vertices[vertexIndex]+(section.normals[vertexIndex]*Offset),
				Selection ? Selection->weights[vertexIndex] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Jitter(FRandomStream &randomStream, FVector min, FVector max, USelectionSet *Selection /*=nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Jitter")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;

	// Iterate over the sections, and the vertices in each section.
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			const FVector randomJitter = FVector(
				randomStream.FRandRange(min.X, max.X),
				randomStream.FRandRange(min.Y, max.Y),
				randomStream.FRandRange(min.Z, max.Z)
			);
			vertex = FMath::Lerp(
				vertex,
				vertex+randomJitter,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
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
	if (this->sections.Num()!=TargetMeshGeometry->sections.Num())
	{
		UE_LOG(
			MDTLog, Warning, TEXT("Lerp: Cannot lerp geometries with different numbers of sections, %d compared to %d"),
			this->sections.Num(), TargetMeshGeometry->sections.Num()
		);
		return;
	}

	// Iterate over the sections, and the vertices in the sections.  Do it by index so we
	// can access the same data from TargetMeshGeometry
	int32 nextSelectionIndex = 0;
	for (int32 sectionIndex = 0; sectionIndex<this->sections.Num(); sectionIndex++)
	{
		if (this->sections[sectionIndex].vertices.Num()!=TargetMeshGeometry->sections[sectionIndex].vertices.Num())
		{
			UE_LOG(
				MDTLog, Warning, TEXT("Lerp: Cannot lerp geometries with different numbers of vertices, %d compared to %d for section %d"),
				this->sections[sectionIndex].vertices.Num(), TargetMeshGeometry->sections[sectionIndex].vertices.Num(), sectionIndex
			);
			return;
		}

		for (int32 vertexIndex = 0; vertexIndex<this->sections[sectionIndex].vertices.Num(); ++vertexIndex)
		{
			// Get the existing data from the two components.
			FVector vertexFromThis = this->sections[sectionIndex].vertices[vertexIndex];
			FVector vertexFromTarget = TargetMeshGeometry->sections[sectionIndex].vertices[vertexIndex];

			// TODO: World/local logic should live here.
			this->sections[sectionIndex].vertices[vertexIndex] = FMath::Lerp(
				vertexFromThis, vertexFromTarget,
				Alpha * (Selection ? Selection->weights[nextSelectionIndex++] : 1.0f)
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
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			vertex = FMath::Lerp(
				vertex,
				Position,
				Alpha * (Selection ? Selection->weights[nextSelectionIndex++] : 1.0f)
			);
		}
	}
}

bool UMeshGeometry::LoadFromStaticMesh(UStaticMesh *staticMesh, int32 LOD /*= 0*/)
{
	// If there's no static mesh we have nothing to do..
	if (!staticMesh)
	{
		UE_LOG(MDTLog, Warning, TEXT("LoadFromStaticMesh: No StaticMesh provided"));
		return false;
	}

	// Clear any existing geometry.
	this->sections.Empty();

	// Iterate over the sections
	const int32 numSections = staticMesh->GetNumSections(LOD);
	for (int meshSectionIndex = 0; meshSectionIndex<numSections; ++meshSectionIndex)
	{
		// Create the geometry for the section
		FSectionGeometry sectionGeometry;

		// Copy the static mesh's geometry for the section to the struct.
		UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(
			staticMesh, LOD, meshSectionIndex,
			sectionGeometry.vertices, sectionGeometry.triangles,
			sectionGeometry.normals, sectionGeometry.uvs, sectionGeometry.tangents
		);

		// Load vertex colors with default values for as many vertices as needed
		sectionGeometry.vertexColors.InsertDefaulted(0, sectionGeometry.vertices.Num());

		// Add the finished struct to the mesh's section list
		this->sections.Emplace(sectionGeometry);
	}

	// Warn if the mesh doesn't look valid.  For now return it anyway but at least let
	// them know..
	CheckGeometryIsValid(TEXT("LoadFromStaticMesh"));

	// All done
	return true;
}

void UMeshGeometry::Rotate(FRotator Rotation /*= FRotator::ZeroRotator*/, FVector CenterOfRotation /*= FVector::ZeroVector*/, USelectionSet *Selection)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Rotate")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			vertex = FMath::Lerp(
				vertex,
				CenterOfRotation+Rotation.RotateVector(vertex-CenterOfRotation),
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
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
	auto normalizedAxis = Axis.GetSafeNormal();
	if (normalizedAxis.IsNearlyZero(0.01f))
	{
		UE_LOG(MDTLog, Warning, TEXT("RotateAroundAxis: Could not normalize Axis, zero vector?"));
		return;
	}

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			FVector closestPointOnLine = FMath::ClosestPointOnInfiniteLine(CenterOfRotation, CenterOfRotation+Axis, vertex);
			FVector offsetFromClosestPoint = vertex-closestPointOnLine;
			float scaledRotation = FMath::Lerp(
				0.0f, AngleInDegrees,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
			FVector rotatedOffset = offsetFromClosestPoint.RotateAngleAxis(scaledRotation, normalizedAxis);
			vertex = closestPointOnLine+rotatedOffset;
		}
	}
}

bool UMeshGeometry::SaveToProceduralMeshComponent(
	UProceduralMeshComponent *proceduralMeshComponent,
	bool createCollision)
{
	// If there's no PMC we have nothing to do..
	if (!proceduralMeshComponent)
	{
		UE_LOG(MDTLog, Warning, TEXT("SaveToProceduralMeshComponent: No ProceduralMeshComponent provided"));
		return false;
	}

	// Clear the geometry
	proceduralMeshComponent->ClearAllMeshSections();

	// Iterate over the mesh sections, creating a PMC MeshSection for each one.
	int32 nextSectionIndex = 0;
	for (auto section:this->sections)
	{
		// Create the PMC section with the StaticMesh's data.
		proceduralMeshComponent->CreateMeshSection_LinearColor(
			nextSectionIndex++, section.vertices, section.triangles, section.normals, section.uvs,
			section.vertexColors, section.tangents, createCollision
		);
	}
	return true;
}

void UMeshGeometry::Scale(FVector Scale3d /*= FVector(1, 1, 1)*/, FVector CenterOfScale /*= FVector::ZeroVector*/, USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Scale")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			vertex = FMath::Lerp(
				vertex,
				CenterOfScale+(vertex-CenterOfScale) * Scale3d,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::ScaleAlongAxis(
	FVector CenterOfScale /*= FVector::ZeroVector*/, FVector Axis /*= FVector::UpVector*/, float Scale /*= 1.0f*/,
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
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			FVector closestPointOnLine = FMath::ClosestPointOnInfiniteLine(CenterOfScale, CenterOfScale+Axis, vertex);
			FVector offsetFromClosestPoint = vertex-closestPointOnLine;
			FVector scaledPointOnLine = Scale * (closestPointOnLine-CenterOfScale)+CenterOfScale;
			vertex = FMath::Lerp(vertex, scaledPointOnLine+offsetFromClosestPoint, Selection ? Selection->weights[nextSelectionIndex++] : 1.0f);
		}
	}
}

USelectionSet *UMeshGeometry::SelectAll()
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectAll: Cannot create new SelectionSet"));
	}
	newSelectionSet->CreateSelectionSet(this->GetTotalVertexCount());
	newSelectionSet->SetAllWeights(1.0f);
	return newSelectionSet;
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
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectAll: Cannot create new SelectionSet"));
	}


	// Set up all of the noise details from the parameters provided
	FastNoise noise;
	noise.SetSeed(Seed);
	noise.SetFrequency(Frequency);
	noise.SetInterp((FastNoise::Interp)NoiseInterpolation);
	noise.SetNoiseType((FastNoise::NoiseType)NoiseType);
	noise.SetFractalOctaves(FractalOctaves);
	noise.SetFractalLacunarity(FractalLacunarity);
	noise.SetFractalGain(FractalGain);
	noise.SetFractalType((FastNoise::FractalType) FractalType);
	noise.SetCellularDistanceFunction((FastNoise::CellularDistanceFunction) CellularDistanceFunction);
	/// \todo Is this needed.. ?  FastNoise doesn't seem to have a SetPositionWarpAmp param
	///noise.SetPositionWarpAmp(PositionWarpAmp);

	// Iterate over the sections, and the vertices in each section.
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			// Apply the noise transform to the vertex and use the transformed vertex for the noise generation
			const FVector transformedVertex = Transform.TransformPosition(vertex);
			float NoiseValue = noise.GetNoise(transformedVertex.X, transformedVertex.Y, transformedVertex.Z);
			newSelectionSet->weights.Emplace(NoiseValue);
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectBySection(int32 SectionIndex)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectBySection: Cannot create new SelectionSet"));
	}

	// Iterate over the sections, and the vertices in each section
	int32 currentSectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			// Add a new weight- 1.0 if section indices match, 0.0 otherwise.
			newSelectionSet->weights.Emplace(SectionIndex==currentSectionIndex ? 1.0f : 0.0f);
		}
		// Increment the current section index, we've finished with this section
		currentSectionIndex++;
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectByTexture(UTexture2D *Texture2D, ETextureChannel TextureChannel /*=ETextureChannel::Red*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
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
	int32 textureWidth = MipMap0->SizeX;
	int32 textureHeight = MipMap0->SizeY;
	FByteBulkData *BulkData = &MipMap0->BulkData;

	// Check we got the data and lock it
	if (!BulkData)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectByTexture: Could not lock bulk data for texture"));
		return nullptr;
	}
	FColor *colorArray = static_cast<FColor*>(BulkData->Lock(LOCK_READ_ONLY));

	// Iterate over the sections, and the vertices in each section.
	for (auto &section:this->sections)
	{
		for (auto &uv:section.uvs)
		{
			// Convert our UV to a texture index.
			int32 textureX = (int32)FMath::RoundHalfFromZero(uv.X * textureWidth);
			int32 textureY = (int32)FMath::RoundHalfFromZero(uv.Y * textureHeight);

			/// \todo Wrap/Clamp
			textureX = FMath::Clamp(textureX, 0, textureWidth-1);
			textureY = FMath::Clamp(textureY, 0, textureHeight-1);

			// Get the color and access the correct channel.
			int32 index = (textureY * textureWidth)+textureX;
			FLinearColor color = colorArray[index];

			switch (TextureChannel)
			{
				case ETextureChannel::Red:
					newSelectionSet->weights.Emplace(color.R);
					break;
				case ETextureChannel::Green:
					newSelectionSet->weights.Emplace(color.G);
					break;
				case ETextureChannel::Blue:
					newSelectionSet->weights.Emplace(color.B);
					break;
				case ETextureChannel::Alpha:
					newSelectionSet->weights.Emplace(color.A);
					break;
			}
		}
	}

	// Unlock the texture data
	BulkData->Unlock();

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectFacing(FVector Facing /*= FVector::UpVector*/, float InnerRadiusInDegrees /*= 0*/, float OuterRadiusInDegrees /*= 30.0f*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectFacing: Cannot create new SelectionSet"));
	}

	// Normalize the facing vector.
	if (!Facing.Normalize())
	{
		UE_LOG(MDTLog, Error, TEXT("SelectFacing: Cannot normalize Facing vector"));
		return newSelectionSet;
	}

	// Calculate the selection radius- we need it for falloff
	float selectionRadius = OuterRadiusInDegrees-InnerRadiusInDegrees;

	// Iterate over the sections, and the the normals in the sections.
	for (auto &section:this->sections)
	{
		for (auto normal:section.normals)
		{
			const FVector normalizedNormal = normal.GetSafeNormal();

			if (normalizedNormal.IsNearlyZero(0.01f))
			{
				UE_LOG(MDTLog, Warning, TEXT("SelectFacing: Cannot normalize normal vector"));
				newSelectionSet->weights.Emplace(0);
			}
			else
			{
				// Calculate the dot product between the normal and the Facing.
				const float angleToNormal = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(normal, Facing)));
				const float angleBias = 1.0f-FMath::Clamp((angleToNormal-InnerRadiusInDegrees)/selectionRadius, 0.0f, 1.0f);
				newSelectionSet->weights.Emplace(angleBias);
			}
		}
	}

	return newSelectionSet;
}

USelectionSet *UMeshGeometry::SelectInVolume(FVector CornerA, FVector CornerB)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectInVolume: Cannot create new SelectionSet"));
	}

	// Get the minimum/maximum of X, Y, and Z from the corner vectors so we can check
	const float minX = FMath::Min(CornerA.X, CornerB.X);
	const float maxX = FMath::Max(CornerA.X, CornerB.X);
	const float minY = FMath::Min(CornerA.Y, CornerB.Y);
	const float maxY = FMath::Max(CornerA.Y, CornerB.Y);
	const float minZ = FMath::Min(CornerA.Z, CornerB.Z);
	const float maxZ = FMath::Max(CornerA.Z, CornerB.Z);

	// Iterate over the sections, and the vertices in each section
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			// We only need to know if the vertex is between min/max inclusive.
			const bool vertexInVolume =
				(vertex.X>=minX)&&(vertex.X<=maxX)&&
				(vertex.Y>=minY)&&(vertex.Y<=maxY)&&
				(vertex.Z>=minZ)&&(vertex.Z<=maxZ);
			// Add a new weighting based on whether it's insider or outside
			newSelectionSet->weights.Emplace(vertexInVolume ? 1.0f : 0.0f);
		}
	}

	return newSelectionSet;
}


USelectionSet * UMeshGeometry::SelectLinear(FVector LineStart, FVector LineEnd, bool Reverse /*= false*/, bool LimitToLine /*= false*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectLinear: Cannot create new SelectionSet"));
	}

	// Do the reverse if needed..
	if (Reverse)
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
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			// Get the nearest point on the line
			const FVector NearestPointOnLine = FMath::ClosestPointOnLine(LineStart, LineEnd, vertex);

			// If we've hit one of the end points then return the limits
			if (NearestPointOnLine==LineEnd)
			{
				newSelectionSet->weights.Emplace(LimitToLine ? 0.0f : 1.0f);
			}
			else if (NearestPointOnLine==LineStart)
			{
				newSelectionSet->weights.Emplace(0.0f);
			}
			else
			{
				// Get the distance to the two start point- it's the ratio we're after.
				float DistanceToLineStart = (NearestPointOnLine-LineStart).Size();
				newSelectionSet->weights.Emplace(DistanceToLineStart/LineLength);
			}
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNear(FVector center /*=FVector::ZeroVector*/, float innerRadius/*=0*/, float outerRadius/*=100*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectNear: Cannot create new SelectionSet"));
	}

	// Calculate the selection radius- we need it for falloff
	const float selectionRadius = outerRadius-innerRadius;

	// Iterate over the sections, and the vertices in each section.
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			const float distanceFromCenter = (vertex-center).Size();
			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			const float distanceBias = 1.0f-FMath::Clamp((distanceFromCenter-innerRadius)/selectionRadius, 0.0f, 1.0f);
			newSelectionSet->weights.Emplace(distanceBias);
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNearLine(FVector lineStart, FVector lineEnd, float innerRadius /*=0*/, float outerRadius/*= 100*/, bool lineIsInfinite/* = false */)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectNearLine: Cannot create new SelectionSet"));
	}

	// Calculate the selection radius- we need it for falloff
	const float selectionRadius = outerRadius-innerRadius;

	// Iterate over the sections, and the vertices in each section.
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			// Get the distance from the line based on whether we're looking at an infinite line or not.
			const FVector nearestPointOnLine = lineIsInfinite ?
				FMath::ClosestPointOnInfiniteLine(lineStart, lineEnd, vertex) :
				FMath::ClosestPointOnLine(lineStart, lineEnd, vertex);

			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			const float distanceToLine = (vertex-nearestPointOnLine).Size();
			const float distanceBias = 1.0f-FMath::Clamp((distanceToLine-innerRadius)/selectionRadius, 0.0f, 1.0f);
			newSelectionSet->weights.Emplace(distanceBias);
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNearSpline(USplineComponent *spline, FTransform transform, float innerRadius /*= 0*/, float outerRadius /*= 100*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("SelectNearSpline: Cannot create new SelectionSet"));
	}

	// Calculate the selection radius- we need it for falloff
	const float selectionRadius = outerRadius-innerRadius;

	// Iterate over the sections, and the vertices in each section.
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
// Convert the vertex location to local space- and then get the nearest point on the spline in local space.
			const FVector closestPointOnSpline = spline->FindLocationClosestToWorldLocation(
				transform.TransformPosition(vertex),
				ESplineCoordinateSpace::Local
			);
			const float distanceFromSpline = (vertex-closestPointOnSpline).Size();
			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			const float distanceBias = 1.0f-FMath::Clamp((distanceFromSpline-innerRadius)/selectionRadius, 0.0f, 1.0f);
			newSelectionSet->weights.Emplace(distanceBias);
		}
	}

	return newSelectionSet;
}

FVector UMeshGeometry::NearestPointOnPlane(FVector Vertex, FVector PointOnPlane, FVector PlaneNormal)
{
	// This is based on:
	//  https://www.gamedev.net/forums/topic/395194-closest-point-on-plane--distance/
	PlaneNormal.Normalize();
	const float distanceToPlane =
		FVector::PointPlaneDist(Vertex, PointOnPlane, PlaneNormal.GetSafeNormal());
	return (Vertex - (PlaneNormal * distanceToPlane));
}

float UMeshGeometry::MiniumProjectionPlaneDistance(FVector projection)
{
	// The projection needs to be normalized to act as plane
	projection = projection.GetSafeNormal();
	if (projection.IsZero()) {
		return 0;
	}

	// Get the radius and store it- we don't want to keep getting it
	const float radius = GetRadius();

	// Iterate over the sections, and the vertices in each section.
	float furthestPlane;
	bool haveProcessedFirstVertex = false;
	for (auto &section : this->sections)
	{
		for (auto &vertex : section.vertices)
		{
			// Get the nearest point from the origin to a plane with the
			// supplied projection and passing through the vector.
			const FVector nearestPointOnVertexPlane =
				NearestPointOnPlane(FVector::ZeroVector, vertex, projection);

			// Check we're on the correct side of the plane
			const FPlane plane = FPlane(nearestPointOnVertexPlane, -projection);
			// Do some vector maths to work out which side of the plane we're on.
			const bool dotTest = FVector::DotProduct(vertex.GetSafeNormal(), -projection.GetSafeNormal()) >=0;

			const float distanceFromVertexToPlane = nearestPointOnVertexPlane.Size() * (dotTest ? 1 : -1);

			/*
			UE_LOG(MDTLog, Warning, TEXT("Vertex %s, Plane pos: %s, norm %s, Dist %f, Dot %s, On %f"),
				*vertex.ToString(),
				*nearestPointOnVertexPlane.ToString(),
				*projection.ToString(),
				distanceFromVertexToPlane,
				dotTest ? TEXT("Yes") : TEXT("No"),
				plane.PlaneDot(vertex)
			);
			*/

			// Update furthestPlane info
			furthestPlane =
				haveProcessedFirstVertex ?
				FMath::Max(furthestPlane, distanceFromVertexToPlane) :
				distanceFromVertexToPlane;
			haveProcessedFirstVertex = true;
		}
	}
	return furthestPlane;
}

bool UMeshGeometry::SelectionSetIsRightSize(USelectionSet *selection, FString NodeNameForWarning) const
{
	// No selection set is fine...
	if (!selection)
	{
		return true;
	}

	// Get the sizes
	const int32 selectionSetSize = selection->Size();
	const int32 geometrySize = GetTotalVertexCount();

	// Check them
	if (selectionSetSize!=geometrySize)
	{
		UE_LOG(
			MDTLog, Warning, TEXT("%s: Selection set is the wrong size, %d weights in set for %d vertices in mesh"),
			*NodeNameForWarning, selectionSetSize, geometrySize
		);
		return false;
	}
	return true;
}

void UMeshGeometry::Spherize(float SphereRadius /*= 100.0f*/, float FilterStrength /*= 1.0f*/, FVector SphereCenter /*= FVector::ZeroVector*/, USelectionSet *Selection)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Spherize")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			const FVector vertexRelativeToCenter = vertex-SphereCenter;

			// Calculate the required length- incorporating both the SphereRadius and Selection.
			const float targetVectorLength = FMath::Lerp(vertexRelativeToCenter.Size(), SphereRadius, FilterStrength * (Selection ? Selection->weights[nextSelectionIndex++] : 1.0f));

			vertex = SphereCenter+(vertexRelativeToCenter * targetVectorLength);
		}
	}
}

void UMeshGeometry::Transform(FTransform Transform /*= FTransform::Identity*/, FVector CenterOfTransform /*= FVector::ZeroVector*/, USelectionSet *Selection /*= nullptr*/)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Transform")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			vertex = FMath::Lerp(
				vertex,
				CenterOfTransform+Transform.TransformPosition(vertex-CenterOfTransform),
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Translate(FVector delta, USelectionSet *Selection)
{
	// Check selectionSet size- log and abort if there's a problem. 
	if (!SelectionSetIsRightSize(Selection, TEXT("Translate")))
	{
		return;
	}

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section:this->sections)
	{
		for (auto &vertex:section.vertices)
		{
			vertex = FMath::Lerp(
				vertex,
				vertex+delta,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}
