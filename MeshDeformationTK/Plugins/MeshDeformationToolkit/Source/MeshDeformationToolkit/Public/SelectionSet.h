// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "UObject/NoExportTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "SelectionSet.generated.h"

/// This stores a set of weightings for a selection set.
///
/// The initial use for this is to provide the vertex weightins for *MeshGeometry*, but
/// it should be possible to use this for other selection weightings in the future.
///
/// The *SelectionSetBPLibrary* contains a lot of helper functions for this which allow
/// selection sets to be modified, as such these methods are not exposed to Blueprint as
/// having versions which change values and those which return a new SelectionSet would
/// just be confusing.
///
/// \todo Add a Type enum to allow SelectionSets to be used for more than just vertices.
/// \todo Add a method to check the type/weight count so that we can check if a SelectionSet
///       can be used

UCLASS(BlueprintType)
class MESHDEFORMATIONTOOLKIT_API USelectionSet: public UObject
{
	GENERATED_BODY()

public:
	/// The weights this set contains.
	UPROPERTY(BlueprintReadWrite, Category=SelectionSet)
		TArray<float> weights;

	/// Creates a selection set of the size provided with zero weights.
	/// \param size			The number of items in the selection set
	void CreateSelectionSet(int32 size);

	/// Empties the set, setting the size to zero.
	void Empty();

	/// Sets the weight of all values in the selection set to the weight provided.
	///
	/// This will preserve the number of elements in the set, only the values will change.
	///
	/// \param weight		The weight to assign to all values
	USelectionSet *SetAllWeights(float weight);

	/// Randomize the weights of the selection set
	///
	/// This will preserve the number of elements in the set, only the values will change.
	///
	/// \param randomStream			The RandomStream to use for the source
	/// \param minWeight			The minimum value of the random weightings
	/// \param maxWeight			The maximum value of the random weightings
	USelectionSet *RandomizeWeights(FRandomStream randomStream, float minWeight=0, float maxWeight=1);

	/// Return the number of weights in the selection set.
	UFUNCTION(BlueprintPure, meta=(Keywords="length count maximum num"))
		int32 Size() const;
};
