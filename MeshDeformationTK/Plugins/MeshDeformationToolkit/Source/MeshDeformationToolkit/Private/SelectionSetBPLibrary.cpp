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
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Value->Size(), Value->GetOuter(), TEXT("AddFloatToSelectionSet")
	);
	if (!Result) {
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Value->Size(); WeightIndex++)
	{
		Result->Weights[WeightIndex] = Value->Weights[WeightIndex]+Float;
	}

	return Result;
}

USelectionSet *USelectionSetBPLibrary::AddSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided and same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "AddSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("AddSelectionSets")
	);
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = A->Weights[WeightIndex]+B->Weights[WeightIndex];
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("Clamp"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Clamp(Value->Weights[WeightIndex], Min, Max);
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("DivideFloatBySelectionSet"));
	if (!Result)
	{
		return nullptr;
	}
	
	// Set the minimum threshold for division
	const float ZeroThreshold = 0.01;
	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		// We need to make sure the weight is not zero to avoid divide by zero so
		// we'll set it to 'near zero' if it is.
		float Weight = Value->Weights[WeightIndex];
		if (FMath::Abs(Weight)<ZeroThreshold)
		{
			Weight = Weight<0 ? -ZeroThreshold : ZeroThreshold;
		}
		Result->Weights[WeightIndex] = Float/Weight;
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("DivideSelectionSetByFloat"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = Value->Weights[WeightIndex]/Float;
	}

	return Result;
}

USelectionSet * USelectionSetBPLibrary::DivideSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need two SelectionSets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "DivideSelectioSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("DivideSelectionSets"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = A->Weights[WeightIndex]/B->Weights[WeightIndex];
	}

	return Result;
}

USelectionSet * USelectionSetBPLibrary::Ease(
	USelectionSet *Value,
	EEasingFunc::Type EaseFunction /*= EEasingFunc::Linear*/,
	int32 Steps /*= 2*/,
	float BlendExp /*= 2.0f*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("Ease: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("Ease"));
	if (!Result)
	{
		return nullptr;
	}
	// TODO: This can be more efficient with lambdas.
	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		switch (EaseFunction)
		{
			case EEasingFunc::Step:
				Result->Weights[WeightIndex] = FMath::InterpStep<float>(0.f, 1.f, Value->Weights[WeightIndex], Steps);
				break;
			case EEasingFunc::SinusoidalIn:
				Result->Weights[WeightIndex] = FMath::InterpSinIn<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::SinusoidalOut:
				Result->Weights[WeightIndex] = FMath::InterpSinOut<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::SinusoidalInOut:
				Result->Weights[WeightIndex] = FMath::InterpSinInOut<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::EaseIn:
				Result->Weights[WeightIndex] = FMath::InterpEaseIn<float>(0.f, 1.f, Value->Weights[WeightIndex], BlendExp);
				break;
			case EEasingFunc::EaseOut:
				Result->Weights[WeightIndex] = FMath::InterpEaseOut<float>(0.f, 1.f, Value->Weights[WeightIndex], BlendExp);
				break;
			case EEasingFunc::EaseInOut:
				Result->Weights[WeightIndex] = FMath::InterpEaseInOut<float>(0.f, 1.f, Value->Weights[WeightIndex], BlendExp);
				break;
			case EEasingFunc::ExpoIn:
				Result->Weights[WeightIndex] = FMath::InterpExpoIn<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::ExpoOut:
				Result->Weights[WeightIndex] = FMath::InterpExpoOut<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::ExpoInOut:
				Result->Weights[WeightIndex] = FMath::InterpExpoInOut<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::CircularIn:
				Result->Weights[WeightIndex] = FMath::InterpCircularIn<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::CircularOut:
				Result->Weights[WeightIndex] = FMath::InterpCircularOut<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			case EEasingFunc::CircularInOut:
				Result->Weights[WeightIndex] = FMath::InterpCircularInOut<float>(0.f, 1.f, Value->Weights[WeightIndex]);
				break;
			default:
				// Do nothing: linear.
				Result->Weights[WeightIndex] = Value->Weights[WeightIndex];
				break;
		}
	}

	return Result;
}

bool USelectionSetBPLibrary::HaveTwoSelectionSetsOfSameSize(
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


bool USelectionSetBPLibrary::HaveThreeSelectionSetsOfSameSize(
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
	if (SizeA!=SizeB || SizeA!=SizeC)
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

// Note: The BP name and C++ names of this function are different as UFUNCTION() doesn't allow overloading
USelectionSet * USelectionSetBPLibrary::LerpSelectionSetsWithFloat(USelectionSet *A, USelectionSet *B, float Alpha/*=0*/)
{
	// Need two SelectionSets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "LerpSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("LerpSelectionSets"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Lerp(A->Weights[WeightIndex], B->Weights[WeightIndex], Alpha);
	}

	return Result;
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
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("LerpSelectionSets"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Lerp(A->Weights[WeightIndex], B->Weights[WeightIndex], Alpha->Weights[WeightIndex]);
	}

	return Result;

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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("LerpSelectionSetWithFloat"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Lerp(Value->Weights[WeightIndex], Float, Alpha);
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("MaxSelectionSetAgainstFloat"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		//result->weights[i] = Value->weights[i] > Float ? Value->weights[i] : Float;
		Result->Weights[WeightIndex] = FMath::Max(Value->Weights[WeightIndex], Float);
	}

	return Result;
}

