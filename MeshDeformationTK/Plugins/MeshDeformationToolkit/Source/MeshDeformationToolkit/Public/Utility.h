// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "UObject/NoExportTypes.h"
// #include "Utility.generated.h"

class MESHDEFORMATIONTOOLKIT_API Utility
{
public:
	/// Given a plane this returns the nearest point on it to the vertex provided
	static FVector NearestPointOnPlane(FVector Vertex, FVector PointOnPlane, FVector PlaneNormal);
};