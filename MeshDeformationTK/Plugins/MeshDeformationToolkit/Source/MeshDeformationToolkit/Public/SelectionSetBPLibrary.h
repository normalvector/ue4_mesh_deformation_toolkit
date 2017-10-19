// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SelectionSet.h"
#include "Utility.h"L:\GitRepos\ue4_mesh_deformation_toolkit\notes.md
#include "SelectionSetBPLibrary.generated.h"


/// This is a *BlueprintFunctionLibrary* implementing the Blueprint nodes which are
/// used to manipulate *SelectionSet* items, including modifying and combining them.
///
/// These methods are designed to return modified values of SelectionSets rather than
/// change the values provided to them.
UCLASS()
class MESHDEFORMATIONTOOLKIT_API USelectionSetBPLibrary: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/// Add a constant Float to all values of a SelectionSet
	///
	/// \param Value		The SelectionSet to add the constant to (*Value* + Float)
	/// \param Float		The Float to add to the SelectioNSet (Value + *Float*)
	/// \return The result of the SelectionSet + Float
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet + Float",
			CompactNodeTitle = "+",
			ToolTip = "[SelectionSet + Float] Add a constant Float to all values of a SelectionSet",
			Keywords="+ add plus",
			CommutativeAssociativeBinaryOperator="true",
			Category="Math|SelectionSet"
			)
	)
		static USelectionSet *AddFloatToSelectionSet(USelectionSet *Value, float Float=0);

	/// Add two SelectionSets together
	///
	/// \param A			The first SelectionSet to add
	/// \param B			The second SelectionSet to add
	/// \return				The sum of the two SelectionSets
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet + SelectionSet",
			CompactNodeTitle="+",
			ToolTip="[SelectionSet + SelectionSet] Add two SelectionSets together",
			Keywords="+ add plus",
			CommutativeAssociativeBinaryOperator="true",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *AddSelectionSets(USelectionSet *A, USelectionSet *B);

	/// Clamp all values i7n the set to the minimum and maximum provided.
	///
	/// \param Value	The SelectionSet to clamp
	/// \param Min		The minimum value to clamp to
	/// \param Max		The maximum value to clamp to
	/// \return			The clamped SelectionSet.
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Clamp (SelectionSet)",
			CompactNodeTitle="Clamp",
			ToolTip="[Clamp (SelectionSet)] Clamp all values in a SelectionSet to the minimum and maximum provided",
			Category="Math|SelectionSet",
			Keywords="limit mix max range"
		)
	)
		static USelectionSet *Clamp(USelectionSet *Value, float Min=0, float Max=1);

	/// Divides a float by all the values in a SelectionSet
	///
	/// \param Float			The Float to divide [*A*/B]
	/// \param Value			The SelectionSet to divide by [A/*B]
	/// \return				A SelectionSet with the values of Float/Value
	UFUNCTION(
		BlueprintPure,
		meta = (
			DisplayName = "Float / SelectionSet",
			CompactNodeTitle = "/",
			ToolTip="[Float / SelectionSet] Divides a float by all the values in a SelectionSet",
			Keywords = "/ divide division",
			Category = "Math|SelectionSet"
		)
	)
		static USelectionSet *DivideFloatBySelectionSet(float Float = 1, USelectionSet *Value=nullptr);

	/// Divides the values from a SelectionSet by a Float
	///
	/// \param Value			The SelectionSet to divide (*A*/B)
	/// \param Float			The Float to divide by (A/*B*)
	/// \return				A SelectionSet with the values of Value/Float
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet / Float",
			CompactNodeTitle="/",
			ToolTip="[SelectionSet / Float] Divide the values in a SelectionSet by a float",
			Keywords="/ divide division",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *DivideSelectionSetByFloat(USelectionSet *Value, float Float=1);

	/// Divides the values from one SelectionSet by those of another
	///
	/// \param A			The SelectionSet to divide
	/// \param B			The SelectionSet to divide by
	/// \return				A SelectionSet with the values of A/B
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet / SelectionSet",
			CompactNodeTitle="/",
			ToolTip="[SelectionSet / SelectionSet] Divide the weights from one SelectionSet by another",
			Keywords="/ divide division",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *DivideSelectionSets(USelectionSet *A, USelectionSet *B);

	/// Apply an easing function to all values in SelectionSet
	///
	/// \param Value		The SelectionSet to apply the easing function to
	/// \param EaseFunction	The [Easing Function](https://docs.unrealengine.com/latest/INT/API/Runtime/Engine/Kismet/EEasingFunc__Type/index.html) to apply
	/// \param Steps		The number of steps to apply (Only for *Step* easing)
	/// \param BlendExp		Controls the blending of the ease function, only applies for *Ease*, *EaseIn* and *EaseInOut* types
	/// \return				The eased SelectionSet
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Ease (SelectionSet)",
			CompactNodeTitle="Ease",
			ToolTip="[Ease (SelectionSet)] Ease a SelectionSet using a user supplied easing function",
			Category="Math|SelectionSet", Keywords="remap easing smooth in out falloff gradient"
		)
	)
		static USelectionSet *Ease(
			USelectionSet *Value,
			EEasingFunc::Type EaseFunction=EEasingFunc::Linear,
			int32 Steps=2,
			float BlendExp=2.0f
		);

	/// Apply a lerp to blend a SelectionSet against a Float
	///
	/// \param Value	The first SelectionSet to apply the lerp to
	/// \param Float	The second SelectionSet to apply the lerp to
	/// \param Alpha	The blend factor between the two SelectionSets, 0=Original Value, 1=Replaced with Float
	/// \return The lerped SelectionSet
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Lerp (SelectionSet, Float)",
			CompactNodeTitle="Lerp",
			ToolTip="[Lerp (SelectionSet, Float)] Blend a SelectionSet against a float",
			Category="Math|SelectionSet",
			Keywords="blend linear interpolate alpha"
		)
	)
		static USelectionSet *LerpSelectionSetWithFloat(USelectionSet *Value, float Float, float Alpha=0);

	/// Apply a lerp to blend two SelectionSets together based on a given alpha
	///
	/// \param A		The first SelectionSet to apply the lerp to
	/// \param B		The second SelectionSet to apply the lerp to
	/// \param Alpha	The blend factor between the two SelectionSets, 0=A, 1=B
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Lerp(SelectionSet, SelectionSet) with Float",
			CompactNodeTitle = "Lerp",
			ToolTip="[Lerp(SelectionSet, SelectionSet) with Float] Blend two SelectionSets together using a given alpha",
			Category="Math|SelectionSet",
			Keywords="blend linear interpolate alpha"
		)
	)
		static USelectionSet *LerpSelectionSetsWithFloat(USelectionSet *A, USelectionSet *B, float Alpha=0);

	/// Apply a lerp to blend two SelectionSets together based on a third SelectionSet
	///
	/// \param A		The first SelectionSet to apply the lerp to
	/// \param B		The second SelectionSet to apply the lerp to
	/// \param Alpha	The SelectionSet with blend factors between the two
	///					SelectionSets, 0=A, 1=B
	UFUNCTION(
		BlueprintPure,
		meta = (
			DisplayName = "Lerp(SelectionSet, SelectionSet) with SelectionSet",
			CompactNodeTitle = "Lerp",
			ToolTip="[Lerp(SelectionSet, SelectionSet) with SelectionSet] Blend two SelectionSets together with alphas taken from a third SelectionSet",
			Category = "Math|SelectionSet",
			Keywords = "blend linear interpolate alpha"
		)
	)
		static USelectionSet *LerpSelectionSetsWithSelectionSet(
			USelectionSet *A,
			USelectionSet *B,
			USelectionSet *Alpha
		);

	/// Return the maximum of a SelectionSet and a Float
	///
	/// This can be viewed as the 'bottom half' of a clamp, making sure all values of a SelectionSet
	/// are at least equal to the Float provided.
	///
	/// \param Value		The SelectionSet to apply the filter to
	/// \param Float		The Float to compare the SelectionSet values with
	/// \return The result of the maximum of the SelectionSet and the Float
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Max (SelectionSet, Float)",
			CompactNodeTitle="Max",
			ToolTip="[Max (SelectionSet, Float)] Return the maximum of a SelectionSet and a float",
			Category="Math|SelectionSet",
			Keywords="limit"
		)
	)
		static USelectionSet *MaxSelectionSetAgainstFloat(USelectionSet *Value, float Float);

	/// Return the maximum value from two SelectionSets
	///
	/// This can be used to combine two SelectionSets with the 'highest' value for height map manipulation and so on.
	///
	/// \param A			The first SelectionSet to obtain maximum values from
	/// \param B			The second SelectionSet to obtain maximum values from
	/// \return The SelectionSet with the maximum values from A and B
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Max (SelectionSet, SelectionSet)",
			CompactNodeTitle="Max",
			ToolTip="[Max (SelectionSet, SelectionSet)] Return the maximum of two SelectionSets",
			Category="Math|SelectionSet",
			Keywords="limit"
		)
	)
		static USelectionSet *MaxSelectionSets(USelectionSet *A, USelectionSet *B);

	/// Return the minimum of a SelectionSet and a Float
	///
	/// This can be viewed as the 'top half' of a clamp, making sure all values of a SelectionSet
	/// are capped by the Float provided.
	///
	/// \param Value		The SelectionSet to apply the filter to
	/// \param Float		The Float to compare the SelectionSet values with
	/// \return The result of the minimum of the SelectionSet and the Float
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Min (SelectionSet, Float)",
			CompactNodeTitle="Min",
			ToolTip="[Min (SelectionSet, Float)] Return the minimum of a SelectionSet and a float",
			Category="Math|SelectionSet",
			Keywords="limit"
		)
	)
		static USelectionSet *MinSelectionSetAgainstFloat(USelectionSet *Value, float Float);

	/// Return the minimum value from two SelectionSets
	///
	/// This can be used to combine two SelectionSets with the 'lowest' value for height map manipulation and so on.
	///
	/// \param A			The first SelectionSet to obtain minimum values from
	/// \param B			The second SelectionSet to obtain minumum values from
	/// \return The SelectionSet with the minimum values from A and B
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Min (SelectionSet, SelectionSet)",
			CompactNodeTitle="Min",
			ToolTip="[Min (SelectionSet, SelectionSet)] Return the minimum of two SelectionSets",
			Category="Math|SelectionSet",
			Keywords="limit"
		)
	)
		static USelectionSet *MinSelectionSets(USelectionSet *A, USelectionSet *B);

	///  Multiplies the values of a SelectionSet by a Float
	///
	/// \param Value		The SelectionSet to multiply by the float (*Value* \* Float)
	/// \param Float		The Float to multiply the SelectionSet by (Value \* *Float*)
	/// \return The result of Value \* Float
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet * Float",
			CompactNodeTitle="*",
			ToolTip="[SelectionSet * Float] Multiply the values in a SelectionSet by a float",
			Keywords="* multiply times",
			CommutativeAssociativeBinaryOperator="true",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *MultiplySelctionSetByFloat(USelectionSet *Value, float Float=1);

	/// Multiplies the values of two SelectionSets
	///
	/// \param A			The first SelectionSet to multiply (*A*\*B)
	/// \param B			The second SelectionSet to multiply (A\**B*)
	/// \return				The result of the two SelectionSets multiplied by each other
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet * SelectionSet",
			CompactNodeTitle="*",
			ToolTip="[SelectionSet * SelectionSet] Multiplies the values of two SelectionSets",
			Keywords="* multiply times",
			CommutativeAssociativeBinaryOperator="true",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *MultiplySelectionSets(USelectionSet *A, USelectionSet *B);

	/// Returns a SelectionSet with values 1- those of another SelectionSet
	///
	/// If a SelectionSet is normalized to the range 0-1 then this will reverse it.
	///
	/// \param Value		The SelectionSet to apply OneMinus to
	/// \return				The result of 1-Value
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="OneMinus (SelectionSet)",
			CompactNodeTitle="OneMinus",
			ToolTip="[OneMinus (SelectionSet)] Return 1-SelectionSet.  If the SelectionSet is in the 0-1 range this will reverse it",
			Category="Math|SelectionSet",
			Keywords="oneminus minus - negate subtrace take invert reverse")
	)
		static USelectionSet *OneMinus(USelectionSet *Value);

	/// Return a SelectionSet with values based on those of another SelectionSet raised
	/// to a power  (SelectionSet ^ Power)
	///
	///
	/// \param Value		The SelectionSet to raise
	/// \param Exp			The exponent to raise it to
	/// \return				The result of Value^Exp
	UFUNCTION(
		BlueprintPure,
		meta = (
		DisplayName = "Power (SelectionSet, Float)",
		CompactNodeTitle = "Power",
		ToolTip = "[Power (SelectionSet, Float)] Return a SelectionSet raised to the Exp-th power",
		Keywords = "exponont",
		Category = "Math|SelectionSet"
	)
	)
		static USelectionSet *Power(USelectionSet *Value, float Exp);

	/// Randomizes a SelectionSet's values between two limits
	///
	/// \param Value		The source SelectionSet
	/// \param RandomStream	The source for random numbers
	/// \param Min			The minimum limit for the random weights
	/// \param Max			The maximum limit for the random weights
	/// \return				The new random SelectionSet
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Randomize (SelectionSet)",
			CompactNodeTitle="Randomize",
			ToolTip="[Randomize (SelectionSet)] Randomizes a SelectionSet's values between two limits",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *Randomize(USelectionSet *Value, FRandomStream &RandomStream, float Min=0, float Max=1);

	/// Remap the values of a SelectionSet to a CurveFloat
	///
	/// The remap will return the T=0 value of the Curve for Weight=0, and the T=Max value of the Curve for Weight=1,
	/// and values in between will be suitably scaled.
	///
	/// \param Value	The SelectionSet to apply the remap to
	/// \param Curve	The CurveFloat to shape the remap
	/// \return The result of the SelectionSet remapped to the Curve
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="RemapToCurve (SelectionSet, Float Curve)",
			CompactNodeTitle="RemapToCurve",
			ToolTip="[RemapToCurve (SelectionSet, Float Curve)] Remap the values of a SelectionSet to a Float Curve",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *RemapToCurve(USelectionSet *Value, UCurveFloat *Curve);

	/// Remaps all of the values between a new min/max
	///
	/// This goes through all of the weightings in the SelectionSet and remaps the lowest one to Min, the highest one
	/// to Max, and all others to a scaled value in between.
	///
	/// This can be useful when dealing with SelectionSets from noise functions and other such effects where there's
	/// no guarantee to the range, and remap it to 0-1 ready for use.
	///
	/// \param Value	The SelectionSet to remap
	///	\param Min		The minimum value to remap to
	/// \param Max		The maximum value to remap to
	/// \return The SelectionSet remapped to Min-Max
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="RemapToRange (SelectionSet)",
			ToolTip="[RemapToRange (SelectionSet)] Remap a SelectionSet to the min/max provided",
			Category="Math|SelectionSet",
			Keywords="clamp limit min max")
	)
		static USelectionSet *RemapToRange(USelectionSet *Value, float Min=0.0f, float Max=1.0f);


	/// Remaps a SelectionSet by applying a number of ripples.
	///
	/// This can be used to create repeated gradients, or to convert a selection into a series
	/// of rings.
	UFUNCTION(
		BlueprintPure,
		meta = (
			DisplayName = "RemapRipple(SelectionSet)",
			ToolTip="[RemapRipple(SelectionSet)] Remap a SelectionSet by 'rippling' it, adding repetitions and optionally converting it to an 'up-down'' pattern",
			Category = "Math|SelectionSet"
			)
		)
		static USelectionSet *RemapRipple(USelectionSet *Value, int32 NumberOfRipples = 4, bool UpAndDown = true);

	/// Set all values of a SelectionSet to the same Float
	///
	/// This can be used to create new SelectionSets to be combined with the original, and will
	/// return a SelectionSet with the same number of weightings and type as Value but where each
	/// value is set to Float.
	///
	/// \param Value		The source SelectionSet
	/// \param Float		The Float which all weights will be set to
	/// \return				The new SelectionSet
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Set (SelectionSet)",
			ToolTip="[Set (SelectionSet)] Set all values in a SelectionSet to the value provided",
			Category="Math|SelectionSet",
			Keywords="constant all"
		)
	)
		static USelectionSet *Set(USelectionSet *Value, float Float=0);

	/// Subtract a constant Float from all values of a SelectionSet
	///
	/// \param Value		The SelectionSet to subtract the constant from (*Value* - Float)
	/// \param Float		The Float to subtract from the SelectioNSet (Value - *Float*)
	/// \return The result of the SelectionSet - Float
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet - Float",
			CompactNodeTitle="-",
			ToolTip="[SelectionSet - Float] Subtract a float from all the values in a SelectionSet",
			Keywords="- subtract minus",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *SubtractFloatFromSelectionSet(USelectionSet *Value, float Float=0);

	/// Subtract the values of a SelectionSet from a constant Float
	///
	/// \param Float		The Float to subtract the SelectionSet from (*Float* - Value)
	/// \param Value		The SelectionSet to subtract from the Float (Float - *Value*)
	/// \return The result of Float-Value
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="Float - SelectionSet",
			CompactNodeTitle="-",
			ToolTip="[Float - SelectionSet] Subtract the values in a SelectionSet from a Float",
			Keywords="+ add plus",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *SubtractSelectionSetFromFloat(float Float, USelectionSet *Value);

	/// Subtract one SelectionSet from another
	///
	/// \param A			The SelectionSet to subtract from (*A*-B)
	/// \param B			The SelectionSet to subtract (A-*B*)
	/// \return				The result of SelectionSet A - SelectionSet B
	UFUNCTION(
		BlueprintPure,
		meta=(
			DisplayName="SelectionSet - SelectionSet",
			CompactNodeTitle="-",
			ToolTip="[SelectionSet - SelectionSet] Subtract the values in a SelectionSet from another SelectionSet",
			Keywords="+ add plus",
			Category="Math|SelectionSet"
		)
	)
		static USelectionSet *SubtractSelectionSets(USelectionSet *A, USelectionSet *B);
};
