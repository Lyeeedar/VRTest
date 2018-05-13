// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SplineComponent.h"
#include <Components/SplineMeshComponent.h>
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "VRMotionController.generated.h"

struct FTeleportTraceResult
{
	TArray<FVector> TracePoints;
	FVector NavMeshLocation;
	FVector TraceLocation;
};

UCLASS()
class VRTEST_API AVRMotionController : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere)
	EControllerHand Hand;

	UPROPERTY(VisibleAnywhere)
	UHapticFeedbackEffect_Base* HapticFeebackEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UMotionControllerComponent* MotionController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* HandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UArrowComponent* ArcDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USplineComponent* ArcSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* GrabSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* ArcEndPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* TeleportCylinder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* Ring;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* Arrow;

	UPROPERTY(VisibleAnywhere, Category = "Grabbing")
	AActor* GrabbedActor;

public:
	UFUNCTION(BlueprintCallable)
	void RumbleController(float _intensity);

	UFUNCTION(BlueprintCallable, Category = "Grabbing")
	AActor* GetActorNearHand();

	UFUNCTION(BlueprintCallable, Category = "Grabbing")
	void GrabActor();

	UFUNCTION(BlueprintCallable, Category = "Grabbing")
	void ReleaseActor();

	UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void ActivateTeleporter();

	UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void DeactivateTeleporter();

	//UFUNCTION(BlueprintCallable, Category = "Teleportation")
	bool TraceTeleportDestination(FTeleportTraceResult& result);

	UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void ClearArc();

	UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void UpdateArcSpline(bool foundValidLocation, TArray<FVector> splinePoints);

	UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void UpdateArcEndpoint(FVector newLocation, bool validLocationFound);

	UFUNCTION(BlueprintCallable, Category = "Teleportation")
	FVector GetTeleportDestination();
	
public:	
	// Sets default values for this actor's properties
	AVRMotionController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TArray<USplineMeshComponent*> SplineMeshes;

	bool wantsToGrip;
	bool isTeleporterActive;
	bool isValidTeleportDest;
};