USelectionSet * USelectionSetBPLibrary::MaxSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "MaxSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("MaxSelectionSets"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Max(A->Weights[WeightIndex], B->Weights[WeightIndex]);
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("MinSelectionSetAgainstFloat"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Min(Value->Weights[WeightIndex], Float);
	}

	return Result;
}

USelectionSet * USelectionSetBPLibrary::MinSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need two selection sets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "MinSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("MinSelectionSets"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Min(A->Weights[WeightIndex], B->Weights[WeightIndex]);
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("MultiplySelectionSetByFloat"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = Value->Weights[WeightIndex]*Float;
	}

	return Result;
}

USelectionSet * USelectionSetBPLibrary::MultiplySelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need two selection sets of same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "MultiplySelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("MultiplySelectionSets"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = A->Weights[WeightIndex]*B->Weights[WeightIndex];
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("OneMinus"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = 1.0f-Value->Weights[WeightIndex];
	}

	return Result;
}


USelectionSet * USelectionSetBPLibrary::Power(USelectionSet *Value, float Exp)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("Power: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("Power"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = FMath::Pow(Value->Weights[WeightIndex], Exp);
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("Randomize"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = RandomStream.FRandRange(Min, Max);
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("RemapToCurve"));
	if (!Result)
	{
		return nullptr;
	}

	// Apply the curve mapping
	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = Curve->GetFloatValue(Value->Weights[WeightIndex]*CurveTimeEnd);
	}

	return Result;
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
	for (int32 WeightIndex = 1; WeightIndex<Value->Size(); WeightIndex++)
	{
		CurrentMinimum = FMath::Min(CurrentMinimum, Value->Weights[WeightIndex]);
		CurrentMaximum = FMath::Max(CurrentMaximum, Value->Weights[WeightIndex]);
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("RemapToRange"));
	if (!Result)
	{
		return nullptr;
	}

	// Check if all values are the same- if so just return a flat result equal to Min.
	if (CurrentMinimum==CurrentMaximum)
	{
		return Set(Result, Min);
	}

	// Perform the remapping
	float Scale = (Max-Min)/(CurrentMaximum-CurrentMinimum);
	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = (Value->Weights[WeightIndex]-CurrentMinimum) * Scale+Min;
	}

	return Result;
}


USelectionSet * USelectionSetBPLibrary::RemapRipple(
	USelectionSet *Value, int32 NumberOfRipples /*= 4*/, bool bUpAndDown /*= true*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		UE_LOG(MDTLog, Warning, TEXT("RemapRipple: Need a SelectionSet"));
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("RemapRipple"));
	if (!Result)
	{
		return nullptr;
	}

	// Perform the remap
	for (int32 WeightIndex=0; WeightIndex<Size; WeightIndex++) {
		const float ScaledValue = Value->Weights[WeightIndex] * NumberOfRipples;
		const bool bIsOdd = (FPlatformMath::FloorToInt(ScaledValue) % 2) ==1;
		const bool bShouldInvert = bUpAndDown && bIsOdd;
	
		Result->Weights[WeightIndex] = bShouldInvert ?
			1.0f-FMath::Fmod(ScaledValue, 1.0f) :
			FMath::Fmod(ScaledValue, 1.0f);
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("Set"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = Float;
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("SubtractFloatFromSelectionSet"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = Value->Weights[WeightIndex]-Float;
	}

	return Result;
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
	const int32 Size = Value->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, Value->GetOuter(), TEXT("SubtractSelectionSetFromFloat"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = Float-Value->Weights[WeightIndex];
	}

	return Result;
}

USelectionSet * USelectionSetBPLibrary::SubtractSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided and same size
	if (!HaveTwoSelectionSetsOfSameSize(A, B, "SubtractSelectionSets"))
	{
		return nullptr;
	}

	// Create a zeroed SelectionSet to store results, sized correctly for performance
	const int32 Size = A->Size();
	USelectionSet *Result = USelectionSet::CreateAndCheckValid(
		Size, A->GetOuter(), TEXT("SubtractSelectionSets"));
	if (!Result)
	{
		return nullptr;
	}

	for (int32 WeightIndex = 0; WeightIndex<Size; WeightIndex++)
	{
		Result->Weights[WeightIndex] = A->Weights[WeightIndex]-B->Weights[WeightIndex];
	}

	return Result;
}