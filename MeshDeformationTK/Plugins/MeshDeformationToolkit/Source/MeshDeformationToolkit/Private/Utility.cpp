// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "Utility.h"

FVector Utility::NearestPointOnPlane(FVector Vertex, FVector PointOnPlane, FVector PlaneNormal)
{
	// This is based on:
	//  https://www.gamedev.net/forums/topic/395194-closest-point-on-plane--distance/
	PlaneNormal.Normalize();
	const float DistanceToPlane =
		FVector::PointPlaneDist(Vertex, PointOnPlane, PlaneNormal.GetSafeNormal());
	return (Vertex-(PlaneNormal * DistanceToPlane));
}
