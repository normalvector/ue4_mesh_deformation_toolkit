// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "UObject/NoExportTypes.h"
#include "SectionGeometry.h"
#include "Math/TransformNonVectorized.h"
#include "Runtime/Engine/Classes/Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "SelectionSet.h"
#include "FastNoise.h"
#include "FastNoiseBPEnums.h"
#include "MeshGeometry.generated.h"

/// \todo Select linear - Select based on a position and a linear falloff
/// \todo Select From Texture - Select the vertices based on a texture accessed from the UV.
/// \todo Think ahead to other procedural tools - Should the "Select" functions be renamed SelectVerts?
/// \todo Should Selects return nullptr or empty array?  (Should return nullptr and the further bits should check SelectionSet)

/// This class stores the geometry for a mesh which can then be mutated by the
/// methods provided to allow a range of topological deformations.
///
/// While it is possible to use this class alone the main intent here is to be the
/// geometry backend for *MeshDeformationComponent*.
///
/// \see MeshDeformationComponent
///
/// \todo Copy/cache MeshGeometry - allows us to do part of it and go back
/// \todo Lerp - Blend between two MeshGeometrys
/// \todo SplineLerp - Lerps along a spline where the binormals drive the spline tangents and normals drive the spline
///                    direction.
/// \todo Output to Static Mesh - Allow the system to write to a static mesh while running in the editor
/// \todo Read from PMC - Allow the system to use a PMC as a source of geometry
/// \todo Read from OBJ - Read in an obj text file
/// \todo Write to OBJ - Output an obj text file

UCLASS(BlueprintType)
class MESHDEFORMATIONTOOLKIT_API UMeshGeometry : public UObject
{
	GENERATED_BODY()

public:
	/// The actual geometry making up this mesh.
	///
	/// This is stored as an array with each element representing the geometry of a single section
	/// of the geometry.
	UPROPERTY(BlueprintReadonly)
		TArray<FSectionGeometry> sections;

	/// Default constructor- creates an empty mesh.
	UMeshGeometry();

	/*
	##################################################
	Load Geometry Data

	All of these functions serve to load the data into the geometry and should have names
	beginning with *Load*.

	If they return anything it should be a boolean indicating success/failure.
	##################################################
	*/

