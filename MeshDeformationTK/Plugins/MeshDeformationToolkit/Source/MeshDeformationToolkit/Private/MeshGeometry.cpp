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

bool UMeshGeometry::LoadFromStaticMesh(UStaticMesh *staticMesh, int32 LOD /*= 0*/)
{
	// If there's no static mesh we have nothing to do..
	if (!staticMesh) {
		UE_LOG(LogTemp, Warning, TEXT("LoadFromStaticMesh: No StaticMesh provided"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Reading mesh geometry from static mesh '%s'"), *staticMesh->GetName());

	// Clear any existing geometry.
	this->sections.Empty();

	const int32 numSections = staticMesh->GetNumSections(LOD);
	UE_LOG(LogTemp, Log, TEXT("Found %d sections for LOD %d"), numSections, LOD);

	// Iterate over the sections
	for (int meshSectionIndex = 0; meshSectionIndex < numSections; ++meshSectionIndex) {
		// Create the geometry for the section
		FSectionGeometry sectionGeometry;

		// Copy the static mesh's geometry for the section to the struct.
		UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(
			staticMesh, LOD, meshSectionIndex,
			sectionGeometry.vertices, sectionGeometry.triangles,
			sectionGeometry.normals, sectionGeometry.uvs, sectionGeometry.tangents
		);
		UE_LOG(LogTemp, Log, TEXT("Section %d: Found %d verts and %d triangles"), meshSectionIndex, sectionGeometry.vertices.Num(), sectionGeometry.triangles.Num() / 3);

		// Load vertex colors with default values for as many vertices as needed
		sectionGeometry.vertexColors.InsertDefaulted(0, sectionGeometry.vertices.Num());

		// Add the finished struct to the mesh's section list
		this->sections.Emplace(sectionGeometry);
	}

	// All done
	return true;
}

bool UMeshGeometry::SaveToProceduralMeshComponent(UProceduralMeshComponent *proceduralMeshComponent, bool createCollision)
{
	// If there's no PMC we have nothing to do..
	if (!proceduralMeshComponent) {
		UE_LOG(LogTemp, Warning, TEXT("SaveToProceduralMeshComponent: No proceduralMeshComponent provided"));
		return false;
	}

	// Clear the geometry
	proceduralMeshComponent->ClearAllMeshSections();

	// Iterate over the mesh sections, creating a PMC MeshSection for each one.
	int32 nextSectionIndex = 0;
	for (auto section : this->sections) {
		UE_LOG(LogTemp, Log, TEXT("Rebuilding section.."));
		// Create the PMC section with the StaticMesh's data.
		proceduralMeshComponent->CreateMeshSection_LinearColor(
			nextSectionIndex++, section.vertices, section.triangles, section.normals, section.uvs,
			section.vertexColors, section.tangents, createCollision
		);
	}
	return true;
}

int32 UMeshGeometry::TotalVertexCount() const
{
	int32 totalVertexCount = 0;
	for (auto section : this->sections) {
		totalVertexCount += section.vertices.Num();
	}
	return totalVertexCount;
}

int32 UMeshGeometry::TotalTriangleCount() const
{
	int32 totalTriangleCount = 0;
	for (auto section : this->sections) {
		totalTriangleCount += section.triangles.Num();
	}
	return totalTriangleCount / 3; // 3pts per triangle
}

FString UMeshGeometry::GetSummary() const
{
	return FString::Printf(TEXT("%d sections, %d vertices, %d triangles"), this->sections.Num(), this->TotalVertexCount(), this->TotalTriangleCount());
}

USelectionSet *UMeshGeometry::SelectAll()
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	newSelectionSet->CreateSelectionSet(this->TotalVertexCount());
	newSelectionSet->SetAllWeights(1.0f);
	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNear(FVector center /*=FVector::ZeroVector*/, float innerRadius/*=0*/, float outerRadius/*=100*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);

	// Iterate over the sections, and the vertices in each section.
	float distanceFromCenter;
	float distanceBias;
	float selectionRadius = outerRadius - innerRadius;

	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			distanceFromCenter = (vertex - center).Size();
			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			distanceBias = 1.0f - FMath::Clamp((distanceFromCenter - innerRadius) / selectionRadius, 0.0f, 1.0f);
			newSelectionSet->weights.Emplace(distanceBias);
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNearSpline(USplineComponent *spline, FTransform transform, float innerRadius /*= 0*/, float outerRadius /*= 100*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);

	// Iterate over the sections, and the vertices in each section.
	float distanceFromSpline;
	float distanceBias;
	FVector closestPointOnSpline;
	float selectionRadius = outerRadius - innerRadius;

	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// Convert the vertex location to local space- and then get the nearest point on the spline in local space.
			closestPointOnSpline = spline->FindLocationClosestToWorldLocation(
				transform.TransformPosition(vertex),
				ESplineCoordinateSpace::Local
			);
			distanceFromSpline = (vertex - closestPointOnSpline).Size();
			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			distanceBias = 1.0f - FMath::Clamp((distanceFromSpline - innerRadius) / selectionRadius, 0.0f, 1.0f);
			newSelectionSet->weights.Emplace(distanceBias);
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectNearLine(FVector lineStart, FVector lineEnd, float innerRadius /*=0*/, float outerRadius/*= 100*/, bool lineIsInfinite/* = false */)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);

	// Iterate over the sections, and the vertices in each section.
	FVector nearestPointOnLine;
	float distanceToLine;
	float distanceBias;
	float selectionRadius = outerRadius - innerRadius;

	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// Get the distance from the line based on whether we're looking at an infinite line or not.
			if (lineIsInfinite) {
				nearestPointOnLine = FMath::ClosestPointOnInfiniteLine(lineStart, lineEnd, vertex);
			} else {
				nearestPointOnLine = FMath::ClosestPointOnLine(lineStart, lineEnd, vertex);
			}
			// Apply bias to map distance to 0-1 based on innerRadius and outerRadius
			distanceToLine = (vertex - nearestPointOnLine).Size();
			distanceBias = 1.0f - FMath::Clamp((distanceToLine - innerRadius) / selectionRadius, 0.0f, 1.0f);
			newSelectionSet->weights.Emplace(distanceBias);
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectFacing(FVector Facing /*= FVector::UpVector*/, float InnerRadiusInDegrees /*= 0*/, float OuterRadiusInDegrees /*= 30.0f*/)
{
	// TODO: Check geometry looks valid (normals.Num == vertices.Num)
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	
	// Normalize the facing vector.
	if (!Facing.Normalize()) {
		// TODO: Better error handling.
		return newSelectionSet;
	}

	// Iterate over the sections, and the the normals in the sections.
	float selectionRadius = OuterRadiusInDegrees - InnerRadiusInDegrees;
	float angleBias;
	float angleToNormal;
	FVector normalizedNormal;
	// As we need normals to we'll use an index-based for loop here for verts.
	for (auto &section : this->sections) {
		for (auto normal : section.normals) {
			normalizedNormal = normal;

			if (normalizedNormal.Normalize()) {
				// Calculate the dot product between the normal and the Facing.
				angleToNormal = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(normal, Facing)));
				angleBias = 1.0f - FMath::Clamp((angleToNormal - InnerRadiusInDegrees) / selectionRadius, 0.0f, 1.0f);
				newSelectionSet->weights.Emplace(angleBias);
			} else {
				newSelectionSet->weights.Emplace(0);
			}
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectByNoise(
	int32 Seed /*= 1337*/,
	float Frequency /*= 0.01*/,
	ENoiseInterpolation NoiseInterpolation /*= ENoiseInterpolation::Quintic*/,
	ENoiseType NoiseType /*= ENoiseType::Simplex */,
	uint8 FractalOctaves /*= 3*/,
	float FractalLacunarity /*= 2.0*/,
	float FractalGain /*= 0.5*/,
	EFractalType FractalType /*= EFractalType::FBM*/,
	ECellularDistanceFunction CellularDistanceFunction /*= ECellularDistanceFunction::Euclidian*/,
	FVector NoiseTranslation /*= FVector::ZeroVector*/,
	FRotator NoiseRotation /*= FRotator::ZeroRotator*/,
	FVector NoiseScale3D /*= FVector(1, 1, 1)*/
) {
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);

	// TODO: Lots of work here!
	FastNoise noise;

	// Set up all of the noise details from the parameters provided
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

	FTransform NoiseTransform = FTransform(NoiseRotation, NoiseTranslation, NoiseScale3D);

	// Iterate over the sections, and the vertices in each section.
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// Apply the noise transform to the vertex and use the transformed vertex for the noise generation
			const FVector transformedVertex = NoiseTransform.TransformPosition(vertex);
			float NoiseValue = noise.GetNoise(transformedVertex.X, transformedVertex.Y, transformedVertex.Z);
			newSelectionSet->weights.Emplace(NoiseValue);
		}
	}

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectByTexture(UTexture2D *Texture2D, ETextureChannel TextureChannel /*=ETextureChannel::Red*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);

	// Check we have a texture and that it's in the right format
	if (!Texture2D) {
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
	UE_LOG(LogTemp, Log, TEXT("Texture res: %d x %d"), textureWidth, textureHeight);

	// Check we got the data and lock it
	if (!BulkData) {
		/// \todo Log an error..
		return nullptr;
	}
	FColor *colorArray = static_cast<FColor*>(BulkData->Lock(LOCK_READ_ONLY));

	// Iterate over the sections, and the vertices in each section.
	for (auto &section : this->sections) {
		for (auto &uv : section.uvs) {
			// Convert our UV to a texture index.
			int32 textureX = (int32)FMath::RoundHalfFromZero(uv.X * textureWidth);
			int32 textureY = (int32)FMath::RoundHalfFromZero(uv.Y * textureHeight);

			/// \todo Wrap/Clamp
			textureX = FMath::Clamp(textureX, 0, textureWidth-1);
			textureY = FMath::Clamp(textureY, 0, textureHeight-1);

			// Get the color and access the correct channel.
			int32 index = (textureY * textureWidth) + textureX;
			//UE_LOG(LogTemp, Log, TEXT("UV %f,%f = %d,%d (%d)"), uv.X, uv.Y, textureX, textureY, index);
			
			FLinearColor color = colorArray[index];
			//UE_LOG(LogTemp, Log, TEXT("Color %f,%f,%f / %f"), color.R, color.G, color.B, color.A);
			
			switch (TextureChannel) {
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
			//newSelectionSet->weights.Emplace(0);
		}
	}

	// Unlock the texture data
	BulkData->Unlock();

	return newSelectionSet;
}

USelectionSet * UMeshGeometry::SelectLinear(FVector LineStart, FVector LineEnd, bool Reverse /*= false*/, bool LimitToLine /*= false*/)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);

	// Do the reverse if needed..
	if (Reverse) {
		FVector TmpVector = LineStart;
		LineStart = LineEnd;
		LineEnd = TmpVector;
	}

	// Calculate the length of the line.
	float LineLength = (LineEnd - LineStart).Size();
	if (LineLength < 0.1) {
		// Lines too close..
		/// \todo Log error message..
		return nullptr;
	}

	// Iterate over the sections, and the vertices in each section
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// Get the nearest point on the line
			FVector NearestPointOnLine = FMath::ClosestPointOnLine(LineStart, LineEnd, vertex);

			// If we've hit one of the end points then return the limits
			if (NearestPointOnLine == LineEnd) {
				newSelectionSet->weights.Emplace(LimitToLine ? 0.0f : 1.0f);
			}
			else if (NearestPointOnLine == LineStart) {
				newSelectionSet->weights.Emplace(0.0f);
			}
			else {
				// Get the distance to the two start point- it's the ratio we're after.
				float DistanceToLineStart = (NearestPointOnLine - LineStart).Size();
				newSelectionSet->weights.Emplace(DistanceToLineStart / LineLength);
			}
		}
	}

	return newSelectionSet;
}

