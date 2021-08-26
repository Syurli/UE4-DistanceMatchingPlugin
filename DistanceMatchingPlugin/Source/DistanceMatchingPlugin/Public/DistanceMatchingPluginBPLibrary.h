// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DistanceMatchingPluginBPLibrary.generated.h"

/*
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu.
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
* Thanks for 小王小王喜气洋洋 ! and his answer . https://answers.unrealengine.com/questions/531204/predict-stop-position-of-character-ahead-in-time.html
* Thanks for Outcast DevSchool  and his cool video : ) https://www.youtube.com/playlist?list=PL8-kndpxmv9iH0t3lnAobGuIKdb7UnwKS
*/
UCLASS()
class UDistanceMatchingPluginBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()


		UFUNCTION(BlueprintPure, Category = "Animation")
		static float GetCurveTime(const UAnimSequence* AnimationSequence, const FName CurveName, const float CurveValue);


	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Predict Stop Location", Keywords = "Distance Matching Predict stop location"), Category = "DistanceMatchingTesting")
		static bool PredictStopLocation(FVector& OutStopLocation, const FVector& CurrentLocation, const FVector& Velocity, const FVector& Acceleration, float Friction, float BrakingDeceleration, const float TimeStep, const int MaxSimulationIterations);
};