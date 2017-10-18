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
UCLASS(
	ClassGroup = (Custom),
	meta = (
		BlueprintSpawnableComponent,
		ToolTip = "This is the main component for the Mesh Deformation Toolkit.  This allows geometry to be loaded, deformed, and saved"
		)
)
class MESHDEFORMATIONTOOLKIT_API UMeshDeformationComponent: public UActorComponent
{
	GENERATED_BODY()

public:
	/// Sets default values for this component's properties
	UMeshDeformationComponent();

	/// This is the mesh geometry currently stored within the component
	UPROPERTY(
		BlueprintReadWrite, Category=MeshDeformationComponent,
		meta=(
			ToolTip="The geometry being processed in this component"
			)
	)
		UMeshGeometry *MeshGeometry=nullptr;

	/*
	##################################################
	Load Geometry Data

	All of these functions serve to load the data into the geometry and should have names
	beginning with *Load*.

	If they return anything it should be a boolean indicating success/failure.
	##################################################
	*/

	/// Loads the geometry from another MeshDeformationComponent into this one.
	///
	/// This replaces any geometry currently stored and creates an independent copy which can be altered
	/// without changing the original.
	///
	/// \param MeshDeformationComponent		This component (Out param, helps with method chaining)
	/// \param SourceMeshGeometry			The MeshGeometry that we're copying
	/// \return *True* if we can copy the geometry, *False* if not.
	UFUNCTION(BlueprintCallable, Category = MeshDeformationComponent,
		meta = (Keywords = "create mesh geometry"))
		bool LoadFromMeshDeformationComponent(
			UMeshDeformationComponent *&MeshDeformationComponent, 
			UMeshDeformationComponent *SourceMeshDeformationComponent
		);

	/// Loads the geometry from a MeshGeometry into this one.
	///
	/// This replaces any geometry currently stored and creates an independent copy which can be altered
	/// without changing the original.
	///
	/// \param MeshDeformationComponent		This component (Out param, helps with method chaining)
	/// \param SourceMeshGeometry			The MeshGeometry that we're copying
	/// \return *True* if we can copy the geometry, *False* if not.
	UFUNCTION(BlueprintCallable, Category = MeshDeformationComponent,
		meta = (Keywords = "create mesh geometry"))
		bool LoadFromMeshGeometry(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UMeshGeometry *SourceMeshGeometry
		);

