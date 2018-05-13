// Fill out your copyright notice in the Description page of Project Settings.

#include "VRMotionController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "XRMotionControllerBase.h"
#include "MotionControllerComponent.h"
#include "AI/Navigation/NavigationSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "HandAnimation.h"
#include "VRCharacter.h"

//---------------------------------------------------------------------------------------------------------------------
void SetModelAndMaterial(UStaticMeshComponent* component, const TCHAR* model, const TCHAR* material)
{
	ConstructorHelpers::FObjectFinder<UStaticMesh> modelFinder(model);
	ConstructorHelpers::FObjectFinder<UMaterialInterface> materialFinder(material);

	component->SetStaticMesh(modelFinder.Object);
	component->SetMaterial(0, materialFinder.Object);
}

//---------------------------------------------------------------------------------------------------------------------
// Sets default values
AVRMotionController::AVRMotionController()
	:
	GrabbedActor(nullptr),
	isTeleporterActive(false),
	wantsToGrip(false),
	isValidTeleportDest(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//ConstructorHelpers::FObjectFinder<UHapticFeedbackEffect_Base> hapticFeedbackFinder(TEXT("/Game/HapticFeedback/HapticImpulse.HapticImpulse"));
	//HapticFeebackEffect = hapticFeedbackFinder.Object;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	MotionController->SetupAttachment(RootComponent);

	ConstructorHelpers::FObjectFinder<USkeletalMesh> meshFinder(TEXT("/Game/VirtualReality/Mannequin/Character/Mesh/MannequinHand_Right.MannequinHand_Right"));
	ConstructorHelpers::FObjectFinder<UMaterialInterface> materialFinder(TEXT("/Game/VirtualReality/Mannequin/Character/Materials/M_HandMat.M_HandMat"));

	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	HandMesh->SkeletalMesh = meshFinder.Object;
	HandMesh->SetMaterial(0, materialFinder.Object);
	//HandMesh->SetAnimInstanceClass(HandAnimation::GetClass());
	HandMesh->SetupAttachment(MotionController);

	ArcDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ArcDirection"));
	ArcDirection->SetupAttachment(HandMesh);

	ArcSpline = CreateDefaultSubobject<USplineComponent>(TEXT("ArcSpline"));
	ArcSpline->SetupAttachment(HandMesh);

	GrabSphere = CreateDefaultSubobject<USphereComponent>(TEXT("GrabSphere"));
	GrabSphere->SetSphereRadius(10.0f);
	GrabSphere->SetHiddenInGame(true);
	GrabSphere->SetupAttachment(HandMesh);

	ArcEndPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArcEndPoint"));
	SetModelAndMaterial(ArcEndPoint, TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Game/VirtualReality/Materials/M_ArcEndpoint.M_ArcEndpoint"));
	ArcEndPoint->SetWorldScale3D(FVector(0.15f, 0.15f, 0.15f));
	ArcEndPoint->SetVisibility(false);
	ArcEndPoint->SetupAttachment(RootComponent);

	TeleportCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportCylinder"));
	SetModelAndMaterial(TeleportCylinder, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Game/VirtualReality/Materials/MI_TeleportCylinderPreview.MI_TeleportCylinderPreview"));
	TeleportCylinder->SetWorldScale3D(FVector(0.75f, 0.75f, 1.0f));
	TeleportCylinder->SetupAttachment(RootComponent);

	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	SetModelAndMaterial(Ring, TEXT("/Game/VirtualReality/Meshes/SM_FatCylinder.SM_FatCylinder"), TEXT("/Game/VirtualReality/Materials/M_ArcEndpoint.M_ArcEndpoint"));
	Ring->SetWorldScale3D(FVector(0.5f, 0.5f, 0.15f));
	Ring->SetupAttachment(TeleportCylinder);

	Arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrow"));
	SetModelAndMaterial(Arrow, TEXT("/Game/VirtualReality/Meshes/BeaconDirection.BeaconDirection"), TEXT("/Game/VirtualReality/Materials/M_ArcEndpoint.M_ArcEndpoint"));
	Arrow->SetupAttachment(TeleportCylinder);
}

//---------------------------------------------------------------------------------------------------------------------
// Called when the game starts or when spawned
void AVRMotionController::BeginPlay()
{
	Super::BeginPlay();

	if (Hand == EControllerHand::Left)
	{
		SetActorScale3D(FVector(1.0f, 1.0f, -1.0f));
		MotionController->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
	}
	else
	{
		MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	}

	TeleportCylinder->SetVisibility(false, true);
}

//---------------------------------------------------------------------------------------------------------------------
// Called every frame
void AVRMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update animation of hand
	//auto animInstance = (UAnimBlueprintGeneratedClass*)HandMesh->GetAnimInstance();
	//animInstance->set;

	if (wantsToGrip)
	{
		HandMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		HandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ClearArc();
	if (isTeleporterActive)
	{
		FTeleportTraceResult result;
		bool foundDest = TraceTeleportDestination(result);
		isValidTeleportDest = foundDest;

		TeleportCylinder->SetVisibility(isValidTeleportDest, true);
		TeleportCylinder->SetWorldLocation(result.NavMeshLocation);

		UpdateArcSpline(isValidTeleportDest, result.TracePoints);
		UpdateArcEndpoint(result.TraceLocation, isValidTeleportDest);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::RumbleController(float _intensity)
{
	auto playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->PlayHapticEffect(HapticFeebackEffect, Hand);
}

//---------------------------------------------------------------------------------------------------------------------
AActor* AVRMotionController::GetActorNearHand()
{
	ConstructorHelpers::FObjectFinder<UInterface> pickupInterfaceFinder(TEXT("/Game/VirtualRealityBP/Blueprints/PickupActorInterface.PickupActorInterface"));
	auto pickupInterface = pickupInterfaceFinder.Object->StaticClass();

	float nearest = FLT_MAX;
	AActor* nearestActor = nullptr;

	auto handPos = GrabSphere->GetComponentLocation();

	TArray<AActor*> overlappingActors;
	GrabSphere->GetOverlappingActors(overlappingActors);
	for (int i = 0; i < overlappingActors.Num(); i++)
	{
		auto actor = overlappingActors[i];
		if (UKismetSystemLibrary::DoesImplementInterface(actor, pickupInterface))
		{
			auto pos = actor->GetActorLocation();
			auto dist = FVector::DistSquared(handPos, pos);

			if (dist < nearest)
			{
				nearestActor = actor;
				nearest = dist;
			}
		}
	}

	return nearestActor;
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::GrabActor()
{
	wantsToGrip = true;

	if (GrabbedActor != nullptr)
	{

	}

	auto nearestActor = GetActorNearHand();
	
	if (nearestActor != nullptr)
	{
		GrabbedActor = nearestActor;

		GrabbedActor->AttachToComponent(MotionController, FAttachmentTransformRules::KeepRelativeTransform);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::ReleaseActor()
{
	wantsToGrip = false;

	if (GrabbedActor != nullptr)
	{
		if (GrabbedActor->GetRootComponent()->GetAttachParent() == MotionController)
		{
			GrabbedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			
			GrabbedActor = nullptr;
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::ActivateTeleporter()
{
	TeleportCylinder->SetVisibility(true, true);
	isTeleporterActive = true;
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::DeactivateTeleporter()
{
	TeleportCylinder->SetVisibility(false, true);
	ArcEndPoint->SetVisibility(false);
	isTeleporterActive = false;
}

//---------------------------------------------------------------------------------------------------------------------
bool AVRMotionController::TraceTeleportDestination(FTeleportTraceResult& _result)
{
	FPredictProjectilePathParams params;
	params.StartLocation = ArcDirection->GetComponentLocation();
	params.LaunchVelocity = ArcDirection->GetForwardVector() * 10.0f;
	params.bTraceWithCollision = true;
	params.ProjectileRadius = 0.0f;
	params.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
	params.bTraceComplex = false;

	FPredictProjectilePathResult result;

	bool collided = UGameplayStatics::PredictProjectilePath(GetWorld(), params, result);

	for (int i = 0; i < result.PathData.Num(); i++)
	{
		_result.TracePoints.Add(result.PathData[i].Location);
	}

	if (collided)
	{
		_result.TraceLocation = result.LastTraceDestination.Location;

		auto navigationSystem = UNavigationSystem::GetNavigationSystem(GetWorld());

		FVector pos;
		collided = UNavigationSystem::K2_ProjectPointToNavigation(GetWorld(), result.LastTraceDestination.Location, pos, nullptr, 0, FVector(1.0f));

		if (collided)
		{
			_result.NavMeshLocation = pos;
		}
	}

	return collided;
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::ClearArc()
{
	for (int i = 0; i < SplineMeshes.Num(); i++)
	{
		SplineMeshes[i]->DestroyComponent();
	}
	SplineMeshes.Empty();

	ArcSpline->ClearSplinePoints();
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::UpdateArcSpline(bool foundValidLocation, TArray<FVector> splinePoints)
{
	if (!foundValidLocation)
	{
		splinePoints.Empty();

		splinePoints.Add(ArcDirection->GetComponentLocation());
		splinePoints.Add(ArcDirection->GetComponentLocation() + ArcDirection->GetForwardVector() * 20.0f);
	}

	for (int i = 0; i < splinePoints.Num(); i++)
	{
		auto point = splinePoints[i];
		ArcSpline->AddSplinePoint(point, ESplineCoordinateSpace::Local);
	}

	ArcSpline->SetSplinePointType(splinePoints.Num() - 1, ESplinePointType::CurveClamped);

	for (int i = 0; i < ArcSpline->GetNumberOfSplinePoints() - 2; i++)
	{
		auto splineMesh = NewObject<USplineMeshComponent>();
		SplineMeshes.Add(splineMesh);
		splineMesh->AttachToComponent(ArcSpline, FAttachmentTransformRules::KeepWorldTransform);

		splineMesh->SetStartAndEnd(
			splinePoints[i], ArcSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local),
			splinePoints[i + 1], ArcSpline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local));
	}
}

//---------------------------------------------------------------------------------------------------------------------
void AVRMotionController::UpdateArcEndpoint(FVector newLocation, bool validLocationFound)
{
	ArcEndPoint->SetVisibility(validLocationFound && isTeleporterActive);
	ArcEndPoint->SetWorldLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);

	FRotator rot;
	FVector pos;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(rot, pos);

	Arrow->SetWorldRotation(FRotator(0, rot.Yaw, 0));
}

//---------------------------------------------------------------------------------------------------------------------
FVector AVRMotionController::GetTeleportDestination()
{
	FRotator rot;
	FVector pos;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(rot, pos);

	FVector offset = FVector(pos.X, pos.Y, 0.0f);

	return TeleportCylinder->GetComponentLocation() - offset;
}