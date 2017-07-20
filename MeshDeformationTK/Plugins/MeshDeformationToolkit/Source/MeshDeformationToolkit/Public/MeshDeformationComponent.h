// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "Components/ActorComponent.h"
#include "MeshGeometry.h"
#include "Runtime/Engine/Classes/Curves/CurveFloat.h"
#include "MeshDeformationComponent.generated.h"

/// *ActorComponent* for easy geometry deformation.
///
/// This is the main class for the *Mesh Deformation Component*, and
/// provides an *ActorComponent* which can be attached to a UE4 Actor to
/// give access to the feature.
///
/// This includes the basic functionality to:
/// * Load geometry data and store it in the component
/// * Select vertices
/// * Apply transformations (Much of this is a delegate
///   to *MeshGeometry* and just repeated here so that we
///   can call the function on the component)
/// * Output the result to a *ProceduralMeshComponent*.
///
/// ## Use of SelectionSet to control Weighting
///
/// All of the transformation functions can take an optional *SelectionSet*
/// which allows the strength of the effect to be controlled on a per-vertex
/// basis, with 0=No effect, 1=Full effect, and values in between meaning partial
/// effect.  Values less than <0 or >1 can be provided but the effects are not guaranteed
/// to be sensible.
///
/// Each transformation function tries to handle the SelectionSet as a linear interpolation
/// in a sensible manner, but this can change depending on the nature of the function.
/// For example *Translate* can treat the weighting as a lerp factor between the points original
/// position and the translation target, but *Rotate* needs to apply their blending to the actual
/// angle of rotation.  If in doubt look at the actual implementation for the function in
/// *MeshGeometry*.

