// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "SelectionSet.h"
#include "SelectionSetBPLibrary.h"

USelectionSet * USelectionSetBPLibrary::AddFloatToSelectionSet(USelectionSet *Value, float Float/*=0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("AddFloatToSelectionSet: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		Value->Size(), Value->GetOuter(), TEXT("AddFloatToSelectionSet")
	);
	if (!result) {
		return nullptr;
	}

	for (int32 i = 0; i<Value->Size(); i++)
	{
		result->Weights[i] = Value->Weights[i]+Float;
	}

	return result;
}

USelectionSet *USelectionSetBPLibrary::AddSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided and same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "AddSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("AddSelectionSets")
	);
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = A->Weights[i]+B->Weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::Clamp(USelectionSet *Value, float Min/*=0*/, float Max/*=1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("Clamp: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("Clamp"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = FMath::Clamp(Value->Weights[i], Min, Max);
	}

	return result;
}


USelectionSet * USelectionSetBPLibrary::DivideFloatBySelectionSet(float Float /*= 1*/, USelectionSet *Value/*=nullptr*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("DivideSelectionSetByFloat: Need a SelectionSet"));
		return nullptr;
	}


	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("DivideFloatBySelectionSet"));
	if (!result)
	{
		return nullptr;
	}
	
	// Set the minimum threshold for division
	const float zeroThreshold = 0.01;
	for (int32 i = 0; i<size; i++)
	{
		// We need to make sure the weight is not zero to avoid divide by zero so
		// we'll set it to 'near zero' if it is.
		float weight = Value->Weights[i];
		if (FMath::Abs(weight)<zeroThreshold)
		{
			weight = weight<0 ? -zeroThreshold : zeroThreshold;
		}
		result->Weights[i] = Float/weight;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::DivideSelectionSetByFloat(USelectionSet *Value, float Float /*= 1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("DivideSelectionSetByFloat: Need a SelectionSet"));
		return nullptr;
	}

	// Float cannot be zero as that would be a/0.
	if (Float==0)
	{
		UE_LOG(MDTLog, Warning, TEXT("DivideSelectionSetByFloat: Cannot divide by zero"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("DivideSelectionSetByFloat"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = Value->Weights[i]/Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::DivideSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need two SelectionSets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "DivideSelectioSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("DivideSelectionSets"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = A->Weights[i]/B->Weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::Ease(USelectionSet *Value, EEasingFunc::Type EaseFunction /*= EEasingFunc::Linear*/, int32 Steps /*= 2*/, float BlendExp /*= 2.0f*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("Ease: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("Ease"));
	if (!result)
	{
		return nullptr;
	}
	// TODO: This can be more efficient with lambdas.
	for (int32 i = 0; i<size; i++)
	{
		switch (EaseFunction)
		{
			case EEasingFunc::Step:
				result->Weights[i] = FMath::InterpStep<float>(0.f, 1.f, Value->Weights[i], Steps);
				break;
			case EEasingFunc::SinusoidalIn:
				result->Weights[i] = FMath::InterpSinIn<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::SinusoidalOut:
				result->Weights[i] = FMath::InterpSinOut<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::SinusoidalInOut:
				result->Weights[i] = FMath::InterpSinInOut<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::EaseIn:
				result->Weights[i] = FMath::InterpEaseIn<float>(0.f, 1.f, Value->Weights[i], BlendExp);
				break;
			case EEasingFunc::EaseOut:
				result->Weights[i] = FMath::InterpEaseOut<float>(0.f, 1.f, Value->Weights[i], BlendExp);
				break;
			case EEasingFunc::EaseInOut:
				result->Weights[i] = FMath::InterpEaseInOut<float>(0.f, 1.f, Value->Weights[i], BlendExp);
				break;
			case EEasingFunc::ExpoIn:
				result->Weights[i] = FMath::InterpExpoIn<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::ExpoOut:
				result->Weights[i] = FMath::InterpExpoOut<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::ExpoInOut:
				result->Weights[i] = FMath::InterpExpoInOut<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::CircularIn:
				result->Weights[i] = FMath::InterpCircularIn<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::CircularOut:
				result->Weights[i] = FMath::InterpCircularOut<float>(0.f, 1.f, Value->Weights[i]);
				break;
			case EEasingFunc::CircularInOut:
				result->Weights[i] = FMath::InterpCircularInOut<float>(0.f, 1.f, Value->Weights[i]);
				break;
			default:
				// Do nothing: linear.
				result->Weights[i] = Value->Weights[i];
				break;
		}
	}

	return result;
}

bool USelectionSetBPLibrary::HaveTwoSelectionSetsOfSameSize(USelectionSet *SelectionA, USelectionSet *SelectionB, FString NodeNameForWarning)
{
	if (!SelectionA||!SelectionB)
	{
		UE_LOG(MDTLog, Warning, TEXT("%s: Need two SelectionSets"), *NodeNameForWarning);
		return false;
	}

	const int32 sizeA = SelectionA->Size();
	const int32 sizeB = SelectionB->Size();
	if (sizeA!=sizeB)
	{
		UE_LOG(
			MDTLog, Warning,
			TEXT("%s: SelectionSets are not the same size (%d and %d"),
			*NodeNameForWarning, sizeA, sizeB
		);
		return false;
	}
	return true;
}


bool USelectionSetBPLibrary::HaveThreeSelectionSetsOfSameSize(USelectionSet *SelectionA, USelectionSet *SelectionB, USelectionSet *SelectionC, FString NodeNameForWarning)
{

	if (!SelectionA||!SelectionB||!SelectionC)
	{
		UE_LOG(MDTLog, Warning, TEXT("%s: Need three SelectionSets"), *NodeNameForWarning);
		return false;
	}

	const int32 sizeA = SelectionA->Size();
	const int32 sizeB = SelectionB->Size();
	const int32 sizeC = SelectionC->Size();
	if (sizeA!=sizeB || sizeA!=sizeC)
	{
		UE_LOG(
			MDTLog, Warning,
			TEXT("%s: SelectionSets are not the same size (%d, %d and %d"),
			*NodeNameForWarning, sizeA, sizeB, sizeC
		);
		return false;
	}
	return true;
}

// Note: The BP name and C++ names of this function are different as UFUNCTION() doesn't allow overloading
USelectionSet * USelectionSetBPLibrary::LerpSelectionSetsWithFloat(USelectionSet *A, USelectionSet *B, float Alpha/*=0*/)
{
	// Need two SelectionSets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "LerpSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("LerpSelectionSets"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = FMath::Lerp(A->Weights[i], B->Weights[i], Alpha);
	}

	return result;
}


