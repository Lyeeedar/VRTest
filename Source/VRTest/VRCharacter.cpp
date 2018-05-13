// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"

/* VR Includes */
#include "HeadMountedDisplay.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/InputSettings.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	BaseTurnRate = 45.f;

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VROriginComp = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	VROriginComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	/* Assign to the VR origin component so any reset calls to the HMD can reset to 0,0,0 relative to this component */
	CameraComp->SetupAttachment(VROriginComp);
	CameraComp->bUsePawnControlRotation = true;

	//LeftMotionController = NewObject<AVRMotionController>();
	//LeftMotionController->Hand = EControllerHand::Left;
	//LeftMotionController->AttachToComponent(VROriginComp, FAttachmentTransformRules::SnapToTargetIncludingScale);

	//RightMotionController = NewObject<AVRMotionController>();
	//RightMotionController->Hand = EControllerHand::Right;
	//RightMotionController->AttachToComponent(VROriginComp, FAttachmentTransformRules::SnapToTargetIncludingScale);
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye);
	
	SetupVROptions();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	// set up gameplay key bindings
	//check(PlayerInputComponent);

	// Bind jump events
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	InputComponent->BindAction("ResetHMDOrigin", IE_Pressed, this, &AVRCharacter::ResetHMDOrigin);

	InputComponent->BindAction("ResetVR", IE_Pressed, this, &AVRCharacter::OnResetVR);

	// Bind movement events
	InputComponent->BindAxis("MoveForward", this, &AVRCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AVRCharacter::MoveSide);

	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AVRCharacter::TurnAtRate);

	// Bind interaction events
	InputComponent->BindAction("GrabLeft", IE_Pressed, this, &AVRCharacter::GrabLeft);
	InputComponent->BindAction("GrabLeft", IE_Released, this, &AVRCharacter::ReleaseLeft);
	InputComponent->BindAction("GrabRight", IE_Pressed, this, &AVRCharacter::GrabRight);
	InputComponent->BindAction("GrabRight", IE_Released, this, &AVRCharacter::ReleaseRight);
}

void AVRCharacter::SetupVROptions()
{
	
}


void AVRCharacter::ResetHMDOrigin()
{
	
}

void AVRCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AVRCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		auto vector = LeftMotionController->MotionController->GetForwardVector();
		vector.Y = 0;

		// add movement in that direction
		AddMovementInput(vector.GetSafeNormal(), Value);
	}
}

void AVRCharacter::MoveSide(float Value)
{
	if (Value != 0.0f)
	{
		auto vector = LeftMotionController->MotionController->GetRightVector();
		vector.Y = 0;

		// add movement in that direction
		AddMovementInput(vector.GetSafeNormal(), Value);
	}
}

void AVRCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AVRCharacter::GrabLeft()
{
	LeftMotionController->GrabActor();
}

void AVRCharacter::GrabRight()
{
	RightMotionController->GrabActor();
}

void AVRCharacter::ReleaseLeft()
{
	LeftMotionController->ReleaseActor();
}

void AVRCharacter::ReleaseRight()
{
	RightMotionController->ReleaseActor();
}

void AVRCharacter::ExecuteTeleport(AVRMotionController* motionController)
{
	if (isTeleporting) { return; }

	if (motionController->isValidTeleportDest)
	{
		isTeleporting = true;
		UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->StartCameraFade(0.0f, 1.0f, 0.5f, FLinearColor::Black);

		FTimerDelegate TimerCallback;
		TimerCallback.BindLambda([this, motionController]
		{
			motionController->DeactivateTeleporter();
			TeleportTo(motionController->GetTeleportDestination(), FRotator());
			UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->StartCameraFade(1.0f, 0.0f, 0.5f, FLinearColor::Black);
			isTeleporting = false;
		});

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, TimerCallback, 5.0f, false);
	}
	else
	{
		motionController->DeactivateTeleporter();
	}
}

void AVRCharacter::TeleportLeftPress()
{
	TeleportPress(LeftMotionController, RightMotionController);
}

void AVRCharacter::TeleportLeftRelease()
{
	TeleportRelease(LeftMotionController, RightMotionController);
}

void AVRCharacter::TeleportRightPress()
{
	TeleportPress(RightMotionController, LeftMotionController);
}

void AVRCharacter::TeleportRightRelease()
{
	TeleportRelease(RightMotionController, LeftMotionController);
}

void AVRCharacter::TeleportPress(AVRMotionController* thisController, AVRMotionController* otherController)
{
	thisController->ActivateTeleporter();
	otherController->DeactivateTeleporter();
}

void AVRCharacter::TeleportRelease(AVRMotionController* thisController, AVRMotionController* otherController)
{
	if (thisController->isTeleporterActive)
	{
		ExecuteTeleport(thisController);
	}
}