USelectionSet *UMeshGeometry::SelectInVolume(FVector CornerA, FVector CornerB) {
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);

	// Get the minimum/maximum of X, Y, and Z from the corner vectors so we can check
	auto minX = FMath::Min(CornerA.X, CornerB.X);
	auto maxX = FMath::Max(CornerA.X, CornerB.X);
	auto minY = FMath::Min(CornerA.Y, CornerB.Y);
	auto maxY = FMath::Max(CornerA.Y, CornerB.Y);
	auto minZ = FMath::Min(CornerA.Z, CornerB.Z);
	auto maxZ = FMath::Max(CornerA.Z, CornerB.Z);

	// Iterate over the sections, and the vertices in each section
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// We only need to know if the vertex is between min/max inclusive.
			const bool vertexInVolume =
				(vertex.X >= minX) && (vertex.X <= maxX) &&
				(vertex.Y >= minY) && (vertex.Y <= maxY) &&
				(vertex.Z >= minZ) && (vertex.Z <= maxZ);
			// Add a new weighting based on whether it's insider or outside
			newSelectionSet->weights.Emplace(vertexInVolume ? 1.0f : 0.0f);
		}
	}

	return newSelectionSet;
}


USelectionSet * UMeshGeometry::SelectBySection(int32 SectionIndex)
{
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(this);
	
	// Keep track of the current section index.
	int32 currentSectionIndex = 0;

	// Iterate over the sections, and the vertices in each section
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// Add a new weight- 1.0 if section indices match, 0.0 otherwise.
			newSelectionSet->weights.Emplace(SectionIndex == currentSectionIndex ? 1.0f : 0.0f);
		}
		// Increment the current section index, we've finished with this section
		currentSectionIndex++;
	}

	return newSelectionSet;
}

