// (c)2017 Paul Golds, released under MIT License.

// This allows all of the FastNoise functionality to be exposed to Blueprint

#pragma once

/// A copy of FastNoise's Interp enum made available to Blueprint.
UENUM(BlueprintType)
enum class ENoiseInterpolation: uint8
{
	Linear		UMETA(DisplayName="Linear"),
	Hermite		UMETA(DisplayName="Hermite"),
	Quintic		UMETA(DisplayName="Quintic")
};

/// A copy of FastNoise's NoiseType enum made available to Blueprint.
UENUM(BlueprintType)
enum class ENoiseType: uint8
{
	Value			UMETA(DisplayName="Value"),
	ValueFractal	UMETA(DisplayName="Value Fractal"),
	Perlin			UMETA(DisplayName="Perlin"),
	PerlinFractal	UMETA(DisplayName="Perlin Fractal"),
	Simplex			UMETA(DisplayName="Simplex"),
	SimplexFractal	UMETA(DisplayName="Simplex Fractal"),
	Cellular		UMETA(DisplayName="Cellular"),
	WhiteNoise		UMETA(DisplayName="White Noise"),
	Cubic			UMETA(DisplayName="Cubic"),
	CubicFractal	UMETA(DisplayName="Cubic Fractal")
};

/// A copy of FastNoise's FractalType enum made available to Blueprint.
UENUM(BlueprintType)
enum class EFractalType: uint8
{
	FBM					UMETA(DisplayName="FBM"),
	Billow				UMETA(DisplayName="Billow"),
	RigidMulti			UMETA(DisplayName="Rigid Multi")
};

/// A copy of FastNoise's CellularDistanceFunction enum made available to Blueprint.
UENUM(BlueprintType)
enum class ECellularDistanceFunction: uint8
{
	Euclidian			UMETA(DisplayName="Euclidian"),
	Manhattan			UMETA(DisplayName="Manhattan"),
	Natural				UMETA(DisplayName="Natural")
};

/// A copy of FastNoise's CellularReturnType enum made available to Blueprint.
UENUM(BlueprintType)
enum class ECellularReturnType: uint8
{
	CellValue			UMETA(DisplayName="Cell Value"),
	/// \todo NoiseLookup is complex and so should be removed - but needs to be here to allow casting
	NoiseLookup			UMETA(DisplayName="Noise Lookup"),
	Distance			UMETA(DisplayName="Distance"),
	Distance2			UMETA(DisplayName="Distance 2"),
	Distance2Add		UMETA(DisplayName="Distance 2 Add"),
	Distance2Sub		UMETA(DisplayName="Distance 2 Sub"),
	Distance2Mul		UMETA(DisplayName="Distance 2 Mul"),
	Distance2Div		UMETA(DisplayName="Distance 2 Div")
};

/// Allow selection of which channel should be used when accessing a texture.
UENUM(BlueprintType)
enum class ETextureChannel: uint8
{
	Red					UMETA(DisplayName="Red"),
	Green				UMETA(DisplayName="Green"),
	Blue				UMETA(DisplayName="Blue"),
	Alpha				UMETA(DisplayName="Alpha")
};
