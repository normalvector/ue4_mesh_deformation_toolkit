// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "UObject/NoExportTypes.h"
// #include "Utility.generated.h"

/// This is a set of general utility functions used by other parts of the plugin.
class MESHDEFORMATIONTOOLKIT_API Utility
{
public:

	/// A useful utility to convert enumerations to strings- mainly for debug purposes.
	///
	/// This is used as follows:
	///   ETeam Team = ETeam::Alpha;
	///   FString message = TEXT("Our enum value: ") + EnumToString(TEXT("ETeam"), static_cast<uint8>(Team));
	///  This prints: "Our enum value: ETeam::Alpha"
	///
	/// This is based on code from https://forums.unrealengine.com/development-discussion/c-gameplay-programming/32761-uenum-and-getvalueasstring
	///
	/// \param Enum				The name of the enum, TEXT("EMyEnum")
	/// \param EnumValue		The value to convert
	static const FString EnumToString(const TCHAR* Enum, int32 EnumValue);

	/// Given a plane this returns the nearest point on it to the vertex provided
	static FVector NearestPointOnPlane(FVector Vertex, FVector PointOnPlane, FVector PlaneNormal);

	/// Utility function which checks that two SelectionSets are provided, and are
	/// the same size.
	///
	/// \param SelectionA			The first SelectionSet to check
	/// \param SelectionB			The second SelectionSet to check
	/// \param NodeNameForWarning	The class calling us- used for error reporting
	static bool HaveTwoSelectionSetsOfSameSize(
		USelectionSet *SelectionA, USelectionSet *SelectionB,
		FString NodeNameForWarning);

	/// Utility function which checks that two SelectionSets are provided, and are
	/// the same size.
	///
	/// \param SelectionA			The first SelectionSet to check
	/// \param SelectionB			The second SelectionSet to check
	/// \param SelectionC			The third SelectionSet to check
	/// \param NodeNameForWarning	The class calling us- used for error reporting
	static bool HaveThreeSelectionSetsOfSameSize(
		USelectionSet *SelectionA, USelectionSet *SelectionB, USelectionSet *SelectionC,
		FString NodeNameForWarning);
};