	/// Loads the geometry from a static mesh
	///
	/// This stores the geometryin *meshGeometry* and replaces any existing
	/// geometry.
	///
	/// \param MeshDeformationComponent		This component (Out param, helps with method chaining)
	/// \param StaticMesh					The mesh to copy the geometry from
	/// \param LOD							A StaticMesh can have multiple meshes for different levels of detail, this specifies which LOD we're taking the information fromaram>
	/// \return *True* if we could read the geometry, *False* if not
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		  meta=(
			  ToolTip="Load geometry from a StaticMesh, replacing anything currently stored",
			  Keywords="create sm"
			  )
	)
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
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Selects all of the vertices at full strength"
			)
	)
		USelectionSet *SelectAll() const;

	/// Selects vertices based on a noise function.
	///
	/// This uses the [FastNoise](https://github.com/Auburns/FastNoise) noise library by Jordan Pack and released under the MIT license.
	/// Not all of these settings are used by each noise type, details on their application is in the
	/// [FastNoise docs](https://github.com/Auburns/FastNoise/wiki/Noise-Settings).
	///
	/// \param Transform					The transform to apply to all of the vertices to allow the positioning
	///										of the noise and effects such as correctly joined landscape tiles
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
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select vertices based on a configurable noise function",
			Keywords="random fastnoise perlin fractal terrain",
			AutoCreateRefTerm="Transform"
			)
	)
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
	UFUNCTION(
		BlueprintPure, Category=MeshGeometry,
		meta=(
			ToolTip="Select all of the vertices in one of the Sections making up a mesh",
			Keywords="material geometry"
			)
	)
		USelectionSet *SelectBySection(int32 SectionIndex) const;

	/// Select vertices from a texture.
	///
	/// Black in the channel = Unselected, White = Fully selected.  Uses UV0 for texture access as that's
	/// what GetSectionFromStaticMesh makes available to us.
	///
	/// \param Texture2D		The Texture to extract the selection channel from
	/// \param TextureChannel	The channel to use for the selection
	/// \return Return the SelectionSet corresponding to the texture channel
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select vertices based on a channel from a texture",
			Keywords="image picture rgb uv"
			)
	)
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
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select vertices with a given normal facing",
			Keywords="facing vector"
			)
	)
		USelectionSet *SelectByNormal(
			FVector Facing=FVector::UpVector,
			float InnerRadiusInDegrees=0,
			float OuterRadiusInDegrees=30.0f
		) const;

	/// Select all of the vertices in a a single section by a range.  This is useful
	/// when you know the vertex ordering of an item.
	///
	/// \param RangeStart		The vertex index of the start of the range
	/// \param RangeEnd			The vertex index of the end of the range
	/// \param RangeStep		The stepping between indices in range.  1=Every vertex, 2=Every other
	///							vertex, 3=Every 3 vertices and so on.
	/// \param SectionIndex		The ID of the section we're taking the range from
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = MeshDeformationComponent,
			  meta = (Keywords = "for section"))
		USelectionSet *SelectByVertexRange(
			int32 RangeStart,
			int32 RangeEnd,
			int32 RangeStep = 1,
			int32 SectionIndex = 0
		);


	/// Select vertices inside a volume defined by two opposite corner points.
	/// \param CornerA						The first corner to define the volume
	/// \param CornerB						The second corner to define the volume
	///
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select vertices inside a volume defined by two opposite corner points",
			Keywords="aabb bounds bounding space"
			)
	)
		USelectionSet *SelectInVolume(FVector CornerA, FVector CornerB) const;

	/// Select vertices linearly between two points.
	///
	/// \param LineStart	The start of the linear gradient where weight=0
	/// \param LineEnd		The end of the linear gradient where weight=1
	/// \param bReverse		Swaps LineStart/LineEnd to allow the linear effect to be reversed
	/// \param bLimitToLine	Whether the effect finishes at the end of the line or if weight=1 continues
	/// \return The SelectionSet with all of the vertices selected according to the gradient
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select vertices linearly between two points",
			Keywords="gradient between"
			)
	)
		USelectionSet *SelectLinear(
			FVector LineStart,
			FVector LineEnd,
			bool bReverse=false,
			bool bLimitToLine=false
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
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select the vertices near a point in space",
			Keywords="close soft"
			)
	)
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
	/// \param bLineIsInfinite	If this is checked then lineStart/lineEnd will treated as two points on an
	///							infinite line instead of being the start/end of a line segment
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select vertices near a line with the provided start/end points",
			Keywords="infinite"
			)
	)
		USelectionSet *SelectNearLine(
			FVector LineStart,
			FVector LineEnd,
			float InnerRadius=0,
			float OuterRadius=100,
			bool bLineIsInfinite=false
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
	UFUNCTION(
		BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Select the vertices near a SplineCommponent",
			Keywords="curve"
			)
	)
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

	/// Conforms the mesh against collision geometry by projecting it along a specified vector.
	///
	/// This is a very expensive operation with a lot of vector math operations and a LineTrace
	/// for each vertex in the source mesh.
	///
	/// \param MeshDeformationComponent	This component
	/// \param WorldContextObject		The object to get the world object from, this is set automatically
	///									in the MeshDeformerComponent Blueprint so the end-user doesn't need
	///									to do it.
	///	\param Transform				The base transformation of the object.  It's important this is
	///									specified as it's needed to position the line traces.
	/// \param IgnoredActors			An optional array of actors which will be ignored by the line trace.
	/// \param Projection				The projection to conform.  Each vertex will be moved along this
	///									vector until it hits something.
	/// \param HeightAdjust				An offset which will be applied to each vertex which collides with
	///									an object.  If this is +ve then the object will be move up and away
	///									from the collision, if this is -ve then the object will be dropped
	///									down through the collided object.
	/// \param bTraceComplex				Whether to use complex polygon-based collision rather than the simpler
	///									collision mesh.
	/// \param CollisionChannel			The collision channel to use for the line-trace operations.
	/// \param Selection				An optional SelectionSet to control the effect on a per-vertex
	///									basis.  If provided this will change the strength of the Projection.
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta = (
			ToolTip="Conforms the mesh against collision geometry by projecting",
			Keywords = "drop drape cloth collision soft trace",
			AutoCreateRefTerm="IgnoredActors",
			WorldContext="WorldContextObject"
			)

	)
		void Conform(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UObject* WorldContextObject,
			FTransform Transform,
			TArray <AActor *> IgnoredActors,
			FVector Projection=FVector(0, 0, -100),
			float HeightAdjust=0,
			bool bTraceComplex=true,
			ECollisionChannel CollisionChannel=ECC_WorldStatic,
			USelectionSet *Selection=nullptr
		);

	/// Conforms the mesh against collision geometry by projecting downwards (-Z).
	///
	/// This is a very expensive operation with a lot of vector math operations and a LineTrace
	/// for each vertex in the source mesh.
	///
	/// \param MeshDeformationComponent	This component
	/// \param WorldContextObject		The object to get the world object from, this is set automatically
	///									in the MeshDeformerComponent Blueprint so the end-user doesn't need\
	///									to do it.
	///	\param Transform				The base transformation of the object.  It's important this is
	///									specified as it's needed to position the line traces.
	/// \param IgnoredActors			An optional array of actors which will be ignored by the line trace.
	/// \param ProjectionLength			The distance (in UU) to drop the geometry by until it hits another
	///									object.
	/// \param HeightAdjust				An offset which will be applied to each vertex which collides with
	///									an object.  If this is +ve then the object will be move up and away
	///									from the collision, if this is -ve then the object will be dropped
	///									down through the collided object.
	/// \param bTraceComplex			Whether to use complex polygon-based collision rather than the simpler
	///									collision mesh.
	/// \param CollisionChannel			The collision channel to use for the line-trace operations.
	/// \param Selection				An optional SelectionSet to control the effect on a per-vertex
	///									basis.  If provided this will change the strength of the Projection.
	UFUNCTION(
		BlueprintCallable, Category = MeshDeformationComponent,
		meta = (
			ToolTip="Conforms the mesh against collision geometry by projecting downwards (-Z)",
			Keywords = "drop drape cloth collision soft trace",
			AutoCreateRefTerm = "IgnoredActors",
			WorldContext = "WorldContextObject"
			)
	)
		void ConformDown(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UObject* WorldContextObject,
			FTransform Transform,
			TArray <AActor *> IgnoredActors,
			float ProjectionLength = 100,
			float HeightAdjust = 0,
			bool bTraceComplex = true,
			ECollisionChannel CollisionChannel = ECC_WorldStatic,
			USelectionSet *Selection = nullptr
		);

	/// Deform the mesh along a spline with more control than UE4's own SplineMeshComponent.
	///
	/// \param MeshDeformationComponent		This component
	/// \param SplineComponent				The spline controlling the shape of the deformation
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
	/// \param SplineProfileCurve			This optional curve will be applied along the entire length of the
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
	UFUNCTION(
		BlueprintCallable,
		Category=MeshDeformationComponent,
		meta=(
			ToolTip="Bend the mesh to follow a SplineComponent",
			Keywords="curve bend"
			)
	)
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

	/// Flip the surface normals.
	///
	/// \param MeshDeformationComponent			This component
	/// \param Selection						The SelectionSet to be applied- this will
	///											be used as a true/false filter based on
	///											whether each weighting is >=0.5.
	UFUNCTION(
		BlueprintCallable, Category = MeshDeformationComponent,
		meta=(
			ToolTip="Flip the surface normals"
			)
	)
		void FlipNormals(
			UMeshDeformationComponent *&MeshDeformationComponent, USelectionSet *Selection = nullptr);

	/// Flip the texture map channel in U (horizontal), V(vertical), both, or neither.
	///
	/// \param MeshDeformationComponent			This component
	/// \param bFlipU							Flip the texture horizontally
	/// \param bFlipV							Flip the texture vertically
	/// \param Selection						The SelectionSet to be applied- this will
	///											be used as a true/false filter based on
	///											whether each weighting is >=0.5.
	UFUNCTION(
		BlueprintCallable, Category = MeshDeformationComponent,
		meta=(
			ToolTip="Flip the texture map channel in U (horizontal), V(vertical), both, or neither"
			)
	)
		void FlipTextureUV(
			UMeshDeformationComponent *&MeshDeformationComponent,
			bool bFlipU = false, bool bFlipV = false,
			USelectionSet *Selection = nullptr);

	/// Moves vertices a specified offset along their own normals
	///
	/// \param MeshDeformationComponent			This component
	/// \param Offset							The distance to offset
	/// \param Selection						The SelectionSet, with the offset being scaled for
	///											each vertex
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta = (
			ToolTip = "Move vertices a specified offset along their own normals",
			Keywords = "normal"
			)
	)
		void Inflate(
			UMeshDeformationComponent *&MeshDeformationComponent,
			float Offset=0.0f,
			USelectionSet *Selection=nullptr
		);

	/// Adds random jitter to the position of the vertices.
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
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Add random jitter to the position of the vertices",
			Keywords="random position"
			)
	)
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
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(
			ToolTip="A linear interpolation against the geometry stored in another MeshDeformationComponent",
			Keywords="blend linear interpolate alpha"
			)
	)
		void Lerp(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UMeshDeformationComponent *TargetMeshDeformationComponent,
			float Alpha=0.0,
			USelectionSet *Selection=nullptr
		);

	/// Does a linear interpolate pulling/pushing all vertices relative to the
	/// vector provided.
	UFUNCTION(
		BlueprintCallable, Category = MeshDeformationComponent,
		meta = (
			ToolTip="A linear interpolate pulling/pushing vertices relative to the position provided",
			Keywords = "blend linear interpolate alpha pull push"
			)
	)
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
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta = (
			ToolTip="Rotate the vertices round a specified center"
			)
	)
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
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Rotate vertices around an arbitrary axis",
			Keywords="twist screw"
			)
	)
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
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Scale the mesh about a specified center",
			Keywords="size"
			)
	)
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
	UFUNCTION(
		BlueprintCallable,
		Category=MeshDeformationComponent,
		meta=(
			ToolTip="Scale along an arbitrary axis",
			Keywords="size"
			)
	)
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
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Morph geometry into a sphere by moving points along their normals",
			Keywords="ball"
			)
	)
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
	UFUNCTION(
		BlueprintCallable, Category = MeshDeformationComponent,
		meta = (
		ToolTip = "Applies Scale/Rotate/Translate as a single operation using a Transform",
		Keywords = "move scale size rotate"
	)
	)
		void Transform(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FTransform Transform,
			FVector CenterOfTransform = FVector::ZeroVector,
			USelectionSet *Selection = nullptr
		);

	/// Applies a translate to the UV coordinates to allow control of texture mapping.
	///
	/// The order of the operations will be Scale/Rotate/Translate as documented
	/// [here](https://docs.unrealengine.com/latest/INT/API/Runtime/Core/Math/FTransform/index.html).
	///
	/// \param MeshDeformationComponent		This component
	/// \param Transform					The transformation to apply
	/// \param CenterOfTransform			The center of the transformation, in local space
	/// \param Selection					The SelectionSet, if not specified then all vertices
	///										will be transformed at full strength
		UFUNCTION(
			BlueprintCallable, Category = MeshDeformationComponent,
			meta = (
			DisplayName = "Transform UV",
			Keywords = "texture coordinates"
		)
		)
		void TransformUV(
			UMeshDeformationComponent *&MeshDeformationComponent,
			FTransform Transform,
			FVector2D CenterOfTransform = FVector2D::ZeroVector,
			USelectionSet *Selection = nullptr
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
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Move all vertices by the provided vector",
			Keywords="move delta"
			)
	)
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
	/// \param bCreateCollision				Whether to create a collision shape for it
	/// \return *True* if the update was successful, *False* if not
	UFUNCTION(
		BlueprintCallable, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Save the current geometry to a ProceduralMeshComponent, replacing any existing geometry",
			Keywords="pmc output write"
			)
	)
		bool SaveToProceduralMeshComponent(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UProceduralMeshComponent *ProceduralMeshComponent,
			bool bCreateCollision
		);

	/// Save the current geometry to a *StaticMesh*, replacing the geometry in the
	/// mesh provided.  This will only work inside the Editor, this can't be done
	/// in-game.
	/// 
	/// \param MeshDeformationComponent		This component (Out param, helps with method chaining)
	/// \param StaticMesh					The mesh to replace
	/// \param ProceduralMeshComponent		A ProceduralMeshComponent which will be used to build
	///										all of the data structures that StaticMesh needs.
	/// \param Materials					An array of materials which will be applied to the
	///										built mesh.
	/// \return *True* if the update was successful, *False* if not
	UFUNCTION(
		BlueprintCallable, Category = MeshDeformationComponent,
		meta = (
			ToolTip="Save the current geometry to a StaticMesh, replacing any existing content [EDITOR ONLY]",
			Keywords = "sm output write"
			)
	)
		bool SaveToStaticMesh(
			UMeshDeformationComponent *&MeshDeformationComponent,
			UStaticMesh *StaticMesh,
			UProceduralMeshComponent *ProceduralMeshComponent,
			TArray<UMaterialInstance *> Materials);

	/*
	##################################################
	Utility

	These are general utility functions which don't fit into any other category.
	If they serve as general data access they should be Pure and have a name starting with *Get*.
	##################################################
	*/

	/// Return a boolean indicating if we have geometry loaded.
	UFUNCTION(
		BlueprintPure, Category = MeshDeformationComponent,
		meta = (
			ToolTip="Do we have geometry loaded?"
			)
	)
		bool HasGeometry();

	/// Return the bounding box for all of the vertices in the mesh.
	UFUNCTION(
		BlueprintCallable, BlueprintPure, Category=MeshDeformationComponent,
		meta=(
			ToolTip="Get the bounding box for the mesh",
			Keywords="size limits bounds min max"
			)
	)
		FBox GetBoundingBox() const;

	/// Get a brief description of this geometry in the form *"4 sections, 1000 vertices, 500 triangles"*
	///
	/// This is mainly for debug purposes and making sure things have not broken.
	///
	/// \return A text summary
	UFUNCTION(
		BlueprintCallable, BlueprintPure, Category = MeshGeometry,
		meta = (
			ToolTip = "Get a brief description of the mesh, eg. '4 sections, 1000 vertices, 500 triangles'",
			Keywords="info string verts points tris polys faces sections mesh"
			)
	)
		FString GetSummary() const;

	/// Return the number of total triangles in the geometry.
	///
	/// This is the combined sum of the triangles in each of the sections which make up this mesh.
	///
	/// \return The total triangle count
	UFUNCTION(
		BlueprintCallable, BlueprintPure, Category=MeshGeometry,
		meta=(
			ToolTip="Get the number of triangles in the mesh",
			Keywords="tris polys polygons faces"
			)
	)
		int32 GetTotalTriangleCount() const;

	/// Return the number of total vertices in the geometry.
	///
	/// This is the combined sum of the vertices in each of the sections which make up this mesh.
	///
	/// \return The total vertex count
	UFUNCTION(
		BlueprintCallable, BlueprintPure, Category=MeshGeometry,
		meta=(
			ToolTip="Get the number of vertices in the mesh",
			Keywords="verts points"
			)
	)
		int32 GetTotalVertexCount() const;
};