// Note: The BP name and C++ names of this function are different as UFUNCTION() doesn't allow overloading
USelectionSet * USelectionSetBPLibrary::LerpSelectionSetsWithSelectionSet(USelectionSet *A, USelectionSet *B, USelectionSet *Alpha)
{
	// Need three SelectionSets of same size
	if (!HaveThreeSelectionSetsOfSameSize(A, B, Alpha, "LerpSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("LerpSelectionSets"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = FMath::Lerp(A->Weights[i], B->Weights[i], Alpha->Weights[i]);
	}

	return result;

}

USelectionSet * USelectionSetBPLibrary::LerpSelectionSetWithFloat(USelectionSet *Value, float Float, float Alpha /*= 0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("LerpSelectionSetWithFloat: No SelectionSet provided"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("LerpSelectionSetWithFloat"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = FMath::Lerp(Value->Weights[i], Float, Alpha);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MaxSelectionSetAgainstFloat(USelectionSet *Value, float Float)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("MaxSelectionSetAgainstFloat: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("MaxSelectionSetAgainstFloat"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		//result->weights[i] = Value->weights[i] > Float ? Value->weights[i] : Float;
		result->Weights[i] = FMath::Max(Value->Weights[i], Float);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MaxSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "MaxSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("MaxSelectionSets"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = FMath::Max(A->Weights[i], B->Weights[i]);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MinSelectionSetAgainstFloat(USelectionSet *Value, float Float)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("MinSelectionSetAgainstFloat: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("MinSelectionSetAgainstFloat"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = FMath::Min(Value->Weights[i], Float);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MinSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need two selection sets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "MinSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("MinSelectionSets"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = FMath::Min(A->Weights[i], B->Weights[i]);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MultiplySelctionSetByFloat(USelectionSet *Value, float Float/*=1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("MultiplySelectionSetByFloat: Need a SelectionSet"))
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("MultiplySelectionSetByFloat"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = Value->Weights[i]*Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MultiplySelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need two selection sets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "MultiplySelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("MultiplySelectionSets"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = A->Weights[i]*B->Weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::OneMinus(USelectionSet *Value)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("OneMinus: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("OneMinus"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = 1.0f-Value->Weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::Randomize(USelectionSet *Value, FRandomStream &RandomStream, float Min/*=0*/, float Max/*=1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("Randomize: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("Randomize"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = RandomStream.FRandRange(Min, Max);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::RemapToCurve(USelectionSet *Value, UCurveFloat *Curve)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("RemapToCurve: No SelectionSet provided"));
		return nullptr;
	}

	// Need a Curve
	if (!Curve)
	{
		UE_LOG(MDTLog, Warning, TEXT("RemapToCurve: No Curve provided"));
		return nullptr;
	}

	// Get the time limits of the curve- we'll scale by the end
	float CurveTimeStart, CurveTimeEnd;
	Curve->GetTimeRange(CurveTimeStart, CurveTimeEnd);

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("RemapToCurve"));
	if (!result)
	{
		return nullptr;
	}

	// Apply the curve mapping
	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = Curve->GetFloatValue(Value->Weights[i]*CurveTimeEnd);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::RemapToRange(USelectionSet *Value, float Min /*= 0.0f*/, float Max /*= 1.0f*/)
{
	// Need a SelectionSet with at least one value
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("RemapToRange: No SelectionSet provided"));
		return nullptr;
	}
	if (Value->Size()==0)
	{
		UE_LOG(MDTLog, Warning, TEXT("RemapToRange: SelectionSet has no weights, need at least one item"));
		return nullptr;
	}

	// Find the current minimum and maximum.
	float CurrentMinimum = Value->Weights[0];
	float CurrentMaximum = Value->Weights[0];
	for (int32 i = 1; i<Value->Size(); i++)
	{
		CurrentMinimum = FMath::Min(CurrentMinimum, Value->Weights[i]);
		CurrentMaximum = FMath::Max(CurrentMaximum, Value->Weights[i]);
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("RemapToRange"));
	if (!result)
	{
		return nullptr;
	}

	// Check if all values are the same- if so just return a flat result equal to Min.
	if (CurrentMinimum==CurrentMaximum)
	{
		return Set(result, Min);
	}

	// Perform the remapping
	float Scale = (Max-Min)/(CurrentMaximum-CurrentMinimum);
	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = (Value->Weights[i]-CurrentMinimum) * Scale+Min;
	}

	return result;
}


