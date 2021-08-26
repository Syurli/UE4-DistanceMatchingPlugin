// Copyright Epic Games, Inc. All Rights Reserved.

#include "DistanceMatchingPluginBPLibrary.h"
#include "DistanceMatchingPlugin.h"
#include "Animation/AnimCurveCompressionCodec_UniformIndexable.h"
#include "Animation/AnimInstance.h"
#include "Animation/SmartName.h"

UDistanceMatchingPluginBPLibrary::UDistanceMatchingPluginBPLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

float UDistanceMatchingPluginBPLibrary::GetCurveTime(const UAnimSequence* AnimationSequence, const FName CurveName, const float CurveValue)
{
	if (AnimationSequence == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Animation Sequence ptr"));
		return 0.0f;
	}

	// Get curve SmartName*
	FSmartName CurveSmartName;
	AnimationSequence->GetSkeleton()->GetSmartNameByName(USkeleton::AnimCurveMappingName, CurveName, CurveSmartName);

	// Create a buffered access to times and values in curve*
	const FAnimCurveBufferAccess CurveBuffer = FAnimCurveBufferAccess(AnimationSequence, CurveSmartName.UID);

	// The number of elements in curve*
	const int NumSamples = static_cast<int>(CurveBuffer.GetNumSamples());
	const int LastIndex = NumSamples - 1;

	if (NumSamples < 2)
	{
		return 0.0f;
	}

	// Corner cases*
	if (CurveValue <= CurveBuffer.GetValue(0))
	{
		return CurveBuffer.GetTime(0);
	}
	if (CurveValue >= CurveBuffer.GetValue(LastIndex))
	{
		return  CurveBuffer.GetTime(LastIndex);
	}

	// Binary search*
	int32 NextIndex = 1;
	int32 Count = LastIndex - NextIndex;
	while (Count > 0)
	{
		const int32 Step = Count / 2;
		const int32 Middle = NextIndex + Step;

		if (CurveValue > CurveBuffer.GetValue(Middle))
		{
			NextIndex = Middle + 1;
			Count -= Step + 1;
		}
		else
		{
			Count = Step;
		}
	}

	const int32 PrevIndex = NextIndex - 1;
	const float PrevCurveValue = CurveBuffer.GetValue(PrevIndex);
	const float NextCurveValue = CurveBuffer.GetValue(NextIndex);
	const float PrevCurveTime = CurveBuffer.GetTime(PrevIndex);
	const float NextCurveTime = CurveBuffer.GetTime(NextIndex);

	// Find time by two nearest known points on the curve*
	const float Diff = NextCurveValue - PrevCurveValue;
	const float Alpha = !FMath::IsNearlyZero(Diff) ? (CurveValue - PrevCurveValue) / Diff : 0.0f;
	return FMath::Lerp(PrevCurveTime, NextCurveTime, Alpha);
}



bool UDistanceMatchingPluginBPLibrary::PredictStopLocation(
	FVector& OutStopLocation,
	const FVector& CurrentLocation,
	const FVector& Velocity,
	const FVector& Acceleration,
	float Friction,
	float BrakingDeceleration,
	const float TimeStep,
	const int MaxSimulationIterations)
{
	const float MIN_TICK_TIME = 1e-6;
	if (TimeStep < MIN_TICK_TIME)
	{
		return false;
	}
	// Apply braking or deceleration
	const bool bZeroAcceleration = Acceleration.IsZero();

	if ((Acceleration | Velocity) > 0.0f)
	{
		return false;
	}

	BrakingDeceleration = FMath::Max(BrakingDeceleration, 0.f);
	Friction = FMath::Max(Friction, 0.f);
	const bool bZeroFriction = (Friction == 0.f);
	const bool bZeroBraking = (BrakingDeceleration == 0.f);

	if (bZeroAcceleration && bZeroFriction)
	{
		return false;
	}

	FVector LastVelocity = bZeroAcceleration ? Velocity : Velocity.ProjectOnToNormal(Acceleration.GetSafeNormal());
	LastVelocity.Z = 0;

	FVector LastLocation = CurrentLocation;

	int Iterations = 0;
	while (Iterations < MaxSimulationIterations)
	{
		Iterations++;

		const FVector OldVel = LastVelocity;

		// Only apply braking if there is no acceleration, or we are over our max speed and need to slow down to it.
		if (bZeroAcceleration)
		{
			// subdivide braking to get reasonably consistent results at lower frame rates
			// (important for packet loss situations w/ networking)
			float RemainingTime = TimeStep;
			const float MaxTimeStep = (1.0f / 33.0f);

			// Decelerate to brake to a stop
			const FVector RevAccel = (bZeroBraking ? FVector::ZeroVector : (-BrakingDeceleration * LastVelocity.GetSafeNormal()));
			while (RemainingTime >= MIN_TICK_TIME)
			{
				// Zero friction uses constant deceleration, so no need for iteration.
				const float dt = ((RemainingTime > MaxTimeStep && !bZeroFriction) ? FMath::Min(MaxTimeStep, RemainingTime * 0.5f) : RemainingTime);
				RemainingTime -= dt;

				// apply friction and braking
				LastVelocity = LastVelocity + ((-Friction) * LastVelocity + RevAccel) * dt;

				// Don't reverse direction
				if ((LastVelocity | OldVel) <= 0.f)
				{
					LastVelocity = FVector::ZeroVector;
					break;
				}
			}

			// Clamp to zero if nearly zero, or if below min threshold and braking.
			const float VSizeSq = LastVelocity.SizeSquared();
			if (VSizeSq <= 1.f || (!bZeroBraking && VSizeSq <= FMath::Square(10)))
			{
				LastVelocity = FVector::ZeroVector;
			}
		}
		else
		{
			FVector TotalAcceleration = Acceleration;
			TotalAcceleration.Z = 0;

			// Friction affects our ability to change direction. This is only done for input acceleration, not path following.
			const FVector AccelDir = TotalAcceleration.GetSafeNormal();
			const float VelSize = LastVelocity.Size();
			TotalAcceleration += -(LastVelocity - AccelDir * VelSize) * Friction;
			// Apply acceleration
			LastVelocity += TotalAcceleration * TimeStep;
		}

		LastLocation += LastVelocity * TimeStep;

		// Clamp to zero if nearly zero, or if below min threshold and braking.
		const float VSizeSq = LastVelocity.SizeSquared();
		if (VSizeSq <= 1.f
			|| (LastVelocity | OldVel) <= 0.f)
		{
			OutStopLocation = LastLocation;
			return true;
		}
	}

	return false;
}
