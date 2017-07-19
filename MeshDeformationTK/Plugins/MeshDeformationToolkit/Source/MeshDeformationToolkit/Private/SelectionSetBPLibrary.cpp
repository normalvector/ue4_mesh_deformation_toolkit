// (c)2017 Paul Golds, released under MIT License.

#include "MeshDeformationToolkit.h"
#include "SelectionSetBPLibrary.h"

USelectionSet * USelectionSetBPLibrary::Clamp(USelectionSet *Value, float Min/*=0*/, float Max/*=1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = FMath::Clamp(Value->weights[i], Min, Max);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::Ease(USelectionSet *Value, EEasingFunc::Type EaseFunction /*= EEasingFunc::Linear*/, int32 Steps /*= 2*/, float BlendExp /*= 2.0f*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	// TODO: This can be more efficient with lambdas.
	for (int32 i = 0; i<size; i++)
	{
		switch (EaseFunction)
		{
			case EEasingFunc::Step:
				result->weights[i] = FMath::InterpStep<float>(0.f, 1.f, Value->weights[i], Steps);
				break;
			case EEasingFunc::SinusoidalIn:
				result->weights[i] = FMath::InterpSinIn<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::SinusoidalOut:
				result->weights[i] = FMath::InterpSinOut<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::SinusoidalInOut:
				result->weights[i] = FMath::InterpSinInOut<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::EaseIn:
				result->weights[i] = FMath::InterpEaseIn<float>(0.f, 1.f, Value->weights[i], BlendExp);
				break;
			case EEasingFunc::EaseOut:
				result->weights[i] = FMath::InterpEaseOut<float>(0.f, 1.f, Value->weights[i], BlendExp);
				break;
			case EEasingFunc::EaseInOut:
				result->weights[i] = FMath::InterpEaseInOut<float>(0.f, 1.f, Value->weights[i], BlendExp);
				break;
			case EEasingFunc::ExpoIn:
				result->weights[i] = FMath::InterpExpoIn<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::ExpoOut:
				result->weights[i] = FMath::InterpExpoOut<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::ExpoInOut:
				result->weights[i] = FMath::InterpExpoInOut<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::CircularIn:
				result->weights[i] = FMath::InterpCircularIn<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::CircularOut:
				result->weights[i] = FMath::InterpCircularOut<float>(0.f, 1.f, Value->weights[i]);
				break;
			case EEasingFunc::CircularInOut:
				result->weights[i] = FMath::InterpCircularInOut<float>(0.f, 1.f, Value->weights[i]);
				break;
			default:
				// Do nothing: linear.
				result->weights[i] = Value->weights[i];
				break;
		}
	}

	return result;
}

USelectionSet *USelectionSetBPLibrary::AddSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!A||!B)
	{
		return nullptr;
	}

	auto result = NewObject<USelectionSet>(A->GetOuter());
	int32 smallestSize = A->weights.Num()<B->weights.Num() ? A->weights.Num() : B->weights.Num();
	result->weights.SetNumZeroed(smallestSize);

	for (int32 i = 0; i<smallestSize; i++)
	{
		result->weights[i] = A->weights[i]+B->weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::SubtractSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!A||!B)
	{
		return nullptr;
	}

	auto result = NewObject<USelectionSet>(A->GetOuter());
	int32 smallestSize = A->weights.Num()<B->weights.Num() ? A->weights.Num() : B->weights.Num();
	result->weights.SetNumZeroed(smallestSize);

	for (int32 i = 0; i<smallestSize; i++)
	{
		result->weights[i] = A->weights[i]-B->weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::AddFloatToSelectionSet(USelectionSet *Value, float Float/*=0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = Value->weights[i]+Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::SubtractFloatFromSelectionSet(USelectionSet *Value, float Float/*=0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = Value->weights[i]-Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::SubtractSelectionSetFromFloat(float Float, USelectionSet *Value)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = Float-Value->weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MultiplySelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!A||!B)
	{
		return nullptr;
	}

	auto result = NewObject<USelectionSet>(A->GetOuter());
	int32 smallestSize = A->weights.Num()<B->weights.Num() ? A->weights.Num() : B->weights.Num();
	result->weights.SetNumZeroed(smallestSize);
	for (int32 i = 0; i<smallestSize; i++)
	{
		result->weights[i] = A->weights[i]*B->weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MultiplySelctionSetByFloat(USelectionSet *Value, float Float/*=1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = Value->weights[i]*Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::DivideSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!A||!B)
	{
		return nullptr;
	}

	auto result = NewObject<USelectionSet>(A->GetOuter());
	int32 smallestSize = A->weights.Num()<B->weights.Num() ? A->weights.Num() : B->weights.Num();
	result->weights.SetNumZeroed(smallestSize);

	for (int32 i = 0; i<smallestSize; i++)
	{
		result->weights[i] = A->weights[i]/B->weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::DivideSelctionSetByFloat(USelectionSet *Value, float Float /*= 1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = Value->weights[i]/Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::OneMinus(USelectionSet *Value)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = 1.0f-Value->weights[i];
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::Set(USelectionSet *Value, float Float/*=0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = Float;
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::Randomize(USelectionSet *Value, FRandomStream RandomStream, float Min/*=0*/, float Max/*=1*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = RandomStream.FRandRange(Min, Max);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MaxSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!A||!B)
	{
		return nullptr;
	}

	auto result = NewObject<USelectionSet>(A->GetOuter());
	int32 smallestSize = A->weights.Num()<B->weights.Num() ? A->weights.Num() : B->weights.Num();
	result->weights.SetNumZeroed(smallestSize);

	for (int32 i = 0; i<smallestSize; i++)
	{
//result->weights[i] = A->weights[i] > B->weights[i] ? A->weights[i] : B->weights[i];
		result->weights[i] = FMath::Max(A->weights[i], B->weights[i]);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MinSelectionSets(USelectionSet *A, USelectionSet *B)
{
	// Need both provided
	if (!A||!B)
	{
		return nullptr;
	}

	auto result = NewObject<USelectionSet>(A->GetOuter());
	int32 smallestSize = A->weights.Num()<B->weights.Num() ? A->weights.Num() : B->weights.Num();
	result->weights.SetNumZeroed(smallestSize);

	for (int32 i = 0; i<smallestSize; i++)
	{
//result->weights[i] = A->weights[i] < B->weights[i] ? A->weights[i] : B->weights[i];
		result->weights[i] = FMath::Min(A->weights[i], B->weights[i]);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MaxSelectionSetAgainstFloat(USelectionSet *Value, float Float)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
//result->weights[i] = Value->weights[i] > Float ? Value->weights[i] : Float;
		result->weights[i] = FMath::Max(Value->weights[i], Float);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::MinSelectionSetAgainstFloat(USelectionSet *Value, float Float)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
//result->weights[i] = Value->weights[i] < Float ? Value->weights[i] : Float;
		result->weights[i] = FMath::Min(Value->weights[i], Float);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::LerpSelectionSets(USelectionSet *A, USelectionSet *B, float Alpha/*=0*/)
{
	// Need both provided
	if (!A||!B)
	{
		return nullptr;
	}

	auto result = NewObject<USelectionSet>(A->GetOuter());
	int32 smallestSize = A->weights.Num()<B->weights.Num() ? A->weights.Num() : B->weights.Num();
	result->weights.SetNumZeroed(smallestSize);

	for (int32 i = 0; i<smallestSize; i++)
	{
		result->weights[i] = FMath::Lerp(A->weights[i], B->weights[i], Alpha);
	}

	return result;

}