void UMeshGeometry::Jitter(FRandomStream &randomStream, FVector min, FVector max, USelectionSet *selection /*=nullptr*/)
{
	// TODO: Check selectionSet size.

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	FVector randomJitter;
	// Iterate over the sections, and the vertices in each section.
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			randomJitter = FVector(
				randomStream.FRandRange(min.X, max.X),
				randomStream.FRandRange(min.Y, max.Y),
				randomStream.FRandRange(min.Z, max.Z)
			);
			vertex = FMath::Lerp(
				vertex,
				vertex + randomJitter,
				selection ? selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Translate(FVector delta, USelectionSet *selection)
{
	// TODO: Check selectionSet size.

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			vertex = FMath::Lerp(
				vertex,
				vertex + delta,
				selection ? selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Rotate(FRotator Rotation /*= FRotator::ZeroRotator*/, FVector CenterOfRotation /*= FVector::ZeroVector*/, USelectionSet *Selection)
{
	// TODO: Check SelectionSet size.

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			vertex = FMath::Lerp(
				vertex,
				CenterOfRotation + Rotation.RotateVector(vertex - CenterOfRotation),
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Scale(FVector Scale3d /*= FVector(1, 1, 1)*/, FVector CenterOfScale /*= FVector::ZeroVector*/, USelectionSet *Selection /*= nullptr*/)
{
	// TODO: Check selectionSet size.

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			vertex = FMath::Lerp(
				vertex,
				CenterOfScale + (vertex - CenterOfScale) * Scale3d,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Transform(FTransform Transform /*= FTransform::Identity*/, FVector CenterOfTransform /*= FVector::ZeroVector*/, USelectionSet *Selection /*= nullptr*/)
{
	// TODO: Check selectionSet size.

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			vertex = FMath::Lerp(
				vertex,
				CenterOfTransform + Transform.TransformPosition(vertex - CenterOfTransform),
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

void UMeshGeometry::Spherize(float SphereRadius /*= 100.0f*/, float FilterStrength /*= 1.0f*/, FVector SphereCenter /*= FVector::ZeroVector*/, USelectionSet *Selection)
{
	// TODO: Check selectionSet size.

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	FVector vertexRelativeToCenter;
	float targetVectorLength;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			vertexRelativeToCenter = vertex - SphereCenter;
			// Calculate the required length- incorporating both the SphereRadius and Selection.
			targetVectorLength = FMath::Lerp(vertexRelativeToCenter.Size(), SphereRadius, FilterStrength * (Selection ? Selection->weights[nextSelectionIndex++] : 1.0f));
			// TODO: Think what happens when this fails?
			if (vertexRelativeToCenter.Normalize()) {
				vertex = SphereCenter + (vertexRelativeToCenter * targetVectorLength);
			}
		}
	}
}

void UMeshGeometry::Inflate(float Offset /*= 0.0f*/, USelectionSet *Selection /*= nullptr*/)
{
	// TODO: Check selectionSet size.
	// TODO: Check normals size.

	// Iterate over the sections, and the the vertices in the sections.
	// As we need normals to we'll use an index-based for loop here for verts.
	for (auto &section : this->sections) {
		for (int32 vertexIndex = 0; vertexIndex < section.vertices.Num(); ++vertexIndex) {
			section.vertices[vertexIndex] = FMath::Lerp(
				section.vertices[vertexIndex],
				section.vertices[vertexIndex] + (section.normals[vertexIndex] * Offset),
				Selection ? Selection->weights[vertexIndex] : 1.0f
			);
		}
	}
}

void UMeshGeometry::ScaleAlongAxis(FVector CenterOfScale /*= FVector::ZeroVector*/, FVector Axis /*= FVector::UpVector*/, float Scale /*= 1.0f*/, USelectionSet *Selection /*= nullptr*/)
{
	// TODO Check SelectionSet size
	// TODO: Check non-zero vectors.

	// Iterate over the sections, and the the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			FVector closestPointOnLine = FMath::ClosestPointOnInfiniteLine(CenterOfScale, CenterOfScale + Axis, vertex);
			FVector offsetFromClosestPoint = vertex - closestPointOnLine;
			FVector scaledPointOnLine = Scale * (closestPointOnLine - CenterOfScale) + CenterOfScale;
			vertex = FMath::Lerp(vertex, scaledPointOnLine + offsetFromClosestPoint, Selection ? Selection->weights[nextSelectionIndex++] : 1.0f);
		}
	}
}

void UMeshGeometry::RotateAroundAxis(FVector CenterOfRotation /*= FVector::ZeroVector*/, FVector Axis /*= FVector::UpVector*/, float AngleInDegrees /*= 0.0f*/, USelectionSet *Selection /*= nullptr*/)
{
	// TODO Check SelectionSet size
	// TODO: Check non-zero vectors.

	// Normalize the axis direction.
	auto normalizedAxis = Axis.GetSafeNormal();
	if (normalizedAxis.IsNearlyZero(0.1f)) {
		UE_LOG(LogTemp, Error, TEXT("RotateAroundAxis: Could not normalize Axis, zero vector?"));
		return;
	}

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			FVector closestPointOnLine = FMath::ClosestPointOnInfiniteLine(CenterOfRotation, CenterOfRotation + Axis, vertex);
			FVector offsetFromClosestPoint = vertex - closestPointOnLine;
			float scaledRotation = FMath::Lerp(
				0.0f, AngleInDegrees,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
			FVector rotatedOffset = offsetFromClosestPoint.RotateAngleAxis(scaledRotation, normalizedAxis);
			vertex = closestPointOnLine + rotatedOffset;
		}
	}
}

void UMeshGeometry::Lerp(UMeshGeometry *TargetMeshGeometry, float Alpha /*= 0.0f*/, USelectionSet *Selection /*= nullptr*/) {
	// TODO: Check SelectionSet size

	// Iterate over the sections, and the vertices in the sections.  Do it by index so we
	// can access the same data from TargetMeshGeometry
	int32 nextSelectionIndex = 0;

	if (!TargetMeshGeometry) {
		UE_LOG(LogTemp, Error, TEXT("Lerp: No TargetMeshGeometry"));
		return;
	}
	if (this->sections.Num() != TargetMeshGeometry->sections.Num()) {
		UE_LOG(
			LogTemp, Error, TEXT("Lerp: Cannot lerp geometries with different numbers of sections, %d compared to %d"),
			this->sections.Num(), TargetMeshGeometry->sections.Num()
		);
		return;
	}

	for (int32 sectionIndex = 0; sectionIndex < this->sections.Num(); sectionIndex++) {
		UE_LOG(
			LogTemp, Error, TEXT("Lerp: SECTION INDEX %d"),
			sectionIndex
		);

		if (this->sections[sectionIndex].vertices.Num() != TargetMeshGeometry->sections[sectionIndex].vertices.Num()) {
			UE_LOG(
				LogTemp, Error, TEXT("Lerp: Cannot lerp geometries with different numbers of vertices, %d compared to %d for section %d"),
				this->sections[sectionIndex].vertices.Num(), TargetMeshGeometry->sections[sectionIndex].vertices.Num(), sectionIndex
			);
			return;
		}

		for (int32 vertexIndex = 0; vertexIndex < this->sections[sectionIndex].vertices.Num(); ++vertexIndex) {
			// Get the existing data from the two components.
			FVector vertexFromThis = this->sections[sectionIndex].vertices[vertexIndex];
			FVector vertexFromTarget = TargetMeshGeometry->sections[sectionIndex].vertices[vertexIndex];

			UE_LOG(
				LogTemp, Error, TEXT("Lerp: Vertex index %d"),
				vertexIndex
			);
			// TODO: World/local logic should live here.
			this->sections[sectionIndex].vertices[vertexIndex] = FMath::Lerp(
				vertexFromThis, vertexFromTarget,
				Alpha * (Selection ? Selection->weights[nextSelectionIndex++] : 1.0f)
			);
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
) {
	if (!SplineComponent) {
		UE_LOG(LogTemp, Error, TEXT("FitToSpline: No SplineComponent"));
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
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// Remap the X position into the StartPosition/EndPosition range, then multiply by SplineLength to get a value we
			// can use for lookup.
			const float distanceAlongSpline = FMath::GetMappedRangeValueClamped(rangeX, rangePosition, vertex.X) * splineLength;
			
			// If we have either profile curve we now need to find the position at a given point.  For efficiency
			//   we can combine this with MeshScale.
			float combinedMeshScale = MeshScale;
			if (ProfileCurve) {
				// Get the range of the curve
				float profileCurveMin;
				float profileCurveMax;
				ProfileCurve->GetTimeRange(profileCurveMin, profileCurveMax);
				FVector2D profileCurveRange = FVector2D(profileCurveMin, profileCurveMax);

				const float positionAlongCurve =
					FMath::GetMappedRangeValueClamped(fullSplineRange, profileCurveRange, distanceAlongSpline);
				combinedMeshScale = combinedMeshScale * ProfileCurve->GetFloatValue(positionAlongCurve);
			}
			if (SectionProfileCurve) {
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
			FVector splineVertexPosition = location + (rightVector * vertex.Y * combinedMeshScale) + (upVector * vertex.Z * combinedMeshScale);
			vertex = FMath::Lerp(
				vertex, splineVertexPosition,
				Selection ? Selection->weights[nextSelectionIndex++] : 1.0f
			);
		}
	}
}

/*

bool UKismetSystemLibrary::LineTraceSingle(UObject* WorldContextObject, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceChannel);

static const FName LineTraceSingleName(TEXT("LineTraceSingle"));
FCollisionQueryParams Params = ConfigureCollisionParams(LineTraceSingleName, bTraceComplex, ActorsToIgnore, bIgnoreSelf, WorldContextObject);

UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
bool const bHit = World->LineTraceSingleByChannel(OutHit, Start, End, CollisionChannel, Params);

#if ENABLE_DRAW_DEBUG
DrawDebugLineTraceSingle(World, Start, End, DrawDebugType, bHit, OutHit, TraceColor, TraceHitColor, DrawTime);
#endif

return bHit;
}

*/
void UMeshGeometry::Conform(
	UObject* WorldContextObject,
	FTransform Transform,
	TArray <AActor *> IgnoredActors,
	FVector Projection /*= FVector(0, 0, -100)*/,
	bool TraceComplex /*=true*/,
	ECollisionChannel CollisionChannel /*= ECC_WorldStatic*/,
	USelectionSet *Selection /*= nullptr */
) {
	// Get the world content we're operating in
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if (!World) {
		UE_LOG(LogTemp, Error, TEXT("Conform: Cannot access game world"));
		return;
	}

	// Prepare the trace query parameters
	const FName traceTag("ConformTraceTag");
	//FCollisionQueryParams traceQueryParams = ConfigureCollisionParams(traceTag, TraceComplex, IgnoredActors, false, WorldContextObject);
	FCollisionQueryParams traceQueryParams = FCollisionQueryParams(); //  traceTag, TraceComplex, IgnoredActors);
	traceQueryParams.TraceTag = traceTag;
	traceQueryParams.bTraceComplex = TraceComplex;
	// TODO: Ignored actors..


	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			// Apply the transform to the vertex to obtain the start of the vector.
			FVector traceStart= Transform.TransformPosition(vertex);

			// Calculate the projection end by applying the SelectionSet to filter it
			float weight = Selection ? Selection->weights[nextSelectionIndex++] : 1.0f;
			FVector scaledProjection = Projection * weight;
			FVector traceEnd = Transform.TransformPosition(vertex + Projection);

			// Do the projection
			FHitResult hitResult;
			// TODO: Check world validity.
			bool hitSuccess = World->LineTraceSingleByChannel(
				hitResult,
				traceStart, traceEnd,
				CollisionChannel, traceQueryParams, FCollisionResponseParams()
			);

			UE_LOG(LogTemp, Log, TEXT("Projecting %s : %s to %s"), *vertex.ToString(), *traceStart.ToString(), *traceEnd.ToString());
			UE_LOG(LogTemp, Log, TEXT("Hit result: %s"), *hitResult.ToString());
			// If the collision happened then use that as the new vertex value,
			// otherwise use the trace end
			vertex = hitResult.bBlockingHit ?
				Transform.InverseTransformPosition(hitResult.ImpactPoint) :
				Transform.InverseTransformPosition(traceEnd);
			
			//vertex = Transform.InverseTransformPosition(traceEnd);
		}
	}
}
 

FBox UMeshGeometry::GetBoundingBox() {
	// Track the two corners of the bounding box
	FVector min = FVector::ZeroVector;
	FVector max = FVector::ZeroVector;

	// When we hit the first vertex we'll need to set both Min and Max
	//  to it as we'll have no comparison
	bool haveProcessedFirstVector = false;

	// Iterate over the sections, and the vertices in the sections.
	int32 nextSelectionIndex = 0;
	for (auto &section : this->sections) {
		for (auto &vertex : section.vertices) {
			if (haveProcessedFirstVector) {
				// Do the comparison of both min/max.
				min.X = FMath::Min(min.X, vertex.X);
				min.Y = FMath::Min(min.Y, vertex.Y);
				min.Z = FMath::Min(min.Z, vertex.Z);

				max.X = FMath::Max(max.X, vertex.X);
				max.Y = FMath::Max(max.Y, vertex.Y);
				max.Z = FMath::Max(max.Z, vertex.Z);
			}
			else {
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