// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "Kismet/KismetMathLibrary.h"
#include "SelectionSet.h"

USelectionSet * USelectionSet::CreateAndCheckValid(int32 RequiredSize, UObject *OuterObject, FString NodeNameForWarning)
{
	// Create the results at the correct size and zero it.
	USelectionSet *NewSelectionSet = NewObject<USelectionSet>(OuterObject);
	if (!NewSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("%s: Cannot create new SelectionSet"), *NodeNameForWarning);
		return nullptr;
	}

	// Set size.
	NewSelectionSet->CreateSelectionSet(RequiredSize);

	return NewSelectionSet;
}

void USelectionSet::CreateSelectionSet(int32 Size)
{
	this->Empty();
	Weights.AddZeroed(Size);
}

void USelectionSet::Empty()
{
	Weights.Empty();
}

USelectionSet *USelectionSet::RandomizeWeights(FRandomStream &RandomStream, float Min /*= 0*/, float Max /*= 1*/)
{
	for (auto &Weight:Weights)
	{
		Weight = RandomStream.FRandRange(Min, Max);
	}
	return this;
}

USelectionSet *USelectionSet::SetAllWeights(float Value)
{
	for (auto &Weight:Weights)
	{
		Weight = Value;
	}
	return this;
}

int32 USelectionSet::Size() const
{
	return Weights.Num();
}
