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
		TArray<float> Weights;

	/// Create an empty selection set with the provided outer item and
	/// also log errors if there are any problems.
	///
	/// \param RequiredSize			The size of the SelectionSet
	/// \param OuterObject			The GameObject which will own this SelectionSet
	/// \param NodeNameForWarning	The name of the node to display in case of error
	static USelectionSet *CreateAndCheckValid(
		int32 RequiredSize, UObject *OuterObject, FString NodeNameForWarning);

	/// Creates a selection set of the size provided with zero weights.
	/// \param Size			The number of items in the selection set
	void CreateSelectionSet(int32 Size);

	/// Empties the set, setting the size to zero.
	void Empty();

	/// Sets the weight of all values in the selection set to the weight provided.
	///
	/// This will preserve the number of elements in the set, only the values will change.
	///
	/// \param Weight		The weight to assign to all values
	USelectionSet *SetAllWeights(float Weight);

	/// Randomize the weights of the selection set
	///
	/// This will preserve the number of elements in the set, only the values will change.
	///
	/// \param RandomStream			The RandomStream to use for the source
	/// \param MinWeight			The minimum value of the random weightings
	/// \param MaxWeight			The maximum value of the random weightings
	USelectionSet *RandomizeWeights(FRandomStream &RandomStream, float MinWeight=0, float MaxWeight=1);

	/// Return the number of weights in the selection set.
	UFUNCTION(BlueprintPure, meta=(Keywords="length count maximum num"))
		int32 Size() const;
};