USelectionSet * USelectionSetBPLibrary::RemapRipple(
	USelectionSet *Value, int32 NumberOfRipples /*= 4*/, bool UpAndDown /*= true*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("RemapRipple: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("RemapRipple"));
	if (!result)
	{
		return nullptr;
	}

	// Perform the remap
	for (int32 i=0; i<size; i++) {
		const float scaledValue = Value->Weights[i] * NumberOfRipples;
		const bool isOdd = (FPlatformMath::FloorToInt(scaledValue) % 2) ==1;
		const bool shouldInvert = UpAndDown && isOdd;
	
		result->Weights[i] = shouldInvert ?
			1.0f-FMath::Fmod(scaledValue, 1.0f) :
			FMath::Fmod(scaledValue, 1.0f);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::Set(USelectionSet *Value, float Float/*=0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("Set: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("Set"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::SubtractFloatFromSelectionSet(USelectionSet *Value, float Float/*=0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("SubtractFloatFromSelectionSet: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("SubtractFloatFromSelectionSet"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = Value->Weights[i]-Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::SubtractSelectionSetFromFloat(float Float, USelectionSet *Value)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("SubtractSelectionSetFromFloat: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = Value->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, Value->GetOuter(), TEXT("SubtractSelectionSetFromFloat"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = Float-Value->Weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::SubtractSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided and same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "SubtractSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 size = A->Size();
	USelectionSet *result = USelectionSet::CreateAndCheckValid(
		size, A->GetOuter(), TEXT("SubtractSelectionSets"));
	if (!result)
	{
		return nullptr;
	}

	for (int32 i = 0; i<size; i++)
	{
		result->Weights[i] = A->Weights[i]-B->Weights[i];
	}

	return result;
}