USelectionSet * USelectionSetBPLibrary::LerpSelectionSetWithFloat(USelectionSet *Value, float Float, float Alpha /*= 0*/)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = FMath::Lerp(Value->weights[i], Float, Alpha);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::RemapToCurve(USelectionSet *Value, UCurveFloat *Curve)
{
	// Need a SelectionSet
	if (!Value)
	{
		return nullptr;
	}

	// Need a Curve
	if (!Curve)
	{
		return nullptr;
	}

	// Get the time limits of the curve- we'll scale by the end
	float CurveTimeStart, CurveTimeEnd;
	Curve->GetTimeRange(CurveTimeStart, CurveTimeEnd);

	// Create the results at the correct size and zero it.
	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	auto size = Value->weights.Num();
	result->weights.SetNumZeroed(size);

	// Apply the curve mapping
	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = Curve->GetFloatValue(Value->weights[i]*CurveTimeEnd);
	}

	return result;
}

USelectionSet * USelectionSetBPLibrary::RemapToRange(USelectionSet *Value, float Min /*= 0.0f*/, float Max /*= 1.0f*/)
{
	// Need a SelectionSet, and it needs at least one value.
	if (!Value)
	{
		return nullptr;
	}
	int32 size = Value->weights.Num();
	if (size==0)
	{
		return nullptr;
	}

	// Find the current minimum and maximum.
	float CurrentMinimum = Value->weights[0];
	float CurrentMaximum = Value->weights[0];
	for (int32 i = 1; i<size; i++)
	{
		CurrentMinimum = FMath::Min(CurrentMinimum, Value->weights[i]);
		CurrentMaximum = FMath::Max(CurrentMaximum, Value->weights[i]);
	}

	// Create the results at the correct size and zero it.
	USelectionSet *result = NewObject<USelectionSet>(Value->GetOuter());
	result->weights.SetNumZeroed(size);

	// Check if all values are the same- if so just return a flat result equal to Min.
	if (CurrentMinimum==CurrentMaximum)
	{
		return Set(result, Min);
	}

	// Perform the remapping
	float Scale = (Max-Min)/(CurrentMaximum-CurrentMinimum);
	for (int32 i = 0; i<size; i++)
	{
		result->weights[i] = (Value->weights[i]-CurrentMinimum) * Scale+Min;
	}

	return result;
}