	/// Loads the geometry from a static mesh
	/// 
	/// This replaces any geometry currently stored.
	///
	/// \param staticMesh					The mesh to copy the geometry from
	/// \param LOD							A StaticMesh can have multiple meshes for different levels of detail, this specifies which LOD we're taking the information fromaram>
	/// \return *True* if we could read the geometry, *False* if not
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta=(Keywords="create sm"))
		bool LoadFromStaticMesh(UStaticMesh *staticMesh, int32 LOD = 0);

	/*
	##################################################
	Select Vertices

	All of these functions serve to select vertices based on some criteria.  They should all
	have names beginning with *Select*, and return a new USelectionSet.
	##################################################
	*/

	/// Selects all of the vertices at full strength.
	///
	/// \return A *SelectionSet* with full strength
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry)
		USelectionSet *SelectAll();

	/// Selects vertices based on a noise function.
	///
	/// This uses the [FastNoise](https://github.com/Auburns/FastNoise) noise library by Jordan Pack and released under the MIT license.
	/// Not all of these settings are used by each noise type, details on their application is in the
	/// [FastNoise docs](https://github.com/Auburns/FastNoise/wiki/Noise-Settings).
	///
	/// \param Seed							The seed for the random number generator
	/// \param Frequency					The frequency of the noise, the higher the value the more detail
	/// \param NoiseInterpolation			The interpolation used to smooth between noise values
	/// \param NoiseType					The type of noise we're using
	/// \param FractalOctaves				The number of fractal octaves to apply
	/// \param FractalLacunarity			Set the fractal lacunarity, the higher the value the more space the
	///										the fractal will fill up
	/// \param FractalGain					The strength of the fractal
	/// \param FractalType					The type of fractal being used
	/// \param CellularDistanceFunction		The function used to calculate the value for a given point.
	/// \param NoiseTransform				An optional transformation which will be applied to each vertex's position
	///										before it is used for noise generation, if not supplied an identity transform
	///										will be used which does not modify the vertex position.  Useful for making tiling terrain
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "random fastnoise perlin fractal terrain"))
		USelectionSet *SelectByNoise(
			int32 Seed = 1337,
			float Frequency = 0.01,
			ENoiseInterpolation NoiseInterpolation = ENoiseInterpolation::Quintic,
			ENoiseType NoiseType = ENoiseType::Simplex,
			uint8 FractalOctaves = 3,
			float FractalLacunarity = 2.0,
			float FractalGain = 0.5,
			EFractalType FractalType = EFractalType::FBM,
			ECellularDistanceFunction CellularDistanceFunction = ECellularDistanceFunction::Euclidian,
			FVector NoiseTranslation = FVector::ZeroVector,
			FRotator NoiseRotation = FRotator::ZeroRotator,
			FVector NoiseScale3D = FVector(1, 1, 1)
		);

	/// Select all of the vertices which go to make up one of the Sections that a mesh
	/// can consist of.  This can be thought of as the same as a Material slot for many
	/// uses.
	///
	/// \param SectionIndex
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "material geometry"))
		USelectionSet *SelectBySection(int32 SectionIndex);

	/// Select vertices from a texture.
	///
	/// Black in the channel = Unselected, White = Fully selected.  Uses UV0 for texture access as that's
	/// what GetSectionFromStaticMesh makes available to us.
	///
	/// \param Texture2D					The texture object to sample
	/// \param TextureChannel				Which channel (RGBA) of the texture to use
	/// \return The SelectionSet for the texture channel
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "image picture rgb uv"))
		USelectionSet *SelectByTexture(UTexture2D *Texture2D, ETextureChannel TextureChannel = ETextureChannel::Red);

	/// Selects vertices with a given normal facing
	///
	/// This does a smooth linear selection based on the angle from the specified normal direction.
	/// \param Facing		The facing to select, in world space
	/// \param InnerRadiusInDegrees	The inner radius in degrees, all vertices with a normal within
	///								this deviation from Facing will be selected at full strength.
	/// \param OuterRadiusInDegrees	The outer radius in degrees, all vertices with a normal greater
	///								than this deviation from Facing will not be selected.
	/// \return A *SelectionSet* for the selected vertices
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "normal vector"))
		USelectionSet *SelectFacing(FVector Facing = FVector::UpVector, float InnerRadiusInDegrees = 0, float OuterRadiusInDegrees = 30.0f);

	/// Select vertices inside a volume defined by two opposite corner points.
	/// \param CornerA						The first corner to define the volume
	/// \param CornerB						The second corner to define the volume
	///
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "aabb bounds bounding space"))
		USelectionSet *SelectInVolume(FVector CornerA, FVector CornerB);

	/// Select vertices linearly between two points.
	///
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "gradient between"))
		USelectionSet *SelectLinear(FVector LineStart, FVector LineEnd, bool Reverse = false, bool LimitToLine = false);

	/// Selects the vertices near a point in space.
	///
	/// This does a smooth linear radial selection based on distance form the point provided.
	///
	/// \param center		The center of the selection in local space
	/// \param innerRadius	The inner radius, all points inside this will be selected at
	///								maximum strength
	/// \param outerRadius	The outer radius, all points outside this will not be selected
	/// \return A *SelectionSet* for the selected vertices
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "close soft"))
		USelectionSet *SelectNear(FVector center = FVector::ZeroVector, float innerRadius = 0, float outerRadius = 100);

	/// Selects vertices near a line segment with the provided start/end points.
	///
	/// This does a smooth linear selection based on the distance from the line points provided.
	///
	/// \param lineStart		The position of the start of the line in local space
	/// \param lineEnd			The position of the end of the line in local spac3e
	/// \param innerRadius		The inner radius, all points closer to the line segment than this distance
	///							will be selected at maximum strength
	/// \param outerRadius		The outer radius, all points further from the line segment than this distance
	///							will not be selected
	/// \param lineIsInfinite	If this is checked then lineStart/lineEnd will treated as two points on an
	///							infinite line instead of being the start/end of a line segment
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "infinite"))
		USelectionSet *SelectNearLine(FVector lineStart, FVector lineEnd, float innerRadius = 0, float outerRadius = 100, bool lineIsInfinite = false);

	/// Selects the vertices near a Spline, allowing curves to easily guide deformation.
	///
	/// This does a smooth linear radial selection based on distance from the spline provided.
	///
	/// \param spline		The spline to be used for the selection
	/// \param transform	This is the transform to convert from local->world space and is normally GetOwner()->GetTransform()
	/// \param innerRadius	The inner radius, all points closer to the spline than this distance
	///						will be selected at maximum strength.
	/// \param outerRadius	The outer radius, all points further from the spline than this distance
	///						will not be selected
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "curve"))
		USelectionSet *SelectNearSpline(USplineComponent *spline, FTransform transform, float innerRadius = 0, float outerRadius = 100);

	/*
	##################################################
	Transform Vertices

	All of these functions serve to manipulate the underlying geometry.

	They should all have the MDC they were called on as an output pin for chaining
	##################################################
	*/

	// there was a hit, false otherwise	*/
	//		UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "LineTraceByChannel", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	//		static bool LineTraceSingle(UObject* WorldContextObject, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	UFUNCTION(
		BlueprintCallable, Category = MeshGeometry,
		meta = (AutoCreateRefTerm = "IgnoredActors", WorldContext = "WorldContextObject", Keywords = "drop drape cloth collision soft trace")
	)
		void Conform(
			UObject* WorldContextObject,
			FTransform Transform,
			TArray <AActor *> IgnoredActors,
			FVector Projection = FVector(0, 0, -100),
			float HeightAdjust = 0,
			bool TraceComplex = true,
			ECollisionChannel CollisionChannel = ECC_WorldStatic,
			USelectionSet *Selection = nullptr
		);

	/// Deform the mesh along a spline with more control than UE4's own SplineMeshComponent.
	///
	/// \param StartPosition				The position (0 to 1) on the spline that the mesh should start, 
	///										defaults to 0 which is the start of the spline.  Changing
	///										this allows a mesh to be mapped to different parts of the
	///										spline allowing the mesh to appear to be moving or growing
	///										along the spline, or allowing multiple meshes to be mapped
	///										to different portions of the spline.
	/// \param EndPosition   				The position (0 to 1) on the spline that the mesh should end, 
	///										defaults to 1 which is the end of the spline.  Changing
	///										this allows a mesh to be mapped to different parts of the
	///										spline allowing the mesh to appear to be moving or growing
	///										along the spline, or allowing multiple meshes to be mapped
	///										to different portions of the spline.
	/// \param MeshScale					Global setting to control the size of the deformed mesh, allowing
	///										a 'thicker or thinner' mesh to be produced.
	/// \param ProfileCurve					This optional curve will be applied along the entire length of the
	///										spline and allows control of the profile of the mesh so you can
	///										make sure parts thicker/thinner than others.  As this is applied
	///										to the entire spline if you set StartPosition/EndPosition only part
	///										of this curve will be used.
	/// \param SectionProfileCurve			This optional curve will be applied between StartPosition and EndPosition
	///										and allows control of the profile of the mesh so you can
	///										make sure parts thicker/thinner than others.  As this is applied
	///										only within StartPosition and EndPosition it can be used to shape the
	///										mesh regardless of the overall spline's length.
	///	\param Selection					The SelectionSet controlling how strongly the spline applies to each vertex.
	///										At present this is a simple position-based lerp and may not be too useful.
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "curve bend"))
		void FitToSpline(
			USplineComponent *SplineComponent,
			float StartPosition = 0.0f,
			float EndPosition = 1.0f,
			float MeshScale = 1.0f,
			UCurveFloat *SplineProfileCurve = nullptr,
			UCurveFloat *SectionProfileCurve = nullptr,
			USelectionSet *Selection = nullptr
		);

	/// Moves vertices a specified offset along their own normals
	///
	/// \param Offset							The distance to offset
	/// \param Selection						The SelectionSet, with the offset being scaled for
	///											each vertex
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "normal"))
		void Inflate(float Offset = 0.0f, USelectionSet *Selection = nullptr);

	/// Adds random jitter to the position of the points.
	///
	///  The jitter will be a vector randomly selected
	///  (with [continuous uniform distribution]() between *Min* and *Max*, and will
	///  be scaled by each vertex's selection weights if they're provided.
	///
	/// \param randomStream					The random stream to source numbers from
	/// \param min							The minimum jittered offset
	/// \param max							The maximum jittered offset
	/// \param selection					The selection weights, if not specified
	///										then all points will be jittered at
	///										maximum strength
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "random position"))
		void Jitter(FRandomStream &randomStream, FVector min, FVector max, USelectionSet *selection = nullptr);

	/// Does a linear interpolate with another MeshGeometry object, storing the result in this MeshGeometry.
	///
	/// The lerp is just applied in local space so may not be perfect with a lot of models.  This will only apply
	/// to the vertex positions and so while it will handle different topologies it will keep the triangle data
	/// from this rather than do anything clever.
	///
	/// \todo Allow this to work in either local or world space.
	///
	/// \param TargetMeshGeometry			The geometry to blend with
	/// \param Alpha						The alpha of the blend, 0=Return this Mesh
	/// \param Selection					The SelectionSet which controls the blend between the two MeshGeometry items
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "blend linear interpolate alpha"))
		void Lerp(UMeshGeometry *TargetMeshGeometry, float Alpha = 0.0, USelectionSet *Selection = nullptr);

	/// Rotates the vertices of the mesh a specified amount round the specified position.
	/// 
	/// If a SelectionSet is provided then the actual rotator will be scaled accordingly allowing
	/// whorls and similar to be easily created.
	///
	/// \param Rotation							The rotation to apply
	/// \param CenterOfRotation					The center of rotation in local space
	/// \param Selection						The selection weights, if not specified
	///											then all points will be rotated by the full rotation
	///											specified
	UFUNCTION(BlueprintCallable, Category = MeshGeometry)
		void Rotate(FRotator Rotation = FRotator::ZeroRotator, FVector CenterOfRotation = FVector::ZeroVector, USelectionSet *Selection = nullptr);
	
	/// Rotate vertices about an arbitrary axis
	///
	/// This allows more freedom than the standard 'Rotate around X, Y, and Z' and is more flexible
	/// than the standard approach even if it is less intuitive.
	///
	/// \param CenterOfRotation				The center of the rotation operation in local space
	/// \param Axis							The axis to rotate about
	/// \param AngleInDegrees				The angle to rotate the vertices about
	/// \param Selection					The SelectionSet which controls the amount of rotation
	///										applied to each vertex.
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "twist screw"))
		void RotateAroundAxis(FVector CenterOfRotation = FVector::ZeroVector, FVector Axis = FVector::UpVector, float AngleInDegrees = 0.0f, USelectionSet *Selection = nullptr);

	/// Scale the selected points on a per-axis basis about a specified center
	///
	/// \param Scale3d							The per-axis scaling
	/// \param CenterOfScale					The center of the scaling operation in local space
	/// \param Selection						The selection weights, if not specified then all
	///											vertices will be scaled fully by the specified scale
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "size"))
		void Scale(FVector Scale3d = FVector(1, 1, 1), FVector CenterOfScale = FVector::ZeroVector, USelectionSet *Selection = nullptr);

	/// Scale an object along an arbitrary axis
	///
	/// This allows objects to be scaled along any axis, not just the normal XYZ, and so is more
	/// flexible than the standard approach, even thought it is less intuitive.
	///
	/// \param CenterOfScale					The center of the scale operation, in local space
	/// \param Axis								The axis to scale along
	/// \param Scale							The ratio to scale by
	/// \param Selection						The SelectionSet which controls the weighting of the
	///											scale for each vertex.  If not provided then the scale
	///											will apply at full strength to all vertices.
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "size"))
		void ScaleAlongAxis(FVector CenterOfScale = FVector::ZeroVector, FVector Axis = FVector::UpVector, float Scale = 1.0f, USelectionSet *Selection = nullptr);

	/// Morph a mesh into a sphere by moving points along their normal
	///
	/// \param SphereRadius					The radius of the sphere to morph to
	/// \param FilterStrength				The strength of the effect, 0=No effect, 1=Full effect.
	///	\param SphereCenter					The center of the sphere
	/// \param Selection					The SelectionSet, if specified this will be multiplied
	///										by FilterStrength to allow each vertex's morph to be
	///										individually controlled.
	/// \todo Should group the sphere parameters together
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "ball"))
		void Spherize(float SphereRadius = 100.0f, float FilterStrength = 1.0f, FVector SphereCenter = FVector::ZeroVector, USelectionSet *Selection = nullptr);

	/// Applies Scale/Rotate/Translate as a single operation using a combined transform.
	///
	/// The order of the operations will be Scale/Rotate/Translate as documented
	/// [here](https://docs.unrealengine.com/latest/INT/API/Runtime/Core/Math/FTransform/index.html).
	///
	/// \param Transform					The transformation to apply
	/// \param CenterOfTransform			The center of the transformation, in local space
	/// \param Selection					The SelectionSet, if not specified then all vertices
	///										will be transformed at full strength
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "move scale size rotate"))
		void Transform(FTransform Transform, FVector CenterOfTransform = FVector::ZeroVector, USelectionSet *Selection = nullptr);

	/// Move all selected by the provided delta vector.
	///
	///  If a *SelectionSet* is provided the delta will be weighted according to the vertex's
	///   selection weight.
	///
	/// \param delta							The translation delta in local space
	/// \param selection						The selection weights, if not specified
	///											then all points will be moved by the
	///											full delta translation
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "move delta"))
		void Translate(FVector delta, USelectionSet *selection);

	/*
	##################################################
	Save Geometry Data

	All of these functions serve to pass the geometry data into some external
	system, and should have a name starting with *Save*.

	If they return anything it should be a boolean indicating success/failure.
	##################################################
	*/

	/// Save the current geometry to a *ProceduralMeshComponent*.
	/// 
	/// This will rebuild the mesh, completely replacing any geometry it has there.
	///
	/// \param proceduralMeshComponent		The target *ProceduralMeshComponent
	/// \param createCollision				Whether to create a collision shape for it
	/// \return *True* if the update was successful, *False* if not
	UFUNCTION(BlueprintCallable, Category = MeshGeometry,
		meta = (Keywords = "pmc output write"))
		bool SaveToProceduralMeshComponent(UProceduralMeshComponent *proceduralMeshComponent, bool createCollision);

	/*
	##################################################
	Utility

	These are general utility functions which don't fit into any other category.
	If they serve as general data access they should be Pure and have a name starting with *Get*.
	##################################################
	*/

	/// Return the bounding box for all of the vertices in the mesh.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "size limits bounds min max"))
		FBox GetBoundingBox();

	/// Get a brief description of this geometry in the form *"4 sections, 1000 vertices, 500 triangles"*
	///
	/// This is mainly for debug purposes and making sure things have not broken.
	///
	/// \return A text summary
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "info string verts points tris polys faces sections mesh"))
		FString GetSummary() const;

	/// Return the number of total triangles in the geometry.
	///
	/// This is the combined sum of the triangles in each of the sections which make up this mesh.
	///
	/// \return The total triangle count
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (Keywords = "tris polys polygons faces"))
		int32 TotalTriangleCount() const;

	/// Return the number of total vertices in the geometry.
	///
	/// This is the combined sum of the vertices in each of the sections which make up this mesh.
	///
	/// \return The total vertex count
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta=(Keywords="verts points"))
		int32 TotalVertexCount() const;
};
