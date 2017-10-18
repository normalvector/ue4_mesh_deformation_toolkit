// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "Utility.h"

bool Utility::HaveTwoSelectionSetsOfSameSize(
	USelectionSet *SelectionA,
	USelectionSet *SelectionB,
	FString NodeNameForWarning)
{
	if (!SelectionA||!SelectionB)
	{
		UE_LOG(MDTLog, Warning, TEXT("%s: Need two SelectionSets"), *NodeNameForWarning);
		return false;
	}

	const int32 SizeA = SelectionA->Size();
	const int32 SizeB = SelectionB->Size();
	if (SizeA!=SizeB)
	{
		UE_LOG(
			MDTLog, Warning,
			TEXT("%s: SelectionSets are not the same size (%d and %d"),
			*NodeNameForWarning, SizeA, SizeB
		);
		return false;
	}
	return true;
}

bool Utility::HaveThreeSelectionSetsOfSameSize(
	USelectionSet *SelectionA,
	USelectionSet *SelectionB,
	USelectionSet *SelectionC,
	FString NodeNameForWarning)
{

	if (!SelectionA||!SelectionB||!SelectionC)
	{
		UE_LOG(MDTLog, Warning, TEXT("%s: Need three SelectionSets"), *NodeNameForWarning);
		return false;
	}

	const int32 SizeA = SelectionA->Size();
	const int32 SizeB = SelectionB->Size();
	const int32 SizeC = SelectionC->Size();
	if (SizeA!=SizeB||SizeA!=SizeC)
	{
		UE_LOG(
			MDTLog, Warning,
			TEXT("%s: SelectionSets are not the same size (%d, %d and %d"),
			*NodeNameForWarning, SizeA, SizeB, SizeC
		);
		return false;
	}
	return true;
}

FVector Utility::NearestPointOnPlane(FVector Vertex, FVector PointOnPlane, FVector PlaneNormal)
{
	// This is based on:
	//  https://www.gamedev.net/forums/topic/395194-closest-point-on-plane--distance/
	PlaneNormal.Normalize();
	const float DistanceToPlane =
		FVector::PointPlaneDist(Vertex, PointOnPlane, PlaneNormal.GetSafeNormal());
	return (Vertex-(PlaneNormal * DistanceToPlane));
}