/// \see UActorComponent
/// \see MeshGeometry
/// \see SelectionSet
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MESHDEFORMATIONTOOLKIT_API UMeshDeformationComponent: public UActorComponent
{
	GENERATED_BODY()

public:
	/// Sets default values for this component's properties
	UMeshDeformationComponent();

	/// This is the mesh geometry currently stored within the component
	UPROPERTY(BlueprintReadonly, Category=MeshDeformationComponent)
		UMeshGeometry *MeshGeometry=nullptr;

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
	/// This stores the geometryin *meshGeometry* and replaces any existing
	/// geometry.
	///
	/// \param MeshDeformationComponent		This component (Out param, helps with method chaining)
	/// \param StaticMesh					The mesh to copy the geometry from
	/// \param LOD							A StaticMesh can have multiple meshes for different levels of detail, this specifies which LOD we're taking the information fromaram>
	/// \return *True* if we could read the geometry, *False* if not
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="create sm"))
		bool LoadFromStaticMesh(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UStaticMesh *StaticMesh,
			int32 LOD=0
		);

	/*
	##################################################
	Select Vertices

	All of these functions serve to select vertices based on some criteria.  They should all
	have names beginning with *Select*, and return a new USelectionSet.
	##################################################
	*/

	/// Selects all of the vertices at full strength.
	///
	/// /return A *SelectionSet* with full strength
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent)
		USelectionSet *SelectAll() const;

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
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="random fastnoise perlin fractal terrain", AutoCreateRefTerm="Transform"))
		USelectionSet *SelectByNoise(
			FTransform Transform,
			int32 Seed=1337,
			float Frequency=0.01,
			ENoiseInterpolation NoiseInterpolation=ENoiseInterpolation::Quintic,
			ENoiseType NoiseType=ENoiseType::Simplex,
			uint8 FractalOctaves=3,
			float FractalLacunarity=2.0,
			float FractalGain=0.5,
			EFractalType FractalType=EFractalType::FBM,
			ECellularDistanceFunction CellularDistanceFunction=ECellularDistanceFunction::Euclidian
		) const;

	/// Select all of the vertices which go to make up one of the Sections that a mesh
	/// can consist of.  This can be thought of as the same as a Material slot for many
	/// uses.
	///
	/// \param SectionIndex
	UFUNCTION(BlueprintPure, Category=MeshGeometry,
			  meta=(Keywords="material geometry"))
		USelectionSet *SelectBySection(int32 SectionIndex) const;

	/// Select vertices from a texture.
	///
	/// Black in the channel = Unselected, White = Fully selected.  Uses UV0 for texture access as that's
	/// what GetSectionFromStaticMesh makes available to us.
	///
	/// \param Texture2D		The Texture to extract the selection channel from
	/// \param TextureChannel	The channel to use for the selection
	/// \return Return the SelectionSet corresponding to the texture channel
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="image picture rgb uv"))
		USelectionSet *SelectByTexture(
			UTexture2D *Texture2D,
			ETextureChannel TextureChannel=ETextureChannel::Red
		) const;

	/// Selects vertices with a given normal facing
	///
	/// This does a smooth linear selection based on the angle from the specified normal direction.
	/// \param Facing		The facing to select, in world space
	/// \param InnerRadiusInDegrees	The inner radius in degrees, all vertices with a normal within
	///								this deviation from Facing will be selected at full strength.
	/// \param OuterRadiusInDegrees	The outer radius in degrees, all vertices with a normal greater
	///								than this deviation from Facing will not be selected.
	/// \return A *SelectionSet* for the selected vertices
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="normal vector"))
		USelectionSet *SelectFacing(
			FVector Facing=FVector::UpVector,
			float InnerRadiusInDegrees=0,
			float OuterRadiusInDegrees=30.0f
		) const;

	/// Select vertices inside a volume defined by two opposite corner points.
	/// \param CornerA						The first corner to define the volume
	/// \param CornerB						The second corner to define the volume
	///
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="aabb bounds bounding space"))
		USelectionSet *SelectInVolume(FVector CornerA, FVector CornerB) const;

	/// Select vertices linearly between two points.
	///
	/// \param LineStart	The start of the linear gradient where weight=0
	/// \param LineEnd		The end of the linear gradient where weight=1
	/// \param Reverse		Swaps LineStart/LineEnd to allow the linear effect to be reversed
	/// \param LimitToLine	Whether the effect finishes at the end of the line or if weight=1 continues
	/// \return The SelectionSet with all of the vertices selected according to the gradient
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="gradient between"))
		USelectionSet *SelectLinear(
			FVector LineStart,
			FVector LineEnd,
			bool Reverse=false,
			bool LimitToLine=false
		) const;

	/// Selects the vertices near a point in space.
	///
	/// This does a smooth linear radial selection based on distance form the point provided.
	///
	/// \param Center		The center of the selection in local space
	/// \param InnerRadius	The inner radius, all points inside this will be selected at
	///								maximum strength
	/// \param OuterRadius	The outer radius, all points outside this will not be selected
	/// \return A *SelectionSet* for the selected vertices
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="close soft"))
		USelectionSet *SelectNear(
			FVector Center=FVector::ZeroVector,
			float InnerRadius=0,
			float OuterRadius=100
		) const;

	/// Selects vertices near a line segment with the provided start/end points.
	///
	/// This does a smooth linear selection based on the distance from the line points provided.
	///
	/// \param LineStart		The position of the start of the line in local space
	/// \param LineEnd			The position of the end of the line in local spac3e
	/// \param InnerRadius		The inner radius, all points closer to the line segment than this distance
	///							will be selected at maximum strength
	/// \param OuterRadius		The outer radius, all points further from the line segment than this distance
	///							will not be selected
	/// \param LineIsInfinite	If this is checked then lineStart/lineEnd will treated as two points on an
	///							infinite line instead of being the start/end of a line segment
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="infinite"))
		USelectionSet *SelectNearLine(
			FVector LineStart,
			FVector LineEnd,
			float InnerRadius=0,
			float OuterRadius=100,
			bool LineIsInfinite=false
		) const;

	/// Selects the vertices near a Spline, allowing curves to easily guide deformation.
	///
	/// This does a smooth linear radial selection based on distance from the spline provided.
	///
	/// \param Spline		The spline to be used for the selection
	/// \param InnerRadius	The inner radius, all points closer to the spline than this distance
	///						will be selected at maximum strength.
	/// \param OuterRadius	The outer radius, all points further from the spline than this distance
	///						will not be selected
	UFUNCTION(BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="curve"))
		USelectionSet *SelectNearSpline(
			USplineComponent *Spline,
			float InnerRadius=0,
			float OuterRadius=100
		) const;

	/*
	##################################################
	Transform Vertices

	All of these functions serve to manipulate the underlying geometry.

	They should all have the MDC they were called on as an output pin for chaining
	##################################################
	*/

	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(AutoCreateRefTerm="IgnoredActors", WorldContext="WorldContextObject", Keywords="drop drape cloth collision soft trace")
	)
		void Conform(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UObject* WorldContextObject,
			FTransform Transform,
			TArray <AActor *> IgnoredActors,
			FVector Projection=FVector(0, 0, -100),
			float HeightAdjust=0,
			bool TraceComplex=true,
			ECollisionChannel CollisionChannel=ECC_WorldStatic,
			USelectionSet *Selection=nullptr
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
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="curve bend"))
		void FitToSpline(
			UMeshDeformationComponent *&MeshDeformationComponent,
			USplineComponent *SplineComponent,
			float StartPosition=0.0f,
			float EndPosition=1.0f,
			float MeshScale=1.0f,
			UCurveFloat *SplineProfileCurve=nullptr,
			UCurveFloat *SectionProfileCurve=nullptr,
			USelectionSet *Selection=nullptr
		);

	/// Moves vertices a specified offset along their own normals
	///
	/// \param MeshDeformationComponent			This component
	/// \param Offset							The distance to offset
	/// \param Selection						The SelectionSet, with the offset being scaled for
	///											each vertex
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="normal"))
		void Inflate(
			UMeshDeformationComponent *&MeshDeformationComponent,
			float Offset=0.0f,
			USelectionSet *Selection=nullptr
		);

	/// Adds random jitter to the position of the points.
	///
	///  The jitter will be a vector randomly selected
	///  (with [continuous uniform distribution]() between *Min* and *Max*, and will
	///  be scaled by each vertex's selection weights if they're provided.
	///
	/// \param MeshDeformationComponent		This component (Out param, helps with method chaining)
	/// \param RandomStream					The random stream to source numbers from
	/// \param Min							The minimum jittered offset
	/// \param Max							The maximum jittered offset
	/// \param Selection					The selection weights, if not specified
	///										then all points will be jittered at
	///										maximum strength
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="random position"))
		void Jitter(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FRandomStream &RandomStream,
			FVector Min,
			FVector Max,
			USelectionSet *Selection
		);

	/// Does a linear interpolate with the geometry stored in another MeshDeformationComponent.
	///
	/// The lerp is just applied in local space so may not be perfect with a lot of models.  This will only apply
	/// to the vertex positions and so while it will handle different topologies it will keep the triangle data
	/// from this rather than do anything clever.
	///
	/// \todo Allow this to work in either local or world space.
	///
	/// \param MeshDeformationComponent			This component
	/// \param TargetMeshDeformationComponent	The component with geometry to blend with
	/// \param Alpha							The alpha of the blend, 0=Return this Mesh
	/// \param Selection						The SelectionSet which controls the blend between the two MeshGeometry items
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="blend linear interpolate alpha"))
		void Lerp(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UMeshDeformationComponent *TargetMeshDeformationComponent,
			float Alpha=0.0,
			USelectionSet *Selection=nullptr
		);

	/// Does a linear interpolate pulling/pushing all vertices relative to the
	/// vector provided.
	UFUNCTION(BlueprintCallable, Category = MeshDeformationComponent,
			  meta = (Keywords = "blend linear interpolate alpha pull push"))
		void LerpVector(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FVector Position, float Alpha = 0.0, USelectionSet *Selection = nullptr);

	/// Rotates the vertices of the mesh a specified amount round the specified position.
	///
	/// If a SelectionSet is provided then the actual rotator will be scaled accordingly allowing
	/// whorls and similar to be easily created.
	///
	/// \param MeshDeformationComponent			This component
	/// \param Rotation							The rotation to apply
	/// \param CenterOfRotation					The center of rotation in local space
	/// \param Selection						The selection weights, if not specified
	///											then all points will be rotated by the full rotation
	///											specified
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent)
		void Rotate(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FRotator Rotation=FRotator::ZeroRotator,
			FVector CenterOfRotation=FVector::ZeroVector,
			USelectionSet *Selection=nullptr
		);

	/// Rotate vertices about an arbitrary axis
	///
	/// This allows more freedom than the standard 'Rotate around X, Y, and Z' and is more flexible
	/// than the standard approach even if it is less intuitive.
	///
	/// \param MeshDeformationComponent		This component
	/// \param CenterOfRotation				The center of the rotation operation in local space
	/// \param Axis							The axis to rotate about
	/// \param AngleInDegrees				The angle to rotate the vertices about
	/// \param Selection					The SelectionSet which controls the amount of rotation
	///										applied to each vertex.
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="twist screw"))
		void RotateAroundAxis(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FVector CenterOfRotation=FVector::ZeroVector,
			FVector Axis=FVector::UpVector,
			float AngleInDegrees=0.0f,
			USelectionSet *Selection=nullptr
		);

	/// Scale the selected points on a per-axis basis about a specified center
	///
	/// \param MeshDeformationComponent			This component
	/// \param Scale3d							The per-axis scaling
	/// \param CenterOfScale					The center of the scaling operation in local space
	/// \param Selection						The selection weights, if not specified then all
	///											vertices will be scaled fully by the specified scale
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="size"))
		void Scale(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FVector Scale3d=FVector(1, 1, 1),
			FVector CenterOfScale=FVector::ZeroVector,
			USelectionSet *Selection=nullptr
		);

	/// Scale an object along an arbitrary axis
	///
	/// This allows objects to be scaled along any axis, not just the normal XYZ, and so is more
	/// flexible than the standard approach, even thought it is less intuitive.
	///
	/// \param MeshDeformationComponent			This component
	/// \param CenterOfScale					The center of the scale operation, in local space
	/// \param Axis								The axis to scale along
	/// \param Scale							The ratio to scale by
	/// \param Selection						The SelectionSet which controls the weighting of the
	///											scale for each vertex.  If not provided then the scale
	///											will apply at full strength to all vertices.
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="size"))
		void ScaleAlongAxis(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FVector CenterOfScale=FVector::ZeroVector,
			FVector Axis=FVector::UpVector,
			float Scale=1.0f,
			USelectionSet *Selection=nullptr
		);

	/// Morph a mesh into a sphere by moving points along their normal
	///
	/// \param MeshDeformationComponent		This component
	/// \param SphereRadius					The radius of the sphere to morph to
	/// \param FilterStrength				The strength of the effect, 0=No effect, 1=Full effect.
	///	\param SphereCenter					The center of the sphere
	/// \param Selection					The SelectionSet, if specified this will be multiplied
	///										by FilterStrength to allow each vertex's morph to be
	///										individually controlled.
	/// \todo Should group the sphere parameters together
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="ball"))
		void Spherize(
			UMeshDeformationComponent *&MeshDeformationComponent,
			float SphereRadius=100.0f,
			float FilterStrength=1.0f,
			FVector SphereCenter=FVector::ZeroVector,
			USelectionSet *Selection=nullptr
		);

	/// Applies Scale/Rotate/Translate as a single operation using a combined transform.
	///
	/// The order of the operations will be Scale/Rotate/Translate as documented
	/// [here](https://docs.unrealengine.com/latest/INT/API/Runtime/Core/Math/FTransform/index.html).
	///
	/// \param MeshDeformationComponent		This component
	/// \param Transform					The transformation to apply
	/// \param CenterOfTransform			The center of the transformation, in local space
	/// \param Selection					The SelectionSet, if not specified then all vertices
	///										will be transformed at full strength
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="move scale size rotate"))
		void Transform(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FTransform Transform,
			FVector CenterOfTransform=FVector::ZeroVector,
			USelectionSet *Selection=nullptr
		);

	/// Move all selected by the provided delta vector.
	///
	///  If a *SelectionSet* is provided the delta will be weighted according to the vertex's
	///   selection weight.
	///
	/// \param MeshDeformationComponent			This component
	/// \param Delta							The translation delta in local space
	/// \param Selection						The selection weights, if not specified
	///											then all points will be moved by the
	///											full delta translation
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="move delta"))
		void Translate(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FVector Delta,
			USelectionSet *Selection
		);

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
	/// \param MeshDeformationComponent		This component (Out param, helps with method chaining)
	/// \param ProceduralMeshComponent		The target *ProceduralMeshComponent
	/// \param CreateCollision				Whether to create a collision shape for it
	/// \return *True* if the update was successful, *False* if not
	UFUNCTION(BlueprintCallable, Category=MeshDeformationComponent,
			  meta=(Keywords="pmc output write"))
		bool SaveToProceduralMeshComponent(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UProceduralMeshComponent *ProceduralMeshComponent,
			bool CreateCollision
		);

	UFUNCTION(BlueprintPure, Category = MeshDeformationComponent)
		bool HasGeometry();

	/*
	##################################################
	Utility

	These are general utility functions which don't fit into any other category.
	If they serve as general data access they should be Pure and have a name starting with *Get*.
	##################################################
	*/

	/// Return the bounding box for all of the vertices in the mesh.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=MeshDeformationComponent,
			  meta=(Keywords="size limits bounds min max"))
		FBox GetBoundingBox() const;

	/// Get a brief description of this geometry in the form *"4 sections, 1000 vertices, 500 triangles"*
	///
	/// This is mainly for debug purposes and making sure things have not broken.
	///
	/// \return A text summary
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=MeshGeometry,
			  meta=(Keywords="info string verts points tris polys faces sections mesh"))
		FString GetSummary() const;

	/// Return the number of total triangles in the geometry.
	///
	/// This is the combined sum of the triangles in each of the sections which make up this mesh.
	///
	/// \return The total triangle count
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=MeshGeometry,
			  meta=(Keywords="tris polys polygons faces"))
		int32 GetTotalTriangleCount() const;

	/// Return the number of total vertices in the geometry.
	///
	/// This is the combined sum of the vertices in each of the sections which make up this mesh.
	///
	/// \return The total vertex count
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=MeshGeometry,
			  meta=(Keywords="verts points"))
		int32 GetTotalVertexCount() const;
};
