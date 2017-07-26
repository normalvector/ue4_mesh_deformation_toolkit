// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "Kismet/KismetMathLibrary.h"
#include "SelectionSet.h"

USelectionSet * USelectionSet::CreateAndCheckValid(int32 RequiredSize, UObject *OuterObject, FString NodeNameForWarning)
{
	// Create the results at the correct size and zero it.
	USelectionSet *newSelectionSet = NewObject<USelectionSet>(OuterObject);
	if (!newSelectionSet)
	{
		UE_LOG(MDTLog, Error, TEXT("%s: Cannot create new SelectionSet"), *NodeNameForWarning);
		return nullptr;
	}

	// Set size.
	newSelectionSet->CreateSelectionSet(RequiredSize);

	return newSelectionSet;
}

void USelectionSet::CreateSelectionSet(int32 size)
{
	this->Empty();
	Weights.AddZeroed(size);
}

void USelectionSet::Empty()
{
	Weights.Empty();
}

USelectionSet *USelectionSet::RandomizeWeights(FRandomStream &randomStream, float min /*= 0*/, float max /*= 1*/)
{
	for (auto &weight:Weights)
	{
		weight = randomStream.FRandRange(min, max);
	}
	return this;
}

USelectionSet *USelectionSet::SetAllWeights(float weight)
{
	for (auto &weightItr:Weights)
	{
		weightItr = weight;
	}
	return this;
}

int32 USelectionSet::Size() const
{
	return Weights.Num();
}
