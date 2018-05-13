// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRMotionController.h"
#include "VRCharacter.generated.h"

class UInputComponent;

UCLASS(config = Game)
class AVRCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComp;

	/* Component to specify origin for the HMD */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USceneComponent* VROriginComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class AVRMotionController* LeftMotionController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class AVRMotionController* RightMotionController;

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

public:
	// Sets default values for this character's properties
	AVRCharacter();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	void SetupVROptions();

	/* Resets HMD Origin position and orientation */
	void ResetHMDOrigin();

	/**
	* Called via input to turn at a given rate.
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float Rate);

	void MoveForward(float Val);
	void MoveSide(float Val);

	void GrabLeft();
	void GrabRight();
	void ReleaseLeft();
	void ReleaseRight();

	void TeleportLeftPress();
	void TeleportLeftRelease();
	void TeleportRightPress();
	void TeleportRightRelease();

	void TeleportPress(AVRMotionController* thisController, AVRMotionController* otherController);
	void TeleportRelease(AVRMotionController* thisController, AVRMotionController* otherController);

	void OnResetVR();

	void ExecuteTeleport(AVRMotionController* motionController);

	bool isTeleporting;
};